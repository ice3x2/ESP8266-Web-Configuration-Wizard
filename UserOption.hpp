#pragma once

class UserOption {

    private:
        String _name;
        String _value;
        String _defaultValue;
        bool _isNull;

    public:
        UserOption(String name, String defValue, bool isNull) : _name(name), _defaultValue(defValue), _isNull(isNull), _value("") {
        }


        const char* getName() {
            return _name.c_str();
        }

        const char* getDefaultValue() {
            return _defaultValue.c_str();
        }

        const char* getValue() {
			if(_value.length() == 0) return _defaultValue.c_str();
            return _value.c_str();
        }

        bool isNull() {
            return _isNull;
        }

        void setIsNull(bool isNull) {
            _isNull = isNull;
        }

        void setName(String name) {
            _name = name;
        }

        void setValue(String value) {
            _value = value;
        }

        void setDefaultValue(String defValue) {
            _defaultValue = defValue;
        }

};