/***C*********************************************************************************************
**
** SRC-FILE     :   cmd_lib.cpp
**                                        
** PROJECT      :   BTCAR
**                                                                
** SRC-VERSION  :   
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

#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#include "cmd_lib.h"

//---------------------------------------------------------------------------

T_UI_CMD* lookup_cp_cmd(WCHAR *pc_cmd_arg, T_UI_CMD *pt_cmd_lib){

    while(pt_cmd_lib->pc_name){
        if (_wcsicmp(pc_cmd_arg, pt_cmd_lib->pc_name) == 0){
            break;
        }
        pt_cmd_lib++;
    } // end of command scan while

    return pt_cmd_lib;
}

int update_cmd_int(WCHAR *pc_cmd_arg, T_UI_CMD_FIELD *pt_fields, int n_update){

    int  n_rc;
    int  n_value;
    WCHAR  *pc_cmd, *pc_value;
    int n_skip_tokenization_fl;
    T_UI_CMD_FIELD *pt_curr_field;

    n_rc = TRUE;

    if (!(*pc_cmd_arg)) return n_rc;

    pc_cmd = wcstok(pc_cmd_arg, L" ");
    
    n_skip_tokenization_fl = 0;

    while(pc_cmd){
        if (pc_cmd[0] != L'-') {
            fwprintf(stderr, L"Command field name must follow '-' symbol\n");
            n_rc = FALSE;
            break;
        }else {
            pc_cmd++; // Move pointer over '-'
        }

        // Scan fields table and find cmd field in fields table
        pt_curr_field = pt_fields;

        n_rc = FALSE;
        while(pt_curr_field->pc_name){
            if (_wcsicmp(pt_curr_field->pc_name, pc_cmd) == 0){
                // Field name found here

                // Get Field value
                pc_value = wcstok(NULL, L" ");
                if (pc_value ==  NULL){
                    fwprintf(stderr, L"Value not specified \n");
                    break;
                }

                // Field name was found and valu exist

                // Update field if requried
                switch (pt_curr_field->e_type){
                    case CFT_NUM: 
                    {
                        if (swscanf_s( pc_value, L"%i", &n_value) != 1){
                            fwprintf(stderr, L"Cannot recognize value %s \n", pc_value);
                            break;
                        }
                        
                        if (n_update) {
                            pt_curr_field->dw_val = (DWORD)n_value;
                        }
                        break;
                    }
                    case CFT_TXT: 
                    {
                        WCHAR *pc_dst = pt_curr_field->pc_str;
                        int n_dst_size = pt_curr_field->n_max_len;

                        while(pc_value){
                            if (n_update){
                                wcsncpy_s(pc_dst, n_dst_size, pc_value, _TRUNCATE);
                            }
                            pc_dst += wcslen(pc_value);
                            n_dst_size -= wcslen(pc_value);

                            pc_value = wcstok(NULL, L" ");

                            // check is another token started
                            if (pc_value && pc_value[0] == L'-') 
                            {
                                n_skip_tokenization_fl = TRUE;
                                pc_cmd = pc_value;
                                break;
                            }

                            if (pc_value && n_dst_size > 0){
                                *pc_dst++ = L' ';
                                n_dst_size--;
                            }
                        }
                        break;
                    }
                    case CFT_RAW8 : 
                    {
                        BYTE *p_raw_dst = pt_curr_field->pt_raw_buff->pb_buff;
                        int n_dst_size = pt_curr_field->n_max_len;

                        while(pc_value) {
                            BYTE raw_val;
                            if (swscanf_s( pc_value, L"%hhd", &raw_val) != 1){
                                fwprintf(stderr, L"Cannot recognize value %s \n", pc_value);
                                break;
                            }

                            if (n_update){
                                memcpy(p_raw_dst, &raw_val, sizeof(raw_val));
                            }
                            n_dst_size -= sizeof(raw_val);
                            p_raw_dst += sizeof(raw_val);

                            pc_value = wcstok(NULL, L" ");

                            // check is another token started
                            if (pc_value && pc_value[0] == L'-') 
                            {
                                n_skip_tokenization_fl = TRUE;
                                pc_cmd = pc_value;
                                break;
                            }
                        }
                        pt_curr_field->pt_raw_buff->dw_len = 
                            (p_raw_dst - pt_curr_field->pt_raw_buff->pb_buff);
                        break;
                    }
                    case CFT_RAW16: 
                    {
                        BYTE *p_raw_dst = pt_curr_field->pt_raw_buff->pb_buff;
                        int n_dst_size = pt_curr_field->n_max_len;

                        while(pc_value) {
                            WORD raw_val;
                            if (swscanf_s( pc_value, L"%hd", &raw_val) != 1){
                                fwprintf(stderr, L"Cannot recognize value %s \n", pc_value);
                                break;
                            }

                            if (n_update){
                                memcpy(p_raw_dst, &raw_val, sizeof(raw_val));
                            }
                            n_dst_size -= sizeof(raw_val);
                            p_raw_dst += sizeof(raw_val);

                            pc_value = wcstok(NULL, L" ");

                            // check is another token started
                            if (pc_value && pc_value[0] == L'-') 
                            {
                                n_skip_tokenization_fl = TRUE;
                                pc_cmd = pc_value;
                                break;
                            }
                        }
                        pt_curr_field->pt_raw_buff->dw_len = 
                            (p_raw_dst - pt_curr_field->pt_raw_buff->pb_buff);
                        break;
                    }
                } // End of field type switch

                n_rc = TRUE;
                break;  // break from field scan

            }// End of field match
            pt_curr_field++;
        } // End of field scan parse
 
        // Check field not found
        if (!pt_curr_field->pc_name){
            fwprintf(stderr, L"Field %s not recognized\n", pc_cmd);
        }

        // l_rc == TRUE only if field found and specified value fit to field
        if (!n_rc) break;

        // Read next field from argument string
        if (!n_skip_tokenization_fl)
            pc_cmd = wcstok(NULL, L" ");

        n_skip_tokenization_fl = FALSE;

    }// End of Command parse while

    return n_rc;
}

T_UI_CMD* decomposite_cp_cmd (WCHAR *pc_cmd_arg, T_UI_CMD *pt_cmd, int n_update)
{

#define MAX_CMD_ARG_LEN 1024

    WCHAR   ca_cmd[MAX_CMD_ARG_LEN];
    WCHAR   *pc_cmd_token, *pc_cmd_line_end;

    int n_rc = FALSE;

    // Make command local copy to use STRTOK
    if (!(*pc_cmd_arg)) return NULL;
    wcscpy_s(ca_cmd, MAX_CMD_ARG_LEN, pc_cmd_arg);

    // Read command
    pc_cmd_token = wcstok(ca_cmd, L" ");
    if (!(*pc_cmd_token)) return NULL;

    pt_cmd = lookup_cp_cmd(pc_cmd_token, pt_cmd);

    // Update fields if command was found
    if (pt_cmd->pt_fields){

        // Calc last element address
        pc_cmd_line_end = &ca_cmd[wcslen(pc_cmd_arg)];

        // Move pointer over command if not end of line
        pc_cmd_token += wcslen(pc_cmd_token);
        if (pc_cmd_token != pc_cmd_line_end) pc_cmd_token ++;

        // Parse command WITH fields update
        n_rc = update_cmd_int(pc_cmd_token, pt_cmd->pt_fields, n_update);
    }else{
        fwprintf(stderr, L"Command not found %s\n", pc_cmd_token);
    }

    return (n_rc ? pt_cmd : NULL);
}

BYTE *add_tlv_raw(BYTE *pb_msg_buff, E_UI_IO_TLV_TYPE e_type, T_RAW_BUF *pt_raw_buff)
{
    size_t b_len = pt_raw_buff->dw_len;

    *pb_msg_buff++ = (BYTE)e_type;          // type
    *pb_msg_buff++ = (BYTE)b_len;           // len in bytes
    if (b_len > 0 && pt_raw_buff && pt_raw_buff->pb_buff) {
        memcpy(pb_msg_buff, pt_raw_buff->pb_buff, b_len);
    }
    return (pb_msg_buff + b_len);
}

BYTE *add_tlv_str(BYTE *pb_msg_buff, E_UI_IO_TLV_TYPE e_type, WCHAR *pc_str)
{
    size_t t_str_len_b;

    t_str_len_b = wcslen(pc_str) * 2 + 2;    // +2 for NUL termination

    if (t_str_len_b > MAXBYTE)
        return pb_msg_buff;

    *pb_msg_buff++ = (BYTE)e_type;          // type
    *pb_msg_buff++ = (BYTE)t_str_len_b;     // len in bytes
    memcpy(pb_msg_buff, pc_str, t_str_len_b);
    return (pb_msg_buff + t_str_len_b);

}

BYTE *add_tlv_dword(BYTE *pb_msg_buff, E_UI_IO_TLV_TYPE e_type, DWORD dw_val)
{
    size_t b_len = sizeof(DWORD);

    *pb_msg_buff++ = (BYTE)e_type;          // type
    *pb_msg_buff++ = (BYTE)b_len;            // len in bytes
    memcpy(pb_msg_buff, &dw_val, b_len);
    return (pb_msg_buff + b_len);
}

BYTE* get_tlv_tl(BYTE *pb_buff, T_UI_IO_TLV *t_tlv)
{
    t_tlv->type = (E_UI_IO_TLV_TYPE)*pb_buff++;
	if (t_tlv->type == UI_IO_TLV_TYPE_NONE)
		return NULL;

    t_tlv->len  = (int)*pb_buff++;
	return pb_buff;
}

BYTE* get_tlv_v_str(BYTE *pb_buff, T_UI_IO_TLV *t_tlv)
{
    t_tlv->val_str =  (WCHAR*)malloc(t_tlv->len);
	if (t_tlv->val_str == NULL)
    {
        wprintf(L"Att:Something wrong @ %d\n", __LINE__);
        return NULL;
    }

    memcpy(t_tlv->val_str, pb_buff, t_tlv->len);
    return (pb_buff + t_tlv->len);
}

BYTE* get_tlv_v_str_n(BYTE *pb_buff, T_UI_IO_TLV *t_tlv, int n_len)
{
    t_tlv->val_str =  (WCHAR*)malloc(n_len);
    memset(t_tlv->val_str, 0, n_len);

	if (t_tlv->val_str == NULL)
    {
        wprintf(L"Att:Something wrong @ %d\n", __LINE__);
        return NULL;
    }

    memcpy(t_tlv->val_str, pb_buff, t_tlv->len);
    return (pb_buff + t_tlv->len);
}

BYTE* get_tlv_v_dword (BYTE *pb_buff, T_UI_IO_TLV *t_tlv)
{
	memcpy(&t_tlv->val_dword, pb_buff, sizeof(DWORD));
	return (pb_buff + sizeof(DWORD));
}

BYTE* get_tlv_v_raw_n(BYTE *pb_buff, T_UI_IO_TLV *t_tlv, int n_len)
{
    if (n_len < 0)
    {
        wprintf(L"Att:Something wrong @ %d\n", __LINE__);
        return NULL;
    }
    t_tlv->val_raw = (T_RAW_BUF*)malloc(sizeof(T_RAW_BUF));
	if (t_tlv->val_raw == NULL)
    {
        wprintf(L"Att:Something wrong @ %d\n", __LINE__);
        return NULL;
    }

    t_tlv->val_raw->pb_buff = (BYTE*)malloc(n_len);

	if (t_tlv->val_raw->pb_buff == NULL)
    {
        wprintf(L"Att:Something wrong @ %d\n", __LINE__);
        return NULL;
    }
    t_tlv->val_raw->dw_len = *pb_buff;
    pb_buff += sizeof(t_tlv->val_raw->dw_len);

    // Skip data copy from pb_buff to TLV as useless
    pb_buff += t_tlv->val_raw->dw_len;

    return pb_buff;
}


