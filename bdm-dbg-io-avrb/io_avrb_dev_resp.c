/***C*********************************************************************************************
**
** SRC-FILE     :   io_devresp.cpp
**                                        
** PROJECT      :   Coldfire BDM
**                                                                
** SRC-VERSION  :   
**              
** DATE         :   
**
** AUTHOR       :   
**
** DESCRIPTION  :   Device's responses process routines
**                  Typicall function parses incoming data packet from the device, convert it to 
**                  user friendly format and sends to user interface (IO pipe) as a text string.
**                  
** COPYRIGHT    :   
**
****C*E******************************************************************************************/
#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#include "cmd_lib.h"
#include "dbg-io.h"
#include "json-c/json.h"
#include "memf.h"

T_MEM_FILE *memf_rsp;

void memf_entry_printf (T_MEM_ENTRY *pt_mem_entry) 
{
    wprintf(L"%hs (%d)", 
        pt_mem_entry->name,
        pt_mem_entry->size);

    size_t idx = 0;
    int col_cnt = 0;
    int cols = 
        pt_mem_entry->width == 1 ? 16 :
        pt_mem_entry->width == 2 ?  8 :
        pt_mem_entry->width == 4 ?  4 : 16;
    DWORD curr_addr = pt_mem_entry->addr;

    WCHAR *wprint_format = L"%d";

    if (0 == _stricmp(pt_mem_entry->format, "x")) {
        switch (pt_mem_entry->width) {
            case 1: wprint_format = L"0x%02X"; break;
            case 2: wprint_format = L"0x%04X"; break;
            case 4: wprint_format = L"0x%08X"; break;
        }

    } else if (0 == _stricmp(pt_mem_entry->format, "d")) {
        switch (pt_mem_entry->width) {
            case 1: wprint_format = L" %-3d"; break;
            case 2: wprint_format = L" %-5d"; break;
            case 4: wprint_format = L" %d"; break;
        }
    }

    while(idx < pt_mem_entry->size) {
        DWORD n;
        if (col_cnt == 0) {
            wprintf(L"\n  %08X: ", curr_addr);
        }

        switch (pt_mem_entry->width) {
            case 1: n =  (BYTE)&pt_mem_entry->data[idx]; break;
            case 2: n =  (WORD)&pt_mem_entry->data[idx]; break;
            case 4: n = (DWORD)&pt_mem_entry->data[idx]; break;
        }
        idx += pt_mem_entry->width;
        curr_addr += pt_mem_entry->width;

        col_cnt ++;
        if (col_cnt == cols) col_cnt = 0;
         
        wprintf(wprint_format, n);
    }
    wprintf(L"\n");

}

void proceed_mem_read_resp (T_DEV_RSP *pt_dev_rsp, WCHAR *pc_cmd_resp_out, size_t t_max_resp_len)
{
#pragma pack(push, 1)
    struct t_rsp_read {
        DWORD addr;
        DWORD len;
        BYTE data[0];
    } *pt_rsp  = (struct t_rsp_read *)pt_dev_rsp->pb_data;
#pragma pack(pop)

    T_MEM_ENTRY *pt_mem_entry;

    if (memf_rsp == NULL) {
        wprintf(L"Can't proceed @%d", __LINE__);
    }

    pt_mem_entry = &memf_rsp->t_entry_template;
    if (!pt_mem_entry->is_valid) {
        wprintf(L"Can't proceed @%d", __LINE__);
    }
    
    if (!memf_rsp->obj) {
        wprintf(L"Can't proceed @%d", __LINE__);
    }

    /*
     *  {
     *     "name"  : "stat0",
     *     "addr"  : "0x12345",
     *     "size"  : "0x123456",
     *     "width" : 2,
     *     "format": "d"
     *  },
     */

    json_object *j_mem_block = json_object_new_object();
    
    json_object_object_add(j_mem_block, "name"  , json_object_new_string(pt_mem_entry->name));
    json_object_object_add(j_mem_block, "addr"  , json_object_new_int(pt_mem_entry->addr));  // Q: Get it from response or merge with existing?
    json_object_object_add(j_mem_block, "size"  , json_object_new_int(pt_mem_entry->size));
    json_object_object_add(j_mem_block, "width" , json_object_new_int(pt_mem_entry->width));
    json_object_object_add(j_mem_block, "format", json_object_new_string(pt_mem_entry->format));
    
    json_object *j_data_arr = json_object_new_array();
    json_object_object_add(j_mem_block, "data", j_data_arr);

    size_t idx = 0;

    char *j_data_format = "%d";

    if (0 == _stricmp(pt_mem_entry->format, "x")) {
        switch (pt_mem_entry->width) {
            case 1: j_data_format = "0x%02X"; break;
            case 2: j_data_format = "0x%04X"; break;
            case 4: j_data_format = "0x%08X"; break;
        }

    }

    while(idx < pt_rsp->len) {
        char tmp_str[64];
        DWORD n;

        // Q: byte swap?
        switch (pt_mem_entry->width) {
            case 1: n = (BYTE) &pt_rsp->data[idx]; break;
            case 2: n = (WORD) &pt_rsp->data[idx]; break;
            case 4: n = (DWORD)&pt_rsp->data[idx]; break;
        }
        idx += pt_mem_entry->width;

        sprintf_s(tmp_str, sizeof(tmp_str), j_data_format, n);
        json_object_array_add(j_data_arr, json_object_new_string(tmp_str));
    }
  
    json_object *j_mem_block_arr = json_object_object_get(memf_rsp->obj, "mem_blocks");
    if (j_mem_block_arr) {
        json_object_array_add(j_mem_block_arr, j_mem_block);
    }

    pt_mem_entry->data = pt_rsp->data;
    memf_entry_printf(pt_mem_entry);

    pt_mem_entry->is_valid = 0;
    
}


void proceed_loopback_resp (T_DEV_RSP *pt_resp, WCHAR *pc_cmd_resp_out, size_t t_max_resp_len)
{
    swprintf(pc_cmd_resp_out, t_max_resp_len, L"Response: %s\n", (WCHAR*)pt_resp->pb_data);
}

void proceed_dev_sign_resp (T_DEV_RSP *pt_resp, WCHAR *pc_cmd_resp_out, size_t t_max_resp_len)
{
    BYTE *pb_data;
    WCHAR   ca_sign[256] = L"";

    pb_data = pt_resp->pb_data;

    // Convert signature to Multibyte string
    size_t n_chars;
    mbstowcs_s(
            &n_chars,
            ca_sign,
            sizeof(ca_sign)>>1,     // sign len
            (char*)(&pb_data[2]),   // sign text
            _TRUNCATE);

    ca_sign[n_chars-1] = 0;   // assure null termination

    swprintf(pc_cmd_resp_out, t_max_resp_len, L"Device signature:\n%s. Hardware version %d.%d\n", ca_sign, pb_data[0], pb_data[1]);

}

int dev_response_processing (T_DEV_RSP *pt_resp, WCHAR *pc_cmd_resp_out, size_t t_max_resp_len)
{

    pc_cmd_resp_out[0] = L'\0';

    switch (pt_resp->b_cmd)
    {
    case 0x11: // Device signature
        proceed_dev_sign_resp(pt_resp, pc_cmd_resp_out, t_max_resp_len);
        break;
    case 0x17: // Loopback. Just return input as is in WCHAR format
        proceed_loopback_resp(pt_resp, pc_cmd_resp_out, t_max_resp_len);
        break;

    case 0x22: // Memory read
        proceed_mem_read_resp(pt_resp, pc_cmd_resp_out, t_max_resp_len);
        break;
    default:
        wprintf(L"Shit happens");
        while (1);
    }

    wprintf(L"UI INIT <-- IO : CMD Resp: %s\n", pc_cmd_resp_out);

    return TRUE;
}