
#include "region.h"
#include "assert.h" 

#include "memf.h"



region_t region;

extern uint16_t crc16(uint8_t *buff, size_t size);

void region_set_data(region_t *pt_region, uint8_t *data, size_t size)
{
    if (size != region.size) {
        wprintf(L"Protocol error @%d\n", __LINE__);
        assert(0);
//        return;
    }

    if (pt_region->data) {
        free(pt_region->data);
        pt_region->data = NULL;
    }
    pt_region->data = malloc(pt_region->size);
    memcpy(pt_region->data, data, pt_region->size);
}

void region_set_crc(region_t *pt_region)
{
    WORD new_crc = crc16(pt_region->data + 2, pt_region->size - 2);
    *(WORD*)pt_region->data = Swap16(new_crc);
}

int region_check_crc(region_t *pt_region)
{
    WORD crc = crc16(pt_region->data + 2, pt_region->size - 2);
    return (*(WORD*)pt_region->data == Swap16(crc));
}
