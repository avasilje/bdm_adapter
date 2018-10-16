/***C*********************************************************************************************
**
** SRC-FILE     :   io_avrb_ui_cmd.cpp
**
** PROJECT      :   Coldfire BDM
**
** AUTHOR       :   AV
**
** DESCRIPTION  :   Set of UI cmd for AVR Backdoor
**
** COPYRIGHT    :
**
****C*E******************************************************************************************/
#include <windows.h>
#include <wchar.h>
#include "cmd_lib.h"
#include "io_avrb_ui_cmd.h"
#include "io_avrb_dev_cmd.h"

//---------------------------------------------------------------------------
s_cmd_sign gt_cmd_sign = {
    {NULL, CFT_LAST, 0, 0}
};

//---------------------------------------------------------------------------
s_cmd_bdm_break gt_cmd_bdm_break = {
    {L"ADDR"  ,  CFT_NUM,      0,           0},
    {NULL, CFT_LAST, 0, 0}
};


//---------------------------------------------------------------------------
s_cmd_bdm_go gt_cmd_bdm_go = {
    {NULL, CFT_LAST, 0, 0}
};

//---------------------------------------------------------------------------
s_cmd_bdm_reset gt_cmd_bdm_reset = {
    {NULL, CFT_LAST, 0, 0}
};

//---------------------------------------------------------------------------
s_cmd_bdm_read gt_cmd_bdm_read = {
    {L"ADDR"  ,  CFT_NUM,      0,           0},
    {L"LEN"   ,  CFT_NUM,      0,           0},
    {NULL, CFT_LAST, 0, 0}
};

//---------------------------------------------------------------------------

BYTE raw_buff[BDM_WRITE_DATA_LEN] = {0};
T_RAW_BUF t_bdm_write_buff {
    0,
    &raw_buff[0]
};

s_cmd_bdm_write gt_cmd_bdm_write8 = {
    {L"ADDR"  ,  CFT_NUM,      0,           0},
    {L"BUFF"  ,  CFT_RAW8,     BDM_WRITE_DATA_LEN,  (DWORD)&t_bdm_write_buff},
    {NULL, CFT_LAST, 0, 0}
};

s_cmd_bdm_write gt_cmd_bdm_write16 = {
    {L"ADDR"  ,  CFT_NUM,      0,           0},
    {L"BUFF"  ,  CFT_RAW16,    BDM_WRITE_DATA_LEN,  (DWORD)&t_bdm_write_buff},
    {NULL, CFT_LAST, 0, 0}
};

//---------------------------------------------------------------------------

WCHAR ca_lbs_str[LOOPBACK_STRING_DATA_LEN] = L"--LOOPBACK_TEST_STRING--";
s_cmd_loopback gt_cmd_loopback = {
    {L"STR"    ,  CFT_TXT,      LOOPBACK_STRING_DATA_LEN,  (DWORD)ca_lbs_str},
    {NULL, CFT_LAST, 0, 0}
};

#if 0
//---------------------------------------------------------------------------
t_cmd_???_tag gt_cmd_??? = {
    {L"???"      ,  CFT_NUM,      0,           0},
    {NULL, CFT_LAST, 0, 0}
};


//---------------------------------------------------------------------------
#define xxx_WR_DATA_LEN 128
WCHAR ca_xxx_wr_data[xxx_WR_DATA_LEN];
t_cmd_xxx_wr_tag gt_cmd_xxx_wr = {
    {L"DATA"    ,  CFT_TXT,      xxx_WR_DATA_LEN,  (DWORD)ca_xxx_wr_data},
    {NULL, CFT_LAST, 0, 0}
};

#endif // #if 0


T_UI_CMD gta_io_ui_cmd[] = {
        { L"SIGN",     (T_UI_CMD_FIELD*)&gt_cmd_sign,        (void*)cmd_io_sign      },
        { L"BR",       (T_UI_CMD_FIELD*)&gt_cmd_bdm_break,   (void*)cmd_io_bdm_break },
        { L"GO",       (T_UI_CMD_FIELD*)&gt_cmd_bdm_go,      (void*)cmd_io_bdm_go    },
        { L"RST",      (T_UI_CMD_FIELD*)&gt_cmd_bdm_reset,   (void*)cmd_io_bdm_reset },
        { L"RD",       (T_UI_CMD_FIELD*)&gt_cmd_bdm_read,    (void*)cmd_io_bdm_read  },
        { L"WR8",      (T_UI_CMD_FIELD*)&gt_cmd_bdm_write8,  (void*)cmd_io_bdm_write8  },
        { L"WR16",     (T_UI_CMD_FIELD*)&gt_cmd_bdm_write16, (void*)cmd_io_bdm_write16 },
        { L"LOOPBACK", (T_UI_CMD_FIELD*)&gt_cmd_loopback,  (void*)cmd_io_loopback  },
        { 0, 0 }
};


WCHAR gca_pipe_name[] = L"\\\\.\\pipe\\io_avrb";
WCHAR gca_ui_init_str[] = L"AVRB UI";


//---------------------------------------------------------------------------


