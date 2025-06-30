/********************************************************************
 * Copyright (c) 2020 Kneron, Inc. All Rights Reserved.
 *
 * The information contained herein is property of Kneron, Inc.
 * Terms and conditions of usage are described in detail in Kneron
 * STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information.
 * NO WARRANTY of ANY KIND is provided. This heading must NOT be removed
 * from the file.
 ********************************************************************/
/**@addtogroup  KDRV_UART KDRV_UART
 * @{
 * @brief       Kneron UART driver
 *
 * @details     Here are the design highlight points:\n
 *              * The architecture adopts a lightweight non-thread design\n
 *              * ISR driven architecture.\n
 *              * Can support both synchronous and asynchronous mode\n
 *              * Utilizes FIFO advantage to reduce interrupts and improve robust to accommodate more latency than normal.
 * @version     v1.0
 * @copyright   Copyright (C) 2020 Kneron, Inc. All rights reserved.
 */

#ifndef __KDRV_UART_H_
#define __KDRV_UART_H_

#include "cmsis_os2.h"
#include "kdrv_status.h"
#include "kneron_kl720.h"
#include "Driver_USART.h"

#pragma anon_unions
/*******************************************************************
 * for register address / bits definition
 ********************************************************************/
/** @brief Structure of Receiver Buffer Register (RBR , Offset: 0x00 for Read)*/
typedef struct {
    uint32_t RBR                : 8;    /* Receive Data Port */ /* Read-only 0 */
                                        /* If FIFOs are enabled: Once a character has been assembled at the receive shift register, the contents
                                         * will be written to RXFIFO. This RBR refers to RXFIFO location which the current read pointer idicates.
                                         * If FIFOs are not enabled: Once a character has been assembled at the receive shift register, the contents 
                                         * will be written to RXFIFO. This RBR refers to RXFIFO location which the read pointer !!¡Ó0!!L indicates.
                                         */
    uint32_t rsvd               : 24;
} kdrv_uart_rbr_t;

/** @brief Structure of Transmitter Holding Register (THR , Offset: 0x00 for Write) */
typedef struct {
    uint32_t THR                : 8;    /* Transmit Data Port */ /* Write-only 0 */
                                        /* If FIFOs are enabled: Once the transmit shift register is empty, the contents of THR will be written to the transmit shift
                                         * register for the transmitted data shift out. This THR refers to TXFIFO location which the current read pointer indicates.
                                         * If FIFOs are not enabled: Once the transmit shift register is empty, the contents of THR will be written to the transmit shift
                                         * register for the transmitted data shift out. This THR refers to TXFIFO location which the read pointer !!¡Ó0!!L indicates.
                                         * Before writing this register, the user must ensure that FTUART010 is ready to accept data for transmission by checking if the THR Empty flag is set in LSR.
                                         */
    uint32_t rsvd               : 24;
} kdrv_uart_thr_t;

/** @brief Structure of Interrupt Enable Register (IER , Offset: 0x04) */
typedef struct {
    uint32_t RD_available       : 1;    /* Default Value 0 */
                                        /* This bit enables the Received Data Available Interrupt (And the character reception timeout interrupts in the FIFO mode) when set to logic 1. */
    uint32_t THR_empty          : 1;    /* Default Value 0 */
                                        /* This bit enables the Transmitter Holding Register Empty Interrupt when set to logic 1. */
    uint32_t RL_status          : 1;    /* Default Value 0 */
                                        /* This bit enables the Receiver Line Status Interrupt when set to logic 1. */
    uint32_t MODEM_status       : 1;    /* Default Value 0 */
                                        /* This bit enables the modem status interrupt when set to logic 1. */
    uint32_t RTSEn              : 1;    /* Default Value 0 */
                                        /* RTS flow control enabled 1: Enable the RTS hardware flow control, 0: Disable the RTS hardware flow control */
                                        /* When RTSEn is set to !¢FD1!|, the loopback mode must not be set to !¢FD1!| and FIFO must be enabled (FCR[0]). */
    uint32_t CTSEn              : 1;    /* Default Value 0 */
                                        /* CTS flow control enabled 1: Enable the CTS hardware flow control, 0: Disable the CTS hardware flow control */
                                        /* When CTSEn is set to !¢FD1!|, the loopback mode must not be set to !¢FD1!| and FIFO must be enabled (FCR[0]). */
    uint32_t DTREn              : 1;    /* Default Value 0 */
                                        /* DTR flow control enabled 1: Enable the DTR hardware flow control, 0: Disable the DTR hardware flow control */
                                        /* When DTREn is set to !¢FD1!|, the loopback mode must not be set to !¢FD1!| and FIFO must be enabled (FCR[0]). */
    uint32_t DSREn              : 1;    /* Default Value 0 */
                                        /* DSR flow control enabled 1: Enable the DSR hardware flow control, 0: Disable the DSR hardware flow control */
                                        /* When DSREn is set to !¢FD1!|, the loopback mode must not be set to !¢FD1!| and FIFO must be enabled (FCR[0]). */
    uint32_t rsvd2              : 24;   /* Reserved */
} kdrv_uart_ier_t;

/** @brief Structure of Interrupt Identification Register (IIR , Offset: 0x08 Read_Only) */
typedef struct {
    uint32_t int_pending    : 1;    /* Interrupt Pending: This bit can be used in a prioritized interrupt environment to indicate whether an interrupt is pending. */
                                    /* 0: An interrupt is pending and the IIR contents may be used as a pointer to the appropriate interrupt service routine.
                                     * 1: No interrupt is pending. */
    uint32_t int_id_code    : 2;    /* Interrupt Identification Code: These bits identify the highest priority interrupt that is pending. */
    uint32_t fifo_mode_only : 1;    /* In the 16450 mode, this bit is 0. In the FIFO mode, this bit is set along with bit 2 when a timeout interrupt is pending. */
    uint32_t tx_fifo_full   : 1;    /* This bit is set to !¢FD1!| when TX FIFO is full. */
    uint32_t rsvd           : 1;
    uint32_t fifo_mode_en   : 2;    /* These two bits are set when FCR[0] is set to !¢FD1!|. */
    uint32_t rsvd1          : 24;
} kdrv_uart_iir_t;

/** @brief Structure of FIFO Control Register (FCR , Offset: 0x08 for Write) */
typedef struct {
    uint32_t fifo_en        : 1;    /* Set this bit to logic 1 enables both the transmit and receive FIFOs (As well as the Status FIFO). Change this bit will automatically reset both FIFOs. */
                                    /* In the FIR mode, the device driver should always set this bit to !¢FD1!|. */
    uint32_t rx_fifo_reset  : 1;    /* Set this bit to logic 1 clears all bytes in Rx FIFO and resets the counter logic to 0. The shift register is not cleared, so any reception active will continue. 
                                     * Set this bit also clears the Status FIFO. This bit will automatically return to zero. */
    uint32_t tx_fifo_reset  : 1;    /* Sett this bit to logic 1 clears all bytes in TX FIFO and resets the counter logic to 0. The shift register is not cleared, so any reception active will continue.
                                     * This bit will automatically return to zero. */
    uint32_t DMA_mode       : 1;    /* This bit selects the UART DMA mode. The DMA mode affects the way in which the DMA signaling outputs pins (irda_nrxrdy and irda_ntxrdy) behave. */
    uint32_t txfifo_trgl    : 2;    /* Set the trigger level of TX FIFO interrupt. */
    uint32_t rxfifo_trgl    : 2;    /* Set the trigger level of the RX FIFO interrupt. */
    uint32_t rsvd           : 24;
} kdrv_uart_fcr_t;

/** @brief Structure of Line Control Register (LCR Offset: 0x0C) */
typedef struct {
    uint32_t wl0            : 1;    /* This bit along with WL1 defines the word length of the data being transmitted and received. */
    uint32_t wl1            : 1;    /* This bit along with WL0 defines the word length of the data being transmitted and received. */
    uint32_t stop_bits      : 1;    /* This bit selects the number of stop bits to be transmitted. If cleared, only one stop bit will be transmitted.
                                     * If set, two stop bits (1.5 with 5-bit data) will be transmitted before the start bit of the next character. The receiver always checks only one stop bit. */
    uint32_t parity_enable  : 1;    /* This bit is the Parity Enable bit. 
                                     * If this bit is a !¢FD1!|, a Parity bit will be generated (Transmit data) or checked (Receive data) between the last data word bit and Stop bit of the serial data.*/
    uint32_t even_parity    : 1;    /* This bit is the Even Parity Select bit. */
                                    /* If parity_enable is !¢FD1!| and even_parity is !¢FD0!|, an odd number of logic 1s will be transmitted or checked in the data word bits and Parity bit. */
                                    /* If parity_enable is !¢FD1!| and even_parity is !¢FD1!|, an even number of 1s will be transmitted or checked. */
    uint32_t stick_parity   : 1;    /* If bits[5:3] are logic 1, the Parity bit will be transmitted and checked as !¢FD0!|. 
                                     * If parity_enable and stick_parity are !¢FD1!| and even_parity is !¢FD0!|, then the Parity bit will be transmitted and checked as !¢FD1!|. 
                                     * If stick_parity is !¢FD0!|, Stick Parity will be disabled. */
    uint32_t set_break      : 1;    /* This bit causes a break condition to be transmitted to the receiving UART. 
                                     * 1: The serial output (io_irda_sout) is forced to the Spacing (Logic 0) state. 
                                     * 0: The break is disabled. */ /* ???? */
    uint32_t DLAB           : 1;    /* Divisor Latch Access Bit (DLAB) */
                                    /* This bit must be set in order to access the DLL, DLM and PSR registers which program the division constants for the baud rate divider and the prescaler. */
    uint32_t rsvd           : 24;
} kdrv_uart_lcr_t;

/** @brief Structure of Modem Control Register (MCR , Offset: 0x10)
* @details  By writing this register, user can set the modem control outputs (io_irda_ndtr and io_irda_nrts). \n
* This register also controls the loopback mode, and provides the general purpose outputs.
*/
typedef struct {
    uint32_t DTR            : 1;    /* Data Terminal Ready: active low output, io_irda_ndtr. A 1 in this bit makes io_irda_ndtr output a !¢FD0!|. When this bit is cleared, io_irda_ndtr outputs a !¢FD1!|. */
    uint32_t RTS            : 1;    /* Request to Send: active low output, io_irda_nrts, in the same way as bit 0 controls io_irda_ndtr. */
    uint32_t out1           : 1;    /* This bit controls the general purpose, active low, output io_irda_nout1, in the same way as bit 0 controls io_irda_ndtr. */
    uint32_t out2           : 1;    /* This bit controls the general purpose, active low, output io_irda_nout2, in the same way as bit 0 controls io_irda_ndtr. */
    uint32_t loopback       : 1;    /* Loopback mode control bit: Loopback mode is intended to test the UART or SIR communication. */
    uint32_t DMAmode2       : 1;    /* Selects the UART/SIR DMA mode: The DMA mode2 affects the way in which the DMA signaling output pins, irda_nrxrdy and irda_ntxrdy, behave. */
    uint32_t out3           : 1;    /* This bit controls the general purpose, active low, output io_irda_nout3, in the same way as bit 0 controls io_irda_ndtr. */
    uint32_t rsvd           : 25;
} kdrv_uart_mcr_t;


/** @brief Structure of Line Status Register (LSR, Offset: 0x14 for Read)
* @details  This register informs users of the status of the transmitter and the receiver. \n
* In order to acquire the information about a received character, LSR must be read before reading the received character from RBR.
*/
typedef struct {
    uint32_t data_ready     : 1;    /* This bit is set if one or more characters have been received and are waiting in the receiver FIFO for users to read. 
                                     * It is cleared to 0 by reading all of the data in the Receiver Buffer Register or FIFO. */
    uint32_t overrun_err    : 1;    /* When this bit is set, a character will be completely assembled in the Receiver Shift Register without having free space to put it in the receive FIFO or buffer register. 
                                     * When an overrun condition appears, the result is different depending on whether the 16-byte FIFO is active or not. */
                                    /* If FIFO is inactive, only a 1-character Receiver Buffer Register will be available, and the unread data in this RBR will not be overwritten with the new character just received. */
                                    /* If FIFO is active, the character just received in the Receiver Shift Register will be overwritten, but the data already present in FIFO will not be changed. 
                                     * The Overrun Error flag is set as soon as the overrun condition appears. */
                                    /* This bit will not be queued in FIFO if it is active. This bit will be cleared as soon as LSR is read. */
    uint32_t parity_err     : 1;    /* When this bit is set, it indicates that the parity of the received character is wrong according to the current setting in LCR. This bit is queued in the receive FIFO; 
                                     * therefore, it is associated with the particular character that had the error. LSR must be read before RBR: Each time a character is read from RBR,
                                     * the next character passes to the top of FIFO and LSR is loaded with the queued error flags corresponding to this top-of-the-FIFO character.
                                     * This bit is cleared as soon as LSR is read. */
    uint32_t framing_err    : 1;    /* This bit indicates that the received character did not have a valid stop bit. This bit is queued in the receiver FIFO in the same way as the Parity Error bit.
                                     * When a framing error is detected, the receiver tries to resynchronize; if the next sample is again a zero, it will be taken as the beginning of a possible new start bit.
									 * This bit is cleared as soon as LSR is read. */
	uint32_t break_int		: 1;	/* This bit is set to !¢FD1!| if the receiver line input, io_irda_sin, is held at zero for a complete character time. This bit is cleared as soon as LSR is read.*/
	uint32_t THR_empty		: 1;	/* This bit indicates that UART is ready to accept a new character for transmission. In addition, this bit will cause UART to issue an interrupt to CPU when IER [1] is set.*/
									/* In the non-FIFO mode, this bit is set whenever the 1-byte THR is empty. If THR holds data to be transmitted, this bit will be immediately set when this data is passed to TSR. */
									/* In the FIFO mode, this bit is set when the transmitter FIFO is completely empty, being 0 if there is at least one byte in FIFO waiting to be passed to the TSR for transmission. */
	uint32_t tx_empty		: 1;	/* This bit will be !¢FD1!| when both THR (Or TX FIFO) and TSR(Transmitter Shift Register) are empty. */
									/* Reading this bit as !¢FD1!| indicates that no transmission will currently occur in the io_irda_sout output pin, and that the transmission line is idle.
									 * As soon as new data is written in THR, this bit will be cleared. */
	uint32_t fifo_data_err	: 1;	/* If FIFO is disabled (16450 mode), this bit will always be zero. */
									/* If FIFO is active, this bit will be set as soon as any data character in the receive FIFO has parity or framing error or the break indication is active. 
									 * This bit will be cleared when CPU reads LSR and the rest of the data in the receive FIFO do not have any of these three associated flags on. */
    uint32_t rsvd           : 24;
} kdrv_uart_lsr_t;


/** @brief Structure ofTesting Register (TST Offset: 0x14 for Write)
* @details  This register provides internal diagnostic capabilities if the circuit is hardware implemented. The loopback mode is supported only in the UART and SIR modes.\n
* For FIR, because there is only one CRC module in FTUART010, the FIR transmit and receive cannot be tested with the loopback mode.
*/
typedef struct {
	uint32_t TEST_PAR_ERR	: 1;	/* When set, FTUART010 generates incorrect parity during the UART transmission. */
	uint32_t TEST_FRM_ERR	: 1;	/* When set, FTUART010 generates a logic 0 STOP bit during the UART transmission. */
	uint32_t TEST_BAUDGEN	: 1;	/* This bit is used to improve the baud-rate generated toggle rate. */
	uint32_t TEST_PHY_ERR	: 1;	/* When set, FTUART010 generates incorrect 4PPM encoding chips during the FIR transmission. */
	uint32_t TEST_CRC_ERR	: 1;	/* When set, FTUART010 generates incorrect CRC during the FIR transmission. */
    uint32_t rsvd           : 27;
} kdrv_uart_tst_t;

/** @brief Structure of Modem Status Register (MSR , Offset: 0x18) */
typedef struct {
	uint32_t Delta_CTS		: 1;	/* If set, it means that the io_irda_ncts input has changed since the last time the microprocessor read this bit. */
	uint32_t Delta_DSR		: 1;	/* If set, it means that the io_irda_ndsr input has changed since the last time the microprocessor read this bit. */ 
	uint32_t Trail_edge_R1	: 1;	/* This bit is set when a trailing edge is detected in the io_irda_nri input pin; that is to say, when io_irda_nri changes from 0 to 1. */
	uint32_t Delta_DCD		: 1;	/* The delta-DCD flag. If set, it means that the io_irda_ndcd input has changed since the last time the microprocessor read this bit. */
	uint32_t CTS			: 1;	/* Clear To Send (CTS), which is the complement of the io_irda_ncts input. */
	uint32_t DSR			: 1;	/* Data Set Ready (DSR), which is the complement of the io_irda_ndsr input. */
	uint32_t RI				: 1;	/* Ring Indicator (RI), which is the complement of the io_irda_nri input. */
	uint32_t DCD			: 1;	/* Data Carrier Detect (DCD), which is the complement of the io_irda_ndcd input. */
    uint32_t rsvd           : 24;
} kdrv_uart_msr_t;

/** @brief Structure of Scratch Pad Register (SPR , Offset: 0x1C) */
typedef struct {
	uint32_t user_data		: 8;
	uint32_t rsvd			: 24;
} kdrv_uart_spr_t;

/** @brief Structure of Mode Definition Register (MDR , O ffset: 0x20) */
typedef struct {
	uint32_t mode_sel		: 2;	/* 00 UART(Default); 01 SIR; 10 FIR; 11 Rsvd */
	uint32_t SIP_BYCPU		: 1;	/* 0: The controller in transmission mode always sends a 1.6-¢FGgs pulse at the end of a transmission frame when TX FIFO is empty.
									 * 1: The transmission of a 1.6-¢FGgs pulse depends on the ACR[4]. CPU should keep a timer and set the ACR[4] bit at least once in every 500 ms. */
	uint32_t FMEND_MD		: 1;	/* 0: Apply the Frame-Length Counter method; 1: Apply the Set End of Transmission bit method */
	uint32_t DMA_EN			: 1;	/* When set to !¢FD1!|, the DMA mode of operation is enabled. 
									 * When data transfers are performed by a DMA controller transmit and/or receive data, interrupts in the PIO mode should be disabled to avoid the spurious interrupts. */
	uint32_t FIR_INV_RX		: 1;	/* This bit is to support the optical transceivers with the receive signals of the opposite polarity (Active high instead of active low). 
									 * When set to !¢FD1!|, an inverter is placed on the receiver input signal path. In the SIR mode, this bit is useless. */
	uint32_t IR_INV_TX		: 1;	/* When set, FTUART010 generates the inverted FIR or SIR pulse during transmission. */
	uint32_t rsvd			: 25;
} kdrv_uart_mdr_t;

/** @brief Structure of Feature Register (Feature, O ffset: 0x68) */
typedef struct {
	uint32_t FIFO_DEPTH		: 4;	/* HwCfg 4!|b0001: TX/RX FIFOs are 16-byte deep. 4!|b0010: TX/RX FIFOs are 32-byte deep. 4!|b0100: TX/RX FIFOs are 64-byte deep. 4!|b1000: TX/RX FIFOs are 128-byte deep. */
	uint32_t IrDA_INSIDE	: 1;	/* 1: FTUART010 contains the IrDA function. 0: FTUART010 is a pure UART. */
	uint32_t rsvd			: 27;
} kdrv_uart_feature_t;

//when DLAB = 1
/** @brief Structure of Baud Rate Divisor Latch LSB (DLL, Offset: 0x00 when DLAB = 1) */
typedef struct {
	uint32_t DLL		    : 8;    /* Default Value 1 *//* Baud Rate Divisor Latch Least Significant Byte */
	uint32_t rsvd			: 24;
} kdrv_uart_dll_t;

/** @brief Structure of Baud Rate Divisor Latch MSB (DLM, Offset: 0x04 when DLAB = 1) */
typedef struct {
	uint32_t DLM    		: 8;    /* Default Value 0 *//* Baud Rate Divisor Latch Most Significant Byte */
	uint32_t rsvd			: 24;
} kdrv_uart_dlm_t;

/** @brief Structure of Prescaler Register (PSR , Offset: 0x 08 when DLAB = 1) */
typedef struct {
	uint32_t PSR    		: 5;    /* Default Value 1 *//* Prescaler Value */
	uint32_t rsvd			: 27;
} kdrv_uart_psr_t;

typedef volatile union {
    uint32_t array[9];
    struct
    {
        union
        {
            struct
            {
				union
				{
					uint32_t    kdrv_uart_rbr;		/* 0x00 Read */
					uint32_t    kdrv_uart_thr;		/* 0x00 Write */
				};
                uint32_t    kdrv_uart_ier;			/* 0x04 */
				union
				{
					uint32_t    kdrv_uart_iir;		/* 0x08 Read */
					uint32_t    kdrv_uart_fcr;		/* 0x08 Write */
				};
                uint32_t    kdrv_uart_lcr;			/* 0x0C */
                uint32_t    kdrv_uart_mcr;			/* 0x10 */
				union
				{
					uint32_t    kdrv_uart_lsr;		/* 0x14 Read */
					uint32_t    kdrv_uart_tst;	/* 0x14 Write */
				};
				uint32_t    kdrv_uart_msr;		/* 0x18 */
				uint32_t    kdrv_uart_spr;			/* 0x1C */
				uint32_t    kdrv_uart_mdr;			/* 0x20 */
            } dw;                         //double word

            struct
            {
				union
				{
					kdrv_uart_rbr_t		kdrv_uart_rbr;		/* 0x00 Read */
					kdrv_uart_thr_t		kdrv_uart_thr;		/* 0x00 Write */
				};
				kdrv_uart_ier_t		kdrv_uart_ier;			/* 0x04 */
				union
				{
					kdrv_uart_iir_t		kdrv_uart_iir;		/* 0x08 Read */
					kdrv_uart_fcr_t		kdrv_uart_fcr;		/* 0x08 Write */
				};
                kdrv_uart_lcr_t     kdrv_uart_lcr;			/* 0x0C */
                kdrv_uart_mcr_t     kdrv_uart_mcr;			/* 0x10 */
				union
				{
					kdrv_uart_lsr_t     kdrv_uart_lsr;		/* 0x14 Read */
					kdrv_uart_tst_t     kdrv_uart_tst;	/* 0x14 Write */
				};
                kdrv_uart_msr_t     kdrv_uart_msr;		/* 0x18 */
                kdrv_uart_spr_t     kdrv_uart_spr;			/* 0x1C */
                kdrv_uart_mdr_t     kdrv_uart_mdr;			/* 0x20 */
            } bf;
        };
    } st;
} U_regUART_ctrl;

typedef volatile union {
    union
    {
        struct
        {
            uint32_t kdrv_uart_feature;
        } dw;
        struct
        {
            kdrv_uart_feature_t kdrv_uart_feature;
        } bf;
    };
} U_regUART_feature;

typedef volatile union {
    uint32_t array[3];
    struct
    {
        union
        {
            struct
            {
                uint32_t kdrv_uart_dll;
                uint32_t kdrv_uart_dlm;
                uint32_t kdrv_uart_psr;
            } dw;                         //double word

            struct
            {
                kdrv_uart_dll_t kdrv_uart_dll;
                kdrv_uart_dlm_t kdrv_uart_dlm;
                kdrv_uart_psr_t kdrv_uart_psr;
            } bf;
        };
    } st;
} U_regUART_baudrate;

#define regUART0_ctrl ((volatile U_regUART_ctrl *)UART0_REG_BASE)
#define regUART0_baudrate ((volatile U_regUART_baudrate *)UART0_REG_BASE)
#define regUART0_feature ((volatile U_regUART_feature *)(UART0_REG_BASE+0x68))
#define regUART1_ctrl ((volatile U_regUART_ctrl *)UART1_REG_BASE)
#define regUART1_baudrate ((volatile U_regUART_baudrate *)UART1_REG_BASE)
#define regUART1_feature ((volatile U_regUART_feature *)(UART1_REG_BASE+0x68))

#define regUART_ctrl(n)  ((volatile U_regUART_ctrl *)(UART0_REG_BASE+(n*0x100000)))
#define regUART_baudrate(n)  ((volatile U_regUART_baudrate *)(UART0_REG_BASE+(n*0x100000)))
#define regUART_feature(n)  ((volatile U_regUART_feature *)(UART0_REG_BASE+(n*0x100000)+0x68))

/**@brief Enumerations of UART baud rate.*/
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

/**@brief Enumerations of UART parity*/
typedef enum {
    PARITY_NONE = 0,    /**< Enum 0, Disable Parity */
    PARITY_ODD,             /**< Enum 1, Odd Parity */
    PARITY_EVEN,            /**< Enum 2, Even Parity */
    PARITY_MARK,            /**< Enum 3, Stick odd Parity */
    PARITY_SPACE            /**< Enum 4, Stick even Parity */
} kdrv_uart_parity_t;

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

/** UART user callback function with specified uart event. Note that this is callback form ISR context. */
typedef void (*kdrv_uart_callback_t)(uint32_t event);

#define UART_RX_DONE            BIT0
#define UART_TX_DONE            BIT1
#define UART_REVEIVE_COMPLETE   BIT2
#define UART_TRANSFER_COMPLETE  BIT3
#define UART_RX_TIMEOUT         BIT4
#define FLAGS_UART_ALL_EVENTS    (UART_RX_DONE|UART_TX_DONE)

#define MAX_FIFO_RX     14
#define MAX_FIFO_TX     13
/**@brief Enumerations of UART mode parameters. */
typedef enum
{
    UART_MODE_ASYN_RX = 0x1,    /**< Enum 0x1,UART asynchronous receiver mode. */
    UART_MODE_ASYN_TX = 0x2,    /**< Enum 0x2,UART asynchronous transmitter mode. */
    UART_MODE_SYNC_RX = 0x4,    /**< Enum 0x4,UART synchronous receiver mode. */
    UART_MODE_SYNC_TX = 0x8     /**< Enum 0x8,UART synchronous transmitter mode. */
} kdrv_uart_mode_t;

/**@brief Enumerations of UART device instance parameters. */
typedef enum
{
    UART0_DEV = 0,          /**< Enum 0, UART device instance 0 */
    UART1_DEV,              /**< Enum 1, UART device instance 1 */
    TOTAL_UART_DEV          /**< Enum 2, Total UART device instances */
} kdrv_uart_dev_id_t;

/**@brief Enumerations of UART port parameters. */
typedef enum
{
    DRVUART_PORT0 = 0,  /**< Enum 0, UART port 0 */
    DRVUART_PORT1 = 1,  /**< Enum 1, UART port 1 */
} DRVUART_PORT;

/**@brief Enumerations of UART control hardware signals */
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

/**@brief Structure of UART configuration parameters. */
typedef struct
{
    uint32_t baudrate;      /**< UART baud rate. */
    uint8_t data_bits;      /**< UART data bits, a data character contains 5~8 data bits. */
    uint8_t frame_length;   /**< UART frame length, non-zero value for FIR mode*/
    uint8_t stop_bits;      /**< UART stop bit, a data character is proceded by a start bit \n
                                 and is followed by an optional parity bit and a stop bit. */
    uint8_t parity_mode;    /**< UART partity mode, see @ref UART_PARITY_DEF */
    bool fifo_en;           /**< UART fifo mode. */
    #ifdef UART_RX_IRQ
    bool irq_en;
    #endif
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
    volatile uint32_t write_idx; // Write index
    volatile uint32_t read_idx; // Read index
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
/**@brief Structure of UART FIFO configuration parameters. */
typedef struct
{
    bool bEnFifo;
    uint8_t fifo_trig_level;
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
kdrv_status_t kdrv_uart_configure_baudrate_change(kdrv_uart_handle_t handle, uint8_t nBaudrateIdx);
uint32_t kdrv_uart_baud_rate_array_search(uint8_t nBaudrateIdx);


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
 * @brief           Check whether UART TX is busy from all UART ports
 *
 * @param[in]       N/A
 * @return          kdrv_status_t,  see @ref kdrv_status_t
 */
kdrv_status_t kdrv_uart_check_tx_busy(void);

/**
 * @brief           uart debug console port init
 *
 * @param[in]       uart_dev              uart device id, @ref kdrv_uart_dev_id_t
 * @param[in]       baudrate              uart baud rate
 * @param[in]       cb                    callback function
 * @return          uart initialize status, see @ref kdrv_status_t
 */
kdrv_status_t kdrv_uart_console_init(uint8_t uart_dev, uint32_t baudrate, kdrv_uart_callback_t cb);

uint32_t kdrv_uart_GetRxBufSize(kdrv_uart_handle_t handle);
uint32_t kdrv_uart_GetWriteIndex(kdrv_uart_handle_t handle);
uint32_t kdrv_uart_GetReadIndex(kdrv_uart_handle_t handle);
uint32_t kdrv_uart_SetWriteIndex(kdrv_uart_handle_t handle, uint32_t index);
uint32_t kdrv_uart_SetReadIndex(kdrv_uart_handle_t handle, uint32_t index);
int32_t kdrv_uart_gets(DRVUART_PORT com_port, char *buf);
extern uint32_t UART_PORT[2];
extern uint32_t uart_baud_rate_map[11];
extern uart_drv_ctx_t gDrvCtx;
extern uint32_t uart_get_status(DRVUART_PORT port_no);
#endif //__KDRV_UART_H_
/** @}*/

