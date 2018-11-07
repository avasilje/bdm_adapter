/***C*********************************************************************************************
**
** SRC-FILE     :   io_devcmd.cpp
**                                        
** PROJECT      :   Coldfire BDM
**                                                                
** SRC-VERSION  :   
**              
** DATE         :   22/01/2011
**
** AUTHOR       :   AV
**
** DESCRIPTION  :   The file defines the set of functions to convert user 
**                  commands from UI to device commands.
**                  Device - AVR Back door
**                  Typicaly function extracts data from incoming command,
**                  repack them into binary packet and sends to the device.
**                  
** COPYRIGHT    :   (c) 2011 Andrejs Vasiljevs. All rights reserved.
**
****C*E************************************************************************/
#include <windows.h>
#include <wchar.h>
#include <stdio.h>

#include "cmd_lib.h"
#include "dbg-io.h"
#include "region.h"
#include "io_avrb_ui_cmd.h"
#include "io_avrb_dev_cmd.h"

#include "memf.h"

extern void (*mem_read_cb)(BYTE *data, size_t len, WCHAR *pc_cmd_resp_out, size_t t_max_resp_len);
extern void (*mem_write_cb)(BYTE *data, size_t len, WCHAR *pc_cmd_resp_out, size_t t_max_resp_len);
extern int memf_entry_printf (T_MEM_ENTRY *pt_mem_entry, WCHAR *pc_cmd_resp_out, size_t t_max_resp_len);
extern void memf_entry_printf_sinlge (T_MEM_ENTRY *pt_mem_entry, WCHAR **ppc_cmd_resp_out, size_t *pt_max_resp_len);
extern void rsp_printf(WCHAR **ppc_cmd_resp_out, size_t *pt_max_resp_len, const WCHAR *format, ...);

extern int json_object_object_get_int(const struct json_object *jso, const char *key, int *val);

static void region_write_get_data_cb(BYTE *data, size_t len, WCHAR *pc_cmd_resp_out, size_t t_max_resp_len);
static void region_read_next_region();
static void region_write_next_region(WCHAR *pc_cmd_resp_out, size_t t_max_resp_len);


#define REGION_WR_RESP_STR_LEN 65536        // !! must be <= RESP_STR_LEN 65536
WCHAR gwca_region_write_resp_str[REGION_WR_RESP_STR_LEN];
WCHAR *gpwc_region_write_resp_out;
size_t gt_max_region_write_resp_len;

T_MEM_FILE *memf_cmd;

#pragma pack(push, 1)
struct t_cmd_hdr {
    BYTE    b_mark;
    BYTE    b_cmd;
    DWORD   dw_len;
};
#pragma pack(pop)

void cmd_io_sign (void)
{
    int n_rc;
    DWORD dw_bytes_to_write;

    #pragma pack(1)
    struct t_cmd_dev_sign {
        struct t_cmd_hdr hdr;
        // No payload
    } t_cmd = { {0x11, 0x00} };

    // -----------------------------------------------
    // --- write data from UI command to MCU command
    // ------------------------------------------------
    n_rc = TRUE;

    // Verify command arguments
    // ...

    if (!n_rc)
        goto cmd_sign_error;
    
    // ----------------------------------------
    // --- Initiate data transfer to MCU
    // ----------------------------------------
    dw_bytes_to_write = sizeof(t_cmd.hdr) + t_cmd.hdr.dw_len;

    dev_tx(dw_bytes_to_write, (BYTE*)&t_cmd, L"SIGN");
    return;

cmd_sign_error:

    wprintf(L"command error : DEV_SIGN\n");
    return;

}

void cmd_io_bdm_break (void)
{
    int n_rc;
    DWORD dw_bytes_to_write;

    #pragma pack(push, 1)
    struct t_cmd_bdm_break {
        struct t_cmd_hdr hdr;
        DWORD    dw_addr;
    } t_cmd = { {0xD2, 0x16, 0x04}, 0x00000000 };
    #pragma pack(pop)

    // -----------------------------------------------
    // --- write data from UI command to DEV command
    // ------------------------------------------------
    n_rc = TRUE;

    t_cmd.dw_addr = gt_cmd_bdm_break.addr.dw_val;

    if (n_rc) {
        // ----------------------------------------
        // --- Initiate data transfer to MCU
        // ----------------------------------------
        dw_bytes_to_write = sizeof(t_cmd.hdr) + t_cmd.hdr.dw_len;

        dev_tx(dw_bytes_to_write, (BYTE*)&t_cmd, L"BREAK");
        return;
    }

    wprintf(L"command error : BREAK\n");
    return;

}

void cmd_io_bdm_go (void)
{
    int n_rc;
    DWORD dw_bytes_to_write;

    #pragma pack(push, 1)
    struct t_cmd_bdm_go {
        struct t_cmd_hdr hdr;
    } t_cmd = { {0xD2, 0x17, 0x00} };
    #pragma pack(pop)

    // -----------------------------------------------
    // --- write data from UI command to DEV command
    // ------------------------------------------------
    n_rc = TRUE;

    if (n_rc) {
        // ----------------------------------------
        // --- Initiate data transfer to MCU
        // ----------------------------------------
        dw_bytes_to_write = sizeof(t_cmd.hdr) + t_cmd.hdr.dw_len;

        dev_tx(dw_bytes_to_write, (BYTE*)&t_cmd, L"GO");
        return;
    }

    wprintf(L"command error : GO\n");
    return;

}

void cmd_io_bdm_reset (void)
{
    int n_rc;
    DWORD dw_bytes_to_write;

    #pragma pack(push, 1)
    struct t_cmd_bdm_reset {
        struct t_cmd_hdr hdr;
    } t_cmd = { {0xD2, 0x18, 0x00} };
    #pragma pack(pop)

    // -----------------------------------------------
    // --- write data from UI command to DEV command
    // ------------------------------------------------
    n_rc = TRUE;

    if (n_rc) {
        // ----------------------------------------
        // --- Initiate data transfer to MCU
        // ----------------------------------------
        dw_bytes_to_write = sizeof(t_cmd.hdr) + t_cmd.hdr.dw_len;

        dev_tx(dw_bytes_to_write, (BYTE*)&t_cmd, L"RESET");
        return;
    }

    wprintf(L"command error : RESET\n");
    return;
}

void cmd_io_bdm_read (void)
{

}


void cmd_io_bdm_write_mem (struct s_cmd_bdm_write *pt_write_bdm )
{
    int n_rc;
    DWORD dw_bytes_to_write;

    #pragma pack(push, 1)
    static struct t_cmd_bdm_write {
        struct t_cmd_hdr hdr;
        DWORD addr;
        DWORD len;
        BYTE  ba_buff[BDM_WRITE_DATA_LEN];
    } t_cmd = { {0xD2, 0x21, 0x00}, 0, 0, {0}};
    #pragma pack(pop)

    // -----------------------------------------------
    // --- write data from UI command to DEV command
    // ------------------------------------------------
    n_rc = TRUE;

    t_cmd.hdr.dw_len = sizeof(t_cmd) - sizeof(struct t_cmd_hdr) - sizeof(t_cmd.ba_buff) +
        pt_write_bdm->data.pt_raw_buff->dw_len;

    memcpy(t_cmd.ba_buff, pt_write_bdm->data.pt_raw_buff->pb_buff, t_cmd.hdr.dw_len);

    // ----------------------------------------
    // --- Initiate data transfer to MCU
    // ----------------------------------------
    dw_bytes_to_write = sizeof(t_cmd.hdr) + t_cmd.hdr.dw_len;

    dev_tx(dw_bytes_to_write, (BYTE*)&t_cmd, L"LOOPBACK");
    return;
}

void cmd_io_bdm_write8 (void)
{
    cmd_io_bdm_write_mem (&gt_cmd_bdm_write8);
}

void cmd_io_bdm_write16 (void)
{
    cmd_io_bdm_write_mem (&gt_cmd_bdm_write16);
}

void cmd_io_bdm_write_mem_entry (T_MEM_ENTRY *pt_mem_entry)
{
    wprintf(L"    Write mem: %08X : %d\n", 
        pt_mem_entry->addr, pt_mem_entry->size);

    int n_rc;
    DWORD dw_bytes_to_write;

#pragma pack(push, 1)
    static struct t_cmd_read {
        struct t_cmd_hdr hdr;
        DWORD addr;
        DWORD len;
        BYTE  ba_buff[BDM_WRITE_DATA_LEN];
    } t_cmd = { { 0xD2, 0x21, 0x00 }, 0, 0, {0}};
#pragma pack(pop)

    // -----------------------------------------------
    // --- write data from UI command to DEV command
    // ------------------------------------------------
    n_rc = TRUE;

    t_cmd.hdr.dw_len = sizeof(t_cmd) - sizeof(struct t_cmd_hdr) - sizeof(t_cmd.ba_buff) +
        pt_mem_entry->size;

    t_cmd.addr = pt_mem_entry->addr;
    t_cmd.len = pt_mem_entry->size;

    // TODO: or split data transfer to second transaction
    memcpy(t_cmd.ba_buff, pt_mem_entry->data, pt_mem_entry->size);

    // ----------------------------------------
    // --- Initiate data transfer to MCU
    // ----------------------------------------
    dw_bytes_to_write = sizeof(t_cmd.hdr) + t_cmd.hdr.dw_len;

    dev_tx(dw_bytes_to_write, (BYTE*)&t_cmd, L"WRITE MEM");
    // TODO: Try send data as an another transaction

    return;
}

void cmd_io_bdm_read_mem_entry (T_MEM_ENTRY *pt_mem_entry)
{
    wprintf(L"    Read mem: %08X : %d\n", 
        pt_mem_entry->addr, pt_mem_entry->size);

    int n_rc;
    DWORD dw_bytes_to_write;

#pragma pack(push, 1)
    static struct t_cmd_read {
        struct t_cmd_hdr hdr;
        DWORD addr;
        DWORD len;
    } t_cmd = { { 0xD2, 0x22, 0x00 }, 0, 0 };
#pragma pack(pop)

    // -----------------------------------------------
    // --- write data from UI command to DEV command
    // ------------------------------------------------
    n_rc = TRUE;

    t_cmd.hdr.dw_len = sizeof(t_cmd) - sizeof(t_cmd.hdr);

    t_cmd.addr = pt_mem_entry->addr;
    t_cmd.len = pt_mem_entry->size;

    // ----------------------------------------
    // --- Initiate data transfer to MCU
    // ----------------------------------------
    dw_bytes_to_write = sizeof(t_cmd.hdr) + t_cmd.hdr.dw_len;
    
    dev_tx(dw_bytes_to_write, (BYTE*)&t_cmd, L"READ MEM");
    return;
}

void cmd_io_bdm_fread_cont(void)
{
    T_MEM_ENTRY t_mem_cmd_entry;
    int err = 0;

    if (memf_cmd == NULL || memf_rsp == NULL) {
        return;
    }

    // -----------------------------------------------
    // --- Get and proceed next entry from memf_cmd 
    // ------------------------------------------------
    do {
        T_MEM_ENTRY *pt_mem_rsp_entry = &memf_rsp->t_entry_template;

        if (memf_cmd_get_next(memf_cmd, &t_mem_cmd_entry)) {
            // abnormal exit - parse error
            err = -1;
            break;
        }

        if (!t_mem_cmd_entry.is_valid) {
            // No more entries
            err = -2;
            break;
        }

        // Prepare template for response
        pt_mem_rsp_entry->is_valid = 1;
        pt_mem_rsp_entry->name = t_mem_cmd_entry.name;
        pt_mem_rsp_entry->addr = t_mem_cmd_entry.addr;
        pt_mem_rsp_entry->size = t_mem_cmd_entry.size;
        pt_mem_rsp_entry->width = t_mem_cmd_entry.width;
        pt_mem_rsp_entry->format = t_mem_cmd_entry.format;
        pt_mem_rsp_entry->data = NULL;

        cmd_io_bdm_read_mem_entry(&t_mem_cmd_entry);

    } while (0);

    if (err) {
        memf_cmd_close(memf_cmd);
        memf_cmd = NULL;

        memf_rsp_close(memf_rsp);
        memf_rsp = NULL;
    }

}

void cmd_io_bdm_fread(void)
{
    if (memf_cmd == NULL) {
        memf_cmd = memf_cmd_init(gt_cmd_bdm_fread.fin.pc_str);

        if (memf_rsp) {
            // Close previous response if still opened for any reason
            memf_rsp_close(memf_rsp);
            memf_rsp = NULL;
        }
        memf_rsp = memf_rsp_init(gt_cmd_bdm_fread.fout.pc_str);
    }

    // Rework required
    // cmd_io_bdm_read_mem_entry(&t_mem_cmd_entry);

    // cmd_io_bdm_fread_cont();
}

json_object* mem_entry_to_block(T_MEM_ENTRY *pt_mem_entry)
{
    char tmp_str[64];
    /*
        *  {
        *     "name"  : "stat0",
        *     "addr"  : "0x12345",      
        *     "size"  : "0x123456",     // Optional 4 by default
        *     "width" : 2,              // Optional 4 by default
        *     "format": "d"             // Optional 'd' by default
        *     "wop" : "none"            // Optional write operation "none" by def (enum: "inc", "dec")
        *  },
        */

    json_object *j_mem_block = json_object_new_object();
    
    json_object_object_add(j_mem_block, "name"  , json_object_new_string(pt_mem_entry->name));

    sprintf_s(tmp_str, sizeof(tmp_str), "0x%08X", pt_mem_entry->addr);
    json_object_object_add(j_mem_block, "addr"  , json_object_new_string(tmp_str));

    if (pt_mem_entry->size != 4) {
        json_object_object_add(j_mem_block, "size"  , json_object_new_int(pt_mem_entry->size));
    }

    if (pt_mem_entry->width != 4) {
        json_object_object_add(j_mem_block, "width" , json_object_new_int(pt_mem_entry->width));
    }

    if (0 != _stricmp(pt_mem_entry->format, "d")) {
        json_object_object_add(j_mem_block, "format", json_object_new_string(pt_mem_entry->format));
    }

    if (0 != _stricmp(pt_mem_entry->wop, "none")) {
        json_object_object_add(j_mem_block, "wop", json_object_new_string(pt_mem_entry->wop));
    }

    json_object *j_data_arr = json_object_new_array();

    char *j_data_format = "%d";

    if (0 == _stricmp(pt_mem_entry->format, "x")) {
        switch (pt_mem_entry->width) {
            case 1: j_data_format = "0x%02X"; break;
            case 2: j_data_format = "0x%04X"; break;
            case 4: j_data_format = "0x%08X"; break;
        }
    }

    size_t idx = 0;
    while(idx < pt_mem_entry->size) {
        DWORD n;

        // Q: byte swap?
        switch (pt_mem_entry->width) {
            case 1: n = *(BYTE*) &pt_mem_entry->data[idx]; break;
            case 2: n = Swap16(*(WORD*) &pt_mem_entry->data[idx]); break;
            case 4: n = Swap32(*(DWORD*)&pt_mem_entry->data[idx]); break;
        }
        idx += pt_mem_entry->width;

        sprintf_s(tmp_str, sizeof(tmp_str), j_data_format, n);
        json_object_array_add(j_data_arr, json_object_new_string(tmp_str));
    }
  
    json_object_object_add(j_mem_block, "data", j_data_arr);

    // AV:POC. Get back data just added - done. works
    //struct json_object *poc;
    //if (json_object_object_get_ex(j_mem_block, "data", &poc)) {
    //    struct json_object *val;
    //    val = json_object_array_get_idx(poc, 0);
    //}

    return j_mem_block;
}

void region_write_set_crc_cb(BYTE *data, size_t len, WCHAR *pc_cmd_resp_out, size_t t_max_resp_len)
{
    mem_write_cb = NULL;
    struct rsp_mem_write_s *rsp = (struct rsp_mem_write_s *)data;

    region_write_next_region(pc_cmd_resp_out, t_max_resp_len);

}

void region_write_set_data_cb(BYTE *data, size_t len, WCHAR *pc_cmd_resp_out, size_t t_max_resp_len)
{
    int err = 0;
    T_MEM_ENTRY t_mem_entry;
    T_MEM_ENTRY t_mem_entry_crc;
    T_MEM_ENTRY *pt_mem_entry = NULL;

    mem_write_cb = NULL;
    struct rsp_mem_write_s *rsp = (struct rsp_mem_write_s *)data;

    // Check response 
    // if ((int)rsp->len < 0) ... error

    do {

        if (memf_cmd_get_next(memf_cmd, &t_mem_entry)) {
            err = -1;   // abnormal exit - parse error
            break;
        }
        
        pt_mem_entry = &t_mem_entry;

        if (!pt_mem_entry->is_valid) {
            err = -2;   // No more entries
            break;
        }

        /* 
         * Write data from entry to just read mem buf and to the device
         */

        if (pt_mem_entry->prev_data) {
            free(pt_mem_entry->prev_data);
            pt_mem_entry->prev_data = NULL;
        }

        BYTE *region_data_ptr = region.data + pt_mem_entry->addr - region.addr;
        pt_mem_entry->prev_data = malloc(sizeof(DWORD));
        *(DWORD*) pt_mem_entry->prev_data = *(DWORD*) region_data_ptr;

        // Apply operation
        if (0 == _stricmp(pt_mem_entry->wop, "none")) {
            ;    
        } else  if (0 <= _stricmp(pt_mem_entry->wop, "inc")) {
            int inc_data = 0;
            int prev = Swap32(*(DWORD*)region_data_ptr);
            sscanf_s(pt_mem_entry->wop, "inc%d", &inc_data);
            *(DWORD*)pt_mem_entry->data = Swap32(inc_data + prev);
            *(DWORD*)region_data_ptr = Swap32(inc_data + prev);
        }
        *(DWORD*)region_data_ptr = *(DWORD*)pt_mem_entry->data;

        memf_entry_printf_sinlge(pt_mem_entry, &gpwc_region_write_resp_out, &gt_max_region_write_resp_len);

        //T_MEM_ENTRY mem_region_data = {
        //    .addr = region.addr,
        //    .size = region.size
        //};

        mem_write_cb = region_write_set_data_cb;
        
    } while (0);


    if (err == -2) {
        // No more entries - update CRC
        region_set_crc(&region);

        t_mem_entry_crc.addr = region.addr;
        t_mem_entry_crc.size = 2;
        t_mem_entry_crc.data = region.data;

        pt_mem_entry = &t_mem_entry_crc;
        mem_write_cb = region_write_set_crc_cb;
    }

    if (pt_mem_entry) {
        cmd_io_bdm_write_mem_entry(pt_mem_entry);
    }

    return;

}

void region_write_get_data_cb(BYTE *data, size_t len, WCHAR *pc_cmd_resp_out, size_t t_max_resp_len)
{
    mem_read_cb = NULL;
    int err = 0;
    int crc_valid = 0;
    
    // -----------------------------------------------
    // --- Write mem block entries from region data
    // ------------------------------------------------
    if (memf_cmd->mem_blocks == NULL) {
        wprintf(L"Bad format @%d\n", __LINE__);
        return;
    }

    region_set_data(&region, data, len);

    const char *crc_is_valid = region_check_crc(&region) ? "Valid" : "Corrupted";

    rsp_printf(&gpwc_region_write_resp_out, &gt_max_region_write_resp_len, L"\n        Region: %hs (0x%08X, %d, %hs)\n", 
        region.name,
        region.addr,
        region.size,
        crc_is_valid);

    // Initiate entries write to the devices
    struct rsp_mem_write_s rsp_dummy = {0};
    region_write_set_data_cb((BYTE*)&rsp_dummy, sizeof(rsp_dummy), pc_cmd_resp_out, t_max_resp_len);
}

void region_write_get_len_cb(BYTE *data, size_t len, WCHAR *pc_cmd_resp_out, size_t t_max_resp_len)
{
    mem_read_cb = NULL;

    region.size = Swap32(*(DWORD*)data) + 16;
    // Read whole region
    T_MEM_ENTRY mem_region_data = {
        .addr = region.addr,
        .size = region.size
    };

    mem_read_cb = region_write_get_data_cb;
    cmd_io_bdm_read_mem_entry(&mem_region_data);
}

void region_write_next_region(WCHAR *pc_cmd_resp_out, size_t t_max_resp_len)
{

    struct json_object *j_region =
        json_object_array_get_idx(memf_cmd->mem_regions, memf_cmd->mem_regions_it);

    memf_cmd->mem_regions_it++;

    if (j_region == NULL) {
        wprintf(L"No more regions @%d\n", __LINE__);

        if (pc_cmd_resp_out) {
            wcscpy_s(pc_cmd_resp_out, gt_max_region_write_resp_len, gwca_region_write_resp_str);
        }

        memf_cmd_close(memf_cmd);
        memf_cmd = NULL;

        return;
    }

    if (!json_object_object_get_ex(j_region, "mem_blocks", &memf_cmd->mem_blocks)) {
        return NULL;
    } else {
        if (json_object_get_type(memf_cmd->mem_blocks) != json_type_array) {
            wprintf(L"Bad format");
            return;
        }
        memf_cmd->mem_blocks_it = 0;
    }

    /*
     * Get region name & addr
     */
    struct json_object *val;
    if (json_object_object_get_ex(j_region, "name", &val)) {
        region.name = json_object_get_string(val);
    } else {
        region.name = "";
    }

    T_MEM_ENTRY mem_region_len = {
        .addr = 0,  // Filled from json
        .size = 4
    };

    if (json_object_object_get_int(j_region, "addr", &region.addr)) {
        wprintf(L"Element not found @%d", __LINE__);
        return;
    }
    /*
     * Read region length
     */
    mem_region_len.addr = region.addr + 4;
    
    mem_read_cb = region_write_get_len_cb;
    cmd_io_bdm_read_mem_entry(&mem_region_len);
}

void cmd_io_bdm_fwrite_region(void)
{
    if (memf_cmd != NULL) {
        return;
    }
    memf_cmd = memf_cmd_init(gt_cmd_bdm_fread.fin.pc_str);

    if (memf_rsp) {
        // Close previous response if still opened for any reason
        // Response is not in use for write
        memf_rsp_close(memf_rsp);
        memf_rsp = NULL;
    }

    gpwc_region_write_resp_out = gwca_region_write_resp_str;
    gt_max_region_write_resp_len = REGION_WR_RESP_STR_LEN;

    rsp_printf(&gpwc_region_write_resp_out, &gt_max_region_write_resp_len, L"\n\n    File: %s \n",
        gt_cmd_bdm_fread.fin.pc_str); 

    region_write_next_region(NULL, 0);

    return;
}

void region_read_get_data_cb(BYTE *data, size_t len, WCHAR *pc_cmd_resp_out, size_t t_max_resp_len)
{
    mem_read_cb = NULL;
    T_MEM_ENTRY t_mem_cmd_entry;
    T_MEM_ENTRY *pt_mem_entry = &t_mem_cmd_entry;
    int err = 0;
    char tmp_str[64];
    
    if (len != region.size) {
        wprintf(L"Protocol error @%d\n", __LINE__);
        return;
    }
    
    region_set_data(&region, data, len);
    const char *crc_is_valid = region_check_crc(&region)? "Valid" : "Corrupted";

    rsp_printf(&pc_cmd_resp_out, &t_max_resp_len, L"Region: %hs (0x%08X, %d, %hs)\n", 
        region.name,
        region.addr,
        region.size,
        crc_is_valid);

    // -----------------------------------------------
    // --- Fill mem block entries from region data
    // ------------------------------------------------
    int blocks_auto = 0;
    DWORD blocks_auto_width = 4; 
    DWORD blocks_auto_addr = region.addr + blocks_auto_width;
    size_t blocks_auto_size = region.size - blocks_auto_width; 

    if (memf_cmd->mem_blocks == NULL) {
        blocks_auto = 1;
    }

    do {

        if (blocks_auto) {
            if (blocks_auto_size < 4) {
                pt_mem_entry->is_valid = 0;
            } else {
                pt_mem_entry->name = "";
                pt_mem_entry->addr = blocks_auto_addr;
                pt_mem_entry->size = blocks_auto_width;
                pt_mem_entry->width = blocks_auto_width;
                pt_mem_entry->format = "d";
                pt_mem_entry->data = NULL;
                
                pt_mem_entry->is_valid = 1;

                blocks_auto_size -= blocks_auto_width;
                blocks_auto_addr += blocks_auto_width;
            }

        } else {

            if (memf_cmd_get_next(memf_cmd, pt_mem_entry)) {        // JSON BLOCK --> mem_entry
                err = -1;   // abnormal exit - parse error
                break;
            }
        }

        if (!pt_mem_entry->is_valid) {
            err = -2;   // No more entries
            break;
        }

        pt_mem_entry->prev_data = pt_mem_entry->data;
        pt_mem_entry->data = data + pt_mem_entry->addr - region.addr;

        json_object* j_mem_block = mem_entry_to_block(pt_mem_entry);
        json_object_array_add(memf_rsp->mem_blocks, j_mem_block);

        memf_entry_printf_sinlge(pt_mem_entry, &pc_cmd_resp_out, &t_max_resp_len);

    } while (1);

    /*
     *  Add region JSON to response 
     */ 

    // Compose region object
    struct json_object*  j_region = json_object_new_object();
    json_object_object_add(j_region, "name"  , json_object_new_string(region.name));
    sprintf_s(tmp_str, sizeof(tmp_str), "0x%08X", region.addr);
    json_object_object_add(j_region, "addr"  , json_object_new_string(tmp_str));
    //json_object_object_add(j_region, "size", json_object_new_int(region.size));
    json_object_object_add(j_region, "valid"  , json_object_new_string(crc_is_valid));

    // Add data to region object
    json_object_object_add(j_region, "mem_blocks", memf_rsp->mem_blocks);
    //memf_rsp->mem_blocks = NULL;

    // Add region to root's regions array
    json_object_array_add(memf_rsp->mem_regions, j_region);

    region_read_next_region();
}

void region_read_get_len_cb(BYTE *data, size_t len, WCHAR *pc_cmd_resp_out, size_t t_max_resp_len)
{
    mem_read_cb = NULL;

    region.size = Swap32(*(DWORD*)data) + 16;
    // Read whole region
    T_MEM_ENTRY mem_region_data = {
        .addr = region.addr,
        .size = region.size
    };

    mem_read_cb = region_read_get_data_cb;
    cmd_io_bdm_read_mem_entry(&mem_region_data);
}

static void region_read_next_region()
{
    struct json_object *j_region =
        json_object_array_get_idx(memf_cmd->mem_regions, memf_cmd->mem_regions_it);

    memf_cmd->mem_regions_it++;

    if (j_region == NULL) {
        wprintf(L"No more regions @%d\n", __LINE__);

        json_object_object_add(memf_rsp->obj, "mem_regions", memf_rsp->mem_regions);

        memf_cmd_close(memf_cmd);
        memf_cmd = NULL;

        memf_rsp_close(memf_rsp);
        memf_rsp = NULL;

        return;
    }

    memf_rsp->mem_blocks = json_object_new_array();
    memf_rsp->mem_blocks_it = 0;

    if (!json_object_object_get_ex(j_region, "mem_blocks", &memf_cmd->mem_blocks)) {
        wprintf(L"Can't find memory blocks - mode auto");
        // return NULL;
    } else {
        if (json_object_get_type(memf_cmd->mem_blocks) != json_type_array) {
            wprintf(L"Bad format");
            return;
        }
        memf_cmd->mem_blocks_it = 0;
    }

    /*
     * Get region name & addr
     */
    struct json_object *val;
    if (json_object_object_get_ex(j_region, "name", &val)) {
        region.name = json_object_get_string(val);
    } else {
        region.name = "";
    }

    T_MEM_ENTRY mem_region_len = {
        .addr = 0,  // Filled from json
        .size = 4
    };

    if (json_object_object_get_int(j_region, "addr", &region.addr)) {
        wprintf(L"Element not found @%d", __LINE__);
        return;
    }

    /*
     * Read region length
     */
    mem_region_len.addr = region.addr + 4;
    
    mem_read_cb = region_read_get_len_cb;
    cmd_io_bdm_read_mem_entry(&mem_region_len);
}


void cmd_io_bdm_fread_region(void)
{
    if (memf_cmd != NULL) {
        return;
    }
    memf_cmd = memf_cmd_init(gt_cmd_bdm_fread.fin.pc_str);

    if (memf_rsp) {
        // Close previous response if still opened for any reason
        memf_rsp_close(memf_rsp);
        memf_rsp = NULL;
    }
    memf_rsp = memf_rsp_init(gt_cmd_bdm_fread.fout.pc_str);

    region_read_next_region();
    
    return;
}


void cmd_io_bdm_fwrite(void)
{
}
void cmd_io_loopback(void)
{
    int n_rc;
    DWORD dw_bytes_to_write;

#pragma pack(push, 1)
    struct t_cmd_loopback {
        struct t_cmd_hdr hdr;
        WCHAR  ca_lbs[LOOPBACK_STRING_DATA_LEN];
    } t_cmd = { { 0xD2, 0x41, 0x00 }, { 0 } };
#pragma pack(pop)

    // -----------------------------------------------
    // --- write data from UI command to DEV command
    // ------------------------------------------------
    n_rc = TRUE;

    size_t t_str_len;
    t_str_len = wcslen(gt_cmd_loopback.lbs.pc_str);
    t_cmd.hdr.dw_len = (t_str_len * 2 + 2);

    memcpy(t_cmd.ca_lbs, gt_cmd_loopback.lbs.pc_str, t_cmd.hdr.dw_len);
    // ----------------------------------------
    // --- Initiate data transfer to MCU
    // ----------------------------------------
    dw_bytes_to_write = sizeof(t_cmd.hdr) + t_cmd.hdr.dw_len;

    dev_tx(dw_bytes_to_write, (BYTE*)&t_cmd, L"LOOPBACK");
    return;

}

#if  0   // Command template                    
void cmd_io_???()
{

    int n_rc;
    DWORD dw_bytes_to_write;

    #pragma pack(1)
    struct t_cmd_???{
        struct t_cmd_hdr hdr;
        BYTE/WORD    uc/us_???;
    } t_cmd = { {0x??, 0x??}, 0x?? ... };

    // -----------------------------------------------
    // --- write data from UI command to DEV command
    // ------------------------------------------------
    n_rc = TRUE;

    // Verify command arguments
    // ...

    t_cmd.??? = (WORD/BYTE)gt_cmd_???.???.dw_val;

    if (!n_rc)
        goto cmd_io_???_error;
    
    // ----------------------------------------
    // --- Initiate data transfer to MCU
    // ----------------------------------------
    dw_bytes_to_write = sizeof(t_cmd.hdr) + t_cmd.hdr.uc_len;

    dev_tx(dw_bytes_to_write, (BYTE*)&t_cmd, L"???");
    return;

cmd_io_???_error:

    wprintf(L"command error : ???\n");
    return;

}

#endif // #if 0
                       
