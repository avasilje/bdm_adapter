#pragma once

#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#include <stdint.h>

typedef struct region_s {
    const CHAR *name;
    DWORD addr;        
    DWORD size;
    BYTE *data;
} region_t;

extern region_t region;

extern void region_set_data(region_t *pt_region, uint8_t *data, size_t size);
extern void region_set_crc(region_t *pt_region);
extern int region_check_crc(region_t *pt_region);


