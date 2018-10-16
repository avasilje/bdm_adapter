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
#include "cmd_lib.h"
#include "dbg-io.h"
#include "io_avrb_ui_cmd.h"
#include "io_avrb_dev_cmd.h"

#pragma pack(1)
struct t_cmd_hdr {
    BYTE    uc_cmd;
    BYTE    uc_len;
};


void cmd_io_sign (void)
{
    int n_rc;
    DWORD dw_byte_to_write;

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
    dw_byte_to_write = sizeof(t_cmd.hdr) + t_cmd.hdr.uc_len;

    dev_tx(dw_byte_to_write, (BYTE*)&t_cmd, L"SIGN");
    return;

cmd_sign_error:

    wprintf(L"command error : DEV_SIGN\n");
    return;

}

void cmd_io_bdm_break (void)
{
    int n_rc;
    DWORD dw_byte_to_write;

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
        dw_byte_to_write = sizeof(t_cmd.hdr) + t_cmd.hdr.uc_len;

        dev_tx(dw_byte_to_write, (BYTE*)&t_cmd, L"BREAK");
        return;
    }

    wprintf(L"command error : BREAK\n");
    return;

}

void cmd_io_bdm_go (void)
{
    int n_rc;
    DWORD dw_byte_to_write;

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
        dw_byte_to_write = sizeof(t_cmd.hdr) + t_cmd.hdr.uc_len;

        dev_tx(dw_byte_to_write, (BYTE*)&t_cmd, L"GO");
        return;
    }

    wprintf(L"command error : GO\n");
    return;

}

void cmd_io_bdm_reset (void)
{
    int n_rc;
    DWORD dw_byte_to_write;

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
        dw_byte_to_write = sizeof(t_cmd.hdr) + t_cmd.hdr.uc_len;

        dev_tx(dw_byte_to_write, (BYTE*)&t_cmd, L"RESET");
        return;
    }

    wprintf(L"command error : RESET\n");
    return;
}

void cmd_io_bdm_read (void)
{

}


void cmd_io_bdm_write (struct s_cmd_bdm_write *pt_write_bdm )
{
    int n_rc;
    DWORD dw_byte_to_write;

    #pragma pack(push, 1)
    static struct t_cmd_bdm_write {
        struct t_cmd_hdr hdr;
        DWORD addr;
        DWORD len;
        BYTE  ba_buff[BDM_WRITE_DATA_LEN];
    } t_cmd = {{0x21, 0x00}, {0}};
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
    dw_byte_to_write = sizeof(t_cmd.hdr) + t_cmd.hdr.uc_len;

    dev_tx(dw_byte_to_write, (BYTE*)&t_cmd, L"LOOPBACK");
    return;
}

void cmd_io_bdm_write8 (void)
{
    cmd_io_bdm_write (&gt_cmd_bdm_write8);
}

void cmd_io_bdm_write16 (void)
{
    cmd_io_bdm_write (&gt_cmd_bdm_write16);
}

void cmd_io_loopback (void)
{
    int n_rc;
    DWORD dw_byte_to_write;

    #pragma pack(1)
    struct t_cmd_loopback {
        struct t_cmd_hdr hdr;
        WCHAR  ca_lbs[LOOPBACK_STRING_DATA_LEN];
    } t_cmd = {{0x17, 0x00}, {0}};

    // -----------------------------------------------
    // --- write data from UI command to DEV command
    // ------------------------------------------------
    n_rc = TRUE;

    size_t t_str_len;
    t_str_len = wcslen(gt_cmd_loopback.lbs.pc_str);
    t_cmd.hdr.uc_len = t_str_len*2 + 2;

    memcpy(t_cmd.ca_lbs, gt_cmd_loopback.lbs.pc_str, t_cmd.hdr.uc_len);
    // ----------------------------------------
    // --- Initiate data transfer to MCU
    // ----------------------------------------
    dw_byte_to_write = sizeof(t_cmd.hdr) + t_cmd.hdr.uc_len;

    dev_tx(dw_byte_to_write, (BYTE*)&t_cmd, L"LOOPBACK");
    return;

}

#if  0   // Command template                    
void cmd_io_???()
{

    int n_rc;
    DWORD dw_byte_to_write;

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
    dw_byte_to_write = sizeof(t_cmd.hdr) + t_cmd.hdr.uc_len;

    dev_tx(dw_byte_to_write, (BYTE*)&t_cmd, L"???");
    return;

cmd_io_???_error:

    wprintf(L"command error : ???\n");
    return;

}

#endif // #if 0
                       
