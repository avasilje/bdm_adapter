/***C*********************************************************************************************
**
** SRC-FILE     :   
**                                        
** PROJECT      :   CMD line 
**                                                                
** DATE         :   
**
** AUTHOR       :   AV
**
** DESCRIPTION  :   
**                  
** COPYRIGHT    :   
**
****C*E******************************************************************************************/
#ifndef __CMD_LIB_H__
#define __CMD_LIB_H__

typedef enum e_cmd_field_type
{
    CFT_NUM,  
    CFT_TXT, 
    CFT_RAW8, 
    CFT_RAW16, 
    CFT_RESERVED = 9, 
    CFT_LAST = -1
}E_CMD_FIELD_TYPE;

typedef struct s_raw_buf {
    DWORD dw_len;
    BYTE *pb_buff;
} T_RAW_BUF;

typedef struct s_cmd_field
{
    WCHAR   *pc_name;
    E_CMD_FIELD_TYPE e_type;

    int     n_max_len;

    union {
        DWORD   dw_val;
        WCHAR   *pc_str;
        T_RAW_BUF *pt_raw_buff;
    };
} T_UI_CMD_FIELD;

typedef struct t_cmd_tag
{
    WCHAR           *pc_name;
    T_UI_CMD_FIELD  *pt_fields;
    void            *ctx;
}T_UI_CMD;

typedef void (F_DEV_CMD)(void);

// --- Functions -----------

T_UI_CMD* lookup_cp_cmd(WCHAR *pc_cmd_arg, T_UI_CMD *pt_cmd_lib);
//int check_cp_cmd(WCHAR *pc_cmd_arg, T_UI_CMD_FIELD *pt_fields);
T_UI_CMD* decomposite_cp_cmd(WCHAR *pc_cmd_arg, T_UI_CMD *pt_cmd, int n_update);

/*
    TLV structure in IO --> UI interface

  \
  |
  +--IO2UI
      |
      +---UI CMD [START, END, CMD]
      |      |
      |      |
      |      +-- START DATA <not impl> 
      |      |
      |      +-- END DATA <not impl> 
      |      |
      |      +-- CMD NAME [STR]
      |             |
      |             +-- FLD NAME[STR]
      |             +-- FLD TYPE
      |             +-- FLD LEN
      |             +-- CMD FLD CNT
      |
      +-- CMD_RESP [MSG STR]
    
*/


// --- UI initialization related stuff ---
typedef enum E_UI_IO_TLV_TYPE_tag
{ 
    UI_IO_TLV_TYPE_NONE = 0,

    UI_IO_TLV_TYPE_UI_CMD       = 0x20,      // UI initialization message. Val = [START, CMD, END]
    UI_IO_TLV_TYPE_CMD_RSP,
    
    UI_IO_TLV_TYPE_CMD_NAME     = 0x30,
    UI_IO_TLV_TYPE_CMD_FLD_CNT,
    UI_IO_TLV_TYPE_CMD_INIT_STR,

    UI_IO_TLV_TYPE_FLD_NAME     = 0x40,
    UI_IO_TLV_TYPE_FLD_TYPE,
    UI_IO_TLV_TYPE_FLD_LEN,
    UI_IO_TLV_TYPE_FLD_VAL
} E_UI_IO_TLV_TYPE;

typedef enum E_IO_UI_UI_CMD_tag
{ 
    IO_UI_UI_CMD_START  = 0x10,     // Start UI initialization
    IO_UI_UI_CMD_CDEF   = 0x20,     // UI CMD definition
    IO_UI_UI_CMD_END    = 0x30,     // End UI initialization
} E_IO_UI_UI_CMD;


typedef struct T_UI_IO_TLV_tag
{
    E_UI_IO_TLV_TYPE    type;
    int                 len;
    union {
        WCHAR               *val_str;
        DWORD               val_dword;
        T_RAW_BUF           *val_raw;
    };
} T_UI_IO_TLV;

BYTE    *add_tlv_str     (BYTE *pb_msg_buff, E_UI_IO_TLV_TYPE e_type, WCHAR *pc_str);
BYTE    *add_tlv_dword   (BYTE *pb_msg_buff, E_UI_IO_TLV_TYPE e_type, DWORD dw_val);
BYTE    *add_tlv_raw     (BYTE *pb_msg_buff, E_UI_IO_TLV_TYPE e_type, T_RAW_BUF *pt_raw_buff);

BYTE    *get_tlv_tl      (BYTE *pb_buff, T_UI_IO_TLV *t_tlv);
BYTE    *get_tlv_v_str   (BYTE *pb_buff, T_UI_IO_TLV *t_tlv);
BYTE    *get_tlv_v_str_n (BYTE *pb_buff, T_UI_IO_TLV *t_tlv, int n_len);
BYTE    *get_tlv_v_raw_n (BYTE *pb_buff, T_UI_IO_TLV *t_tlv, int n_len);
BYTE    *get_tlv_v_dword (BYTE *pb_buff, T_UI_IO_TLV *t_tlv);

#endif // __CMD_LIB_H__