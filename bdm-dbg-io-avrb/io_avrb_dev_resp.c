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

#include "memf.h"

T_MEM_FILE *memf_rsp;
void (*mem_read_cb)(BYTE *data, size_t len, WCHAR *pc_cmd_resp_out, size_t t_max_resp_len);
void (*mem_write_cb)(BYTE *data, size_t len, WCHAR *pc_cmd_resp_out, size_t t_max_resp_len);

int memf_entry_printf (T_MEM_ENTRY *pt_mem_entry, WCHAR *pc_cmd_resp_out, size_t t_max_resp_len) 
{
    int t_max_resp_len_orig = t_max_resp_len;
    int wchars;
    wchars = swprintf(pc_cmd_resp_out, t_max_resp_len, L"%hs (%d)", 
        pt_mem_entry->name,
        pt_mem_entry->size);
    
    pc_cmd_resp_out += wchars;
    t_max_resp_len -= wchars;

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
            case 1: wprint_format = L" %-4d"; break;
            case 2: wprint_format = L" %-6d"; break;
            case 4: wprint_format = L" %-12d"; break;
        }
    }

    while(idx < pt_mem_entry->size) {
        DWORD n;
        if (col_cnt == 0) {
            wchars = swprintf(pc_cmd_resp_out, t_max_resp_len, L"\n  %08X: ", curr_addr);
            pc_cmd_resp_out += wchars;
            t_max_resp_len -= wchars;

        }

        switch (pt_mem_entry->width) {
            case 1: n = *(BYTE*)&pt_mem_entry->data[idx]; break;
            case 2: n = Swap16(*(WORD*)&pt_mem_entry->data[idx]); break;
            case 4: n = Swap32(*(DWORD*)&pt_mem_entry->data[idx]); break;
        }
        idx += pt_mem_entry->width;
        curr_addr += pt_mem_entry->width;

        col_cnt ++;
        if (col_cnt == cols) col_cnt = 0;
         
        wchars = swprintf(pc_cmd_resp_out, t_max_resp_len, wprint_format, n);
        pc_cmd_resp_out += wchars;
        t_max_resp_len -= wchars;
    }
    wchars = swprintf(pc_cmd_resp_out, t_max_resp_len, L"\n");
    pc_cmd_resp_out += wchars;
    t_max_resp_len -= wchars;

    return t_max_resp_len_orig - t_max_resp_len;
}

void rsp_printf(WCHAR **ppc_cmd_resp_out, size_t *pt_max_resp_len, const WCHAR *format, ...)
{

    va_list  va_args;

    va_start(va_args, format);

    int wchars = vswprintf(*ppc_cmd_resp_out, *pt_max_resp_len, format, va_args); 

    va_end(va_args);
    
    if (wchars < 0) {
        wchars = swprintf(*ppc_cmd_resp_out, *pt_max_resp_len, L"%s", L"..."); 
        *pt_max_resp_len = 0;
    } else {
        *ppc_cmd_resp_out += wchars;
        *pt_max_resp_len -= wchars;
    }

}

void memf_entry_printf_sinlge (T_MEM_ENTRY *pt_mem_entry, WCHAR **ppc_cmd_resp_out, size_t *pt_max_resp_len) 
{
    rsp_printf(ppc_cmd_resp_out, pt_max_resp_len, L"%-16hs (%d)", 
        pt_mem_entry->name,
        pt_mem_entry->size);

    int col_cnt = 0;
    int cols = 
        pt_mem_entry->width == 1 ? 16 :
        pt_mem_entry->width == 2 ?  8 :
        pt_mem_entry->width == 4 ?  4 : 16;
    DWORD curr_addr = pt_mem_entry->addr;

    WCHAR *wprint_format1 = L"%d";
    WCHAR *wprint_format2 = L"%d";

    switch (pt_mem_entry->width) {
        case 1: wprint_format1 = L"0x%02X"; break;
        case 2: wprint_format1 = L"0x%04X"; break;
        case 4: wprint_format1 = L"0x%08X"; break;
    }

    switch (pt_mem_entry->width) {
        case 1: wprint_format2 = L" %-4d"; break;
        case 2: wprint_format2 = L" %-6d"; break;
        case 4: wprint_format2 = L" %-12d"; break;
    }

    rsp_printf(ppc_cmd_resp_out, pt_max_resp_len, L"  %08X: ", curr_addr);

    DWORD n;
    switch (pt_mem_entry->width) {
        case 1: n = *(BYTE*)pt_mem_entry->data; break;
        case 2: n = Swap16(*(WORD*)pt_mem_entry->data); break;
        case 4: n = Swap32(*(DWORD*)pt_mem_entry->data); break;
    }
         
    rsp_printf(ppc_cmd_resp_out, pt_max_resp_len, wprint_format1, n);
    rsp_printf(ppc_cmd_resp_out, pt_max_resp_len, wprint_format2, n);

    if (pt_mem_entry->prev_data) {
        DWORD n_prev = Swap32(*(DWORD*)pt_mem_entry->prev_data);
        if (n != n_prev) {
            rsp_printf(ppc_cmd_resp_out, pt_max_resp_len, wprint_format2, n_prev);
            rsp_printf(ppc_cmd_resp_out, pt_max_resp_len, wprint_format2, n-n_prev);
        }
    }

    rsp_printf(ppc_cmd_resp_out, pt_max_resp_len, L"\n");

    return;
}

void proceed_mem_write_resp (T_DEV_RSP *pt_dev_rsp, WCHAR *pc_cmd_resp_out, size_t t_max_resp_len)
{
    struct rsp_mem_write_s *pt_rsp = (struct rsp_mem_write_s *)pt_dev_rsp->pb_data;

    if (mem_write_cb) {
        mem_write_cb((BYTE*)pt_rsp, sizeof(*pt_rsp), pc_cmd_resp_out, t_max_resp_len);
    }
    return;
 }

void proceed_break_resp(pt_resp, pc_cmd_resp_out, t_max_resp_len) {
    wprintf(L"Dummy @%d", __LINE__);
}

void proceed_go_resp(pt_resp, pc_cmd_resp_out, t_max_resp_len) {
    wprintf(L"Dummy @%d", __LINE__);
}

void proceed_reset_resp(pt_resp, pc_cmd_resp_out, t_max_resp_len) {
    wprintf(L"Dummy @%d", __LINE__);                 
}

void proceed_mem_read_resp (T_DEV_RSP *pt_dev_rsp, WCHAR *pc_cmd_resp_out, size_t t_max_resp_len)
{
    char tmp_str[64];

#pragma pack(push, 1)
    struct rsp_read_s {
        DWORD addr;
        DWORD len;
        BYTE data[0];
    } *pt_rsp  = (struct rsp_read_s *)pt_dev_rsp->pb_data;
#pragma pack(pop)

    if (mem_read_cb) {
        mem_read_cb(pt_rsp->data, pt_rsp->len, pc_cmd_resp_out, t_max_resp_len);
    }
    return;

    /*  TODO: Move code below to the call back */
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

    sprintf_s(tmp_str, sizeof(tmp_str), "0x%08X", pt_mem_entry->addr);
    json_object_object_add(j_mem_block, "addr"  , json_object_new_string(tmp_str));  // Q: Get it from response or merge with existing?

    json_object_object_add(j_mem_block, "size"  , json_object_new_int(pt_mem_entry->size));
    json_object_object_add(j_mem_block, "width" , json_object_new_int(pt_mem_entry->width));
    json_object_object_add(j_mem_block, "format", json_object_new_string(pt_mem_entry->format));
    
    json_object *j_data_arr = json_object_new_array();

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
        DWORD n;

        // Q: byte swap?
        switch (pt_mem_entry->width) {
            case 1: n = *(BYTE*) &pt_rsp->data[idx]; break;
            case 2: n = Swap16(*(WORD*) &pt_rsp->data[idx]); break;
            case 4: n = Swap32(*(DWORD*)&pt_rsp->data[idx]); break;
        }
        idx += pt_mem_entry->width;

        sprintf_s(tmp_str, sizeof(tmp_str), j_data_format, n);
        json_object_array_add(j_data_arr, json_object_new_string(tmp_str));
    }
  
    json_object_object_add(j_mem_block, "data", j_data_arr);
    json_object_array_add(memf_rsp->mem_blocks, j_mem_block);

    pt_mem_entry->data = pt_rsp->data;
    memf_entry_printf(pt_mem_entry, pc_cmd_resp_out, t_max_resp_len);

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
    case 0x16:  
        proceed_break_resp(pt_resp, pc_cmd_resp_out, t_max_resp_len);
        break;
    case 0x17:  
        proceed_go_resp(pt_resp, pc_cmd_resp_out, t_max_resp_len);
        break;
    case 0x18:  
        proceed_reset_resp(pt_resp, pc_cmd_resp_out, t_max_resp_len);
        break;
    case 0x21: // Memory write
        proceed_mem_write_resp(pt_resp, pc_cmd_resp_out, t_max_resp_len);
        break;
    case 0x22: // Memory read
        proceed_mem_read_resp(pt_resp, pc_cmd_resp_out, t_max_resp_len);
        break;

    default:
        wprintf(L"Shit happens");
        while (1);
    }

    wprintf(L"UI <-- IO: CMD Resp: %s\n", pc_cmd_resp_out);

    return TRUE;
}
