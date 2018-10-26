#pragma once

typedef struct {
    int is_valid;
    const CHAR *name;
    DWORD addr;
    DWORD size;
    BYTE  id;
    int   width;
    const CHAR *format;
    BYTE *data;
} T_MEM_ENTRY;

typedef struct {
    BYTE id;
    CHAR *file_name;
    WCHAR *name;
    json_tokener* json_tokener;
    struct json_object_iterator it;
    struct json_object_iterator itEnd;
    struct json_object* obj;

    struct json_object *mem_blocks;
    int mem_blocks_it;
    int mem_blocks_size;

    T_MEM_ENTRY t_entry_template;
} T_MEM_FILE;


T_MEM_FILE *memf_cmd_init(WCHAR *file_name);
T_MEM_FILE *memf_rsp_init(WCHAR *file_name); 

int memf_cmd_get_next(T_MEM_FILE *memf, T_MEM_ENTRY *entry);
void memf_cmd_close(T_MEM_FILE *memf);
void memf_rsp_close(T_MEM_FILE *memf);

extern T_MEM_FILE *memf_rsp;
