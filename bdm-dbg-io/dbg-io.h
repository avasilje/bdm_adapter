#ifndef __DBG_IO_H__
#define __DBG_IO_H__

void dev_tx (DWORD dw_bytes_to_write, BYTE *pc_cmd, const WCHAR *pc_cmd_name);

typedef struct dev_rsp_s
{
    BYTE b_cmd;
    WORD s_len;
    BYTE *pb_data;
} T_DEV_RSP;


#endif // !__DBG_IO_H__
