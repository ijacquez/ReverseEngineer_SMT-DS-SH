#include <iostream>
#include <fstream>

#include "text.hpp"

Text::Init::Init() {
    std::ifstream in;

    /* XXX: Catch errors */
    in.open("files/symbol_jp.bin", std::ios::binary);

    in.seekg(0, std::ifstream::end);
    _symbol_jp_len = in.tellg();
    in.seekg(std::ifstream::beg);

    _symbol_jp = new uint8_t[_symbol_jp_len / sizeof(_symbol_jp[0])];

    in.read((char*)_symbol_jp, _symbol_jp_len);
    in.close();

    /* XXX: Catch errors */
    in.open("files/ascii.bin", std::ios::binary);

    in.seekg(0, std::ifstream::end);
    _ascii_len = in.tellg();
    in.seekg(std::ifstream::beg);

    _ascii = new uint8_t[_ascii_len / sizeof(_ascii[0])];

    in.read((char*)_ascii, _ascii_len);
    in.close();
}

Text::Text(uint32_t id, uint16_t* data, uint32_t length, uint32_t offset) :
    _text_id(id), _text_data(data), _text_length(length),
    _text_offset(offset) {
}

Text::Text(Text& text) :
    _text_id(text._text_id), _text_data(text._text_data),
    _text_length(text._text_length), _text_offset(text._text_offset) {
}

uint32_t const Text::id(void) const {
    return _text_id;
}

uint32_t const Text::offset(void) const {
    return _text_offset;
}

uint32_t const Text::length(void) const {
    return _text_length / sizeof(uint16_t);
}

void Text::output(void) {
    for (uint16_t* text_data = _text_data;
         text_data <= (_text_data + _text_length); text_data++) {
        uint8_t* data = NULL;
        uint16_t word;

        word = BIG2LE_16(*text_data);
        data = (uint8_t*)&word;

        if (data[0] == 0xFF) {
            _transform_control(data[1]);

            if (data[1] == 0x00)
                return;
        } else {
            uint32_t idx;

            if (data[0] >= 0x80) {
                uint16_t token;

                token = ((data[0] - 0xC5) << 6) | (data[1] & 0x7F);
                idx = token * 3;

                if (idx < _symbol_jp_len) {
                    _transform_symbol_jp((_symbol_jp[idx + 2] << 16) |
                                         (_symbol_jp[idx + 1] << 8) |
                                         _symbol_jp[idx],
                                         word);
                }
            } else if (data[0] > 0x00) {
                idx = data[1] - 0x20;
                if (idx < _ascii_len)
                    _transform_ascii(_ascii[idx]);
                else
                    _transform_unknown(word);
            }
        }
    }
}

void Text::_transform_symbol_jp(uint32_t value, uint16_t raw_value) {
}

void Text::_transform_ascii(uint8_t value) {
}

void Text::_transform_control(uint8_t value) {
}

void Text::_transform_unknown(uint16_t value) {
}

uint8_t* Text::_ascii;
size_t Text::_ascii_len;
uint8_t* Text::_symbol_jp;
size_t Text::_symbol_jp_len;

Text::Init Text::initializer;

Text2JSON::Text2JSON(Text& text, json_t* root_object) :
    Text::Text(text) {
    _object = json_object();
    json_array_append(root_object, _object);

    json_t* json_value;

    json_value = json_integer(text.id());
    json_object_set(_object, "id", json_value);
    json_decref(json_value);

    json_value = json_integer(text.offset());
    json_object_set(_object, "offset", json_value);
    json_decref(json_value);

    json_value = json_integer(text.length());
    json_object_set(_object, "length", json_value);
    json_decref(json_value);

    _array_text = json_array();
    json_object_set(_object, "text", _array_text);
    json_decref(_array_text);
}

Text2JSON::~Text2JSON() {
    json_decref(_object);
}

void Text2JSON::_transform_symbol_jp(uint32_t value, uint16_t raw_value) {
    json_t* json_value;

    json_value = json_string((char *)&value);
    json_array_append(_array_text, json_value);
    json_decref(json_value);
}

void Text2JSON::_transform_ascii(uint8_t value) {
}

void Text2JSON::_transform_control(uint8_t value) {
    json_t* json_value;
    uint16_t control;

    if (value == 0x00)
        return;

    switch (value) {
    case 0x16:
    case 0x17:
    case 0x18:
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
        value = value - 0x16;
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x06:
    case 0x84:
    case 0x85:
        control = 0xFF00 | value;
        break;
    default:
        control = 0x8000 | value;
    }

    json_value = json_integer(control);
    json_array_append(_array_text, json_value);
    json_decref(json_value);
}

void Text2JSON::_transform_unknown(uint16_t value) {
}
