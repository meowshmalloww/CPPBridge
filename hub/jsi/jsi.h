#pragma once
#include <string>
#include <vector>
#include <iostream>

namespace facebook {
    namespace jsi {
        
        class Runtime {};
        
        // 1. Strings
        class String {
            std::string _str;
        public:
            String(std::string s) : _str(s) {}
            static String createFromUtf8(Runtime& rt, const std::string& s) { return String(s); }
            std::string utf8(Runtime& rt) const { return _str; }
        };

        // 2. Arrays
        class Array {
            size_t _size;
        public:
            Array(Runtime& rt, size_t size) : _size(size) {
                 // In a real app, this holds data. In the mock, we just pretend.
            }
            void setValueAtIndex(Runtime& rt, size_t i, double value) {
                // Mock: pretend to set value
            }
        };

        // 3. PropNameID (Helper for looking up names)
        class PropNameID {
            std::string _name;
        public:
            PropNameID(std::string n) : _name(n) {}
            static PropNameID forUtf8(Runtime& rt, const std::string& name) { return PropNameID(name); }
            std::string utf8(Runtime& rt) const { return _name; }
        };

        // 4. Value (Can hold anything)
        class Value {
            double internalNumber;
            std::string internalString;
            bool isStr = false;
        public:
            // Constructors to accept different types
            Value(double d) : internalNumber(d) {}
            Value(int i) : internalNumber((double)i) {}
            Value(String s) : internalString(s.utf8(*(Runtime*)nullptr)), isStr(true), internalNumber(0) {} 
            Value(Array a) : internalNumber(0) {} // Array acts like 0 in mock

            static Value undefined() { return Value(0); }
            
            double asNumber() const { return internalNumber; }
            std::string asString(Runtime& rt) const { return internalString; }
        };

        // 5. HostObject (The Bridge Base)
        class HostObject {
        public:
            virtual ~HostObject() {}
            virtual Value get(Runtime& rt, const PropNameID& name) { return Value::undefined(); }
            virtual std::vector<PropNameID> getPropertyNames(Runtime& rt) { return {}; }
        };

        #define JSI_EXPORT 
    }
}