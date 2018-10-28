#include <windows.h>
#include <stdio.h>
#include <wchar.h>

#include "json-c/json.h"
#include "memf.h"

int json_object_object_get_int(const struct json_object *jso, const char *key, int *val) {

    if (!val || !key || !jso) return -1;

    struct json_object *jso_val;

    if (json_object_object_get_ex(jso, key, &jso_val)) {
        if (json_object_get_type(jso_val) == json_type_string) {
            const char *s = json_object_get_string(jso_val);
            if (!s || 1 != sscanf_s(s, "%i", val)) {
                return -1;
            }
        }
        else {
            *val = json_object_get_int(jso_val);
        }
    } else {
        return -1;
    }
    return 0;
}


T_MEM_FILE *memf_rsp_init(WCHAR *file_name_W)
{
    /*
     *
     */

    T_MEM_FILE *memf = (T_MEM_FILE*)malloc(sizeof(T_MEM_FILE));
    memset(memf, 0, sizeof(T_MEM_FILE));

    char file_name[1024];
    size_t converted;
    wcstombs_s(&converted, file_name, sizeof(file_name), file_name_W, sizeof(file_name));
    memf->file_name = _strdup(file_name);

    memf->obj = json_object_new_object();

    memf->mem_blocks = json_object_new_array();

    memf->mem_blocks_size = 0;
    memf->mem_blocks_it = 0;

    return memf;
}

int memf_rsp_put_next(T_MEM_FILE *memf, T_MEM_ENTRY *mem_entry)
{
    return 0;
}

T_MEM_FILE *memf_cmd_init(WCHAR *file_name_W)
{

    /*
     * 
     */
    T_MEM_FILE *memf = (T_MEM_FILE*)malloc(sizeof(T_MEM_FILE));
    memset(memf, 0, sizeof(T_MEM_FILE));

    char file_name[1024];
    size_t converted;
    wcstombs_s(&converted, file_name, sizeof(file_name), file_name_W, sizeof(file_name));

    memf->file_name = _strdup(file_name);

    struct json_object*  fobj = json_object_from_file(file_name);
    
    if (!json_object_object_get_ex(fobj, "mem_blocks", &memf->mem_blocks)) {
        wprintf(L"Can't find memory blocks");
        return NULL;
    }

    if (json_object_get_type(memf->mem_blocks) != json_type_array) {
        wprintf(L"Bad format");
        return NULL;
    }

    memf->mem_blocks_size = json_object_array_length(memf->mem_blocks);
    memf->mem_blocks_it = 0;

    return memf;
}

int memf_cmd_get_next(T_MEM_FILE *memf, T_MEM_ENTRY *mem_entry)
{
    if (!mem_entry) {
        return -1;
    }

    mem_entry->is_valid = 0;

    if (memf->mem_blocks_it >= memf->mem_blocks_size) {
        // Not an error 
        return 0;
    }

    struct json_object *j_mem_entry = json_object_array_get_idx(memf->mem_blocks, memf->mem_blocks_it);

    json_bool rc = 1;

    // Mandatory fields
    struct json_object *val;

    if (json_object_object_get_ex(j_mem_entry, "name", &val)) {
        mem_entry->name = json_object_get_string(val);
    } else {
        mem_entry->name = "";
    }

    
    if (json_object_object_get_int(j_mem_entry, "addr", &mem_entry->addr)) {
        wprintf(L"Element not found @%d", __LINE__);
        return -1;
    }

    if (json_object_object_get_int(j_mem_entry, "size", &mem_entry->size)) {
        wprintf(L"Element not found @%d", __LINE__);
        return -1;
    }

    // Optional fields
    if (json_object_object_get_ex(j_mem_entry, "width", &val)) {
        mem_entry->width = json_object_get_int(val);
    }

    mem_entry->format = "d";
    if (json_object_object_get_ex(j_mem_entry, "format", &val)) {
        mem_entry->format = json_object_get_string(val);
    }
    mem_entry->is_valid = 1;

    memf->mem_blocks_it++;
    return 0;
}

void memf_cmd_close(T_MEM_FILE *memf)
{
    if (memf->json_tokener) {
        json_tokener_free(memf->json_tokener);
        memf->json_tokener = NULL;
    }
    //json_tokener_free(memf->obj);
    free(memf);
}

void memf_rsp_close(T_MEM_FILE *memf)
{
    if (memf_rsp->file_name == NULL) {
        memf_rsp->file_name = "default_frd_out.json";
    }

    json_object_object_add(memf_rsp->obj, "mem_blocks", memf_rsp->mem_blocks);

    json_object_to_file_ext(memf_rsp->file_name, memf_rsp->obj, JSON_C_TO_STRING_PRETTY);

    json_object_put(memf_rsp->obj);

    free(memf);
}




#if 0
memf->json_tokener = json_tokener_new();

//json_object* o = json_tokener_parse_ex(json_tokener, in, len);

memf->obj = json_tokener_parse(file_content);

memf->it = json_object_iter_begin(memf->obj);
memf->itEnd = json_object_iter_end(memf->obj);

while (!json_object_iter_equal(&memf->it, &memf->itEnd)) {
    wprintf(L"%hs\n",
        json_object_iter_peek_name(&memf->it));
    json_object_iter_next(&memf->it);
};

json_object_object_foreach(memf->obj, mem_block_key, mem_block_val)
{
    int a = json_object_get_type(mem_block_val);
    wprintf(L"%d - %hs\n", a, mem_block_key);
}

struct json_object*  fobj = json_object_from_file("stst");
json_object_object_foreach(fobj, fkey, fval)
{
    int a = json_object_get_type(fval);
    wprintf(L"%d - %hs\n", a, fkey);

    json_object_object_foreach(fval, fblock_key, fblock_val)
    {
        int a = json_object_get_type(fblock_val);
        wprintf(L"%d - %hs\n", a, fblock_key);
    }

}

HANDLE h_json_file;

h_json_file = CreateFile(file_name,      // File name
    GENERIC_READ,                        // Access mode
    FILE_SHARE_READ,                     // Share Mode
    NULL,                                // Security Attributes
    OPEN_EXISTING,                       // Creation Disposition
    0,                                   // File Flags&Attr
    NULL);                               // hTemplate

if (h_json_file == INVALID_HANDLE_VALUE) {
    wprintf(L"Can't open file %s", file_name);
    return NULL;
}

DWORD file_size = GetFileSize(
    h_json_file,
    NULL
);

char *file_content = (char*)malloc(file_size);
file_content[file_size - 1] = 0;

int n_rc = ReadFile(
    h_json_file,                            //__in         HANDLE hFile,
    file_content,                           //__out        LPVOID lpBuffer,
    file_size,                              //__in         DWORD nNumberOfBytesToRead,
    NULL,                                   //__out_opt    LPDWORD lpNumberOfBytesRead,
    NULL);                                  //__inout_opt  LPOVERLAPPED lpOverlapped

CloseHandle(h_json_file);

#endif

#if 0
json_object *jobj = NULL;
const char *mystring = NULL;
int stringlen = 0;
enum json_tokener_error jerr;
do {
    mystring = ...  // get JSON string, e.g. read from file, etc...
        stringlen = strlen(mystring);
    jobj = json_tokener_parse_ex(tok, mystring, stringlen);
} while ((jerr = json_tokener_get_error(tok)) == json_tokener_continue);
if (jerr != json_tokener_success) {
    fprintf(stderr, "Error: %s\n", json_tokener_error_desc(jerr));
    // Handle errors, as appropriate for your application.
}
if (tok->char_offset < stringlen) // XXX shouldn't access internal fields
{
    // Handle extra characters after parsed object as desired.
    // e.g. issue an error, parse another object from that point, etc...
}

json_tokener* json_tokener;
con->json_tokener = json_tokener_new();
json_tokener_free(con->json_tokener);
json_object* o = json_tokener_parse_ex(con->json_tokener, in, len);
ws_dispatch(o);
static void ws_dispatch(json_object* o);
dlog_time();
dlog("dispatch: %s\n", json_object_to_json_string(o));

json_object* request;
if (!json_object_object_get_ex(o, "request", &request)) {
    goto cleanup;
}
const char* request_str = json_object_get_string(request);
#endif
