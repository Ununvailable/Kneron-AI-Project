/**
 * @file        kmdw_console.h
 * @brief       log message to console APIs
 *
 * @copyright   Copyright (c) 2020 Kneron Inc. All rights reserved.
 */

#ifndef __KMDW_CONSOLE_H__
#define __KMDW_CONSOLE_H__

#include <stdio.h>
#include "base.h"
#include "kmdw_status.h"
#include "kdrv_uart.h"
#include "kmdw_memory.h"

#define MAKE_CONSOLE_PARAMS(prefix, __cmds) \
    kmdw_console_reg(CONSOLE_##prefix##_START_IDX, \
                     CONSOLE_##prefix##_END_IDX, \
                     __cmds)

typedef void (*console_cmd_func)(void);
typedef void (*console_wait_func)(void);
struct console_cmd_item {
    char *desc;
    console_cmd_func func;
};


struct console_cmd_items {
    unsigned int start_idx;
    unsigned int end_idx;
    struct console_cmd_item *cmd_ctx;
};

#define LOG_NONE        0    /**< log level for none */
#define LOG_CRITICAL    BIT0 /**< 1 */
#define LOG_ERROR       BIT1 /**< 2 */
#define LOG_USER        BIT2 /**< 3 */
#define LOG_INFO        BIT3 /**< 4 */

#define LOG_TRACE       BIT4 /**< 5 */
#define LOG_DBG         BIT5 /**< 6 */
#define LOG_PROFILE     BIT6 /**< 7 */
#define LOG_CUSTOM      BIT7 /**< 0 */       /**< log level for special purpose debugging */

typedef void (*print_callback)(const char *log);

extern kmdw_status_t kmdw_console_queue_init(void);
extern void kmdw_console_set_log_level_scpu(uint32_t level);
extern void kmdw_console_set_log_level_ncpu(uint32_t level);
extern char kmdw_console_getc(void);
extern void kmdw_console_putc(char Ch);
extern void kmdw_console_puts(char *str);
extern int kmdw_console_echo_gets(char *buf, int len);
extern void kmdw_console_hook_callback(print_callback print_cb);
extern void kmdw_console_wait_rx_done(kdrv_uart_handle_t handle);
extern void kmdw_console_wait_tx_done(kdrv_uart_handle_t handle);
extern kmdw_status_t kmdw_uart_console_init(uint8_t uart_dev, uint32_t baudrate);
extern kmdw_status_t kmdw_uart_uninitialize(void);
extern uint32_t kmdw_console_get_log_level_scpu(void);
extern void kmdw_printf_nocrlf(const char *fmt, ...);    /* variable arguments */

extern uint32_t kmdw_console_get_log_level_ncpu(void);
void kmdw_level_printf(int level, const char *fmt, ...);
void kmdw_level_ipc_printf(int level, const char *fmt, ...);

#define kmdw_printf(fmt, ...)       kmdw_level_printf(LOG_CUSTOM, fmt, ##__VA_ARGS__)

#ifdef LOG_ENABLE

#define DSG(__format__, ...)        {kmdw_level_printf(LOG_CUSTOM, __format__ "\n", ##__VA_ARGS__);}
#define DSG_NOLF(__format__, ...)   {kmdw_level_printf(LOG_CUSTOM, __format__, ##__VA_ARGS__);}

#define dbg_msg(fmt, ...)           kmdw_level_printf(LOG_DBG, "[DBG] "fmt, ##__VA_ARGS__)
#define trace_msg(fmt, ...)         kmdw_level_printf(LOG_TRACE, "[TRACE] "fmt, ##__VA_ARGS__)
#define info_msg(fmt, ...)          kmdw_level_printf(LOG_INFO, "[INFO] "fmt, ##__VA_ARGS__)
#define err_msg(fmt, ...)           kmdw_level_printf(LOG_ERROR, "[ERROR] "fmt, ##__VA_ARGS__)
#define critical_msg(fmt, ...)      kmdw_level_printf(LOG_CRITICAL, "[CRITICAL] "fmt, ##__VA_ARGS__)
#define profile_msg(fmt, ...)       kmdw_level_printf(LOG_PROFILE, "[PROFILE] "fmt, ##__VA_ARGS__)
#define dlog(fmt, ...)              kmdw_level_printf(LOG_DBG, "[%s][%s] " fmt "\r\n", DEF_LOG_CATEG, __func__, ##__VA_ARGS__)
#define custom_msg(fmt, ...)        kmdw_level_printf(LOG_CUSTOM, fmt, ##__VA_ARGS__)

#else //////////////////////////////////////////////////////////////////////

#define DSG_NOLF(__format__, ...)
#define DSG(__format__, ...)
#define dbg_msg(fmt, ...)
#define trace_msg(fmt, ...)
#define info_msg(fmt, ...)
#define err_msg(fmt, ...)
#define critical_msg(fmt, ...)
#define dlog(fmt, ...)

#endif // LOG_ENABLE ///////////////////////////////////////////////////////

// #def IPC_LOG_ENABLE
#ifdef IPC_LOG_ENABLE

// note: this log function will be called in ISR, please be aware of the kdrv_uart_write conflict with logger_thread.
#define ipc_dbg_msg(fmt, ...)       kmdw_level_ipc_printf(LOG_DBG, "[iDBG] "fmt, ##__VA_ARGS__)
#define ipc_trace_msg(fmt, ...)     kmdw_level_ipc_printf(LOG_TRACE, "[iTRACE] "fmt, ##__VA_ARGS__)
#define ipc_info_msg(fmt, ...)      kmdw_level_ipc_printf(LOG_INFO, "[iINFO] "fmt, ##__VA_ARGS__)
#define ipc_err_msg(fmt, ...)       kmdw_level_ipc_printf(LOG_ERROR, "[iERROR] "fmt, ##__VA_ARGS__)
#define ipc_critical_msg(fmt, ...)  kmdw_level_ipc_printf(LOG_CRITICAL, "[iCRITICAL] "fmt, ##__VA_ARGS__)
#define ipc_profile_msg(fmt, ...)   kmdw_level_ipc_printf(LOG_PROFILE, "[iPROFILE] "fmt, ##__VA_ARGS__)
#define ipc_dlog(fmt, ...)          kmdw_level_ipc_printf(LOG_DBG, "[%s][%s] " fmt "\r\n", DEF_LOG_CATEG, __func__, ##__VA_ARGS__)

#else //////////////////////////////////////////////////////////////////////

#define ipc_dbg_msg(fmt, ...)
#define ipc_trace_msg(fmt, ...)
#define ipc_info_msg(fmt, ...)
#define ipc_err_msg(fmt, ...)
#define ipc_critical_msg(fmt, ...)
#define ipc_profile_msg(fmt, ...)
#define ipc_dlog(fmt, ...)

#endif // IPC_LOG_ENABLE ///////////////////////////////////////////////////

#define dbg_msg_api(fmt, ...)       //dbg_msg(fmt, ##__VA_ARGS__)
#define dbg_msg_app(fmt, ...)       //dbg_msg(fmt, ##__VA_ARGS__)
#define dbg_msg_algo(fmt, ...)      //critical_msg(fmt, ##__VA_ARGS__)
#define dbg_msg_console(fmt, ...)   kmdw_level_printf(LOG_ERROR, fmt"\n", ##__VA_ARGS__)
#define dbg_msg_user(fmt, ...)      kmdw_level_printf(LOG_USER, fmt, ##__VA_ARGS__)
#define dbg_msg_nocrlf(fmt, ...)   printf(fmt, ##__VA_ARGS__);
#define DEV_PKT_LOG_DETAIL
//#define CUSTOMER_SETTING_REMOVE_LOG
#ifdef CUSTOMER_SETTING_REMOVE_LOG
    #define dbg_msg_console(__format__, ...) kmdw_level_printf(LOG_USER, __format__"\n", ##__VA_ARGS__)
    #define dbg_msg_err(__format__, ...) kmdw_level_printf(LOG_ERROR, __format__"\n", ##__VA_ARGS__)
    #define dbg_msg_flash(__format__, ...)   //kmdw_level_printf(LOG_ERROR, __format__"\n", ##__VA_ARGS__)
    #define dbg_msg_camera(__format__, ...)  //kmdw_level_printf(LOG_ERROR, __format__"\n", ##__VA_ARGS__)
    #define dbg_msg_display(__format__, ...) //kmdw_level_printf(LOG_ERROR, __format__"\n", ##__VA_ARGS__)
    #define dbg_msg_touch(__format__, ...)   //kmdw_level_printf(LOG_ERROR, __format__"\n", ##__VA_ARGS__)
    #define dbg_msg_com(__format__, ...) //kmdw_level_printf(LOG_ERROR, __format__"\n", ##__VA_ARGS__)
    #define dbg_msg_gui(__format__, ...) //kmdw_level_printf(LOG_ERROR, __format__"\n", ##__VA_ARGS__)
    #define dbg_msg_app(__format__, ...) //kmdw_level_printf(LOG_ERROR, __format__"\n", ##__VA_ARGS__)
    #define dbg_msg_e2e(__format__, ...) //kmdw_level_printf(LOG_ERROR, __format__"\n", ##__VA_ARGS__)
    #define dbg_msg_api(__format__, ...) //kmdw_level_printf(LOG_ERROR, __format__"\n", ##__VA_ARGS__)
    #define dbg_msg_usb(__format__, ...) //{ kmdw_printf(__format__"\n", ##__VA_ARGS__); }
    #define dbg_msg_algo(__format__, ...) //kmdw_level_printf(LOG_DBG, __format__"\n", ##__VA_ARGS__)
    //#define dlog(__format__, ...) //kmdw_level_printf(LOG_DBG, "[%s][%s] " __format__ "\r\n", DEF_LOG_CATEG, __func__, ##__VA_ARGS__)
    #define dbg_msg_model(__format__, ...) //kmdw_level_printf(LOG_USER, __format__"\n", ##__VA_ARGS__)
    #define dbg_msg_ncpu(__format__, ...) //kmdw_level_printf(LOG_USER, __format__"\n", ##__VA_ARGS__)
    #define dbg_msg_engineering(__format__, ...) //kmdw_user_level_printf(LOG_USER, __format__"\n", ##__VA_ARGS__)
		
		
//		#define ipc_dbg_msg(fmt, ...) kmdw_level_ipc_printf(LOG_DBG, "[DBG] "fmt, ##__VA_ARGS__)
//		#define ipc_trace_msg(fmt, ...) kmdw_level_ipc_printf(LOG_TRACE, "[TRACE] "fmt, ##__VA_ARGS__)
//		#define ipc_info_msg(fmt, ...) kmdw_level_ipc_printf(LOG_INFO, "[INFO] "fmt, ##__VA_ARGS__)
//		#define ipc_err_msg(fmt, ...) kmdw_level_ipc_printf(LOG_ERROR, "[ERROR] "fmt, ##__VA_ARGS__)
//		#define ipc_critical_msg(fmt, ...) kmdw_level_ipc_printf(LOG_CRITICAL, "[CRITICAL] "fmt, ##__VA_ARGS__)
//		#define ipc_profile_msg(fmt, ...) kmdw_level_ipc_printf(LOG_PROFILE, "[PROFILE] "fmt, ##__VA_ARGS__)
//		#define ipc_dlog(fmt, ...) kmdw_level_ipc_printf(LOG_DBG, "[%s][%s] " fmt "\r\n", DEF_LOG_CATEG, __func__, ##__VA_ARGS__)
#else
    #define dbg_msg_err(__format__, ...) kmdw_level_printf(LOG_ERROR, __format__"\n", ##__VA_ARGS__)
    #define dbg_msg_flash(__format__, ...) //kmdw_level_printf(LOG_ERROR, __format__"\n", ##__VA_ARGS__)
    #define dbg_msg_camera(__format__, ...) //kmdw_level_printf(LOG_ERROR, __format__"\n", ##__VA_ARGS__)
    #define dbg_msg_display(__format__, ...) //kmdw_level_printf(LOG_ERROR, __format__"\n", ##__VA_ARGS__)
    #define dbg_msg_touch(__format__, ...) //kmdw_level_printf(LOG_ERROR, __format__"\n", ##__VA_ARGS__)
    #define dbg_msg_com(__format__, ...) //kmdw_level_printf(LOG_ERROR, __format__"\n", ##__VA_ARGS__)
    #define dbg_msg_gui(__format__, ...) //kmdw_level_printf(LOG_ERROR, __format__"\n", ##__VA_ARGS__)
    #define dbg_msg_e2e(__format__, ...) //kmdw_level_printf(LOG_ERROR, __format__"\n", ##__VA_ARGS__)
    #define dbg_msg_usb(__format__, ...) { kmdw_printf(__format__"\n", ##__VA_ARGS__); }
    #define dbg_msg_model(__format__, ...) kmdw_level_printf(LOG_USER, __format__"\n", ##__VA_ARGS__)    
    #define dbg_msg_ncpu(__format__, ...) //kmdw_level_printf(LOG_USER, __format__"\n", ##__VA_ARGS__)
    #define dbg_msg_engineering(__format__, ...) //kdp_user_level_printf(LOG_USER, __format__"\n", ##__VA_ARGS__)
#endif 
    
void kmdw_console_reg(uint16_t start_idx, uint16_t end_idx, struct console_cmd_item *context);
void kmdw_console_entry(console_wait_func fn);

#endif // __KMDW_CONSOLE_H__
