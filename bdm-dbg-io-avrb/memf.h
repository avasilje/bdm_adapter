#pragma once

#include "json-c/json.h"

/*! \brief Toggles the endianism of \a u16 (by swapping its bytes).
 *
 * \param u16 WORD of which to toggle the endianism.
 *
 * \return Value resulting from \a u16 with toggled endianism.
 *
 * \note More optimized if only used with values known at compile time.
 */
#define Swap16(u16) ((WORD)(((WORD)(u16) >> 8) |\
                           ((WORD)(u16) << 8)))

/*! \brief Toggles the endianism of \a u32 (by swapping its bytes).
 *
 * \param u32 DWORD of which to toggle the endianism.
 *
 * \return Value resulting from \a u32 with toggled endianism.
 *
 * \note More optimized if only used with values known at compile time.
 */
#define Swap32(u32) ((DWORD)(((DWORD)Swap16((DWORD)(u32) >> 16)) |\
                           ((DWORD)Swap16((DWORD)(u32)) << 16)))

/*! \brief Toggles the endianism of \a u64 (by swapping its bytes).
 *
 * \param u64 U64 of which to toggle the endianism.
 *
 * \return Value resulting from \a u64 with toggled endianism.
 *
 * \note More optimized if only used with values known at compile time.
 */
//#define Swap64(u64) ((U64)(((U64)Swap32((U64)(u64) >> 32)) |\
//                           ((U64)Swap32((U64)(u64)) << 32)))


typedef struct {
    int is_valid;
    const CHAR *name;
    DWORD addr;
    DWORD size;
    BYTE  id;
    int   width;
    const CHAR *format;
    const CHAR *wop;
    BYTE *data;
    BYTE *prev_data;        // Leak is here
} T_MEM_ENTRY;

typedef struct {
    BYTE id;
    CHAR *file_name;
    WCHAR *name;
    json_tokener* json_tokener;

    int mem_regions_it;

    struct json_object_iterator it;
    struct json_object* obj;

    struct json_object *mem_regions;

    struct json_object *mem_blocks;
    int mem_blocks_it;

    T_MEM_ENTRY t_entry_template;
} T_MEM_FILE;


T_MEM_FILE *memf_cmd_init(WCHAR *file_name);
T_MEM_FILE *memf_rsp_init(WCHAR *file_name); 

int memf_cmd_get_next(T_MEM_FILE *memf, T_MEM_ENTRY *entry);
void memf_cmd_close(T_MEM_FILE *memf);
void memf_rsp_close(T_MEM_FILE *memf);
void mem_entry_set_default(T_MEM_ENTRY *mem_entry);

extern T_MEM_FILE *memf_rsp;
