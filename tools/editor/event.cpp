#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

#include "editor.hpp"
#include "tables.hpp"
#include "text.hpp"
#include "utils.hpp"

class Event {
public:
    Event(const char* filename, const char* name, uint32_t offset = 0,
          uint32_t length = 0) :
        _offset(offset), _text_list(NULL)  {
        _name = std::string(name);

        /* XXX: Catch errors */
        _in.open(filename, std::ios::binary);

        if (length == 0) {
            _in.seekg(0, std::ifstream::end);
            length = _in.tellg();
            _in.seekg(std::ifstream::beg);
        }

        _length = length;
    }

    ~Event() {
        _in.close();

        for (std::vector<Text*>::iterator it = _text_list->begin();
             it != _text_list->end(); it++) {
            delete (*it);
        }

        delete _text_list;
    }

    std::string const name(void) const {
        return _name;
    }

    uint32_t const offset(void) const {
        return _offset;
    }

    uint32_t const length(void) const {
        return _length;
    }

    std::vector<uint16_t, uint16_t*>* event_data(void) {
        return NULL;
    }

    std::vector<Text*>* const text_data(void) {
        if (_text_list != NULL)
            return _text_list;

        uint32_t text_data_offset;
        uint32_t text_list_offset;
        uint32_t ntexts;

        text_data_offset = _read_16(0x00000006);

        /* Get number of text strings; each entry in the list is 2
         * bytes */
        text_list_offset = _read_16(0x00000004);
        ntexts = (text_data_offset - text_list_offset) / sizeof(uint16_t);

        _text_list = new std::vector<Text*>();

        for (uint32_t p = 0; ; p++) {
            uint16_t* text_data;
            uint32_t text_offset;
            uint32_t text_offset_next;
            int32_t text_len;

            if ((p + 1) >= ntexts)
                break;

            text_offset = _read_16(text_list_offset + (p << 1));
            text_offset_next = _read_16(text_list_offset + ((p + 1) << 1));
            text_len = text_offset_next - text_offset;

            if (text_len < 0)
                text_len = _length - (text_offset + text_data_offset);

            text_data = new uint16_t[text_len / sizeof(uint16_t)];
            _text_list->push_back(new Text(p,
                                           text_data,
                                           text_len,
                                           _offset + text_data_offset +
                                           text_offset));

            for (uint32_t d = 0; d < text_len / sizeof(*text_data); d++) {
                text_data[d] =
                    _read_16(text_data_offset + text_offset + (d << 1));
            }
        }

        return _text_list;
    }

private:
    std::ifstream _in;

    std::vector<Text*>* _text_list;

    std::string _name;
    uint32_t _offset;
    uint32_t _length;

    uint16_t _read_16(uint32_t offset) {
        uint16_t value;
        uint32_t tell;

        tell = _in.tellg();

        _in.seekg(_offset + offset, std::ios::beg);
        _in.read((char*)&value, 2);

        _in.seekg(tell, std::ios::beg);

        return BIG2LE_16(value);
    }
};

void event(void) {
    json_t* object;
    json_t* array_text_list;
    json_t* array_events;

    object = json_object();

    array_events = json_array();
    json_object_set(object, "events", array_events);
    json_decref(array_events);

    for (int f = 0; f < 58; f++) {
        const TableInfo* event_info;

        event_info = &table_info_event[f];

        std::string filename = "Extracted/" + std::string(event_info->filename);

        Event event(filename.c_str(), event_info->name, event_info->offset,
                    event_info->length);

        json_t* object_event;
        json_t* json_value;

        object_event = json_object();
        json_array_append(array_events, object_event);

        json_value = json_string(event_info->name);
        json_object_set(object_event, "name", json_value);
        json_decref(json_value);

        json_value = json_string(event_info->filename);
        json_object_set(object_event, "filename", json_value);
        json_decref(json_value);

        json_value = json_integer(event_info->offset);
        json_object_set(object_event, "offset", json_value);
        json_decref(json_value);

        json_value = json_integer(event_info->length);
        json_object_set(object_event, "length", json_value);
        json_decref(json_value);

        array_text_list = json_array();
        json_object_set(object_event, "text_list", array_text_list);
        json_decref(array_text_list);

        for (std::vector<Text*>::iterator tt = event.text_data()->begin();
             tt != event.text_data()->end(); tt++) {
            Text2JSON t2json(*(*tt), array_text_list);

            t2json.output();
        }
    }

    json_dump_file(object, "event.json", JSON_INDENT(0) | JSON_PRESERVE_ORDER);
    json_decref(object);
}
