#ifndef __IO_AVRB_UI_CMD_H__
#define __IO_AVRB_UI_CMD_H__

//----------------------------------------------------------
struct s_cmd_sign{
    T_UI_CMD_FIELD   eomsg;
};

extern struct s_cmd_sign gt_cmd_dev_sign;

//----------------------------------------------------------
struct s_cmd_bdm_break{
    T_UI_CMD_FIELD   addr;
    T_UI_CMD_FIELD   eomsg;
};

extern struct s_cmd_bdm_break gt_cmd_bdm_break;

//----------------------------------------------------------
struct s_cmd_bdm_go{
    T_UI_CMD_FIELD   eomsg;
};

extern struct s_cmd_bdm_go gt_cmd_bdm_go;

//----------------------------------------------------------
struct s_cmd_bdm_reset{
    T_UI_CMD_FIELD   eomsg;
};

//----------------------------------------------------------
struct s_cmd_bdm_read {
    T_UI_CMD_FIELD   addr;
    T_UI_CMD_FIELD   len;
    T_UI_CMD_FIELD   eomsg;
};

extern struct s_cmd_bdm_read gt_cmd_bdm_read;

//----------------------------------------------------------

#define BDM_WRITE_DATA_LEN 240
struct s_cmd_bdm_write{
    T_UI_CMD_FIELD   addr;
    T_UI_CMD_FIELD   data;
    T_UI_CMD_FIELD   eomsg;
};

extern struct s_cmd_bdm_write gt_cmd_bdm_write8;
extern struct s_cmd_bdm_write gt_cmd_bdm_write16;

//----------------------------------------------------------

#define LOOPBACK_STRING_DATA_LEN 128

struct s_cmd_loopback{
    T_UI_CMD_FIELD   lbs;
    T_UI_CMD_FIELD   eomsg;
};

extern struct s_cmd_loopback gt_cmd_loopback;

#if 0
//----------------------------------------------------------
struct t_cmd_???_tag{
    T_UI_CMD_FIELD   ???;
    T_UI_CMD_FIELD   eomsg;
};

extern struct t_cmd_???_tag gt_cmd_???;
#endif // #if 0


#endif // !__IO_AVRB_UI_CMD_H__

