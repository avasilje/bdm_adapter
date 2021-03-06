/***C*********************************************************************************************
**
** SRC-FILE     :   dbg_io.cpp
**                                        
** PROJECT      :   
**                                                                
** SRC-VERSION  :   
**              
** DATE         :   
**
** AUTHOR       :   AV
**
** DESCRIPTION  :   Interconnect between DEVICE & UI 
**                  Provides control logic between UI/IO pipe (commands from user) and DEVICE.
**                  Main functions are pass data between UI and DEVICE, monitoring and 
**                  maintanance of UI and DEVICE connections
**                  
** COPYRIGHT    :   
**
****C*E******************************************************************************************/
#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#include <stdint.h>

#include "FTD2XX.H"
#include "cmd_lib.h"
#include "dbg-io.h"
#include "dbg-io_int.h"

#define DEV_FT_TYPE_UART
// ------------------------- TO DO: move to FW shared header -------------------
#define DBG_BUFF_IDX_DBGL    0
#define DBG_BUFF_IDX_CRSP    1
#define DBG_BUFF_IDX_LAST    2
#define DBG_BUFF_CNT         DBG_BUFF_IDX_LAST

#define DEV_RX_STREAM_HDR_SIZE     1 

// ------------------------- GLOBAL VAR & DEFs --------------------------------
T_IO_FLAGS gt_flags;
HANDLE gha_events[HANDLES_NUM];

HANDLE gh_dump_file;
HANDLE gh_meas_log_file;

#define SIMULATED_PAYLOAD_LEN 10
int g_uart_simulated = 0;
BYTE gdw_uart_simulated_bytes = (SIMULATED_PAYLOAD_LEN + 1 + 4 + 4) + 2;

BYTE gba_uart_simulated_buff[1024] = {
    0xD2,                         //  DBG_BUFF_MARK_RSP
    (SIMULATED_PAYLOAD_LEN + 1 + 4 + 4),        
    0x22,                                        // Memory read
    0x20, 0x00, 0x00, 10,                        // DWORD addr;
    SIMULATED_PAYLOAD_LEN, 0x00, 0x00, 0x00,     // DWORD len
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9
};

// TODO: move to shared area
#pragma pack(push, 1)
typedef struct T_DBGL_HDR_tag {
    uint8_t     uc_mark;
    uint8_t     uc_size;
    uint8_t     uc_fid;
    uint16_t    us_line;
} T_DBGL_HDR;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct T_CRSP_HDR_tag {
    uint8_t      uc_mark;
    uint8_t      uc_act;
    uint32_t     ul_size;
} T_CRSP_HDR;
#pragma pack(pop)

#define CMD_RESP_HEADER_SIZE    sizeof(T_CRSP_HDR)
#define DBG_LOG_HEADER_SIZE     2

T_DEV_RX_STREAM gt_stream_dbgl = {
    DBG_BUFF_IDX_DBGL,
    DBG_LOG_HEADER_SIZE,  
    0,
    {0xCC},
    dev_rx_stream_handler_dbgl
}; // Debug log

T_DEV_RX_STREAM gt_stream_crsp = {
    DBG_BUFF_IDX_CRSP,
    CMD_RESP_HEADER_SIZE,  // Command Response header
    0,
    {0xCC},
    .pf_handler = dev_rx_stream_handler_crsp
}; // Command Response

HANDLE gh_accdbg_file = INVALID_HANDLE_VALUE;

// ---------------------------- IO PIPE ---------------------------------------
HANDLE gh_io_pipe;
OVERLAPPED gt_io_pipe_conn_overlap = { 0 };      // General pipe related event

OVERLAPPED gt_io_pipe_rx_overlap = { 0 };        // Pipe RX related event
OVERLAPPED gt_io_pipe_tx_overlap = { 0 };        // Pipe TX related event

#define IO_PIPE_RX_BUFF_LEN 4096
#define IO_PIPE_TX_BUFF_LEN 4096

WCHAR gca_io_pipe_rx_buff[IO_PIPE_RX_BUFF_LEN];
BYTE  gba_io_pipe_tx_buff[IO_PIPE_TX_BUFF_LEN];


// ---------------------------- DEVICE ----------------------------------------
FT_HANDLE gh_dev;
OVERLAPPED gt_dev_rx_overlapped = { 0 };
OVERLAPPED gt_dev_tx_overlapped = { 0 };

DWORD gdw_dev_bytes_rcv;

int gn_dev_index = 0;
int gn_dev_resp_timeout;

#define RESP_STR_LEN 65536
WCHAR gwca_dev_resp_str[RESP_STR_LEN];

typedef struct T_DEV_RX_tag {
    BYTE                uca_dev_rx_buff[4096];
    T_DEV_RX_STREAM     *pt_curr_stream;
    DWORD               dw_stream_len;          // Number of bytes expected in stream
} T_DEV_RX;

T_DEV_RX  gt_dev_rx = {
    {0},
    NULL,
    0
};

// ---------------------------- UI --------------------------------------------

T_IO_UI gt_io_ui;

// ----------------------------------------------------------------------------


size_t terminate_tlv_list (BYTE *pb_msg_buff)
{
    size_t t_msg_len;

    *pb_msg_buff++ = UI_IO_TLV_TYPE_NONE;

    t_msg_len = pb_msg_buff - &gba_io_pipe_tx_buff[0];

    if (t_msg_len > IO_PIPE_TX_BUFF_LEN || t_msg_len == 0)
    {
        wprintf(L"Att: Something wrong @ %d\n", __LINE__);
        return 0;
    }

    return t_msg_len;
}

int io_ui_init (void)
{
    size_t          t_msg_len;
    BYTE            *pb_msg_buff = &gba_io_pipe_tx_buff[0];

    if (gt_io_ui.pt_curr_cmd != NULL)
    {   
        wprintf(L"Att: Something wrong  %d", __LINE__);
        return TRUE;
    }

    // Initialize current pointer on very first call after UI reset
    gt_io_ui.pt_curr_cmd = gta_io_ui_cmd;

    // Send an INIT START command to UI
    pb_msg_buff = add_tlv_dword(pb_msg_buff, UI_IO_TLV_TYPE_UI_CMD, (DWORD)IO_UI_UI_CMD_START);
    pb_msg_buff = add_tlv_str(pb_msg_buff, UI_IO_TLV_TYPE_CMD_INIT_STR, gca_ui_init_str);

    wprintf(L"UI INIT <-- IO: Init start\n");

    t_msg_len = terminate_tlv_list(pb_msg_buff);
    io_pipe_tx_byte(gba_io_pipe_tx_buff, t_msg_len);

    return TRUE;
}

int io_ui_init_cont (void)
{
    T_UI_CMD        *pt_curr_cmd;
    size_t          t_msg_len;
    BYTE            *pb_msg_buff = &gba_io_pipe_tx_buff[0];

    pt_curr_cmd   = gt_io_ui.pt_curr_cmd;

    if (gt_io_ui.pt_curr_cmd == NULL)
    {
        wprintf(L"Att: Something wrong  %d", __LINE__);
        return TRUE;
    }

    if (!pt_curr_cmd->pc_name)
    {   // All commands processed. Send an INIT END command to UI
        pb_msg_buff = add_tlv_dword(pb_msg_buff, UI_IO_TLV_TYPE_UI_CMD, (DWORD)IO_UI_UI_CMD_END);
        wprintf(L"UI INIT <-- IO: Init end\n");
    }
    else 
    {   // Some commands still there. Continue sending...
        DWORD   dw_fld_cnt;
        T_UI_CMD_FIELD  *pt_curr_field;

        pb_msg_buff = add_tlv_dword(pb_msg_buff, UI_IO_TLV_TYPE_UI_CMD, (DWORD)IO_UI_UI_CMD_CDEF);
        pb_msg_buff = add_tlv_str(pb_msg_buff, UI_IO_TLV_TYPE_CMD_NAME, pt_curr_cmd->pc_name);

        pt_curr_field = pt_curr_cmd->pt_fields;
        dw_fld_cnt = 0;
        while(pt_curr_field->pc_name)
        {
            switch(pt_curr_field->e_type)
            {
            case CFT_NUM:
                dw_fld_cnt++;

                // ATT: Do not change TLV order! NAME, TYPE, LEN, VAL
                pb_msg_buff = add_tlv_str(pb_msg_buff, UI_IO_TLV_TYPE_FLD_NAME, pt_curr_field->pc_name);

                pb_msg_buff = add_tlv_dword(pb_msg_buff, UI_IO_TLV_TYPE_FLD_TYPE, (DWORD)pt_curr_field->e_type);
                pb_msg_buff = add_tlv_dword(pb_msg_buff, UI_IO_TLV_TYPE_FLD_LEN,  (DWORD)pt_curr_field->n_max_len);
                pb_msg_buff = add_tlv_dword(pb_msg_buff, UI_IO_TLV_TYPE_FLD_VAL,  (DWORD)pt_curr_field->dw_val);
                break;

            case CFT_TXT:
                dw_fld_cnt++;

                // ATT: Do not change TLV order! NAME, TYPE, LEN, VAL
                pb_msg_buff = add_tlv_str(pb_msg_buff, UI_IO_TLV_TYPE_FLD_NAME, pt_curr_field->pc_name);

                pb_msg_buff = add_tlv_dword(pb_msg_buff, UI_IO_TLV_TYPE_FLD_TYPE, (DWORD)pt_curr_field->e_type);
                pb_msg_buff = add_tlv_dword(pb_msg_buff, UI_IO_TLV_TYPE_FLD_LEN,  (DWORD)pt_curr_field->n_max_len);
                pb_msg_buff = add_tlv_str  (pb_msg_buff, UI_IO_TLV_TYPE_FLD_VAL,         pt_curr_field->pc_str);
                break;

            case CFT_RAW8:
                dw_fld_cnt++;

                // ATT: Do not change TLV order! NAME, TYPE, LEN, VAL
                pb_msg_buff = add_tlv_str(pb_msg_buff, UI_IO_TLV_TYPE_FLD_NAME, pt_curr_field->pc_name);

                pb_msg_buff = add_tlv_dword(pb_msg_buff, UI_IO_TLV_TYPE_FLD_TYPE, (DWORD)pt_curr_field->e_type);
                pb_msg_buff = add_tlv_dword(pb_msg_buff, UI_IO_TLV_TYPE_FLD_LEN,  (DWORD)pt_curr_field->n_max_len);
                pb_msg_buff = add_tlv_raw  (pb_msg_buff, UI_IO_TLV_TYPE_FLD_VAL,         pt_curr_field->pt_raw_buff);
                break;

            case CFT_RAW16:
                dw_fld_cnt++;

                // ATT: Do not change TLV order! NAME, TYPE, LEN, VAL
                pb_msg_buff = add_tlv_str(pb_msg_buff, UI_IO_TLV_TYPE_FLD_NAME, pt_curr_field->pc_name);

                pb_msg_buff = add_tlv_dword(pb_msg_buff, UI_IO_TLV_TYPE_FLD_TYPE, (DWORD)pt_curr_field->e_type);
                pb_msg_buff = add_tlv_dword(pb_msg_buff, UI_IO_TLV_TYPE_FLD_LEN,  (DWORD)pt_curr_field->n_max_len);
                pb_msg_buff = add_tlv_raw  (pb_msg_buff, UI_IO_TLV_TYPE_FLD_VAL,         pt_curr_field->pt_raw_buff);
                break;

            default:
                // just skip
                break;
            }

            pt_curr_field++;
        } // End of while over all fields

        // Add number of fields as a check sum
        pb_msg_buff = add_tlv_dword(pb_msg_buff, UI_IO_TLV_TYPE_CMD_FLD_CNT, (DWORD)dw_fld_cnt);

        wprintf(L"UI INIT <-- IO: UI CMD: %s\n", pt_curr_cmd->pc_name);

        gt_io_ui.pt_curr_cmd = pt_curr_cmd + 1;

    }

    t_msg_len = terminate_tlv_list(pb_msg_buff);
    io_pipe_tx_byte(gba_io_pipe_tx_buff, t_msg_len);

    return TRUE;
}

int io_ui_cmd_proc (void)
{

    T_UI_CMD   *pt_ui_cmd;

    int n_rc;
    DWORD dw_bytes_transf;

    // Get Overlapped result
    n_rc = GetOverlappedResult(
        gh_io_pipe,                 //  __in   HANDLE hFile,
        &gt_io_pipe_rx_overlap,     //  __in   LPOVERLAPPED lpOverlapped,
        &dw_bytes_transf,           //  __out  LPDWORD lpNumberOfBytesTransferred,
        FALSE                       //  __in   BOOL bWait
        );

    if (!n_rc)
    {
        wprintf(L"Can't read from IO pipe\n");
        gt_flags.io_conn = FL_FALL;
        return FALSE;
    }

    // ----------------------------------
    // --- Process command just read
    // ----------------------------------
    // Add command null termination
    gca_io_pipe_rx_buff[dw_bytes_transf >> 1] = '\0';

    // Lookup command within library & decomposite it's values
    pt_ui_cmd = decomposite_cp_cmd(gca_io_pipe_rx_buff, gta_io_ui_cmd, TRUE);

    // If command not found check for exit or unknow command
    if (!pt_ui_cmd || !pt_ui_cmd->pt_fields)
    {
        wprintf(L"UI --> IO :Unrecognized command %s. Try again\n", gca_io_pipe_rx_buff);
    }
    else
    {
        wprintf(L"UI --> IO : %s\n", gca_io_pipe_rx_buff);

        // Lookup & execute command functor
        if (pt_ui_cmd->ctx)
        {
            F_DEV_CMD *pf_dev_cmd = (F_DEV_CMD*)pt_ui_cmd->ctx;
            pf_dev_cmd();
        }
        else
        {
            wprintf(L"Command not registered %s\n", pt_ui_cmd->pc_name);
        }
    }

    // check command send succesfully, 
    // proceed returned error code, initiate another command read otherwise in case of error
    // ...

    return TRUE;
}

int io_ui_check (void)
{

    // Try to initialize UI over IO pipe if not initialized yet. IO pipe must be connected 
    if ((gt_flags.io_ui == FL_CLR || gt_flags.io_ui == FL_UNDEF) && gt_flags.io_conn != FL_SET)
    {

    }

    // UI init completed
    if (gt_flags.io_ui == FL_RISE)
    {
        gt_flags.io_ui = FL_SET;
    }

    // Something goes wrong during UI init
    if (gt_flags.io_ui == FL_FALL)
    {
        gt_flags.io_ui = FL_CLR;
    }

    // Nothing to do. Everything is OK
    if (gt_flags.io_ui == FL_SET)
    {

    }

    return TRUE;
}

int io_pipe_rx_init (void){

    int n_rc, n_gle;

    n_rc = ReadFile(
        gh_io_pipe,                             //__in         HANDLE hFile,
        gca_io_pipe_rx_buff,                    //__out        LPVOID lpBuffer,
        IO_PIPE_RX_BUFF_LEN,                    //__in         DWORD nNumberOfBytesToRead,
        NULL,                                   //__out_opt    LPDWORD lpNumberOfBytesRead,
        &gt_io_pipe_rx_overlap);                //__inout_opt  LPOVERLAPPED lpOverlapped

    if (!n_rc)
    {
        n_gle = GetLastError();
        if (n_gle != ERROR_IO_PENDING && n_gle != ERROR_IO_INCOMPLETE)
        {
            //wprintf(L"something wrong @ %d. GLE=%d", __LINE__, n_gle);
            gt_flags.io_conn = FL_FALL;
            SetEvent(gha_events[HANDLE_IO_PIPE_CONN]);
            return FALSE;
        }
    }

    return TRUE;

}

int io_pipe_rx_proc (void)
{
    int n_rc;
    DWORD dw_bytes_transf;

    // Get Overlapped result
    n_rc = GetOverlappedResult(
        gh_io_pipe,             //  __in   HANDLE hFile,
        &gt_io_pipe_rx_overlap, //  __in   LPOVERLAPPED lpOverlapped,
        &dw_bytes_transf,       //  __out  LPDWORD lpNumberOfBytesTransferred,
        FALSE                   //  __in   BOOL bWait
        );

    if (!n_rc)
    {
        wprintf(L"Can't read from IO pipe\n");
        gt_flags.io_conn = FL_FALL;
        return FALSE;
    }

    // AV TODO: remove it after TLV wrapping
    if (gca_io_pipe_rx_buff[(dw_bytes_transf >> 1) - 1] != L'\0')
    {
        wprintf(L"Warn: Non terminated msg received @ %d\n", __LINE__);
    }

    // Check message type 
    if (gt_flags.io_ui == FL_CLR && gt_flags.io_conn == FL_SET)
    { 
        wprintf(L"UI --> IO: %s\n", gca_io_pipe_rx_buff);

        if (_wcsicmp(gca_io_pipe_rx_buff, L"INIT") == 0)
        {
            gt_io_ui.pt_curr_cmd = NULL;
            io_ui_init();
        }
        else if (_wcsicmp(gca_io_pipe_rx_buff, L"ACK") == 0)
        {
            io_ui_init_cont();
        }
        else if (_wcsicmp(gca_io_pipe_rx_buff, L"READY") == 0)
        {
            gt_flags.io_ui = FL_RISE;
        }


    }
    else if (gt_flags.io_ui == FL_SET)
    {
        io_ui_cmd_proc();
    }

    return TRUE;
}

int io_pipe_tx_byte (BYTE *pc_io_msg, size_t t_msg_len)
{
    int n_rc, n_gle;

    DWORD dw_n;
    // Write back command response to UI
    n_rc = WriteFile(
        gh_io_pipe,                     //__in         HANDLE hFile,
        pc_io_msg,                      //__in         LPCVOID lpBuffer,
        t_msg_len,                      //__in         DWORD nNumberOfBytesToWrite, // why +2 ??? zero terminating
        &dw_n,                          //__out_opt    LPDWORD lpNumberOfBytesWritten,
        &gt_io_pipe_tx_overlap);        //__inout_opt  LPOVERLAPPED lpOverlapped

    if (!n_rc)
    {
        n_gle = GetLastError();
        if (n_gle != ERROR_IO_PENDING)
        {
            gt_flags.io_conn = FL_FALL;
        }
    }

    return TRUE;
}

int io_pipe_init (void)
{
    gh_io_pipe = CreateNamedPipe(
        gca_pipe_name,                                  //__in      LPCTSTR lpName,
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,      //__in      DWORD dwOpenMode,
        PIPE_TYPE_MESSAGE,                              //__in      DWORD dwPipeMode, 
        2,                                              //__in      DWORD nMaxInstances,
        8 * IO_PIPE_TX_BUFF_LEN,                        //__in      DWORD nOutBufferSize,
        8 * IO_PIPE_RX_BUFF_LEN,                        //__in      DWORD nInBufferSize,
        0,                                              //__in      DWORD nDefaultTimeOut,
        NULL                                            //__in_opt  LPSECURITY_ATTRIBUTES lpSecurityAttributes
        );


    if (gh_io_pipe == INVALID_HANDLE_VALUE){
        wprintf(L"Can't create IO pipe. Error @ %d", __LINE__);
        return FALSE;
    }

    gt_flags.io_conn = FL_CLR;
    gt_flags.io_ui = FL_CLR;

    // Init input buffer
    memset(gca_io_pipe_rx_buff, 0, IO_PIPE_RX_BUFF_LEN);

    // Create IO PIPE data received event
    gha_events[HANDLE_IO_PIPE_RX] = CreateEvent(
        NULL,    // default security attribute 
        FALSE,   // manual-reset event 
        FALSE,   // initial state
        NULL);   // unnamed event object 
    gt_io_pipe_rx_overlap.hEvent = gha_events[HANDLE_IO_PIPE_RX];

    // Creat pipe IO connection event
    gha_events[HANDLE_IO_PIPE_CONN] = CreateEvent(
        NULL,    // default security attribute 
        FALSE,   // manual-reset event 
        FALSE,   // initial state
        NULL);   // unnamed event object 
    gt_io_pipe_conn_overlap.hEvent = gha_events[HANDLE_IO_PIPE_CONN];

    // Trigers connection check at startup. AKA "magic punch"
    SetEvent(gha_events[HANDLE_IO_PIPE_CONN]);

    return TRUE;
}

void io_pipe_close (void)
{
    // Close IO pipe
    if (gh_io_pipe != INVALID_HANDLE_VALUE)
        CloseHandle(gh_io_pipe);

    // Close IO PIPE related events
    if (gha_events[HANDLE_IO_PIPE_RX] != INVALID_HANDLE_VALUE)
        CloseHandle(gha_events[HANDLE_IO_PIPE_RX]);

    if (gha_events[HANDLE_IO_PIPE_CONN] != INVALID_HANDLE_VALUE)
        CloseHandle(gha_events[HANDLE_IO_PIPE_RX]);

    return;
}

int io_pipe_check (void)
{

    int n_rc, n_gle;

    if (gt_flags.io_conn == FL_CLR || gt_flags.io_conn == FL_UNDEF)
    {

        // Wait until IO process connects to pipe
        n_rc = ConnectNamedPipe(gh_io_pipe, &gt_io_pipe_conn_overlap);
        if (n_rc != 0)
        {
            wprintf(L"\nCan't connect to IO pipe. Error @ %d", __LINE__);
            return FALSE;
        }

        n_gle = GetLastError();
        if (n_gle == ERROR_PIPE_CONNECTED)
        {
            gt_flags.io_conn = FL_RISE;
            SetEvent(gha_events[HANDLE_IO_PIPE_CONN]);
            wprintf(L"\nIO pipe %s connected\n", gca_pipe_name);
        }
        else if (n_gle == ERROR_IO_PENDING)
        {
            wprintf(L"\nWaiting IO pipe connection from UI side\n");
        }
        else //(n_gle == ERROR_NO_DATA)
        {
            gt_flags.io_conn = FL_FALL;
            SetEvent(gha_events[HANDLE_IO_PIPE_CONN]);
        }

        return TRUE;
    } // End of Connection CLR

    if (gt_flags.io_conn == FL_SET)
    {
        return TRUE;
    } // End of Connection SET

    // IO pipe  just connected
    if (gt_flags.io_conn == FL_RISE)
    {
        // IO pipe connected. Try to initiate very first read transaction
        n_rc = io_pipe_rx_init();
        if (n_rc)
        {
            gt_flags.io_conn = FL_SET;
        }
        else
        {
            gt_flags.io_conn = FL_CLR;
        }

        return TRUE;
    } // End of Connection RISE

    if (gt_flags.io_conn == FL_FALL)
    {

        wprintf(L"\nDisconnecting IO pipe\n");

        gt_flags.io_conn = FL_CLR;

        // Reset IO_UI 
        gt_flags.io_ui = FL_FALL;

        DisconnectNamedPipe(gh_io_pipe);

        SetEvent(gha_events[HANDLE_IO_PIPE_CONN]);

        return TRUE;
    } // End of Connection FALL


    // PC should never hits here
    return FALSE;

}

int dev_open_uart (int n_dev_indx, FT_HANDLE *ph_device)
{

    FT_STATUS ft_status;
    DWORD   dw_num_devs;
    LONG    devLocation;
    char    devDescriptor[64];
    char    devSerial[64];


    ft_status = FT_ListDevices(&dw_num_devs, NULL, FT_LIST_NUMBER_ONLY);
    if (ft_status != FT_OK) return FALSE;

    if (dw_num_devs == 0){
        // No devices were found
        return FALSE;
    }

    ft_status = FT_ListDevices((void*)gn_dev_index, &devLocation, FT_LIST_BY_INDEX | FT_OPEN_BY_LOCATION);
    if (ft_status != FT_OK) {
        return FALSE;
    }

    ft_status |= FT_ListDevices((void*)gn_dev_index, &devDescriptor, FT_LIST_BY_INDEX | FT_OPEN_BY_DESCRIPTION);
    ft_status |= FT_ListDevices((void*)gn_dev_index, &devSerial, FT_LIST_BY_INDEX | FT_OPEN_BY_SERIAL_NUMBER);

    if (ft_status != FT_OK){
        return FALSE;
    }

#if FT_Classic
    ft_status |= FT_OpenEx((void*)devLocation, FT_OPEN_BY_LOCATION, ph_device);

    ft_status |= FT_SetTimeouts(*ph_device, 500, 500);
    ft_status |= FT_SetLatencyTimer(*ph_device, 2);

    // Divisor selection
    // BAUD = 3000000 / Divisor
    // Divisor = (N + 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875)
    // Divisor = 24 ==> Baud 125000 
    ft_status |= FT_SetDivisor(*ph_device, 3000000 / 125000);

    // Set UART format 8N1
    ft_status |= FT_SetDataCharacteristics(*ph_device, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);

    if (ft_status != FT_OK){
        return FALSE;
    }


    // Just in case
    FT_Purge(*ph_device, FT_PURGE_TX | FT_PURGE_RX);
#else


    // Open a device for overlapped I/O using its serial number
    *ph_device = FT_W32_CreateFile(
        (LPCTSTR)devLocation,
        GENERIC_READ | GENERIC_WRITE,
        0,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FT_OPEN_BY_LOCATION,
        0);

    if (*ph_device == INVALID_HANDLE_VALUE)
    {
        // FT_W32_CreateDevice failed
        return FALSE;
    }

    // ----------------------------------------
    // --- Set comm parameters
    // ----------------------------------------

    FTDCB ftDCB;
    FTTIMEOUTS    ftTimeouts;
    FTCOMSTAT    ftPortStatus;
    DWORD   dw_port_error;

    if (!FT_W32_GetCommState(*ph_device, &ftDCB))
    {
        return FALSE;
    }

    // Divisor selection
    // BAUD = 3000000 / Divisor
    // Divisor = (N + 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875)
    // Divisor = 24 ==> Baud 125000 

    ftDCB.BaudRate = 115200;
    ftDCB.fBinary = TRUE;                       /* Binary Mode (skip EOF check)    */
    ftDCB.fParity = FALSE;                      /* Enable parity checking          */
    ftDCB.fOutxCtsFlow = FALSE;                 /* CTS handshaking on output       */
    ftDCB.fOutxDsrFlow = FALSE;                 /* DSR handshaking on output       */
    ftDCB.fDtrControl = DTR_CONTROL_DISABLE;    /* DTR Flow control                */
    ftDCB.fTXContinueOnXoff = FALSE;

    ftDCB.fErrorChar = FALSE;            // enable error replacement 
    ftDCB.fNull = FALSE;                // enable null stripping 
    ftDCB.fRtsControl = RTS_CONTROL_DISABLE;       // RTS flow control 
    ftDCB.fAbortOnError = TRUE;            // abort reads/writes on error 

    ftDCB.fOutX = FALSE;                        /* Enable output X-ON/X-OFF        */
    ftDCB.fInX = FALSE;                         /* Enable input X-ON/X-OFF         */
    ftDCB.fNull = FALSE;                        /* Enable Null stripping           */
    ftDCB.fRtsControl = RTS_CONTROL_DISABLE;    /* Rts Flow control                */
    ftDCB.fAbortOnError = TRUE;                 /* Abort all reads and writes on Error */

    // 8N1
    ftDCB.ByteSize = 8;                 /* Number of bits/byte, 4-8        */
    ftDCB.Parity = NOPARITY;            /* 0-4=None,Odd,Even,Mark,Space    */
    ftDCB.StopBits = ONESTOPBIT;        /* 0,1,2 = 1, 1.5, 2               */

    if (!FT_W32_SetCommState(*ph_device, &ftDCB))
    {
        return FALSE;
    }

    FT_W32_GetCommState(*ph_device, &ftDCB);

    // Set serial port Timeout values
    FT_W32_GetCommTimeouts(*ph_device, &ftTimeouts);

    ftTimeouts.ReadIntervalTimeout = 0;
    ftTimeouts.ReadTotalTimeoutMultiplier = 0;
    ftTimeouts.ReadTotalTimeoutConstant = 200;
    ftTimeouts.WriteTotalTimeoutConstant = 0;
    ftTimeouts.WriteTotalTimeoutMultiplier = 0;

    FT_W32_SetCommTimeouts(*ph_device, &ftTimeouts);

    FT_W32_ClearCommError(*ph_device, &dw_port_error, &ftPortStatus);
    FT_W32_PurgeComm(*ph_device, PURGE_TXCLEAR | PURGE_RXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

#endif  // End of W32 device init

    return TRUE;
}

int dev_open_fifo (int n_dev_indx, FT_HANDLE *ph_device)
{
    FT_STATUS ft_status;
    DWORD   dw_num_devs;
    LONG    devLocation;
    char    devDescriptor[64];
    char    devSerial[64];

    ft_status = FT_ListDevices(&dw_num_devs, NULL, FT_LIST_NUMBER_ONLY);
    if (ft_status != FT_OK) return FALSE;

    if (dw_num_devs == 0)
    {
        // No devices were found
        return FALSE;
    }

    ft_status = FT_ListDevices((void*)gn_dev_index, &devLocation, FT_LIST_BY_INDEX | FT_OPEN_BY_LOCATION);
    if (ft_status != FT_OK)
    {
        return FALSE;
    }

    ft_status |= FT_ListDevices((void*)gn_dev_index, &devDescriptor, FT_LIST_BY_INDEX | FT_OPEN_BY_DESCRIPTION);
    ft_status |= FT_ListDevices((void*)gn_dev_index, &devSerial, FT_LIST_BY_INDEX | FT_OPEN_BY_SERIAL_NUMBER);

    if (ft_status != FT_OK)
    {
        return FALSE;
    }

    // Open a device for overlapped I/O using its serial number
    *ph_device = FT_W32_CreateFile(
        (LPCTSTR)devLocation,
        GENERIC_READ | GENERIC_WRITE,
        0,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FT_OPEN_BY_LOCATION,
        0);

    if (*ph_device == INVALID_HANDLE_VALUE)
    {
        // FT_W32_CreateDevice failed
        return FALSE;
    }

    ft_status |= FT_SetTimeouts(*ph_device, 1000, 1000);
    ft_status |= FT_SetLatencyTimer(*ph_device, 2);

    if (ft_status != FT_OK)
    {
        return FALSE;
    }


    FT_Purge(*ph_device, FT_PURGE_TX | FT_PURGE_RX);

#if 0
    {
        FT_STATUS n_rc;
        DWORD dwAmountInRxQueue;
        DWORD dwAmountInTxQueue;
        DWORD dwEventStatus;
        DWORD dw_bytes_rcv;

      
        while (1)
        {
            n_rc = FT_GetStatus (*ph_device, &dwAmountInRxQueue, &dwAmountInTxQueue, &dwEventStatus);
            if (FT_OK != n_rc)
            {
                wprintf(L"asdad");
            }

            if (dwAmountInRxQueue == 0)
            {
                return TRUE;
            }

            BYTE uca_tmp[100];

            n_rc = FT_W32_ReadFile(
                *ph_device,
                uca_tmp, 
                sizeof(uca_tmp),
                &gdw_dev_bytes_rcv, 
                &gt_dev_rx_overlapped);


            dw_bytes_rcv = 0;
            n_rc = FT_W32_GetOverlappedResult(
                gh_dev,         
                &gt_dev_rx_overlapped,
                &dw_bytes_rcv,
                TRUE);

            n_rc = FT_GetStatus (*ph_device, &dwAmountInRxQueue, &dwAmountInTxQueue, &dwEventStatus);

        }

    }
#endif
    return TRUE;
}

int dev_open (void)
{

    int n_rc;

    gha_events[HANDLE_DEV_RX] = CreateEvent(
        NULL,  // __in_opt  LPSECURITY_ATTRIBUTES lpEventAttributes,
        FALSE, // __in      BOOL bManualReset,
        FALSE, // __in      BOOL bInitialState,
        NULL   // __in_opt  LPCTSTR lpName
        );
    gt_dev_rx_overlapped.hEvent = gha_events[HANDLE_DEV_RX];

#ifdef DEV_FT_TYPE_UART
    // Try to locate and open device on USB bus
    n_rc = dev_open_uart(gn_dev_index, &gh_dev);
#else
    // Try to locate and open device on USB bus
    n_rc = dev_open_fifo(gn_dev_index, &gh_dev);

#endif 

    if (!n_rc)
    {
        goto dev_open_cleanup;
    }

    // Read device signature
    // ...

dev_open_cleanup:

    // Log activity only if device connection status changed
    if (n_rc)
    {
        if (gt_flags.dev_conn != FL_SET)
        {
            gt_flags.dev_conn = FL_RISE;
            SetEvent(gha_events[HANDLE_NOT_IDLE]);
        }
    }
    else
    {
        if (gt_flags.dev_conn != FL_CLR)
        {
            gt_flags.dev_conn = FL_FALL;
            SetEvent(gha_events[HANDLE_NOT_IDLE]);
        }
    }

    return n_rc;

}

void dev_close (void)
{

    if (gh_dev != INVALID_HANDLE_VALUE)
    {
        FT_W32_CloseHandle(gh_dev);
        gh_dev = INVALID_HANDLE_VALUE;
    }

    if (gha_events[HANDLE_DEV_RX] != INVALID_HANDLE_VALUE)
    {
        CloseHandle(gha_events[HANDLE_DEV_RX]);
        gha_events[HANDLE_DEV_RX] = INVALID_HANDLE_VALUE;
    }

    return;
}
#if 0
int dev_clear_fifos (void)
{

    FT_STATUS t_ft_st;

    if (gt_flags.dev_conn != FL_SET)
    {
        return TRUE;
    }

    t_ft_st = FT_Purge(gh_dev, FT_PURGE_TX | FT_PURGE_RX);
    if (!t_ft_st)
    {
        gt_flags.dev_conn = FL_FALL;
        SetEvent(gha_events[HANDLE_NOT_IDLE]);
    }

    return (t_ft_st == FT_OK);
}
#endif

int dev_rx_init(DWORD dw_dev_resp_req_len, BYTE *pb_dev_rx_buff)
{
    int n_rc, n_gle;

    static DWORD dw_req_len_last;
    static BYTE  *pb_buff_last;         // Pointer to the buffer was in use last time

    if (NULL == pb_dev_rx_buff)
    {
        dw_dev_resp_req_len = dw_req_len_last;
        pb_dev_rx_buff = pb_buff_last;
    }
    else
    {
        dw_req_len_last = dw_dev_resp_req_len;
        pb_buff_last = pb_dev_rx_buff;
    }

    gdw_dev_bytes_rcv = 0;

    n_rc = FT_W32_ReadFile(
        gh_dev,
        pb_dev_rx_buff, 
        dw_dev_resp_req_len,
        &gdw_dev_bytes_rcv, 
        &gt_dev_rx_overlapped);

    if (n_rc == 0)
    {
        n_gle = FT_W32_GetLastError(gh_dev);
        if ( n_gle != ERROR_IO_PENDING) 
        {
            gt_flags.dev_conn = FL_FALL;
            SetEvent(gha_events[HANDLE_NOT_IDLE]);
            return FALSE;
        }
    }

    return TRUE;
}
#if 0
int dev_rx (DWORD dw_len, BYTE *pb_data)
{
   // Read out payload 
    DWORD dw_bytes_rcv;
    int n_rc;

    if (dw_len == 0)
        return TRUE;

    n_rc = FT_W32_ReadFile(
        gh_dev,
        pb_data, 
        dw_len,
        &dw_bytes_rcv, 
        &gt_dev_rx_overlapped);

    if (n_rc == 0)
    {
        n_rc = FT_W32_GetLastError(gh_dev);
        if ( n_rc != ERROR_IO_PENDING) 
        {
            // log an error
            // sent something to UI
            while(1);
        }

        dw_bytes_rcv = 0;
        n_rc = FT_W32_GetOverlappedResult(
            gh_dev,         
            &gt_dev_rx_overlapped,
            &dw_bytes_rcv,
            TRUE);
    }

    if (dw_bytes_rcv != dw_len)
    {  
        wprintf(L"Error in command response\n");
        /* Dump received command */
        // ...

        return FALSE;
    }

    return TRUE;
}
#endif

int dev_rx_proc (DWORD dw_bytes_rcv)
{
    /*
     *  Att: Do not reuse the code below. Its ugly attempt to 
     *       adopt multi streaming protocol to datagramm connection
     */

    BYTE    *pb_stream_buff;
    DWORD   dw_stream_btr, dw_stream_len;
    T_DEV_RX_STREAM *pt_stream;

    // Is new stream started
    if (gt_dev_rx.pt_curr_stream == NULL)
    {   
        if (gdw_dev_bytes_rcv != DEV_RX_STREAM_HDR_SIZE)
        {
            wprintf(L"Garbage received\n");
            gt_flags.dev_conn = FL_FALL;
            SetEvent(gha_events[HANDLE_NOT_IDLE]);
            return TRUE;
        }
         // Get stream ID 
         int n_stream_id;

         n_stream_id  = gt_dev_rx.uca_dev_rx_buff[0] >> 6;
         gt_dev_rx.dw_stream_len = gt_dev_rx.uca_dev_rx_buff[0] & ((1 << 6) -1);

        // Copy data to stream specific buffer
        // and execute corresponding handler
         switch (n_stream_id)
         {
            case DBG_BUFF_IDX_DBGL:
                gt_dev_rx.pt_curr_stream = &gt_stream_dbgl;
                break;
            case DBG_BUFF_IDX_CRSP:
                gt_dev_rx.pt_curr_stream = &gt_stream_crsp;
                break;
            default:
                // Start broken stream handling
                wprintf(L"Garbage received 2\n");
                gt_flags.dev_conn = FL_FALL;
                SetEvent(gha_events[HANDLE_NOT_IDLE]);
                return TRUE;
        }

    }
    // Data received for already started stream
    else 
    {
        pt_stream = gt_dev_rx.pt_curr_stream;
        gt_dev_rx.dw_stream_len -= dw_bytes_rcv;
        pt_stream->dw_wr_idx += dw_bytes_rcv;

        // Call stream handler if number of requested bytes received
        if (pt_stream->dw_btr == pt_stream->dw_wr_idx)
        {
            pt_stream->pf_handler();
        }
        else if (pt_stream->dw_btr < pt_stream->dw_wr_idx)
        {
            wprintf(L"Garbage received 3\n");
            gt_flags.dev_conn = FL_FALL;
            SetEvent(gha_events[HANDLE_NOT_IDLE]);
            return TRUE;
        }
    }

    dw_stream_len = gt_dev_rx.dw_stream_len;
    pt_stream    = gt_dev_rx.pt_curr_stream;

    // Continue read from stream if not finished
    if (dw_stream_len)
    {
        DWORD bytes_remains;

        pb_stream_buff = &pt_stream->ca_buff[pt_stream->dw_wr_idx];

        // Limit btr to number specified by command processor
        dw_stream_btr = dw_stream_len;
        // Reamainder = total requested - already received
        bytes_remains = pt_stream->dw_btr - pt_stream->dw_wr_idx;

        if (dw_stream_len > bytes_remains)
        {
            dw_stream_btr = bytes_remains;
        }
        pb_stream_buff = &pt_stream->ca_buff[pt_stream->dw_wr_idx];
        
    }
    // Initiate new stream
    else
    {
        gt_dev_rx.pt_curr_stream = NULL;
        gt_dev_rx.dw_stream_len = 0;
        pb_stream_buff = gt_dev_rx.uca_dev_rx_buff;
        dw_stream_btr = DEV_RX_STREAM_HDR_SIZE;
    }

    dev_rx_init(dw_stream_btr, pb_stream_buff);
    return TRUE;

}

WCHAR *fid2fname(BYTE uc_fid)
{
#define DBGL_OUT_FID_LEN    512

    static WCHAR wc_out_str[DBGL_OUT_FID_LEN] = L"";

    // Check is FID table valid
    // ...

    swprintf(wc_out_str, DBGL_OUT_FID_LEN-1, L"FID(%d)", uc_fid);

    return wc_out_str;

}


WCHAR *dbgl2str(T_DBGL_HDR *pt_hdr)
{
#define DBGL_OUT_STR_LEN    512

    static WCHAR wc_out_str[DBGL_OUT_STR_LEN] = L"";

    const char *pc_log_str;
    size_t n_chars;

    pc_log_str = ((const char*)pt_hdr) + sizeof(T_DBGL_HDR);

    // Convert signature to Multibyte string
    mbstowcs_s( 
            &n_chars,
            wc_out_str,
            DBGL_OUT_STR_LEN-1,
            pc_log_str,   
            pt_hdr->uc_size - sizeof(T_DBGL_HDR));

    return wc_out_str;
}

void dev_rx_stream_handler_dbgl (void)
{

    T_DBGL_HDR *pt_dbgl_hdr;
    T_DEV_RX_STREAM *pt_stream;

#define DBG_BUFF_MARK_SIZE      2
#define DBG_BUFF_MARK_OVFL      0xDD
#define DBG_BUFF_MARK_RSP       0xD2
#define DBG_BUFF_MARK_LOG       0xD5
#define DBG_BUFF_MARK_LOG_ISR   0xD9


    pt_stream = &gt_stream_dbgl;
    pt_dbgl_hdr = (T_DBGL_HDR *)pt_stream->ca_buff;

     // Check is header received
    if (pt_stream->dw_wr_idx == DBG_LOG_HEADER_SIZE)
    {
        // Check msg mark
        switch (pt_dbgl_hdr->uc_mark)
        {
            case DBG_BUFF_MARK_OVFL:
                wprintf(L"Overflow occur %d times", pt_stream->ca_buff[1]);
                pt_stream->dw_btr = DBG_LOG_HEADER_SIZE;
                pt_stream->dw_wr_idx = 0;
                memset(pt_stream->ca_buff, 0xCC, sizeof(pt_stream->ca_buff));
                return;
            case DBG_BUFF_MARK_LOG:
            case DBG_BUFF_MARK_LOG_ISR:
                // Reguest full header    
                pt_stream->dw_btr = pt_dbgl_hdr->uc_size;
                break;
                // not ready
                break;
            default:
                // Something wrong. Reset pipe
                break;
        }
        return;
    }

    if ( (pt_dbgl_hdr->uc_mark == DBG_BUFF_MARK_LOG) |
         (pt_dbgl_hdr->uc_mark == DBG_BUFF_MARK_LOG_ISR) )
    {
        // Full DBGL message received
        if (pt_stream->dw_wr_idx != pt_dbgl_hdr->uc_size)
        {
            // something wrong
        }

        // AV ACC DBG: Remove after debug!!!
        if (pt_dbgl_hdr->uc_fid == 5)
        {
            DWORD   btw, bwritten;
            if (gh_accdbg_file == INVALID_HANDLE_VALUE)
            {
                gh_accdbg_file = CreateFile(
                    L"acc_bdg.bin",                    //_In_      LPCTSTR lpFileName,
                    (GENERIC_WRITE | GENERIC_READ),    //_In_      DWORD dwDesiredAccess,
                    FILE_SHARE_READ,                   //_In_      DWORD dwShareMode,
                    NULL,                              //_In_opt_  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                    CREATE_ALWAYS,                     //_In_      DWORD dwCreationDisposition,
                    FILE_ATTRIBUTE_NORMAL,             //_In_      DWORD dwFlagsAndAttributes,
                    NULL                               //_In_opt_  HANDLE hTemplateFile
                );
            }

            btw = pt_dbgl_hdr->uc_size - sizeof(T_DBGL_HDR);
            WriteFile(
               gh_accdbg_file,                              //_In_         HANDLE hFile,
               (int8_t *)pt_dbgl_hdr + sizeof(T_DBGL_HDR),  //_In_         LPCVOID lpBuffer,
               btw,                                         //_In_         DWORD nNumberOfBytesToWrite,
               &bwritten,                                   //_Out_opt_    LPDWORD lpNumberOfBytesWritten,
               NULL                                         //_Inout_opt_  LPOVERLAPPED lpOverlapped
            );
        }
        else
        {
            // Everething is OK here proceed with printout
            wprintf(L"DBG_LOG: %s(%d):%s\n", 
                fid2fname(pt_dbgl_hdr->uc_fid),
                pt_dbgl_hdr->us_line,
                dbgl2str(pt_dbgl_hdr));
        }

        pt_stream->dw_btr = DBG_LOG_HEADER_SIZE;
        pt_stream->dw_wr_idx = 0;
        memset(pt_stream->ca_buff, 0xCC, sizeof(pt_stream->ca_buff));
    }

    // Something wrong. Reset pipe
    return;
}

void dev_rx_stream_handler_crsp (void)
{
    int n_rc;
    size_t  t_msg_len;
    BYTE    *pb_msg_buff;
    T_DEV_RSP t_resp;

    T_CRSP_HDR *pt_crsp_hdr;
    T_DEV_RX_STREAM *pt_stream;

    pt_stream = &gt_stream_crsp;
    pt_crsp_hdr = (T_CRSP_HDR *)pt_stream->ca_buff;

    if (pt_crsp_hdr->uc_mark != DBG_BUFF_MARK_RSP)
    {
        while (1);
    }

    // Check is header received
    if (pt_stream->dw_wr_idx == CMD_RESP_HEADER_SIZE)
    {
        // Check is payload present
        if (pt_crsp_hdr->ul_size != CMD_RESP_HEADER_SIZE)
        {
            // Request response reminder
            pt_stream->dw_btr = sizeof(T_CRSP_HDR) + pt_crsp_hdr->ul_size;
            return;
        }
    }
    
    t_resp.b_cmd = pt_crsp_hdr->uc_act;
    t_resp.dw_len = pt_crsp_hdr->ul_size;
    t_resp.pb_data = &pt_stream->ca_buff[0] + CMD_RESP_HEADER_SIZE;

    // Everething is OK here proceed with printout
    n_rc = dev_response_processing(&t_resp, gwca_dev_resp_str, RESP_STR_LEN);

    // Init new command read from stream
    pt_stream->dw_btr = CMD_RESP_HEADER_SIZE;
    pt_stream->dw_wr_idx = 0;

    // If response composed, then send response 
    // message to UI in human readable format 
    if (gwca_dev_resp_str[0] != L'\0')
    {
        pb_msg_buff = &gba_io_pipe_tx_buff[0];
        pb_msg_buff = add_tlv_str( pb_msg_buff, 
                                   UI_IO_TLV_TYPE_CMD_RSP, 
                                   gwca_dev_resp_str);

        t_msg_len = terminate_tlv_list(pb_msg_buff);
        io_pipe_tx_byte(gba_io_pipe_tx_buff, t_msg_len);
    }

    memset(pt_stream->ca_buff, 0xCC, sizeof(pt_stream->ca_buff));

    return;
}

void dev_tx (DWORD dw_bytes_to_write, BYTE *pc_cmd, const WCHAR *pc_cmd_name)
{

    int n_rc, n_gle;
    DWORD dw_bytes_written;

    if (gt_flags.dev_conn != FL_SET) return;

#if 0
    { // print out raw command
        DWORD i;
        wprintf(L"\n--------------------------------- %s : %0X", pc_cmd_name, pc_cmd[0]);

        for (i = 1; i <  dw_bytes_to_write; i++)
        {
            wprintf(L"-%02X", pc_cmd[i]);
        }
        wprintf(L"\n");
    }
#endif

    n_rc = FT_W32_WriteFile(
        gh_dev,
        pc_cmd, 
        dw_bytes_to_write,
        &dw_bytes_written, 
        &gt_dev_tx_overlapped);

    if (!n_rc)
    {
        n_gle = FT_W32_GetLastError(gh_dev);
        if (n_gle != ERROR_IO_PENDING)
        {
            n_rc = FALSE;
            goto cleanup_dev_tx;
        }
    }

    n_rc = FT_W32_GetOverlappedResult(gh_dev, &gt_dev_tx_overlapped, &dw_bytes_written, TRUE);
    if (!n_rc || (dw_bytes_to_write != dw_bytes_written))
    {
        n_rc = FALSE;
        goto cleanup_dev_tx;
    }

    n_rc = TRUE;

    // lock further command processing until response received
    // ...

cleanup_dev_tx:

    if (n_rc)
    {
        wprintf(L"io --> mcu : %s \n", pc_cmd_name);
    }
    else
    {
        wprintf(L"Error writing command %s\n", pc_cmd_name);
        gt_flags.dev_conn = FL_FALL;
        SetEvent(gha_events[HANDLE_NOT_IDLE]);
    }

    return;

}

int dev_check (void)
{

    // Connect to device, Check board presence, ping UI
    if (gt_flags.dev_conn == FL_CLR || gt_flags.dev_conn == FL_UNDEF)
    {
        dev_open();
    }

    // Check device status transition
    if (gt_flags.dev_conn == FL_RISE)
    {
        // DEV CONNECT 0->1
        wprintf(L"Device connected\n");
        gt_flags.dev_conn = FL_SET;

        gt_dev_rx.pt_curr_stream = NULL;
        dev_rx_init(DEV_RX_STREAM_HDR_SIZE, gt_dev_rx.uca_dev_rx_buff);

    }
    else if (gt_flags.dev_conn == FL_FALL)
    {
        // DEV CONNECT 1->0
        dev_close();

        // Clear connection flag
        gt_flags.dev_conn = FL_CLR;

        wprintf(L"Device disconnected\n");
    }
    else if (gt_flags.dev_conn == FL_SET)
    {
        // check device connection status
        // ...
    }

    return TRUE;
}

/***C*F******************************************************************************************
**
** FUNCTION       : Main's loop waiting.
** DATE           :
** AUTHOR         : AV
**
** DESCRIPTION    : Function wait until something happens in main loop.
**                  Following events are supported:
**                      Command from user UI
**                      Response from device
**                      Timeout occured
**                      Not idle event set by internal routines
**
** PARAMETERS     :
**
** RETURN-VALUE   : Returns TRUE if everything is OK, return FALSE if further main loop processing
**                  not reasonanble. Upon FASLE return, Main loop should proceed with "continue".
**
** NOTES          :
**
***C*F*E***************************************************************************************************/
int main_loop_wait (void)
{

    int n_rc;

    n_rc = WaitForMultipleObjects(
        HANDLES_NUM,                 // __in  DWORD nCount,
        gha_events,                  // __in  const HANDLE *lpHandles,
        FALSE,                       // __in  BOOL bWaitAll,
        100                          // __in  DWORD dwMilliseconds
        );


    if (n_rc == WAIT_TIMEOUT)
    {
        //// Check DEV Response TIME OUT
        //// Reinit IO_RX if timer expired
        //if (gn_dev_resp_timeout > 0)
        //{
        //    gn_dev_resp_timeout-- ;
        //    if (gn_dev_resp_timeout == 0)
        //    {
        //        io_pipe_rx_init();  
        //    }
        //}

        if (g_uart_simulated) 
        {
            gdw_dev_bytes_rcv = DEV_RX_STREAM_HDR_SIZE;
            gt_dev_rx.uca_dev_rx_buff[0] = 0;
            gt_dev_rx.uca_dev_rx_buff[0] |= DBG_BUFF_IDX_CRSP << 6;
            gt_dev_rx.uca_dev_rx_buff[0] |= gdw_uart_simulated_bytes & ((1 << 6) - 1);

            gt_stream_crsp.dw_wr_idx = 0;
//            !!! debug this !!!
            dev_rx_proc(gdw_dev_bytes_rcv);

            gdw_dev_bytes_rcv = gdw_uart_simulated_bytes;
            memcpy(gt_dev_rx.uca_dev_rx_buff, gba_uart_simulated_buff, gdw_dev_bytes_rcv);
            dev_rx_proc(gdw_dev_bytes_rcv);
        }
        // Nothing to do
        return TRUE;

    }
    
    else if (n_rc == WAIT_OBJECT_0 + HANDLE_IO_PIPE_CONN)
    {
        n_rc = io_pipe_check();
        return n_rc;
    }

    else if (n_rc == WAIT_OBJECT_0 + HANDLE_IO_PIPE_RX)
    { // Incoming command from IO pipe

        io_pipe_rx_proc();
        
        io_pipe_rx_init();

        return n_rc;
    }
    else if (n_rc == WAIT_OBJECT_0 + HANDLE_DEV_RX)
    {
        n_rc = FT_W32_GetOverlappedResult(
            gh_dev,
            &gt_dev_rx_overlapped,
            &gdw_dev_bytes_rcv,
            TRUE);

        if (n_rc)
        {
            if (gdw_dev_bytes_rcv != 0)
            {
                dev_rx_proc(gdw_dev_bytes_rcv);
            }
            else
            {
                // Timeout. Just repeat 
                // wprintf(L"Dev rx timeout\n");
                dev_rx_init(0, NULL);
            }
        }
        else
        {
            gt_flags.dev_conn = FL_FALL;
            SetEvent(gha_events[HANDLE_NOT_IDLE]);
        }


        return TRUE;
    }
    else if (n_rc == WAIT_OBJECT_0 + HANDLE_NOT_IDLE)
    {
        return TRUE;
    }

    return TRUE;

}

int wmain(int argc, WCHAR *argv[])
{

    if (!io_pipe_init())
        return FALSE;

    gha_events[HANDLE_NOT_IDLE] = CreateEvent(
        NULL,  // __in_opt  LPSECURITY_ATTRIBUTES lpEventAttributes,
        FALSE, // __in      BOOL bManualReset,
        TRUE,  // __in      BOOL bInitialState,
        NULL   // __in_opt  LPCTSTR lpName
        );

    gh_dump_file = CreateFile(L"dump.bin",                     // File name
        GENERIC_READ | GENERIC_WRITE,        // Access mode
        FILE_SHARE_READ | FILE_SHARE_WRITE,  // Share Mode
        NULL,                                // Security Attributes
        CREATE_ALWAYS,                       // Creation Disposition
        0,                                   // File Flags&Attr
        NULL);                               // hTemplate

    gh_meas_log_file = CreateFile(L"meas.log",                  // File name
        GENERIC_READ | GENERIC_WRITE,         // Access mode
        FILE_SHARE_READ | FILE_SHARE_WRITE,   // Share Mode
        NULL,                                 // Security Attributes
        CREATE_ALWAYS,                        // Creation Disposition
        0,                                    // File Flags&Attr
        NULL);                                // hTemplate

    // TODO: extract device index from command line arguments
    gn_dev_index = 0;
    gn_dev_resp_timeout = 0;

    gt_flags.dev_conn = FL_UNDEF;           // in order to report very first connection FAIL

    gt_dev_rx_overlapped.hEvent = gha_events[HANDLE_DEV_RX];

    wprintf(L"                                     \n");
    wprintf(L" ------------------------------------\n");
    wprintf(L" ---      DBG IO proccesor        ---\n");
    wprintf(L" ------------------------------------\n");
    wprintf(L"                                     \n");
    while (gt_flags.exit != FL_SET)
    {

        // ---------------------------------------------------
        // --- Wait until something happens
        // ---------------------------------------------------
        if (!main_loop_wait())
            continue;

        // ---------------------------------------------------
        // --- Check device connection
        // ---------------------------------------------------
        if (!dev_check())
            continue;

        // ---------------------------------------------------
        // --- Check UI state
        // ---------------------------------------------------
        if (!io_ui_check())
            continue;

    }// End of while

    dev_close();
    io_pipe_close();

    wprintf(L"---------------- End of Session ----------------");

    if (gha_events[HANDLE_NOT_IDLE] != INVALID_HANDLE_VALUE)
    {
        CloseHandle(gha_events[HANDLE_NOT_IDLE]);
    }

    return TRUE;

}

#if 0
int send_single_command(int argc, WCHAR *argv[], int n_arg_num){

    WCHAR    ca_cmd_line[CMD_LINE_LENGTH];
    WCHAR    *pc_cmd;

    pc_cmd = ca_cmd_line;
    pc_cmd[0] = 0;

    // combine all remaining command line tokens to one string
    while(n_arg_num < argc ){
        wcsncat_s(pc_cmd, wcs_sizeof(ca_cmd_line), argv[n_arg_num], wcs_sizeof(ca_cmd_line));
        n_arg_num ++;    
    }

    return proceed_cmd(pc_cmd);
}
#endif