#ifndef __NPU_LOG_H__
#define __NPU_LOG_H__

#include <stdio.h>
#include "ipc.h"


void npu_log_init(void);
void npu_log_printf(const char *f, ...);

#define LOG_NONE        0
#define LOG_CRITICAL    BIT0//1
#define LOG_ERROR       BIT1//2
#define LOG_USER        BIT2//3
#define LOG_INFO        BIT3//4

#define LOG_TRACE       BIT4//5
#define LOG_DBG         BIT5//6
#define LOG_PROFILE     BIT6//7
#define LOG_CUSTOM      BIT7//0       /**< log level for special purpose debugging */

#ifdef LOG_ENABLE

extern struct scpu_to_ncpu_s *in_comm_p;

#define MSG(level, format, ...) \
    do { \
        int lvl = in_comm_p->debug_flags & 0x000000FF; \
       if(level & lvl) \
            npu_log_printf(format, ##__VA_ARGS__); \
    } while (0)

#define log_get_level_ncpu()    (in_comm_p->debug_flags & 0x0000000F)



#define LOG(level, format, ...) \
    do { \
        npu_log_printf("[%d|%s@%s,%d] " format, \
            level, __func__, __FILE__, __LINE__, ##__VA_ARGS__ ); \
    } while (0)
     
#define _DSG(__flag__, __format__, ...) { if (__flag__ == LOG_INFO) npu_log_printf(__format__"\n", ##__VA_ARGS__); }
#define DSG(__format__, ...) { npu_log_printf(__format__"\n", ##__VA_ARGS__); }

#define dbg_msg(fmt, ...) MSG(LOG_DBG, fmt, ##__VA_ARGS__)
#define trace_msg(fmt, ...) MSG(LOG_TRACE, fmt, ##__VA_ARGS__)
#define info_msg(fmt, ...) MSG(LOG_INFO, fmt, ##__VA_ARGS__)
#define err_msg(fmt, ...) MSG(LOG_ERROR, fmt, ##__VA_ARGS__)
#define critical_msg(fmt, ...) MSG(LOG_CRITICAL, fmt, ##__VA_ARGS__)
#define profile_msg(fmt, ...) MSG(LOG_PROFILE, fmt, ##__VA_ARGS__)
#define custom_msg(fmt, ...) MSG(LOG_CUSTOM, fmt, ##__VA_ARGS__)

#else

#define LOG(level, format, ...) 
#define _DSG(__flag__, __format__, ...)
#define DSG(__format__, ...)
#define MSG(level, format, ...)
#define dbg_msg(fmt, ...)
#define trace_msg(fmt, ...)
#define info_msg(fmt, ...)
#define err_msg(fmt, ...)
#define critical_msg(fmt, ...)
#define profile_msg(fmt, ...)
#define custom_msg(fmt, ...) 

#endif // LOG_ENABLE

#endif  /* __NPU_LOG_H__ */
