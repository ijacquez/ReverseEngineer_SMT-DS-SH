#ifndef TEXT_HPP_
#define TEXT_HPP_

#include "utils.hpp"

class Text {
public:
    static class Init {
    public:
        Init();
    } initializer;

    Text(uint32_t id, uint16_t* data, uint32_t length, uint32_t offset);
    Text(Text& text);

    uint32_t const id(void) const;
    uint32_t const offset(void) const;
    uint32_t const length(void) const;
    void output(void);

protected:
    virtual void _transform_symbol_jp(uint32_t value, uint16_t raw_value);
    virtual void _transform_ascii(uint8_t value);
    virtual void _transform_control(uint8_t value);
    virtual void _transform_unknown(uint16_t value);

private:
    static uint8_t* _ascii;
    static size_t _ascii_len;
    static uint8_t* _symbol_jp;
    static size_t _symbol_jp_len;

    uint32_t _text_id;
    uint16_t* _text_data;
    uint32_t _text_length;
    uint32_t _text_offset;
};

class Text2JSON : public Text {
public:
    Text2JSON(Text& text, json_t* root_object);
    ~Text2JSON();

protected:
    void _transform_symbol_jp(uint32_t value, uint16_t raw_value);
    void _transform_ascii(uint8_t value);
    void _transform_control(uint8_t value);
    void _transform_unknown(uint16_t value);

private:
    json_t* _object;
    json_t* _array_text;
};

#endif /* !TEXT_HPP_ */
