import sys
import os
import clang.cindex 

HEADER_TEMPLATE = """
#pragma once
#include "../hub/schema.h"
#include "../hub/arena.h"
#include <jsi/jsi.h>
#include <vector>
#include <string>
#include <cstring> 

using namespace facebook;

namespace Bridge {
    %jsi_classes%
}

extern "C" {
    %c_exports%
}
"""

JSI_CLASS_TEMPLATE = """
    class JSI_EXPORT %name%HostObject : public jsi::HostObject {
        Hub::Handle handle;
    public:
        %name%HostObject(Hub::Handle h) : handle(h) {}

        jsi::Value get(jsi::Runtime& rt, const jsi::PropNameID& name) override {
            std::string propName = name.utf8(rt);
            // SMART ARENA LOOKUP
            auto* basePtr = Hub::%arena_name%.access(this->handle);
            if (!basePtr) return jsi::Value::undefined();

            %jsi_logic%

            return jsi::Value::undefined();
        }
        std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime& rt) override {
            std::vector<jsi::PropNameID> names;
            %prop_names%
            return names;
        }
    };
"""

# --- MAP STRUCTS TO ARENAS ---
ARENA_MAP = {
    "CameraFrame": "frameArena",
    "BigData": "bigDataArena",
    "SQLConfig": "sqlArena",
    "ServerRequest": "requestArena",
    "ServerResponse": "responseArena"
}

try:
    if os.name == 'nt':
        clang.cindex.Config.set_library_path('C:\\LLVM\\bin')
except:
    pass

def parse_header(file_path):
    print(f"👁️  Scanning: {file_path}")
    index = clang.cindex.Index.create()
    tu = index.parse(file_path, args=['-x', 'c++', '-std=c++20', '-DGENERATOR_RUNNING'])
    structs = []
    
    def traverse(node):
        if node.kind == clang.cindex.CursorKind.STRUCT_DECL:
            is_exportable = False
            for child in node.get_children():
                if child.kind == clang.cindex.CursorKind.ANNOTATE_ATTR:
                    if child.displayname == "bridge::struct":
                        is_exportable = True
            if is_exportable:
                print(f"  [+] Found Struct: {node.displayname}")
                struct_data = {"name": node.displayname, "fields": []}
                for child in node.get_children():
                    if child.kind == clang.cindex.CursorKind.FIELD_DECL:
                        for annotation in child.get_children():
                            if annotation.displayname == "bridge::field":
                                raw_type = child.type.spelling
                                struct_data["fields"].append({
                                    "name": child.displayname,
                                    "type": raw_type,
                                    "clean_type": raw_type.replace("std::", "").replace(" ", "").replace("\t", ""),
                                    "offset": node.type.get_offset(child.displayname) // 8,
                                    "is_ptr": (child.type.kind == clang.cindex.TypeKind.POINTER)
                                })
                structs.append(struct_data)
        for child in node.get_children():
            traverse(child)
    traverse(tu.cursor)
    return structs

def generate_cpp(structs, output_path):
    print(f"📝 Generating Universal Adapters...")
    jsi_classes_code = ""
    c_exports_code = ""

    for struct in structs:
        s_name = struct["name"]
        
        # DETERMINE CORRECT ARENA
        arena_name = ARENA_MAP.get(s_name, "frameArena") # Default to frameArena if unknown
        
        jsi_logic = ""
        prop_names = ""
        
        for field in struct["fields"]:
            fname = field['name']
            ftype = field['type']
            ctype = field['clean_type']
            offset = field['offset']
            is_ptr = field['is_ptr']

            # --- JSI GENERATION ---
            if is_ptr: 
                jsi_logic += f"""
            if (propName == "{fname}") return jsi::Value((double)0);"""
            elif "vector" in ctype:
                if "uint8_t" in ctype or "unsignedchar" in ctype:
                     jsi_logic += f"""
            if (propName == "{fname}") {{
                auto* vecPtr = reinterpret_cast<{ftype}*>( (uint8_t*)basePtr + {offset} );
                jsi::Array arr = jsi::Array(rt, vecPtr->size());
                for (size_t i = 0; i < vecPtr->size(); ++i) arr.setValueAtIndex(rt, i, (double)(*vecPtr)[i]);
                return arr;
            }}"""
                else:
                    jsi_logic += f"""
            if (propName == "{fname}") {{
                auto* vecPtr = reinterpret_cast<{ftype}*>( (uint8_t*)basePtr + {offset} );
                jsi::Array arr = jsi::Array(rt, vecPtr->size());
                for (size_t i = 0; i < vecPtr->size(); ++i) arr.setValueAtIndex(rt, i, (double)(*vecPtr)[i]);
                return arr;
            }}"""
            elif "string" in ctype:
                jsi_logic += f"""
            if (propName == "{fname}") {{
                auto* strPtr = reinterpret_cast<std::string*>( (uint8_t*)basePtr + {offset} );
                return jsi::String::createFromUtf8(rt, *strPtr);
            }}"""
            else:
                jsi_logic += f"""
            if (propName == "{fname}") {{
                auto* fieldPtr = reinterpret_cast<{ftype}*>( (uint8_t*)basePtr + {offset} );
                return jsi::Value((double)(*fieldPtr));
            }}"""
            prop_names += f'names.push_back(jsi::PropNameID::forUtf8(rt, "{fname}"));\n'

            # --- C-EXPORTS GENERATION ---
            # Correctly use the specific arena for this struct
            
            if not is_ptr and ("int" in ctype or "float" in ctype or "double" in ctype):
                if "vector" not in ctype: 
                    c_exports_code += f"""
    __declspec(dllexport) double {s_name}_get_{fname}(int handleIndex) {{
        auto* ptr = Hub::{arena_name}.access({{ (uint32_t)handleIndex }});
        if (!ptr) return -1;
        auto* valPtr = reinterpret_cast<{ftype}*>( (uint8_t*)ptr + {offset} );
        return (double)*valPtr;
    }}
"""
            if "vector" in ctype or "string" in ctype:
                 c_exports_code += f"""
    __declspec(dllexport) int {s_name}_get_{fname}_size(int handleIndex) {{
        auto* ptr = Hub::{arena_name}.access({{ (uint32_t)handleIndex }});
        if (!ptr) return 0;
        auto* vecPtr = reinterpret_cast<{ftype}*>( (uint8_t*)ptr + {offset} );
        return (int)vecPtr->size();
    }}
"""

        cls_code = JSI_CLASS_TEMPLATE.replace("%name%", s_name)
        cls_code = cls_code.replace("%arena_name%", arena_name) # Inject Correct Arena
        cls_code = cls_code.replace("%jsi_logic%", jsi_logic)
        cls_code = cls_code.replace("%prop_names%", prop_names)
        jsi_classes_code += cls_code

    full_content = HEADER_TEMPLATE.replace("%jsi_classes%", jsi_classes_code)
    full_content = full_content.replace("%c_exports%", c_exports_code)

    with open(output_path, "w") as f:
        f.write(full_content)
    print(f"✅ Universal Bridge Written to: {output_path}")

if __name__ == "__main__":
    root_dir = os.path.dirname(os.path.abspath(__file__))
    generate_cpp(parse_header(os.path.join(root_dir, "../hub/schema.h")), os.path.join(root_dir, "../hub/generated_bridge.cpp"))