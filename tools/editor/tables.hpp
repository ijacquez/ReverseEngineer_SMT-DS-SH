#ifndef TABLES_HPP
#define TABLES_HPP

typedef struct {
    const char* filename;
    const char* name;
    uint32_t offset;
    uint32_t length;
} TableInfo;

extern const TableInfo table_info_event[];

#endif /* !TABLES_HPP */
