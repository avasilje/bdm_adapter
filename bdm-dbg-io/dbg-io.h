#ifndef __DBG_IO_H__
#define __DBG_IO_H__

void dev_tx (DWORD dw_bytes_to_write, BYTE *pc_cmd, const WCHAR *pc_cmd_name);

typedef struct dev_rsp_s
{
    BYTE b_cmd;
    DWORD dw_len;
    BYTE *pb_data;
} T_DEV_RSP;

#pragma pack(push, 1)
struct rsp_mem_write_s {
    DWORD addr;
    DWORD len;
};
#pragma pack(pop)

#endif // !__DBG_IO_H__
