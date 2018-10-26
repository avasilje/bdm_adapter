#ifndef __IO_AVRB_DEV_CMD_H__
#define __IO_AVRB_DEV_CMD_H__

/* Set of function to be called from IO module for each device specific cmd */
void cmd_io_sign(void);
void cmd_io_bdm_break(void);
void cmd_io_bdm_go(void);
void cmd_io_bdm_reset(void);
void cmd_io_bdm_read(void);
void cmd_io_bdm_write8(void);
void cmd_io_bdm_write16(void);
void cmd_io_bdm_fread(void);
void cmd_io_bdm_fwrite(void);
void cmd_io_loopback(void);

#endif

