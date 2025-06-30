#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "kmdw_console.h"
#include "kdrv_uart.h"
#include "project.h"     /*for MSG_PORT */
#include "ipc.h"
#include "cmsis_armcc.h"

#define BACKSP_KEY 0x08
#define RETURN_KEY 0x0D
#define DELETE_KEY 0x7F
#define BELL 0x07

#define MAX_LOG_LENGTH_UART 256
#define DDR_MAX_LOG_COUNT 2000 // if using DDR
#define DDR_LOG_BUFFER_SIZE (1 * 1024 * 1024)

volatile uint32_t cpu_debug_flags = 0;
osThreadId_t logger_tid = NULL;
osMutexId_t logger_mutex_id = NULL;

static kdrv_uart_handle_t handle0 = MSG_PORT;

print_callback _print_callback = NULL;
extern ncpu_to_scpu_result_t *in_comm_p;

#if (defined(UART_RX_ECHO_GET_DBG) && UART_RX_ECHO_GET_DBG == 1)
uint32_t buf_tmp[1024];
char buf_tmp_char[1024];
uint32_t uart_rx_cnt = 0;
uint32_t uart_rx_cnt_tmp = 0;
#endif

logger_mgt_t logger_mgt;

osEventFlagsId_t uart_console_evid;
osSemaphoreId_t uart_send_mutex;

void kmdw_level_printf(int level, const char *fmt, ...)
{
    uint32_t length;
    va_list arg_ptr;

    uint32_t lvl = kmdw_console_get_log_level_scpu();
    lvl >>= 16;

    //if (!((level == LOG_PROFILE && level == lvl) || (level <= lvl))) return;
    if(!(level & lvl)) return;

    if (logger_mgt.init_done == false)
    {
        char buffer[MAX_LOG_LENGTH_UART];
        va_start(arg_ptr, fmt);
        length = vsnprintf((char*)buffer, MAX_LOG_LENGTH_UART - 1, fmt, arg_ptr);
        if(length > MAX_LOG_LENGTH_UART - 1) {
        	length = MAX_LOG_LENGTH_UART - 1;
        }
        buffer[length] = '\0';
        va_end(arg_ptr);
        kdrv_uart_write(handle0, (uint8_t *)buffer, strlen(buffer));

        if (_print_callback)
            _print_callback((const char *)buffer);
    }
    else
    {
        //TODO:adjust logger_thread to low priority in task_handler.h
        //because it is not important

        //log Q full?
        while( ((logger_mgt.w_idx+1)%LOG_QUEUE_NUM) == logger_mgt.r_idx) {      //<-- ((idx+1)%LOG_QUEUE_NUM)

            //yes, Q full, must digest some logs
            //if Q full is often, must review system design(thread priority, resource, process)
            //to avoid this situation

            //TODO:
            //raise logger thread priority
            //osThreadSetPriority(logger_tid, osPriorityRealtime);

            //trigger logger thread to flush logs
            //if( logger_mgt.willing[LOGGER_OUT] == false) {      //<-- why need this?????
                osThreadFlagsSet(logger_tid, FLAG_LOGGER_SCPU_IN);

                //logger thread has highest priority,
                //it gets scheduled immediately after flags set

                //TODO: consider that log Q size is now 200(LOG_QUEUE_NUM)
                //TODO: flushing 200 logs takes very long time and may affect other processs
            //}

            //now, Q should be empty

            //TODO:
            //set priority back to low
            //osThreadSetPriority(logger_tid, osPriorityLow);
            }

        //Start protecting whole log process, it must be atomic process
        osMutexAcquire(logger_mutex_id, osWaitForever);

        //enter SCPU_IN state
        logger_mgt.willing[LOGGER_SCPU_IN] = true;

        //next, we worry that NCPU is also writting log Q in DDR

        //assume it is NCPU turn
        logger_mgt.turn = LOGGER_NCPU_IN;

        __ISB();    //avoid instruction reorder by compiler
        //check NCPU willing status and assume NCPU is writting log Q
        while( logger_mgt.willing[LOGGER_NCPU_IN] && logger_mgt.turn == LOGGER_NCPU_IN )
        {
            osDelay(1);
        }

        //here, NCPU log done
        //start processing SCPU log

        //prepare timestamp
        sprintf((char *)(logger_mgt.p_msg + logger_mgt.w_idx * MAX_LOG_LENGTH_UART), "[%.03f]", (float)osKernelGetTickCount() / osKernelGetTickFreq());
        int timelog_len = strlen((char *)(logger_mgt.p_msg + logger_mgt.w_idx * MAX_LOG_LENGTH_UART));

        //prepare log string
        va_start(arg_ptr, fmt);
        length = vsnprintf((char *)(logger_mgt.p_msg + logger_mgt.w_idx * MAX_LOG_LENGTH_UART + timelog_len), MAX_LOG_LENGTH_UART - 1 - timelog_len, fmt, arg_ptr);
        if(length > MAX_LOG_LENGTH_UART - 1) {
        	length = MAX_LOG_LENGTH_UART - 1;
        }
        *((logger_mgt.p_msg + logger_mgt.w_idx * MAX_LOG_LENGTH_UART) + length + timelog_len) = '\0';
        va_end(arg_ptr);

        //update write index, willing state
        logger_mgt.w_idx = (++logger_mgt.w_idx) % LOG_QUEUE_NUM;
        logger_mgt.willing[LOGGER_SCPU_IN] = false;

        //unprotect log process
        osMutexRelease(logger_mutex_id);

        //finally, trigger logger thread to flush logs
        //if( logger_mgt.willing[LOGGER_OUT] == false) {              //<-- why need this?????
            osThreadFlagsSet(logger_tid, FLAG_LOGGER_SCPU_IN);
        //}
    }
}

void kmdw_level_ipc_printf(int level, const char *fmt, ...)
{
    // note: this log function will be called in ISR, please be aware of the kdrv_uart_write conflict with logger_thread.
    uint16_t length;
    va_list arg_ptr;
    uint32_t lvl = kmdw_console_get_log_level_scpu();
    lvl >>= 16;

    //if ((level == LOG_PROFILE && level == lvl) || (level > 0 && level <= lvl))
    if(level & lvl)
    {
        char buffer[MAX_LOG_LENGTH_UART];
        va_start(arg_ptr, fmt);
        length = vsnprintf((char*)buffer, MAX_LOG_LENGTH_UART - 1, fmt, arg_ptr);
        if(length > MAX_LOG_LENGTH_UART - 1) {
        	length = MAX_LOG_LENGTH_UART - 1;
        }
        buffer[length] = '\0';
        va_end(arg_ptr);
        kdrv_uart_write(handle0, (uint8_t *)buffer, strlen(buffer));
    }
}


void logger_thread(void *arg)
{
    uint32_t flags;
    osMutexAttr_t logger_mutex_attr;

    logger_mgt.w_idx = 0;
    logger_mgt.r_idx = 0;
    logger_mgt.willing[LOGGER_SCPU_IN] = false;
    logger_mgt.willing[LOGGER_NCPU_IN] = false;
    logger_mgt.willing[LOGGER_OUT] = false;
    logger_mgt.p_msg = (uint8_t *)kmdw_ddr_reserve(MAX_LOG_LENGTH_UART * LOG_QUEUE_NUM);
    if( !logger_mgt.p_msg ) {
        printf("[logger] No enough moery reserved\n");
    }

    logger_tid = osThreadGetId();
    if (logger_tid == NULL)
    {
        printf("[logger] osThreadNew failed\n");
    }

    logger_mutex_attr.attr_bits = (osMutexPrioInherit|osMutexRobust);
    logger_mutex_attr.name = NULL;
    logger_mutex_attr.cb_mem = NULL;
    logger_mutex_attr.cb_size = 0;
    logger_mutex_id = osMutexNew(&logger_mutex_attr);

    //set init done flag after thread and mutex are both init done
    logger_mgt.init_done = true;

    while (1)
    {
        flags = osThreadFlagsWait(FLAG_LOGGER_SCPU_IN | FLAG_LOGGER_NCPU_IN, osFlagsWaitAny, osWaitForever);
        osThreadFlagsClear(flags);

        uint8_t *p_str;
        logger_mgt.willing[LOGGER_OUT] = true;
        while( logger_mgt.r_idx != logger_mgt.w_idx )
        {
            p_str = (uint8_t *)(logger_mgt.p_msg + logger_mgt.r_idx * MAX_LOG_LENGTH_UART);
            kdrv_uart_write(handle0, p_str, strlen((char *)p_str));

            if (_print_callback)
                _print_callback((const char *)p_str);

            logger_mgt.r_idx = ((++logger_mgt.r_idx) % LOG_QUEUE_NUM);
        }
        logger_mgt.willing[LOGGER_OUT] = false;
    }
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

__weak uint32_t kmdw_ddr_reserve(uint32_t numbyte)
{
    return 0;
}

//This function is reserved for validation FW code
//because old validation FW still call this old console init function.
//SDK FW doesn't call this function.
kmdw_status_t kmdw_console_queue_init(void)
{
    osThreadAttr_t thread_attr;
    osMutexAttr_t logger_mutex_attr;

    memset(&thread_attr, 0, sizeof(thread_attr));
    thread_attr.stack_size = 1024;
    thread_attr.priority = osPriorityHigh;

    logger_tid = osThreadNew(logger_thread, NULL, &thread_attr);
    if (logger_tid == NULL)
    {
        printf("[logger] osThreadNew failed\n");
        return KMDW_STATUS_ERROR;
    }

    logger_mutex_attr.attr_bits = (osMutexPrioInherit|osMutexRobust);
    logger_mutex_attr.name = NULL;
    logger_mutex_attr.cb_mem = NULL;
    logger_mutex_attr.cb_size = 0;
    logger_mutex_id = osMutexNew(&logger_mutex_attr);

    return KMDW_STATUS_OK;
}

void kmdw_console_putc(char Ch)
{
    char cc;

    if (Ch != '\0')
    {
        cc = Ch;
        kdrv_uart_write(handle0, (uint8_t *)&cc, 1);

        if (_print_callback)
            _print_callback((const char *)&cc);
    }

    if (Ch == '\n')
    {
        cc = '\r';
        kdrv_uart_write(handle0, (uint8_t *)&cc, 1);

        if (_print_callback)
            _print_callback((const char *)&cc);
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
    cpu_debug_flags = (cpu_debug_flags & ~0x00FF0000) | (((level) << 16) & 0x00FF0000);
    kdrv_ncpu_set_scpu_debug_lvl(level);
}

uint32_t kmdw_console_get_log_level_scpu(void)
{
    return (cpu_debug_flags & 0x00FF0000);
}

void kmdw_console_set_log_level_ncpu(uint32_t level)
{
    cpu_debug_flags = (cpu_debug_flags & ~0x000000FF) | ((level) & 0x000000FF);
    kdrv_ncpu_set_ncpu_debug_lvl(level);
}

uint32_t kmdw_console_get_log_level_ncpu(void)
{
    return cpu_debug_flags & 0xff;
}

#define MAX_CONSOLE_CATEGORIES 30
static uint8_t curr_table_idx = 0;
static  struct console_cmd_items console_cmds_table[MAX_CONSOLE_CATEGORIES] = { NULL };
static void _kmdw_console_run_items(uint32_t id)
{
    uint32_t i, j, k;
    struct console_cmd_items *cmds, *next_cmds;

    for (i = 0; i < curr_table_idx; ++i) {
        k = 0;
        cmds = &console_cmds_table[i];
        if (i < (curr_table_idx - 1)) {
            next_cmds = &console_cmds_table[i + 1];
            for (j = cmds->start_idx; j < next_cmds->start_idx; ++j, ++k) {
                if (j == id) {
                    goto found;
                }
            }
        }
        else {
            for (j = cmds->start_idx; j < cmds->end_idx; ++j, ++k) {
                if (j == id) {
                    goto found;
                }
            }
        }
    }
    return;
found:
    if (cmds->cmd_ctx[k].func)
        cmds->cmd_ctx[k].func();
}

void kmdw_console_reg(uint16_t start_idx, uint16_t end_idx, struct console_cmd_item *context)
{
    if (curr_table_idx < MAX_CONSOLE_CATEGORIES) {
    #if (CONSOLE_ITEM_SORT_ENABLE == YES)
        qsort((void*)console_cmds_table, curr_table_idx, sizeof(struct console_cmd_items), _kmdw_console_item_comparator);
    #endif
        console_cmds_table[curr_table_idx].start_idx = start_idx;
        console_cmds_table[curr_table_idx].end_idx = end_idx;
        console_cmds_table[curr_table_idx].cmd_ctx = context;
        ++curr_table_idx;
    }
    
//    for (int i = 0; i < curr_table_idx; ++i) {
//        kprintf("console_cmds_table[%d]=%s \r\n", i, console_cmds_table[i].cmd_ctx->desc);
//    }
}
uint32_t cmd_pool[] = { 65, 66};
uint32_t cmd_idx = 0 ;
void kmdw_console_thread(void *arg)
{
    //uint8_t pre_cmd_idx = 0;
    uint32_t i, j, k;
    int32_t rc = 1;
    uint8_t cmd_size = console_cmds_table[curr_table_idx - 1].end_idx;
    char buf[256];
    console_wait_func fn = (console_wait_func)arg;
    if (fn)
        fn();

    //profiler_stamp(E_MEASURE_THR_CONSOLE_RDY);

    struct console_cmd_items *cmds, *next_cmds;
    while(1)
    {
        uint32_t id = 0;

        if (rc)
            goto cmd_prompt;

        kmdw_printf(" === console Test Kit (%u) === \n", cmd_size);
    
        for (i = 0; i < curr_table_idx; ++i) {
            k = 0;
            cmds = &console_cmds_table[i];
            if (i < (curr_table_idx - 1)) {
                next_cmds = &console_cmds_table[i + 1];
                for (j = cmds->start_idx; j < next_cmds->start_idx; ++j) {
                    if (j < cmds->end_idx) {
                        sprintf(buf, "(%2d) %s : ", (int)j, cmds->cmd_ctx[k++].desc);
                    }
                    else
                        sprintf(buf, "(%2d) %s : ", (int)j, "-r-");
                    kmdw_printf("%s\n", buf);
                }
            }
            else {
                for (j = cmds->start_idx; j < cmds->end_idx; ++j) {
                    sprintf(buf, "(%2d) %s : ", (int)j, cmds->cmd_ctx[k++].desc);
                    kmdw_printf("%s\n", buf);
                }
            }
        }
        //kprintfln("Thread NUM:%d\n",osThreadGetCount());
cmd_prompt:
            if(cmd_idx < (sizeof(cmd_pool)/sizeof(cmd_pool[0])))
            {
                id = cmd_pool[cmd_idx];
                cmd_idx++;
            }
            else
            {
            //if(pre_cmd_array[pre_cmd_idx] == 0){
                kmdw_printf(" \ncommand >>\n");
                rc = kmdw_console_echo_gets(buf, sizeof(buf));
                kmdw_printf("\n");
                if (!rc)
                    continue;
                id = atoi(strtok(buf, " \r\n\t"));
            }

        //}
        // else{
        //     id = pre_cmd_array[pre_cmd_idx];
        //     pre_cmd_idx++;
        // }

        if (id > 0 && id < cmd_size)
        {
            _kmdw_console_run_items(id);

//            user_cmd = id;
        }
    }
}

void kmdw_console_entry(console_wait_func fn)
{

    osThreadAttr_t thread_attr;
    memset(&thread_attr, 0, sizeof(thread_attr));
    thread_attr.stack_size = 3072;
    thread_attr.priority = osPriorityBelowNormal;

    osThreadNew(kmdw_console_thread, NULL, &thread_attr);

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


