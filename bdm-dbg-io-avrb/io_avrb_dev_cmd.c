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

#include "json-c/json.h"

#include "cmd_lib.h"
#include "dbg-io.h"
#include "io_avrb_ui_cmd.h"
#include "io_avrb_dev_cmd.h"

#include "memf.h"

T_MEM_FILE *memf_cmd;

#pragma pack(push, 1)
struct t_cmd_hdr {
    BYTE    uc_cmd;
    BYTE    uc_len;
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
    dw_bytes_to_write = sizeof(t_cmd.hdr) + t_cmd.hdr.uc_len;

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
    } t_cmd = { {0x16, 0x04}, 0x00000000 };
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
        dw_bytes_to_write = sizeof(t_cmd.hdr) + t_cmd.hdr.uc_len;

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
    struct t_cmd_bdm_break {
        struct t_cmd_hdr hdr;
    } t_cmd = { {0x17, 0x00} };
    #pragma pack(pop)

    // -----------------------------------------------
    // --- write data from UI command to DEV command
    // ------------------------------------------------
    n_rc = TRUE;

    if (n_rc) {
        // ----------------------------------------
        // --- Initiate data transfer to MCU
        // ----------------------------------------
        dw_bytes_to_write = sizeof(t_cmd.hdr) + t_cmd.hdr.uc_len;

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
    struct t_cmd_bdm_break {
        struct t_cmd_hdr hdr;
    } t_cmd = { {0x18, 0x00} };
    #pragma pack(pop)

    // -----------------------------------------------
    // --- write data from UI command to DEV command
    // ------------------------------------------------
    n_rc = TRUE;

    if (n_rc) {
        // ----------------------------------------
        // --- Initiate data transfer to MCU
        // ----------------------------------------
        dw_bytes_to_write = sizeof(t_cmd.hdr) + t_cmd.hdr.uc_len;

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
    } t_cmd = { {0x21, 0x00}, 0, 0, {0}};
    #pragma pack(pop)

    // -----------------------------------------------
    // --- write data from UI command to DEV command
    // ------------------------------------------------
    n_rc = TRUE;

    t_cmd.hdr.uc_len = pt_write_bdm->data.pt_raw_buff->b_len;        // !!! check length

    memcpy(t_cmd.ba_buff, pt_write_bdm->data.pt_raw_buff->pb_buff, t_cmd.hdr.uc_len);

    // ----------------------------------------
    // --- Initiate data transfer to MCU
    // ----------------------------------------
    dw_bytes_to_write = sizeof(t_cmd.hdr) + t_cmd.hdr.uc_len;

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

void cmd_io_bdm_read_mem(T_MEM_ENTRY *pt_mem_entry)
{
    wprintf(L" Read mem: %hs - %08X : %d (%d%hs)", 
        pt_mem_entry->name,
        pt_mem_entry->addr,
        pt_mem_entry->size,
        pt_mem_entry->width,
        pt_mem_entry->format);

    int n_rc;
    DWORD dw_bytes_to_write;

#pragma pack(push, 1)
    struct t_cmd_read {
        struct t_cmd_hdr hdr;
        DWORD addr;
        DWORD len;
//        BYTE  id;             // ID is used to combine responses in single output stream/file
    } t_cmd = { { 0x22, 0x00 }, 0, 0 };
#pragma pack(pop)

    // -----------------------------------------------
    // --- write data from UI command to DEV command
    // ------------------------------------------------
    n_rc = TRUE;

    t_cmd.hdr.uc_len = (BYTE)( sizeof(t_cmd) - sizeof(t_cmd.hdr) );

    t_cmd.addr = pt_mem_entry->addr;
    t_cmd.len = pt_mem_entry->size;

    // ----------------------------------------
    // --- Initiate data transfer to MCU
    // ----------------------------------------
    dw_bytes_to_write = sizeof(t_cmd.hdr) + t_cmd.hdr.uc_len;
    
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

        cmd_io_bdm_read_mem(&t_mem_cmd_entry);

    } while (0);

    if (err) {
        memf_cmd_close(memf_cmd);
        memf_cmd_close(memf_rsp);
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
    cmd_io_bdm_fread_cont();
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
    } t_cmd = { { 0x41, 0x00 }, { 0 } };
#pragma pack(pop)

    // -----------------------------------------------
    // --- write data from UI command to DEV command
    // ------------------------------------------------
    n_rc = TRUE;

    size_t t_str_len;
    t_str_len = wcslen(gt_cmd_loopback.lbs.pc_str);
    t_cmd.hdr.uc_len = (BYTE)(t_str_len * 2 + 2);

    memcpy(t_cmd.ca_lbs, gt_cmd_loopback.lbs.pc_str, t_cmd.hdr.uc_len);
    // ----------------------------------------
    // --- Initiate data transfer to MCU
    // ----------------------------------------
    dw_bytes_to_write = sizeof(t_cmd.hdr) + t_cmd.hdr.uc_len;

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
                       
