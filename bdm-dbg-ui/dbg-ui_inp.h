/***C*********************************************************************************************
**
** SRC-FILE     :   dbg_ui_inp.h
**                                        
** PROJECT      :   CMD user interface
**                                                                
** SRC-VERSION  :   
**              
** DATE         :   
**
** AUTHOR       :   AV
**
** DESCRIPTION  :   Interface file for collateral UI functions
**                  
** COPYRIGHT    :   
**
****C*E******************************************************************************************/
#ifndef _CMD_PROC_CMD_PROC_INP_H
#define _CMD_PROC_CMD_PROC_INP_H

#define CMD_LINE_LENGTH 1024

typedef struct cmd_proc_tag{
    void    *pv_cmd_line_info;
    int     n_cur_pos;
    int     n_flag;
    WCHAR   ca_cmd_line[CMD_LINE_LENGTH];

    FILE    *pf_hist;
    void    *pv_cl_hist_buff;
    int     n_cl_hist_size;

} T_CMD_PROC_INFO;

#define CMD_LINE_MODE       1
#define SEND_COMMAND        5
#define SHOW_OPTIONS_HELP   8

//void show_options_help(T_UI_CMD *pt_cmd);

void show_cmd_help(T_UI_CMD *pt_cmd_lib_arg);
void show_options_help(void);

void *init_cmd_proc(WCHAR *pc_hist_filename);
void close_cmd_proc(void *pv_info);

void cmd_proc_prompt(void *pv_cmd_proc_info);
WCHAR* cmd_keys_pressed(void *pv_cmd_info, int *pn_prompt_restore, T_UI_CMD *pt_cmd);

#endif _CMD_PROC_CMD_PROC_INP_H