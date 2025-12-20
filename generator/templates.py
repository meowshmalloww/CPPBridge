# generator/templates.py

JSI_HEADER = """
#pragma once
#include "../hub/schema.h"
#include "../hub/arena.h"
#include <jsi/jsi.h>
#include <type_traits> // <--- REQUIRED for the fix

using namespace facebook;

namespace Bridge {

    class JSI_EXPORT %name%HostObject : public jsi::HostObject {
        Hub::Handle handle;
    public:
        %name%HostObject(Hub::Handle h) : handle(h) {}

        jsi::Value get(jsi::Runtime& rt, const jsi::PropNameID& name) override {
            std::string propName = name.utf8(rt);

            // 1. Get Base Pointer (Once)
            uint8_t* basePtr = (uint8_t*)Hub::frameArena.access(this->handle);
            if (!basePtr) return jsi::Value::undefined();

            // 2. Check Fields
            %fields_logic%

            return jsi::Value::undefined();
        }

        std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime& rt) override {
            std::vector<jsi::PropNameID> names;
            %prop_names%
            return names;
        }
    };
}
"""

FIELD_CASE = """
            if (propName == "%field_name%") {
                // Pointer arithmetic to find the field
                auto* fieldPtr = reinterpret_cast<%type%*>(basePtr + %offset%);

                // FIX: If it is a pointer (like pixels*), return 0.
                // If it is a number, return the value.
                if constexpr (std::is_pointer_v<%type%>) {
                    return jsi::Value((double)0); 
                } else {
                    return jsi::Value((double)(*fieldPtr));
                }
            }
"""