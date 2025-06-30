/**
 * @file        kdrv_uart.h
 * @brief       Kneron UART driver
 * @details     Here are the design highlight points:\n
 *              * The architecture adopts a lightweight non-thread design\n
 *              * ISR driven architecture.\n
 *              * Can support both synchronous and asynchronous mode\n
 *              * Utilizes FIFO advantage to reduce interrupts and improve robust to accommodate more latency than normal.
 * @version     1.0
 * @copyright   (c) 2020 Kneron Inc. All right reserved.
 */
#ifndef __KDRV_UART_H_
#define __KDRV_UART_H_

#ifndef NON_OS
#include "cmsis_os2.h"
#endif
#include "kdrv_status.h"
#include "Driver_USART.h"
#include "system_config.h"
#include "regbase.h"
#include "base.h"
/**
 * @brief Enumerations of UART baud rate.
 */
#define BAUD_921600 (UART_CLOCK / 14745600)     /**< UART baud rate: 921600. */
#define BAUD_460800 (UART_CLOCK / 7372800)      /**< UART baud rate: 460800. */
#define BAUD_115200 (UART_CLOCK / 1843200)      /**< UART baud rate: 115200. */
#define BAUD_57600 (UART_CLOCK / 921600)        /**< UART baud rate: 57600. */
#define BAUD_38400 (UART_CLOCK / 614400)        /**< UART baud rate: 38400. */
#define BAUD_19200 (UART_CLOCK / 307200)        /**< UART baud rate: 19200. */
#define BAUD_14400 (UART_CLOCK / 230400)        /**< UART baud rate: 14400. */
#define BAUD_9600 (UART_CLOCK / 153600)         /**< UART baud rate: 9600. */
#define BAUD_4800 (UART_CLOCK / 76800)          /**< UART baud rate: 4800. */
#define BAUD_2400 (UART_CLOCK / 38400)          /**< UART baud rate: 2400. */
#define BAUD_1200 (UART_CLOCK / 19200)          /**< UART baud rate: 1200. */

/**
 * @brief The definition of UART parity.
 */
#define PARITY_NONE 0       /**< Disable Parity */
#define PARITY_ODD 1        /**< Odd Parity */
#define PARITY_EVEN 2       /**< Even Parity */
#define PARITY_MARK 3       /**< Stick odd Parity */
#define PARITY_SPACE 4      /**< Stick even Parity */

#define MAX_UART_INST 5 //max uart instance


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

#define DEFAULT_SYNC_TIMEOUT_CHARS_TIME 0 //default is no timeout
typedef int32_t kdrv_uart_handle_t;

typedef void (*kdrv_uart_callback_t)(uint32_t event);

#define UART_RX_DONE            BIT0
#define UART_TX_DONE            BIT1
#define UART_REVEIVE_COMPLETE   BIT2
#define UART_TRANSFER_COMPLETE  BIT3
#define UART_RX_TIMEOUT         BIT4
#define FLAGS_UART_ALL_EVENTS    (UART_RX_DONE|UART_TX_DONE)

#define MAX_FIFO_RX     14
#define MAX_FIFO_TX     13
/**
 * @brief Enumerations of UART mode parameters.
 */
typedef enum
{
    UART_MODE_ASYN_RX = 0x1,    /**< Enum 0x1, UART asynchronous receiver mode. */
    UART_MODE_ASYN_TX = 0x2,    /**< Enum 0x2, UART asynchronous transmitter mode. */
    UART_MODE_SYNC_RX = 0x4,    /**< Enum 0x4, UART synchronous receiver mode. */
    UART_MODE_SYNC_TX = 0x8     /**< Enum 0x8,  UART synchronous transmitter mode. */
} kdrv_uart_mode_t;

/**
 * @brief Enumerations of UART device instance parameters.
 */
typedef enum
{
    UART0_DEV = 0,          /**< Enum 0, UART device instance 0 */
    UART1_DEV,              /**< Enum 1, UART device instance 1 */
    UART2_DEV,              /**< Enum 2, UART device instance 2 */
    UART3_DEV,              /**< Enum 3, UART device instance 3 */
    UART4_DEV,              /**< Enum 4, UART device instance 4 */
    TOTAL_UART_DEV          /**< Enum 5, Total UART device instances */
} kdrv_uart_dev_id_t;

/**
 * @brief Enumerations of UART port parameters.
 */
typedef enum
{
    DRVUART_PORT0 = 0,  /**< Enum 0, UART port 0 */
    DRVUART_PORT1 = 1,  /**< Enum 1, UART port 1 */
    DRVUART_PORT2 = 2,  /**< Enum 2, UART port 2 */
    DRVUART_PORT3 = 3,  /**< Enum 3, UART port 3 */
    DRVUART_PORT4 = 4   /**< Enum 4, UART port 4 */
} DRVUART_PORT;

/**
 * @brief Enumerations of UART control hardware signals
 */
typedef enum
{
    UART_CTRL_CONFIG,       /**< Enum 0, set @ref kdrv_uart_config_t */
    UART_CTRL_FIFO_RX,      /**< Enum 1, set @ref kdrv_uart_fifo_config_t */
    UART_CTRL_FIFO_TX,      /**< Enum 2, set @ref kdrv_uart_fifo_config_t */
    UART_CTRL_LOOPBACK,     /**< Enum 3, UART loopback enable */
    UART_CTRL_TX_EN,        /**< Enum 4, UART transmitter enable */
    UART_CTRL_RX_EN,        /**< Enum 5, UART receiver enable */
    UART_CTRL_ABORT_TX,     /**< Enum 6, UART abort transmitter */
    UART_CTRL_ABORT_RX,     /**< Enum 7, UART abort receiver */
    UART_CTRL_TIMEOUT_RX,   /**< Enum 8, UART receiver timeout value */
    UART_CTRL_TIMEOUT_TX    /**< Enum 9, UART transmitter timeout value */
} kdrv_uart_control_t;

/**
 * @brief The structure of UART configuration parameters.
 */
typedef struct
{
    uint32_t baudrate;      /**< UART baud rate. */
    uint8_t data_bits;      /**< UART data bits, a data character contains 5~8 data bits. */
    uint8_t frame_length;   /**< UART frame length, non-zero value for FIR mode*/
    uint8_t stop_bits;      /**< UART stop bit, a data character is proceded by a start bit \n
                                 and is followed by an optional parity bit and a stop bit. */
    uint8_t parity_mode;    /**< UART partity mode, see @ref UART_PARITY_DEF */
    bool fifo_en;           /**< UART fifo mode. */
} kdrv_uart_config_t;

typedef void (*uart_isr_t)(void);

typedef struct
{
    volatile uint8_t tx_busy; // Transmitter busy flag
    volatile uint8_t rx_busy; // Receiver busy flag
    uint8_t tx_underflow;     // Transmit data underflow detected (cleared on start of next send operation)
    uint8_t rx_overflow;      // Receive data overflow detected (cleared on start of next receive operation)
    uint8_t rx_break;         // Break detected on receive (cleared on start of next receive operation)
    uint8_t rx_framing_error; // Framing error detected on receive (cleared on start of next receive operation)
    uint8_t rx_parity_error;  // Parity error detected on receive (cleared on start of next receive operation)
} UART_STATUS_t;

// UART Transfer Information (Run-Time)
typedef struct
{
    volatile uint32_t rx_num; // Total number of data to be received
    volatile uint32_t tx_num; // Total number of data to be send
    volatile uint32_t batch_tx_num; // Total number of data to be send per batch
    volatile uint8_t *rx_buf; // Pointer to in data buffer
    volatile uint8_t *tx_buf; // Pointer to out data buffer
    volatile uint32_t rx_cnt; // Number of data received
    volatile uint32_t tx_cnt; // Number of data sent
} UART_TRANSFER_INFO_t;

// UART Information (Run-Time)
typedef struct
{
    ARM_USART_SignalEvent_t cb_event; // Event callback
    UART_STATUS_t status;             // Status flags
    UART_TRANSFER_INFO_t xfer;        // Transfer information
    uint32_t flags;                   // UART driver flags: UART_FLAG_T
    uint32_t mode;                    // UART mode
} UART_INFO_T;

typedef enum
{
    UART_UNINIT,
    UART_INIT_DONE,
    UART_WORKING,
    UART_CLOSED
} uart_state_t;

// UART Resources definitions
typedef struct
{
    uint32_t irq_num;           // UART TX IRQ Number
    uart_isr_t isr;             // ISR route
    uint32_t fifo_depth;        //16/32/64/128 depth, set by UART_CTRL_CONFIG
    uint32_t tx_fifo_threshold; // FIFO tx trigger threshold
    uint32_t rx_fifo_threshold; // FIFO rx trigger threshold
    uint32_t fifo_len;          // FIFO tx buffer len
    uint32_t clock;             //clock
    uint32_t hw_base;           // hardware base address
} UART_RESOURCES_T;

/* driver instance handle */
typedef struct
{
    uint32_t uart_port;
    uart_state_t state;
    kdrv_uart_config_t config;
    UART_INFO_T info;
    UART_RESOURCES_T res;
    int32_t nTimeOutTx; //Tx timeout (ms) for UART_SYNC_MODE
    int32_t nTimeOutRx; //Rx timeout (ms) for UART_SYNC_MODE
    uint32_t iir;       //store IIR register value (IIR register will be reset once it is
                        //read once, need to store for further process)
} uart_driver_handle_t;


typedef struct
{
    int8_t total_open_uarts;
    bool active_dev[TOTAL_UART_DEV];
    uart_driver_handle_t *uart_dev[MAX_UART_INST];

} uart_drv_ctx_t;
/**
 * @brief The structure of UART FIFO configuration parameters.
 */
typedef struct
{
    bool bEnFifo;               /**< Is FIFO enabled */
    uint8_t fifo_trig_level;    /**< FIFO trigger level */
} kdrv_uart_fifo_config_t;


/**
 * @brief           UART driver initialization, it shall be called once in lifecycle
 *
 * @return          kdrv_status_t         see @ref kdrv_status_t
 */
kdrv_status_t kdrv_uart_initialize(void);

/**
 * @brief           UART driver uninitialization
 *
 * @return          kdrv_status_t         see @ref kdrv_status_t
 */
kdrv_status_t kdrv_uart_uninitialize(void);

/**
 * @brief           Open one UART port and acquire a uart port handle
 *
 * @details         This API will open a UART device (com_port: 0-5) for use.\n
 *                  It will return a UART device handle for future device reference.\n
 *                  The client can choose work mode: asynchronization or synchronization.\n
 *                  Synchronization mode will poll the hardware status to determine send/receiving point,\n
 *                  it will consume more power and introduce more delay to system execution.\n
 *                  But in the case of non-thread light weight environment, such as message log function, this mode is easy and suitable.\n
 *                  Asynchronization mode lets the driver interrupt driven, save more system power and more efficient,\n
 *                  the client needs to have a thread to listen/wait for the event/signal sent from callback function.\n
 *                  Callback function parameter 'callback' will be registered with this device which is mandatory for async mode,\n
 *                  will be invoked whenever Tx/Rx complete or timeout occur.\n
 *                  This callback function should be very thin, can only be used to set flag or send signals
 *
 * @param[out]      handle                a handle of an UART port
 * @param[in]       com_port              UART port id
 * @param[in]       mode                  bit combination of kdrv_uart_mode_t
 * @param[in]       callback              user callback function
 * @return          kdrv_status_t         see @ref kdrv_status_t
 */
kdrv_status_t kdrv_uart_open(kdrv_uart_handle_t *handle, uint8_t com_port, uint32_t mode, kdrv_uart_callback_t callback);

/**
 * @brief           set control for the opened UART port
 *
 * @param[in]       handle                device handle for an UART port
 * @param[in]       prop                  control enumeration
 * @param[in]       val                   pointer to control value/structure
 * @return          kdrv_status_t         see @ref kdrv_status_t
 */
kdrv_status_t kdrv_uart_configure(kdrv_uart_handle_t handle, kdrv_uart_control_t prop, uint8_t *val);

/**
 * @brief           write data to uart port, such as command, parameters, but not suitable for chunk data
 *
 * @details         The client calls this API to send data out to remote side.\n
 *                  Depending on the work mode, a little bit different behavior exists there.\n
 *                  In synchronous mode, the API call will not return until all data was sent out physically;\n
 *                  In asynchronous mode, the API call shall return immediately with UART_API_TX_BUSY.\n
 *                  When all the buffer data is sent out, the client registered callback function will be invoked.\n
 *                  The client shall have a very thin code there to set flags/signals. The client thread shall be listening the signal after this API call.\n
 *
 * @param[in]       handle                device handle for an UART port
 * @param[in]       buf                   data buffer
 * @param[in]       len                   data buffer length
 * @return          kdrv_status_t         see @ref kdrv_status_t
 */
kdrv_status_t kdrv_uart_write(kdrv_uart_handle_t hdl, uint8_t *buf, uint32_t len);

/**
 * @brief           read character data from UART port
 *
 * @param[in]       handle                device handle for an UART port
 * @param[out]      ch                    character data
 * @return          kdrv_status_t         see @ref kdrv_status_t
 */
kdrv_status_t kdrv_uart_get_char(kdrv_uart_handle_t handle, char *ch);

/**
 * @brief           read data from the UART port
 *
 * @details         The client can call this API to receive UART data from remote side.\n
 *                  Depending on the work mode, a little bit different behavior exists there.\n
 *                  In synchronous mode, the API call will not return until all data was received physically.\n
 *                  In asynchronous mode, the API call shall return immediately with UART_API_RX_BUSY.\n
 *                  When enough bytes are received or timeout occurs, the client registered callback function will be invoked.\n
 *                  The client shall have a very thin code there to set flags/signals. The client thread shall be listening the signal after this API call.\n
 *                  The client shall allocate the receiving buffer with max possible receiving length.\n
 *                  When one frame is sent out, after 4 chars transmission time, a timeout interrupt will be generated.
 *
 * @param[in]       handle                device handle for an UART port
 * @param[out]      buf                   data buffer
 * @param[in]       len                   data buffer length
 * @return          kdrv_status_t         see @ref kdrv_status_t
 */
kdrv_status_t kdrv_uart_read(kdrv_uart_handle_t handle, uint8_t *buf, uint32_t len);

/**
 * @brief           close the UART port
 *
 * @param[in]       handle                device handle for an UART port
 * @return          kdrv_status_t         see @ref kdrv_status_t
 */
kdrv_status_t kdrv_uart_close(kdrv_uart_handle_t handle);

/**
 * @brief           get char number in RX buffer 
 *
 * @param[in]       handle                device handle for an UART port
 * @return          number of RX count in the buffer
 */
uint32_t kdrv_uart_get_rx_count(kdrv_uart_handle_t handle);

/**
 * @brief           get char number in TX buffer 
 *
 * @param[in]       handle                device handle for an UART port
 * @return          number of TX count in the buffer
 */
uint32_t kdrv_uart_get_tx_count(kdrv_uart_handle_t handle);

/**
 * @brief           uart debug console port init
 *
 * @param[in]       uart_dev              uart device id, @ref kdrv_uart_dev_id_t
 * @param[in]       baudrate              uart baud rate
 * @param[in]       cb                    callback function
 * @return          uart initialize status, see @ref kdrv_status_t
 */
kdrv_status_t kdrv_uart_console_init(uint8_t uart_dev, uint32_t baudrate, kdrv_uart_callback_t cb);

extern uint32_t UART_PORT[5];
extern uint32_t uart_baud_rate_map[11];
extern uart_drv_ctx_t gDrvCtx;
extern uint32_t uart_get_status(DRVUART_PORT port_no);

#endif //__KDRV_UART_H_
