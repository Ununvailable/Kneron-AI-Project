/**
 * Kneron Peripheral API - UART
 *
 * Copyright (C) 2019 Kneron, Inc. All rights reserved.
 *
 */

//#define UART_DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "kdrv_uart.h"
//#include "kdrv_scu_ext.h"
#include "io.h"
#include "kdrv_clock.h" // for delay_us()
#include "kdrv_cmsis_core.h"


#define BACKSP_KEY 0x08
#define RETURN_KEY 0x0D
#define DELETE_KEY 0x7F
#define BELL 0x07

static kdrv_uart_handle_t handle0 = 0;


#define SERIAL_THR 0x00 /*  Transmitter Holding Register(Write).*/
#define SERIAL_RBR 0x00 /*  Receive Buffer register (Read).*/
#define SERIAL_IER 0x04 /*  Interrupt Enable register.*/
#define SERIAL_IIR 0x08 /*  Interrupt Identification register(Read).*/
#define SERIAL_FCR 0x08 /*  FIFO control register(Write).*/
#define SERIAL_LCR 0x0C /*  Line Control register.*/
#define SERIAL_MCR 0x10 /*  Modem Control Register.*/
#define SERIAL_LSR 0x14 /*  Line status register(Read) .*/
#define SERIAL_MSR 0x18 /*  Modem Status register (Read).*/
#define SERIAL_SPR 0x1C /*  Scratch pad register */
#define SERIAL_DLL 0x0  /*  Divisor Register LSB */
#define SERIAL_DLM 0x4  /*  Divisor Register MSB */
#define SERIAL_PSR 0x8  /* Prescale Divison Factor */

#define SERIAL_MDR 0x20
#define SERIAL_ACR 0x24
#define SERIAL_TXLENL 0x28
#define SERIAL_TXLENH 0x2C
#define SERIAL_MRXLENL 0x30
#define SERIAL_MRXLENH 0x34
#define SERIAL_PLR 0x38
#define SERIAL_FMIIR_PIO 0x3C
#define SERIAL_FEATURE 0x68

/* IER Register */
#define SERIAL_IER_DR 0x1  /* Data ready Enable */
#define SERIAL_IER_TE 0x2  /* THR Empty Enable */
#define SERIAL_IER_RLS 0x4 /* Receive Line Status Enable */
#define SERIAL_IER_MS 0x8  /* Modem Staus Enable */

/* IIR Register */
#define SERIAL_IIR_NONE 0x1    /* No interrupt pending */
#define SERIAL_IIR_RLS 0x6     /* Receive Line Status */
#define SERIAL_IIR_DR 0x4      /* Receive Data Ready */
#define SERIAL_IIR_TIMEOUT 0xc /* Receive Time Out */
#define SERIAL_IIR_TE 0x2      /* THR Empty */
#define SERIAL_IIR_MODEM 0x0   /* Modem Status */

/* FCR Register */
#define SERIAL_FCR_FE 0x1   /* FIFO Enable */
#define SERIAL_FCR_RXFR 0x2 /* Rx FIFO Reset */
#define SERIAL_FCR_TXFR 0x4 /* Tx FIFO Reset */

/* LCR Register */
#define SERIAL_LCR_LEN5 0x0
#define SERIAL_LCR_LEN6 0x1
#define SERIAL_LCR_LEN7 0x2
#define SERIAL_LCR_LEN8 0x3

#define SERIAL_LCR_STOP 0x4
#define SERIAL_LCR_EVEN 0x18        /* Even Parity */
#define SERIAL_LCR_ODD 0x8          /* Odd Parity */
#define SERIAL_LCR_PE 0x8           /* Parity Enable */
#define SERIAL_LCR_SETBREAK 0x40    /* Set Break condition */
#define SERIAL_LCR_STICKPARITY 0x20 /* Stick Parity Enable */
#define SERIAL_LCR_DLAB 0x80        /* Divisor Latch Access Bit */

/* LSR Register */
#define SERIAL_LSR_DR 0x1    /* Data Ready */
#define SERIAL_LSR_OE 0x2    /* Overrun Error */
#define SERIAL_LSR_PE 0x4    /* Parity Error */
#define SERIAL_LSR_FE 0x8    /* Framing Error */
#define SERIAL_LSR_BI 0x10   /* Break Interrupt */
#define SERIAL_LSR_THRE 0x20 /* THR Empty */
#define SERIAL_LSR_TE 0x40   /* Transmitte Empty */
#define SERIAL_LSR_DE 0x80   /* FIFO Data Error */

/* MCR Register */
#define SERIAL_MCR_DTR 0x1   /* Data Terminal Ready */
#define SERIAL_MCR_RTS 0x2   /* Request to Send */
#define SERIAL_MCR_OUT1 0x4  /* output    1 */
#define SERIAL_MCR_OUT2 0x8  /* output2 or global interrupt enable */
#define SERIAL_MCR_LPBK 0x10 /* loopback mode */

/* MSR Register */
#define SERIAL_MSR_DELTACTS 0x1 /* Delta CTS */
#define SERIAL_MSR_DELTADSR 0x2 /* Delta DSR */
#define SERIAL_MSR_TERI 0x4     /* Trailing Edge RI */
#define SERIAL_MSR_DELTACD 0x8  /* Delta CD */
#define SERIAL_MSR_CTS 0x10     /* Clear To Send */
#define SERIAL_MSR_DSR 0x20     /* Data Set Ready */
#define SERIAL_MSR_RI 0x40      /* Ring Indicator */
#define SERIAL_MSR_DCD 0x80     /* Data Carrier Detect */

/* MDR register */
#define SERIAL_MDR_MODE_SEL 0x03
#define SERIAL_MDR_UART 0x0
#define SERIAL_MDR_SIR 0x1
#define SERIAL_MDR_FIR 0x2

/* ACR register */
#define SERIAL_ACR_TXENABLE 0x1
#define SERIAL_ACR_RXENABLE 0x2
#define SERIAL_ACR_SET_EOT 0x4

#define IIR_CODE_MASK 0xf
#define SERIAL_IIR_TX_FIFO_FULL 0x10

#define UART_INITIALIZED (1 << 0)
#define UART_BASIC_CONFIGURED (1 << 2)
#define UART_FIFO_RX_CONFIGURED (1 << 3)
#define UART_FIFO_TX_CONFIGURED (1 << 4)
#define UART_TX_ENABLED (1 << 5)
#define UART_RX_ENABLED (1 << 6)
#define UART_LOOPBACK_ENABLED (1 << 7)

#define SERIAL_FIFO_DEPTH_REG 0x68

#define SERIAL_FIFO_DEPTH_16B 0x1
#define SERIAL_FIFO_DEPTH_32B 0x2
#define SERIAL_FIFO_DEPTH_64B 0x4
#define SERIAL_FIFO_DEPTH_128B 0x8

#define SERIAL_FIFO_TRIG_LVEL_1 0x0
#define SERIAL_FIFO_TRIG_LVEL_4 0x1
#define SERIAL_FIFO_TRIG_LVEL_8 0x2
#define SERIAL_FIFO_TRIG_LVEL_14 0x3

#define UART_IRQ_CNT_DBG 0
/***********************************************************************************
              Global variables
************************************************************************************/
void UART0_ISR(void);
void UART1_ISR(void);

uart_drv_ctx_t gDrvCtx;
static uint32_t UART_PORT[2] = {UART0_REG_BASE, UART1_REG_BASE};
uint32_t uart_baud_rate_map[11] = {BAUD_1200,BAUD_2400,BAUD_4800,BAUD_9600,BAUD_14400 ,BAUD_19200 ,BAUD_38400 ,BAUD_57600 ,BAUD_115200,BAUD_460800,BAUD_921600};
/***********************************************************************************
              local variables
************************************************************************************/

IRQn_Type gUartIRQTbl[2] = {
    UART0_IRQn,   //UART0
    UART1_IRQn,   //UART1
};

uart_isr_t gUartISRs[2] = {
    UART0_ISR,
    UART1_ISR};

uint32_t gUartClk[2] = {
    UART_CLOCK,
    UART_CLOCK};

static bool gDriverInitialized = false;

#if (defined(UART_IRQ_CNT_DBG) && UART_IRQ_CNT_DBG == 1)
volatile uint32_t tx_isr_cnt[TOTAL_UART_DEV]={0};
volatile uint32_t rx_isr_cnt[TOTAL_UART_DEV]={0};
#endif
/***********************************************************************************
              local functions
************************************************************************************/
static void uart_serial_init(DRVUART_PORT port_no, uint32_t baudrate, uint32_t parity, uint32_t num, uint32_t len, uint32_t interruptMode)
{
    uint32_t lcr;

    lcr = regUART_ctrl(port_no)->st.dw.kdrv_uart_lcr & ~SERIAL_LCR_DLAB;
    /* Set DLAB=1 */
    regUART_ctrl(port_no)->st.bf.kdrv_uart_lcr.DLAB = 1;
    /* Set baud rate */
    regUART_baudrate(port_no)->st.dw.kdrv_uart_dlm = ((baudrate & 0xff00) >> 8);
    regUART_baudrate(port_no)->st.dw.kdrv_uart_dll = (baudrate & 0xff);
    lcr &= 0xc0;

    switch (parity)
    {
    case PARITY_NONE:
        //do nothing
        break;
    case PARITY_ODD:
        lcr |= SERIAL_LCR_ODD;
        break;
    case PARITY_EVEN:
        lcr |= SERIAL_LCR_EVEN;
        break;
    case PARITY_MARK:
        lcr |= (SERIAL_LCR_STICKPARITY | SERIAL_LCR_ODD);
        break;
    case PARITY_SPACE:
        lcr |= (SERIAL_LCR_STICKPARITY | SERIAL_LCR_EVEN);
        break;

    default:
        break;
    }

    if (num == 2)
        lcr |= SERIAL_LCR_STOP;

    len -= 5;

    lcr |= len;

    regUART_ctrl(port_no)->st.dw.kdrv_uart_lcr = lcr;
    if (1 == interruptMode)
        regUART_ctrl(port_no)->st.dw.kdrv_uart_fcr = SERIAL_FCR_FE;
}

uint32_t uart_get_status(DRVUART_PORT port_no)
{
    uint32_t status;
    status = regUART_ctrl(port_no)->st.dw.kdrv_uart_lsr;
    return status;
}

static void CheckTxStatus(DRVUART_PORT port_no)
{
    uint32_t status;
    do
    {
        status = uart_get_status(port_no);
    } while (!((status & SERIAL_LSR_THRE) == SERIAL_LSR_THRE)); // wait until Tx ready
}

void CheckRxStatus(DRVUART_PORT port_no)
{
    uint32_t status;
    do
    {
        status = uart_get_status(port_no);
        if((status & SERIAL_IER_DR)==SERIAL_IER_DR) break;
        else osDelay(20);
    } while (1);
}

char uart_get_serial_char(DRVUART_PORT port_no)
{
    char Ch;
    uint32_t status;

    do
    {
        status = regUART_ctrl(port_no)->st.dw.kdrv_uart_lsr;
    } while (!((status & SERIAL_LSR_DR) == SERIAL_LSR_DR)); // wait until Rx ready
    Ch = regUART_ctrl(port_no)->st.bf.kdrv_uart_rbr.RBR;
    return (Ch);
}

#if 0
static int32_t kdrv_uart_get_default_timeout(uint32_t baud)
{
    int32_t timeout;
    switch (baud)
    {
    case BAUD_921600:
    {
        timeout = (DEFAULT_SYNC_TIMEOUT_CHARS_TIME * 100) / 9216;
        break;
    }
    case BAUD_460800:
    {
        timeout = (DEFAULT_SYNC_TIMEOUT_CHARS_TIME * 100) / 4608;
        break;
    }
    case BAUD_115200:
    {
        timeout = (DEFAULT_SYNC_TIMEOUT_CHARS_TIME * 100) / 1152;
        break;
    }
    case BAUD_57600:
    {
        timeout = (DEFAULT_SYNC_TIMEOUT_CHARS_TIME * 100) / 576;
        break;
    }
    case BAUD_38400:
    {
        timeout = (DEFAULT_SYNC_TIMEOUT_CHARS_TIME * 100) / 384;
        break;
    }
    case BAUD_19200:
    {
        timeout = (DEFAULT_SYNC_TIMEOUT_CHARS_TIME * 100) / 192;
        break;
    }
    case BAUD_14400:
    {
        timeout = (DEFAULT_SYNC_TIMEOUT_CHARS_TIME * 100) / 144;
        break;
    }
    case BAUD_9600:
    {
        timeout = (DEFAULT_SYNC_TIMEOUT_CHARS_TIME * 100) / 96;
        break;
    }
    case BAUD_4800:
    {
        timeout = (DEFAULT_SYNC_TIMEOUT_CHARS_TIME * 100) / 48;
        break;
    }
    case BAUD_2400:
    {
        timeout = (DEFAULT_SYNC_TIMEOUT_CHARS_TIME * 100) / 240;
        break;
    }
    case BAUD_1200:
    {
        timeout = (DEFAULT_SYNC_TIMEOUT_CHARS_TIME * 100) / 12;
        break;
    }
    default:
        timeout = DEFAULT_SYNC_TIMEOUT_CHARS_TIME;
    }
    return timeout;
}
#endif

static uart_driver_handle_t *uart_get_drv_hdl(uint16_t port)
{
#ifdef UART_DEBUG
    if (port >= MAX_UART_INST)
    {

        printf("Error: invalid port number\n");

        return NULL;
    }
#endif
    return gDrvCtx.uart_dev[port];
}

/* calculate fifo config: depth and trigger level
   pCfg->bEnFifo and pCfg->fifo_depth was set by client before calling in
*/
static kdrv_status_t kdrv_uart_calculate_fifo_cfg(uart_driver_handle_t *const pDrv, uint32_t val, kdrv_uart_fifo_config_t *pCfg)
{
    int32_t depth;

    if (pCfg->bEnFifo == false)
    {
        pCfg->fifo_trig_level = SERIAL_FIFO_TRIG_LVEL_1;
        return KDRV_STATUS_OK;
    }

    depth = pDrv->res.fifo_depth;

    if (val <= depth * 4)
    {
        pCfg->fifo_trig_level = SERIAL_FIFO_TRIG_LVEL_1;
    }
    else if ((val > depth * 4) && (val <= depth * 8))
    {
        pCfg->fifo_trig_level = SERIAL_FIFO_TRIG_LVEL_4;
    }
    else if ((val > depth * 8) && (val <= depth * 14))
    {
        pCfg->fifo_trig_level = SERIAL_FIFO_TRIG_LVEL_8;
    }
    else if (val > depth * 14)
    {
        pCfg->fifo_trig_level = SERIAL_FIFO_TRIG_LVEL_14;
    }
    return KDRV_STATUS_OK;
}

/**
  \fn          void UART_IRQHandler (UART_RESOURCES  *uart)
  \brief       UART Interrupt handler.
  \param[in]   uart  Pointer to UART resources
*/
static void UART_TX_ISR(uart_driver_handle_t *const uart)
{
    uint32_t status;
    uint32_t ww;
    uint16_t port_no;
    bool bFifo;

    ww = uart->iir;
    bFifo = ((ww & 0xc0) != 0) ? true : false;
    port_no = uart->uart_port;

    uart->info.xfer.tx_cnt += uart->info.xfer.batch_tx_num;
    if (uart->info.xfer.tx_num > uart->info.xfer.tx_cnt)
    {
        if (bFifo == false) //NO FIFO
        {
            do
            {
                status = regUART_ctrl(port_no)->st.dw.kdrv_uart_lsr;
            } while (!((status & SERIAL_LSR_THRE) == SERIAL_LSR_THRE)); // wait until Tx ready
            regUART_ctrl(port_no)->st.bf.kdrv_uart_thr.THR = *(uart->info.xfer.tx_buf++);
        }
        else //FIFO mode
        {
            uint32_t tx_num = MAX_FIFO_TX;
            if (uart->info.xfer.tx_num < uart->info.xfer.tx_cnt+MAX_FIFO_TX)
            {
                tx_num = uart->info.xfer.tx_num - uart->info.xfer.tx_cnt;
            }
            for(uint32_t i = 0; i < tx_num ; i++)
            {
                regUART_ctrl(port_no)->st.bf.kdrv_uart_thr.THR = *(uart->info.xfer.tx_buf++);
            }
            uart->info.xfer.batch_tx_num = tx_num;
        }
    }

    if (uart->info.xfer.tx_num <= uart->info.xfer.tx_cnt)
    {
        // need to add: determine if TX fifo is empty
        status = regUART_ctrl(port_no)->st.dw.kdrv_uart_lsr;
        if ((status & SERIAL_LSR_THRE) == SERIAL_LSR_THRE) //TX empty to make sure FIFO data to tranmit shift
        {
            regUART_ctrl(port_no)->st.bf.kdrv_uart_ier.THR_empty = 0;

            // Clear TX busy flag
            if (uart->info.cb_event)
                uart->info.cb_event(UART_TX_DONE);
            uart->info.status.tx_busy = 0;
        }
    }
#if (defined(UART_IRQ_CNT_DBG) && UART_IRQ_CNT_DBG == 1)
    tx_isr_cnt[uart->uart_port]++;
#endif
}

static void UART_RX_ISR(uart_driver_handle_t *const uart)
{
    uint32_t ww;
    uint16_t port_no;

    ww = uart->iir;

    port_no = uart->uart_port;

    if (((ww & SERIAL_IIR_DR) == SERIAL_IIR_DR) || ((ww & SERIAL_IIR_TIMEOUT) == SERIAL_IIR_TIMEOUT))
    {

        if ((ww & 0xc0) == 0) //non FIFO mode
        {
            uart->info.xfer.rx_buf[uart->info.xfer.write_idx] = (uint8_t)inw(UART_PORT[port_no] + SERIAL_RBR);
            uart->info.xfer.write_idx++;
            if(uart->info.xfer.write_idx >= uart->info.xfer.rx_num)
                uart->info.xfer.write_idx = 0;
            uart->info.xfer.rx_cnt++;

            if (uart->info.xfer.rx_cnt == uart->info.xfer.rx_num)
            {
                ww = inw(UART_PORT[port_no] + SERIAL_IER);
                if (uart->info.mode & UART_MODE_SYNC_RX){
                    ww &= ~SERIAL_IER_DR; // disable Rx empty interrupt
                    outw(UART_PORT[port_no] + SERIAL_IER, ww);
                }
                if (uart->info.cb_event)
                    uart->info.cb_event(UART_RX_DONE);
            }
        }
        else //FIFO mode
        {
            /* Read the data from FIFO buffer */

            ww = inw(UART_PORT[port_no] + SERIAL_LSR);
            if ((ww & SERIAL_LSR_DR) == SERIAL_LSR_DR)
            {
                while((inw(UART_PORT[port_no] + SERIAL_LSR) & SERIAL_LSR_DR) == SERIAL_LSR_DR)
                {
                    uart->info.xfer.rx_buf[uart->info.xfer.write_idx] = (uint8_t)inw(UART_PORT[port_no] + SERIAL_RBR);
                    uart->info.xfer.write_idx++;
                    if(uart->info.xfer.write_idx >= uart->info.xfer.rx_num)
                        uart->info.xfer.write_idx = 0;
                    uart->info.xfer.rx_cnt++;
                }

                if (uart->info.xfer.rx_num <= uart->info.xfer.rx_cnt)
                {
                    ww = inw(UART_PORT[port_no] + SERIAL_IER);
                    if (uart->info.mode & UART_MODE_SYNC_RX){
                        ww &= ~SERIAL_IER_DR; // disable Rx empty interrupt
                        outw(UART_PORT[port_no] + SERIAL_IER, ww);
                    }
                    if (uart->info.cb_event)
                        uart->info.cb_event(UART_RX_DONE);
                }

            }
            else
            {
                ww = uart->iir;
                if ((ww & SERIAL_IIR_TIMEOUT) == SERIAL_IIR_TIMEOUT) // means end of frame
                {

                    while((inw(UART_PORT[port_no] + SERIAL_LSR) & SERIAL_LSR_DR) == SERIAL_LSR_DR)
                    {
                        uart->info.xfer.rx_buf[uart->info.xfer.write_idx] = (uint8_t)inw(UART_PORT[port_no] + SERIAL_RBR);
                        uart->info.xfer.write_idx++;
                        if(uart->info.xfer.write_idx >= uart->info.xfer.rx_num)
                            uart->info.xfer.write_idx = 0;
                        uart->info.xfer.rx_cnt++;
                    }

                    if (uart->info.xfer.rx_num <= uart->info.xfer.rx_cnt)
                    {
                        ww = inw(UART_PORT[port_no] + SERIAL_IER);
											
                        if (uart->info.mode & UART_MODE_SYNC_RX){
                            ww &= ~SERIAL_IER_DR; // disable Rx empty interrupt
                            outw(UART_PORT[port_no] + SERIAL_IER, ww);
                        }
                        if (uart->info.cb_event)
                            uart->info.cb_event(UART_RX_TIMEOUT);
                    }
                }
            }
        }
        if (uart->info.mode & UART_MODE_SYNC_RX){
            uart->info.status.rx_busy = 0;
        }

#if (defined(UART_IRQ_CNT_DBG) && UART_IRQ_CNT_DBG == 1)
        rx_isr_cnt[uart->uart_port]++;
#endif
    }
}

/***********************************************************************************
              global functions
************************************************************************************/

void UART_ISR(uint8_t port_no)
{
    uart_driver_handle_t *pHdl = uart_get_drv_hdl(port_no);

    uint32_t ww = regUART_ctrl(port_no)->st.dw.kdrv_uart_iir;

    pHdl->iir = ww;

    if ((ww & IIR_CODE_MASK) == SERIAL_IIR_RLS) // errors: overrun/parity/framing/break
    {
        ww = regUART_ctrl(port_no)->st.dw.kdrv_uart_lsr; //Read LSR to reset interrupt

        if (ww & SERIAL_LSR_OE)
        {
            pHdl->info.status.rx_overflow = 1;
        }

        if (ww & SERIAL_LSR_PE)
        {
            pHdl->info.status.rx_parity_error = 1;
        }

        if (ww & SERIAL_LSR_BI)
        {
            pHdl->info.status.rx_break = 1;
        }

        if (ww & SERIAL_LSR_FE)
        {
            pHdl->info.status.rx_framing_error = 1;
        }
    }
    if (((ww & IIR_CODE_MASK) == SERIAL_IIR_DR)          // Rx data ready in FIFO
             || ((ww & IIR_CODE_MASK) == SERIAL_IIR_TIMEOUT)) // Character Reception Timeout
    {
        if (pHdl->info.status.rx_busy)
        {
            UART_RX_ISR(pHdl);
        }
        else
        {
            ww = regUART_ctrl(port_no)->st.dw.kdrv_uart_rbr; //Read RBR to reset interrupt
        }
    }
    if ((ww & IIR_CODE_MASK) == SERIAL_IIR_TE) // Transmitter Holding Register Empty
    {
        if (pHdl->info.status.tx_busy)
        {
            UART_TX_ISR(pHdl);
        }
    }
}

void UART0_ISR(void)
{
    UART_ISR(0);
}

void UART1_ISR(void)
{
    UART_ISR(1);
}


/* Init the UART device driver, it shall be called once in lifecycle
*/
kdrv_status_t kdrv_uart_initialize(void)
{
    if (gDriverInitialized == false)
    {
        memset(&gDrvCtx, 0, sizeof(gDrvCtx));
        gDriverInitialized = true;
    }

    return KDRV_STATUS_OK;
}

kdrv_status_t kdrv_uart_console_init(uint8_t uart_dev, uint32_t baudrate, kdrv_uart_callback_t cb)
{
    kdrv_uart_handle_t handle;
    kdrv_status_t sts = kdrv_uart_open(&handle, uart_dev,
                                       UART_MODE_SYNC_RX | UART_MODE_SYNC_TX, cb);
    if (sts != KDRV_STATUS_OK)
        return sts;

    kdrv_uart_config_t cfg;
    cfg.baudrate = uart_baud_rate_map[baudrate];
    cfg.data_bits = 8;
    cfg.frame_length = 0;
    cfg.stop_bits = 1;
    cfg.parity_mode = PARITY_NONE;
    cfg.fifo_en = false;
#ifdef UART_RX_IRQ
    cfg.irq_en = false;
#endif

    sts = kdrv_uart_configure(handle, UART_CTRL_CONFIG, (void *)&cfg);
    if (sts != KDRV_STATUS_OK)
        return sts;
    
    handle0 = uart_dev;
    
    return KDRV_STATUS_OK;
}

kdrv_status_t kdrv_uart_uninitialize(void)
{
    return KDRV_STATUS_OK;
}

/*
  Open one UART port
Input:
  com_port: UART port id
  mode: that is a combination of Tx mode|Rx mode, make sure set both, such as UART_MODE_ASYN_RX|UART_MODE_SYNC_TX
  cb: callback function

Output:
  return device handle: >=0 means success; -1 means open fail
*/
kdrv_status_t kdrv_uart_open(kdrv_uart_handle_t *handle, uint8_t com_port, uint32_t mode, kdrv_uart_callback_t cb)
{
    uint32_t ww;

#ifdef UART_DEBUG
    if (com_port >= TOTAL_UART_DEV)
    {
        printf("Invalid com_port\n");
        return KDRV_STATUS_ERROR;
    }
#endif

    if (gDrvCtx.active_dev[com_port] == true)
    {
#ifdef UART_DEBUG
        printf("This UART device has been opened\n");
#endif
        return KDRV_STATUS_ERROR;
    }

    uart_driver_handle_t *pDrv = (uart_driver_handle_t *)malloc(sizeof(uart_driver_handle_t));
    if (pDrv == NULL)
    {
#ifdef UART_DEBUG
        printf("Error: memory alloc failed\n");
#endif
        return KDRV_STATUS_ERROR;
    }

    if (((mode & UART_MODE_ASYN_TX) || (mode & UART_MODE_ASYN_RX)) && (cb == NULL))
    {
#ifdef UART_DEBUG
        printf("Error: Async mode needs callback function\n");
#endif
        return KDRV_STATUS_ERROR;
    }

    gDrvCtx.uart_dev[com_port] = pDrv;

    pDrv->uart_port = com_port;
    pDrv->state = UART_UNINIT;
    // now use a common capbilities for all UARTs, may introduce an array if diff UART has diff capbilities
    pDrv->info.cb_event = cb;
    pDrv->info.mode = mode;
    pDrv->info.status.tx_busy = 0;
    pDrv->info.status.rx_busy = 0;
    pDrv->info.status.tx_underflow = 0;
    pDrv->info.status.rx_overflow = 0;
    pDrv->info.status.rx_break = 0;
    pDrv->info.status.rx_framing_error = 0;
    pDrv->info.status.rx_parity_error = 0;

    pDrv->info.xfer.tx_num = 0;
    pDrv->info.xfer.rx_num = 0;
    pDrv->info.xfer.tx_cnt = 0;
    pDrv->info.xfer.rx_cnt = 0;

    pDrv->res.irq_num = gUartIRQTbl[com_port];
    pDrv->res.isr = gUartISRs[com_port];

    pDrv->res.fifo_depth = regUART_feature(com_port)->bf.kdrv_uart_feature.FIFO_DEPTH;

    pDrv->res.fifo_len = 16 * ww;

    pDrv->res.hw_base = UART_PORT[com_port];
    pDrv->res.clock = gUartClk[com_port];

    pDrv->nTimeOutRx = DEFAULT_SYNC_TIMEOUT_CHARS_TIME; //suppose default baud = 115200
    pDrv->nTimeOutTx = DEFAULT_SYNC_TIMEOUT_CHARS_TIME; //suppose default baud = 115200

    pDrv->res.rx_fifo_threshold = SERIAL_FIFO_TRIG_LVEL_14;
    pDrv->res.tx_fifo_threshold = SERIAL_FIFO_TRIG_LVEL_14;
    pDrv->config.fifo_en = 0;

    gDrvCtx.total_open_uarts++;
    gDrvCtx.active_dev[com_port] = true;

    pDrv->info.flags |= UART_INITIALIZED;

    // enable clocks / power / irq
    {
        kdrv_uart_fifo_config_t cfg;

        /* config FIFO trigger value with default value or client set via control API*/
        cfg.fifo_trig_level = pDrv->res.rx_fifo_threshold;
        cfg.bEnFifo = pDrv->config.fifo_en;
        kdrv_uart_configure(com_port, UART_CTRL_FIFO_RX, (void *)&cfg);

        cfg.fifo_trig_level = pDrv->res.tx_fifo_threshold;
        cfg.bEnFifo = (cfg.fifo_trig_level == 0) ? false : true;
        kdrv_uart_configure(com_port, UART_CTRL_FIFO_TX, (void *)&cfg);

        NVIC_ClearPendingIRQ((IRQn_Type)pDrv->res.irq_num);
    }

    *handle = (kdrv_uart_handle_t)com_port;

    return KDRV_STATUS_OK;
}

/* close the device
Input:
   handle: device handle
return:
   0 - success; -1 - failure
*/
kdrv_status_t kdrv_uart_close(kdrv_uart_handle_t handle)
{
    uint32_t com_port = handle;

#ifdef UART_DEBUG
    if (com_port >= TOTAL_UART_DEV)
    {
        printf("Invalid parameter\n");
        return KDRV_STATUS_ERROR;
    }

    if (gDrvCtx.active_dev[com_port] == false)
    {
        printf("This UART device has been closed\n");
        return KDRV_STATUS_ERROR;
    }
#endif

    uart_driver_handle_t *pDrv = gDrvCtx.uart_dev[com_port];

    if (pDrv->info.flags == 0)
    {
        // Driver not initialized
        return KDRV_STATUS_ERROR;
    }

    // Reset USART status flags
    pDrv->info.flags = 0;

    free(gDrvCtx.uart_dev[com_port]);
    gDrvCtx.uart_dev[com_port] = NULL;
    gDrvCtx.active_dev[com_port] = false;
    gDrvCtx.total_open_uarts--;

    return KDRV_STATUS_OK;
}

/*
Set control for the device
Input:
    handle: device handle
    prop: control enumeration
    pVal: pointer to control value/structure
return:
    None
*/
kdrv_status_t kdrv_uart_configure(kdrv_uart_handle_t handle, kdrv_uart_control_t prop, uint8_t *pVal)
{
    uint32_t com_port = handle;

#ifdef UART_DEBUG
    if (com_port >= TOTAL_UART_DEV)
    {
        printf("Invalid parameter\n");
        return KDRV_STATUS_ERROR;
    }

    if (gDrvCtx.active_dev[com_port] == false)
    {
        printf("This UART device has been closed\n");
        return KDRV_STATUS_ERROR;
    }
#endif

    uart_driver_handle_t *pDrv = gDrvCtx.uart_dev[com_port];

#ifdef UART_DEBUG
    if ((pDrv->info.flags & UART_INITIALIZED) == 0)
    {
        // Return error, if USART is not initialized
        return KDRV_STATUS_ERROR;
    }
#endif

    DRVUART_PORT port_no = (DRVUART_PORT)pDrv->uart_port;
    switch (prop)
    {
    case UART_CTRL_CONFIG:
    {
        kdrv_uart_config_t cfg = *(kdrv_uart_config_t *)pVal;
        uint32_t baudrate = cfg.baudrate;
        uint32_t parity = cfg.parity_mode;
        uint32_t num = cfg.stop_bits;
        uint32_t len = cfg.data_bits;

        uart_serial_init(port_no, baudrate, parity, num, len, 0);

        pDrv->config.baudrate = baudrate;
        pDrv->config.data_bits = cfg.data_bits;
        pDrv->config.stop_bits = num;
        pDrv->config.parity_mode = parity;
        pDrv->config.fifo_en = cfg.fifo_en;
        #ifdef UART_RX_IRQ
        pDrv->config.irq_en = cfg.irq_en;
        #endif

        pDrv->res.tx_fifo_threshold = 0;
        pDrv->res.rx_fifo_threshold = 0;

        pDrv->nTimeOutTx = 0; //kdrv_uart_get_default_timeout(baudrate);
        pDrv->nTimeOutRx = 0; //kdrv_uart_get_default_timeout(baudrate);

        pDrv->info.flags |= UART_BASIC_CONFIGURED;

        pDrv->state = UART_INIT_DONE;
        break;
    }

    case UART_CTRL_TX_EN:
    {
        if (*pVal == 0)
        {
            regUART_ctrl(port_no)->st.bf.kdrv_uart_ier.THR_empty = 0; // disable TX
            pDrv->info.flags &= ~UART_TX_ENABLED;
        }
        else
        {
            regUART_ctrl(port_no)->st.bf.kdrv_uart_ier.THR_empty = 1; // enable TX
            pDrv->info.flags |= UART_TX_ENABLED;
        }

        break;
    }

    case UART_CTRL_RX_EN:
    {
        if (*pVal == 0)
        {
            regUART_ctrl(port_no)->st.bf.kdrv_uart_ier.RD_available = 0; // disable Rx
            pDrv->info.flags &= ~UART_RX_ENABLED;
        }
        else
        {
            regUART_ctrl(port_no)->st.bf.kdrv_uart_ier.RD_available = 1; // enable Rx
            pDrv->info.flags |= UART_RX_ENABLED;
        }

        break;
    }

    case UART_CTRL_ABORT_TX:
    {
        /* disable Tx interrupt */
        regUART_ctrl(port_no)->st.bf.kdrv_uart_ier.THR_empty = 0;

        /* reset Tx FIFO */
        regUART_ctrl(port_no)->st.bf.kdrv_uart_fcr.tx_fifo_reset = 1;
        regUART_ctrl(port_no)->st.bf.kdrv_uart_fcr.fifo_en = 1;

        pDrv->info.status.tx_busy = 0;

        break;
    }

    case UART_CTRL_ABORT_RX:
    {
        /* disable Rx interrupt */
        regUART_ctrl(port_no)->st.bf.kdrv_uart_ier.RD_available = 0;

        /* reset Rx FIFO */
        regUART_ctrl(port_no)->st.bf.kdrv_uart_fcr.rx_fifo_reset = 1;
        regUART_ctrl(port_no)->st.bf.kdrv_uart_fcr.fifo_en = 1;

        pDrv->info.status.rx_busy = 0;

        break;
    }

    case UART_CTRL_FIFO_RX:
    {
        kdrv_uart_fifo_config_t *pCfg = (kdrv_uart_fifo_config_t *)pVal;
        uint8_t trig_lvl = 0x3 & pCfg->fifo_trig_level;

        if (pCfg->bEnFifo)
        {
            regUART_ctrl(port_no)->st.bf.kdrv_uart_fcr.rxfifo_trgl = trig_lvl;
            regUART_ctrl(port_no)->st.bf.kdrv_uart_fcr.rx_fifo_reset = 1;
            regUART_ctrl(port_no)->st.bf.kdrv_uart_fcr.fifo_en = 1;
        }
        else
        {
            /*
              becasue RX/TX FIFO can only be enable/disabled simutanously,
              cannot be set individually, so set trigger val to 0 for FIFO disable
            */
            regUART_ctrl(port_no)->st.bf.kdrv_uart_fcr.rxfifo_trgl = trig_lvl;
            regUART_ctrl(port_no)->st.bf.kdrv_uart_fcr.rx_fifo_reset = 1;
            regUART_ctrl(port_no)->st.bf.kdrv_uart_fcr.fifo_en = 1;
        }

        pDrv->info.flags |= UART_FIFO_RX_CONFIGURED;

        break;
    }

    case UART_CTRL_FIFO_TX:
    {
        kdrv_uart_fifo_config_t *pCfg = (kdrv_uart_fifo_config_t *)pVal;
        uint8_t trig_lvl = 0x3 & pCfg->fifo_trig_level;

        if (pCfg->bEnFifo)
        {
            regUART_ctrl(port_no)->st.bf.kdrv_uart_fcr.txfifo_trgl = trig_lvl;
            regUART_ctrl(port_no)->st.bf.kdrv_uart_fcr.tx_fifo_reset = 1;
            regUART_ctrl(port_no)->st.bf.kdrv_uart_fcr.fifo_en = 1;
        }
        else
        {
            /*
              becasue RX/TX FIFO can only be enable/disabled simutanously,
              cannot be set individually, so set trigger val to 0 for FIFO disable
            */
            regUART_ctrl(port_no)->st.bf.kdrv_uart_fcr.txfifo_trgl = trig_lvl;
            regUART_ctrl(port_no)->st.bf.kdrv_uart_fcr.tx_fifo_reset = 1;
            regUART_ctrl(port_no)->st.bf.kdrv_uart_fcr.fifo_en = 1;
        }

        pDrv->info.flags |= UART_FIFO_TX_CONFIGURED;

        break;
    }

    case UART_CTRL_LOOPBACK:
    {
        if (*pVal == 0)
        {
            regUART_ctrl(port_no)->st.bf.kdrv_uart_mcr.loopback = 0; // disable loopback
        }
        else
        {
            regUART_ctrl(port_no)->st.bf.kdrv_uart_mcr.loopback = 1; // enable loopback
        }

        pDrv->info.flags |= UART_LOOPBACK_ENABLED;

        break;
    }
    case UART_CTRL_TIMEOUT_RX:
    {
        pDrv->nTimeOutRx = (int32_t)*pVal;

        break;
    }

    case UART_CTRL_TIMEOUT_TX:
    {
        pDrv->nTimeOutTx = (int32_t)*pVal;

        break;
    }

    default:
        break;
    }
    return KDRV_STATUS_OK;
}

kdrv_status_t kdrv_uart_configure_baudrate_change(kdrv_uart_handle_t handle, uint8_t nBaudrateIdx)
{
    kdrv_uart_config_t cfg;
    cfg.baudrate = uart_baud_rate_map[nBaudrateIdx];
    cfg.data_bits = 8;
    cfg.frame_length = 0;
    cfg.stop_bits = 1;
    cfg.parity_mode = PARITY_NONE;
    cfg.fifo_en = true;
 #ifdef UART_RX_IRQ
    cfg.irq_en = false;
 #endif
    kdrv_status_t sts = kdrv_uart_configure(handle, UART_CTRL_CONFIG, (void *)&cfg);

    if (sts != KDRV_STATUS_OK)
        return sts;

//    g_bUartTxDone = false;
//    g_bUartTxDone = false;

    return KDRV_STATUS_OK;
}


uint32_t kdrv_uart_baud_rate_array_search(uint8_t nBaudrateIdx)
{
    if (nBaudrateIdx >= (sizeof(uart_baud_rate_map)/sizeof(uart_baud_rate_map[0])))
    {
        return 0;
    }
    else
    {
        return uart_baud_rate_map[nBaudrateIdx];
    }
}


/*
Write data to KL720 device, such as command, parameters, but not suitable for chunk data
Input:
    hdl: device handle
    buf: data buffer
    len: data buffer length
return:
    driver status
*/
kdrv_status_t kdrv_uart_write(kdrv_uart_handle_t handle, uint8_t *data, uint32_t len)
{
    uint32_t com_port = handle;

#ifdef UART_DEBUG
    if (com_port >= TOTAL_UART_DEV)
    {
        printf("Invalid parameter\n");
        return KDRV_STATUS_ERROR;
    }

    if (gDrvCtx.active_dev[com_port] == false)
    {
        printf("This UART device has been closed\n");
        return KDRV_STATUS_ERROR;
    }
#endif

    uart_driver_handle_t *pDrv = gDrvCtx.uart_dev[com_port];

#ifdef UART_DEBUG
    if ((data == NULL) || (len == 0))
    {
        // Invalid parameters
        return KDRV_STATUS_ERROR;
    }
#endif

    if (pDrv->info.status.tx_busy == 1)
    {
        // Send is not completed yet
        return KDRV_STATUS_UART_TX_RX_BUSY;
    }


    pDrv->info.status.tx_busy = 1;
    uint8_t enable = 1;
    uint32_t write_len;
    write_len = (len>MAX_FIFO_TX)? MAX_FIFO_TX : len;

    // Save transmit buffer info
    pDrv->info.xfer.tx_buf          = (uint8_t *)data;
    pDrv->info.xfer.tx_num          = len;
    pDrv->info.xfer.batch_tx_num    = write_len;
    pDrv->info.xfer.tx_cnt          = 0;
    for(uint32_t i = 0; i<write_len; i++)
    {
        outw(UART_PORT[pDrv->uart_port] + SERIAL_THR, *pDrv->info.xfer.tx_buf++);
    }
    kdrv_uart_configure(pDrv->uart_port, UART_CTRL_TX_EN, &enable);

    NVIC_SetVector((IRQn_Type)pDrv->res.irq_num, (uint32_t)pDrv->res.isr);
    NVIC_EnableIRQ((IRQn_Type)pDrv->res.irq_num);

    if (pDrv->info.mode & UART_MODE_ASYN_TX)
    {
        /* setup TX FIFO
           re-calculate FIFO trigger value based on buffer length
        */
        if (pDrv->config.fifo_en == true)
        {
            kdrv_uart_fifo_config_t cfg;
            cfg.bEnFifo = true;
            cfg.fifo_trig_level = 0; // init with 0
            kdrv_uart_calculate_fifo_cfg(pDrv, len, &cfg);
            kdrv_uart_configure(com_port, UART_CTRL_FIFO_TX, (uint8_t *)&cfg);
        }
        return KDRV_STATUS_UART_TX_RX_BUSY;
    }
    else if (pDrv->info.mode & UART_MODE_SYNC_TX)
    {
        if (pDrv->info.cb_event)
                pDrv->info.cb_event(UART_TRANSFER_COMPLETE);
        else
        {
            while(pDrv->info.status.tx_busy)
            {
                __WFI();
            }
        }
        return KDRV_STATUS_OK;
    }
    else
    {
#ifdef UART_DEBUG
        printf("Error: Sync/Async mode was not set\n");
#endif
        return KDRV_STATUS_ERROR;
    }
}

/*
Read character data from KL720 device
Input:
    handle: device handle
return:
    character data
*/
kdrv_status_t kdrv_uart_get_char(kdrv_uart_handle_t handle, char *ch)
{
    uint32_t com_port = handle;

#ifdef UART_DEBUG
    if (com_port >= TOTAL_UART_DEV)
    {
        printf("Invalid parameter\n");
        return KDRV_STATUS_ERROR;
    }

    if (gDrvCtx.active_dev[com_port] == false)
    {
        printf("This UART device has been closed\n");
        return KDRV_STATUS_ERROR;
    }
#endif

    uart_driver_handle_t *pDrv = gDrvCtx.uart_dev[com_port];

    // Check if receiver is busy
    if (pDrv->info.status.rx_busy == 1)
    {
        return KDRV_STATUS_UART_TX_RX_BUSY;
    }

    pDrv->info.status.rx_busy = 1;

    // Clear RX status
    pDrv->info.status.rx_break = 0;
    pDrv->info.status.rx_framing_error = 0;
    pDrv->info.status.rx_overflow = 0;
    pDrv->info.status.rx_parity_error = 0;

    // Save receive buffer info
    pDrv->info.xfer.rx_buf = (uint8_t *)ch;
    pDrv->info.xfer.rx_cnt = 0;
    pDrv->info.xfer.write_idx = 0;
    pDrv->info.xfer.read_idx = 0;
    // Save number of data to be received
    pDrv->info.xfer.rx_num = 1;

    uint8_t enable = 1;
    kdrv_uart_configure(pDrv->uart_port, UART_CTRL_RX_EN, &enable);

    NVIC_SetVector((IRQn_Type)pDrv->res.irq_num, (uint32_t)pDrv->res.isr);
    NVIC_EnableIRQ((IRQn_Type)pDrv->res.irq_num);

    if (pDrv->info.mode & UART_MODE_ASYN_RX)
    {
        return KDRV_STATUS_UART_TX_RX_BUSY;
    }
    else if (pDrv->info.mode & UART_MODE_SYNC_RX)
    {
        if (pDrv->info.cb_event)
                pDrv->info.cb_event(UART_REVEIVE_COMPLETE);
        else
        {
            while(pDrv->info.status.rx_busy)
            {
                __WFI();
            }
        }
    }
    return KDRV_STATUS_OK;
}

/**
  Read data from UART receiver.
Input
  data:  buffer for receving data
  len:   size  Data buffer size in bytes
  handle:  Driver handle
return
    driver status
*/
kdrv_status_t kdrv_uart_read(kdrv_uart_handle_t handle, uint8_t *data, uint32_t len)
{
    uint32_t com_port = handle;

#ifdef UART_DEBUG
    if (com_port >= TOTAL_UART_DEV)
    {
        printf("Invalid parameter\n");
        return KDRV_STATUS_ERROR;
    }

    if (gDrvCtx.active_dev[com_port] == false)
    {
        printf("This UART device has been closed\n");
        return KDRV_STATUS_ERROR;
    }
#endif

    uart_driver_handle_t *pDrv = gDrvCtx.uart_dev[com_port];

#ifdef UART_DEBUG
    if ((data == NULL) || (len == 0))
    {
        // Invalid parameters
        return KDRV_STATUS_INVALID_PARAM;
    }
#endif

    // Check if receiver is busy
    if (pDrv->info.status.rx_busy == 1)
    {
        return KDRV_STATUS_UART_TX_RX_BUSY;
    }

    pDrv->info.status.rx_busy = 1;

    // Clear RX status
    pDrv->info.status.rx_break = 0;
    pDrv->info.status.rx_framing_error = 0;
    pDrv->info.status.rx_overflow = 0;
    pDrv->info.status.rx_parity_error = 0;

    // Save receive buffer info
    pDrv->info.xfer.rx_buf = (uint8_t *)data;
    pDrv->info.xfer.rx_cnt = 0;
    pDrv->info.xfer.write_idx = 0;
    pDrv->info.xfer.read_idx = 0;
    // Save number of data to be received
    pDrv->info.xfer.rx_num = len;

    uint8_t enable = 1;
    kdrv_uart_configure(pDrv->uart_port, UART_CTRL_RX_EN, &enable);

    NVIC_SetVector((IRQn_Type)pDrv->res.irq_num, (uint32_t)pDrv->res.isr);
    NVIC_EnableIRQ((IRQn_Type)pDrv->res.irq_num);

    if (pDrv->info.mode & UART_MODE_ASYN_RX)
    {
        return KDRV_STATUS_UART_TX_RX_BUSY;
    }
    else if (pDrv->info.mode & UART_MODE_SYNC_RX)
    {
        if (pDrv->info.cb_event)
                pDrv->info.cb_event(UART_REVEIVE_COMPLETE);
            else
            {
            while(pDrv->info.status.rx_busy)
                    {
                __WFI();
            }
        }
        return KDRV_STATUS_OK;
    }
    else
    {
#ifdef UART_DEBUG
        printf("Error: Sync/Async mode was not set\n");
#endif
        return KDRV_STATUS_ERROR;
    }
}

/* get char number in Rx buffer 
Input:
    handle: device handle
Return:
    Received bytes 
*/
uint32_t kdrv_uart_get_rx_count(kdrv_uart_handle_t handle)
{
    uint32_t data;
    uint32_t com_port = handle;

    uart_driver_handle_t *pDrv = gDrvCtx.uart_dev[com_port];
    data = pDrv->info.xfer.rx_cnt;
    return data;
}

uint32_t kdrv_uart_get_tx_count(kdrv_uart_handle_t handle)
{
    uint32_t data;
    uint32_t com_port = handle;

    uart_driver_handle_t *pDrv = gDrvCtx.uart_dev[com_port];
    data = pDrv->info.xfer.tx_cnt;
    return data;
}

uint32_t kdrv_uart_GetRxBufSize(kdrv_uart_handle_t handle)
{
    uint32_t com_port = handle;

    uart_driver_handle_t *pDrv = gDrvCtx.uart_dev[com_port];
    return pDrv->info.xfer.rx_num;
}

uint32_t kdrv_uart_GetWriteIndex(kdrv_uart_handle_t handle)
{
    uint32_t com_port = handle;

    uart_driver_handle_t *pDrv = gDrvCtx.uart_dev[com_port];
    return pDrv->info.xfer.write_idx;
}

uint32_t kdrv_uart_GetReadIndex(kdrv_uart_handle_t handle)
{
    uint32_t com_port = handle;

    uart_driver_handle_t *pDrv = gDrvCtx.uart_dev[com_port];
    return pDrv->info.xfer.read_idx;
}

uint32_t kdrv_uart_SetWriteIndex(kdrv_uart_handle_t handle, uint32_t index)
{
    uint32_t com_port = handle;

    if (com_port >= TOTAL_UART_DEV) {
        //printf("Invalid parameter\n");
        return KDRV_STATUS_INVALID_PARAM;
    }

    if (gDrvCtx.active_dev[com_port] == false)
    {
        //printf("This UART device is not active\n");
        return KDRV_STATUS_INVALID_PARAM;
    }

    uart_driver_handle_t *pDrv = gDrvCtx.uart_dev[com_port];
    pDrv->info.xfer.write_idx = index;
    return KDRV_STATUS_OK;
}

uint32_t kdrv_uart_SetReadIndex(kdrv_uart_handle_t handle, uint32_t index)
{
    uint32_t com_port = handle;

    if (com_port >= TOTAL_UART_DEV) {
        //printf("Invalid parameter\n");
        return KDRV_STATUS_INVALID_PARAM;
    }

    if (gDrvCtx.active_dev[com_port] == false)
    {
        //printf("This UART device is not active\n");
        return KDRV_STATUS_INVALID_PARAM;
    }

    uart_driver_handle_t *pDrv = gDrvCtx.uart_dev[com_port];
    pDrv->info.xfer.read_idx = index;
    return KDRV_STATUS_OK;
}

kdrv_status_t kdrv_uart_check_tx_busy(void)
{
    uint8_t i;
    uart_driver_handle_t *pDrv;

    for(i = 0; i < MAX_UART_INST; i++) {
        pDrv = gDrvCtx.uart_dev[i];
        if (pDrv->info.status.tx_busy == 1)
        {
            // Send is not completed yet
            return KDRV_STATUS_UART_TX_RX_BUSY;
        }
    }
    return KDRV_STATUS_OK;
}

int32_t fputc(int ch, FILE *f)
{
    char cc;

    if (ch != '\0')
    {
        cc = ch;
        kdrv_uart_write(handle0, (uint8_t *)&cc, 1);
    }

    if (ch == '\n')
    {
        cc = '\r';
        kdrv_uart_write(handle0, (uint8_t *)&cc, 1);
    }

    return ch;
}

int32_t fgetc(FILE *f)
{
    char c;
    kdrv_uart_read(handle0, (uint8_t *)&c, 1);
    return c;
}

int32_t puts(const char *str)
{
    const char *cp;
    for (cp = str; *cp != 0; cp++) {
        printf("%c", *cp);
    }
    return 0;
}

void kdrv_uart_put_char(DRVUART_PORT com_port, char Ch)
{
	
    if(Ch!='\0')
    {
        CheckTxStatus(com_port);
        regUART_ctrl(com_port)->st.dw.kdrv_uart_thr = Ch;
    }

    if (Ch == '\n')
    {
        CheckTxStatus(com_port);
        regUART_ctrl(com_port)->st.dw.kdrv_uart_thr = '\n';
    }
}

void kdrv_put_str(DRVUART_PORT com_port, char *str)
{
    char *cp;
    for(cp = str; *cp != 0; cp++)
        kdrv_uart_put_char(com_port, *cp);
}

int32_t kdrv_uart_gets(DRVUART_PORT com_port, char *buf)
{
    char    *cp;
    char    data;
    uint32_t  count;
    count = 0;
    cp = buf;

    do
    {
        kdrv_uart_get_char(com_port,&data);

        switch(data)
        {
            case RETURN_KEY:
                if(count < 256)
                {
                    *cp = '\0';
                    kdrv_uart_put_char(com_port, '\n');
                }
                break;
            case BACKSP_KEY:
            case DELETE_KEY:
                if(count)
                {
                    count--;
                    *(--cp) = '\0';
                    kdrv_put_str(com_port, "\b \b");
                }
                break;
            default:
                if( data > 0x1F && data < 0x7F && count < 256)
                {
                    *cp = (char)data;
                    cp++;
                    count++;
                    kdrv_uart_put_char(com_port, data);
                }
                break;
        }
    } while(data != RETURN_KEY);

  return (count);
}


