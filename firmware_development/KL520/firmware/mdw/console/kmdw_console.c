#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "kmdw_console.h"
#include "project.h"

#include "cmsis_os2.h"
#include "kdrv_cmsis_core.h"

#define UART_RX_ECHO_GET_DBG 0

#define BACKSP_KEY 0x08
#define RETURN_KEY 0x0D
#define DELETE_KEY 0x7F
#define BELL 0x07

#define MAX_LOG_LENGTH (256) //14bytes are necessary amount structure information "sizeof(os_message_t)" for each msg element in rtx_msgqueue.c
#define MAX_LOG_LENGTH_MSGQ (MAX_LOG_LENGTH+14)
#define DDR_LOG_BUFFER_SIZE (1 * 1024 * 1024)
#define DDR_MAX_LOG_COUNT (DDR_LOG_BUFFER_SIZE/MAX_LOG_LENGTH_MSGQ)// if using DDR
static uint32_t scpu_debug_flags = 0;

static kdrv_uart_handle_t handle0 = MSG_PORT;
static osMessageQueueId_t log_msgq = NULL;

print_callback _print_callback = NULL;

#if (defined(UART_RX_ECHO_GET_DBG) && UART_RX_ECHO_GET_DBG == 1)
uint32_t buf_tmp[1024];
char buf_tmp_char[1024];
uint32_t uart_rx_cnt = 0;
uint32_t uart_rx_cnt_tmp = 0;
#endif

osEventFlagsId_t uart_console_evid;
osSemaphoreId_t uart_send_mutex;

__weak uint32_t osKernelIrqStatus(void)
{
    return false;
}

static void _print_to_uart(const char *str)
{
    osMutexAcquire(uart_send_mutex, osWaitForever);
    kdrv_uart_write(handle0, (uint8_t *)str, strlen(str));
    osMutexRelease(uart_send_mutex);
}

kmdw_status_t kmdw_printf(const char *fmt, ...)
{
    va_list arg_ptr;
    static char *buffer_s = NULL;

    if (NULL == buffer_s)
        buffer_s = (char *)malloc(sizeof(char) * MAX_LOG_LENGTH);

    sprintf(buffer_s, "[%.03f] ", (float)osKernelGetTickCount() / osKernelGetTickFreq());
    int pre_len = strlen(buffer_s);

    va_start(arg_ptr, fmt);
    vsnprintf(buffer_s + pre_len, MAX_LOG_LENGTH - 1, fmt, arg_ptr);
    va_end(arg_ptr);

    buffer_s[MAX_LOG_LENGTH - 1] = 0; // just in case

    if (log_msgq == NULL || osThreadGetId() == NULL)
        _print_to_uart(buffer_s);
    else
    {
        osStatus_t oss = osMessageQueuePut(log_msgq, buffer_s, NULL, 0);
        if (oss != osOK)
        {
            //_print_to_uart("[logger] enqueue log1 failed\n");
            return KMDW_STATUS_ERROR;
        }
    }

    return KMDW_STATUS_OK;
}

kmdw_status_t kmdw_level_printf(int level, const char *fmt, ...)
{
    static char *buffer_s = NULL;
    uint32_t lvl = kmdw_console_get_log_level_scpu();
    lvl >>= 16;

    if ((level == LOG_PROFILE && level == lvl) || (level > 0 && level <= lvl))
    {
        va_list arg_ptr;

        if (NULL == buffer_s)
            buffer_s = (char *)malloc(sizeof(char) * MAX_LOG_LENGTH);

        sprintf(buffer_s, "[%.03f] ", (float)osKernelGetTickCount() / osKernelGetTickFreq());
        int pre_len = strlen(buffer_s);

        va_start(arg_ptr, fmt);
        vsnprintf(buffer_s + pre_len, MAX_LOG_LENGTH - 1, fmt, arg_ptr);
        va_end(arg_ptr);

        buffer_s[MAX_LOG_LENGTH - 1] = 0; // just in case

        if (log_msgq == NULL || osThreadGetId() == NULL)
            _print_to_uart(buffer_s);
        else
        {
            osStatus_t oss = osMessageQueuePut(log_msgq, buffer_s, NULL, 0);
            if (oss != osOK)
            {
                //_print_to_uart("[logger] enqueue log failed\n");
                return KMDW_STATUS_ERROR;
            }
        }
    }

    return KMDW_STATUS_OK;
}

void logger_thread(void *arg)
{
    uint8_t log[MAX_LOG_LENGTH];

    void *log_pool = (void *)kmdw_ddr_reserve(DDR_LOG_BUFFER_SIZE);

    if (log_pool)
    {
        osMessageQueueAttr_t msgq_attr;
        memset(&msgq_attr, 0, sizeof(msgq_attr));
        msgq_attr.mq_mem = log_pool;
        msgq_attr.mq_size = DDR_LOG_BUFFER_SIZE;
        memset(log_pool, 0, DDR_LOG_BUFFER_SIZE);

        log_msgq = osMessageQueueNew(DDR_MAX_LOG_COUNT, MAX_LOG_LENGTH, &msgq_attr);
        if (log_msgq == NULL)
        {
            printf("[logger] osMessageQueueNew failed\n");
        }
    }
    while (1)
    {
        osStatus_t oss = osMessageQueueGet(log_msgq, &log[0], NULL, osWaitForever);
        if (oss != osOK)
        {
            _print_to_uart("[logger] dequeue log failed\n");
            // if (_print_callback)
            //     _print_callback("[logger] dequeue log failed\n");
            continue;
        }

        _print_to_uart((const char *)log);

        if (_print_callback)
            _print_callback((const char *)log);
    }
}

__weak uint32_t kmdw_ddr_reserve(uint32_t numbyte)
{
    return 0;
}

void kmdw_console_hook_callback(print_callback print_cb)
{
    _print_callback = print_cb;
}

char kmdw_console_getc(void)
{
    char c;
    kdrv_uart_read(handle0, (uint8_t *)&c, 1);
    return c;
}

void kmdw_console_putc(char Ch)
{
    char cc;

    if (Ch != '\0')
    {
        cc = Ch;
        kdrv_uart_write(handle0, (uint8_t *)&cc, 1);
    }

    if (Ch == '\n')
    {
        cc = '\r';
        kdrv_uart_write(handle0, (uint8_t *)&cc, 1);
    }
}

void kmdw_console_puts(char *str)
{
    char *cp;
    for (cp = str; *cp != 0; cp++)
        kmdw_console_putc(*cp);
}

int kmdw_console_echo_gets(char *buf, int len)
{
    char *cp;
    char data[MAX_FIFO_RX];
    uint32_t count;
    uint32_t exit_while = 0;
    count = 0;
    cp = buf;
    len = 1024;
    #if (defined(UART_RX_ECHO_GET_DBG) && UART_RX_ECHO_GET_DBG == 1)
    uart_rx_cnt=0;
    memset(buf_tmp, 0, sizeof(buf_tmp));
    memset(buf_tmp_char, 0xff, sizeof(buf_tmp_char));
    #endif
    do
    {
        memset(data, 0, MAX_FIFO_RX);
        kdrv_uart_get_char(handle0, data);
        //kdrv_uart_read(handle0, (uint8_t*)data, 1);
        for(uint32_t i = 0; i< gDrvCtx.uart_dev[handle0]->info.xfer.rx_cnt; i++)
        {

            #if (defined(UART_RX_ECHO_GET_DBG) && UART_RX_ECHO_GET_DBG == 1)
            buf_tmp[uart_rx_cnt] = uart_rx_cnt;
            buf_tmp_char[uart_rx_cnt] = data[i];
            uart_rx_cnt++;
            #endif
            if(data[i] == BACKSP_KEY || data[i] == DELETE_KEY)
            {
                if ((count > 0) && (count < len))
                {
                    count--;
                    *(--cp) = '\0';
                    //kmdw_console_puts("\b \b");
                }
                break;
            }
            if(data[i] == RETURN_KEY)
            {
                if (count < len)
                {
                    *cp = '\0';
                    kmdw_console_putc('\n');
                }
                exit_while = 1;
                break;
            }
            else
            {
                if(count < len)
                {
                    *cp = (char)data[i];
                    cp++;
                    count++;
                    kmdw_console_putc(data[i]);
                }
            }
        }
    } while (exit_while == 0);
    #if (defined(UART_RX_ECHO_GET_DBG) && UART_RX_ECHO_GET_DBG == 1)
    uart_rx_cnt_tmp = uart_rx_cnt;
    #endif
    return (count);
}

__weak void kdrv_ncpu_set_scpu_debug_lvl(uint32_t lvl)
{
}

__weak void kdrv_ncpu_set_ncpu_debug_lvl(uint32_t lvl)
{
}

void kmdw_console_set_log_level_scpu(uint32_t level)
{
    scpu_debug_flags = (scpu_debug_flags & ~0x000F0000) | (((level) << 16) & 0x000F0000);
    kdrv_ncpu_set_scpu_debug_lvl(level);
}

uint32_t kmdw_console_get_log_level_scpu(void)
{
    return scpu_debug_flags;
}

void kmdw_console_set_log_level_ncpu(uint32_t level)
{
    kdrv_ncpu_set_ncpu_debug_lvl(level);
}

void kmdw_console_callback(uint32_t event)
{

    if(osKernelGetState() == osKernelRunning && uart_console_evid != NULL)
    {
        if (event & UART_RX_DONE)
        {
            osEventFlagsSet(uart_console_evid, UART_RX_DONE);
        }
        if (event & UART_TX_DONE)
        {
            osEventFlagsSet(uart_console_evid, UART_TX_DONE);
        }
        if (event & UART_RX_TIMEOUT)
        {
            osEventFlagsSet(uart_console_evid, UART_RX_DONE);
        }
    }
    if (event & UART_REVEIVE_COMPLETE)
    {
        kmdw_console_wait_rx_done(MSG_PORT);
    };
    if (event & UART_TRANSFER_COMPLETE)
    {
        kmdw_console_wait_tx_done(MSG_PORT);
    };
}

void kmdw_console_wait_rx_done(kdrv_uart_handle_t handle)
{
    if(osKernelGetState() == osKernelRunning && uart_console_evid != NULL)
    {
        int32_t evt_flg = osEventFlagsWait(uart_console_evid, UART_RX_DONE, osFlagsWaitAny , osWaitForever);
    }
    else
    {
        while((uart_get_status((DRVUART_PORT)handle) & SERIAL_LSR_DR) != SERIAL_LSR_DR)
        {
            if(osKernelIrqStatus() == false)
                __WFI();
            else
                __NOP();
        }
    }
}

void kmdw_console_wait_tx_done(kdrv_uart_handle_t handle)
{
    if(osKernelIrqStatus() == true)
    {
        while((uart_get_status((DRVUART_PORT)handle) & SERIAL_LSR_THRE) != SERIAL_LSR_THRE)
        {
            __NOP();
        }
        gDrvCtx.uart_dev[handle0]->info.status.tx_busy = 0;
    }
    else if(osKernelGetState() == osKernelRunning && uart_console_evid != NULL)
    {
        int32_t evt_flg = osEventFlagsWait(uart_console_evid, UART_TX_DONE, osFlagsWaitAny , osWaitForever);
    }
    else
    {
        while((uart_get_status((DRVUART_PORT)handle) & SERIAL_LSR_THRE) != SERIAL_LSR_THRE)
        {
            __WFI();
        }
    }
}

kmdw_status_t kmdw_uart_console_init(uint8_t uart_dev, uint32_t baudrate)
{
    kdrv_status_t sts = kdrv_uart_console_init(uart_dev, baudrate, kmdw_console_callback);//NULL);//

    if (sts != KDRV_STATUS_OK)
        return KMDW_STATUS_ERROR;

    uart_send_mutex = osMutexNew(NULL); // for uart send usage
    uart_console_evid  = osEventFlagsNew(0);

    if(uart_console_evid == NULL)
        return KMDW_STATUS_ERROR;

    return KMDW_STATUS_OK;
}


kmdw_status_t kmdw_uart_uninitialize(void)
{
    return KMDW_STATUS_OK;
}
