#ifndef __PROTO_H__
#define __PROTO_H__

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/neutrino.h>
#include <sys/resmgr.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <sys/neutrino.h>
#include <sys/mman.h>
#include <string.h>
#include <termios.h>
#include <stdbool.h>

#include <hw/pci.h>
#include <hw/inout.h>

#include <fifo.h>
#include <specific_def.h>
#include <request_struct.h>
#pragma once

#define CPU_CLOCK_MHZ             125000000
#define UART_CHANNEL_COUNT        32
#define ALTERA_FIFO_SIZE          127

#define RTS485_ENABLE_IRQ   0x03000030
#define RTS485_STATUS_IRQL  0x03000020
#define RTS485_STATUS_IRQH  0x03000024
#define RTS485_INFO     0x03000100

#define RTS485_BASE_ADR_V1  0x1000000
#define RTS485_CONTROL_V1   (RTS485_BASE_ADR_V1 + 0x200)
#define RTS485_STATUS_IRQL_V1   (RTS485_BASE_ADR_V1 + 0x208)
#define RTS485_STATUS_IRQH_V1   (RTS485_BASE_ADR_V1 + 0x210)
#define RTS485_CLEAN_IRQL_V1    (RTS485_BASE_ADR_V1 + 0x218)
#define RTS485_CLEAN_IRQH_V1    (RTS485_BASE_ADR_V1 + 0x220)


/*
 *   check status tty with bitmask
 *
 *   devopen     31...5 4 2 1 0
 *               0 ...1 0 0 0 1
 *   it`s means that opened ttyRS0 and ttyRS5
 */
typedef struct dev_list
{
    uint32_t dev_open;
    uint32_t dev_res;
    uint32_t status;
    uint32_t enable;
} dev_list, *p_dev_list;
volatile dev_list dev_l;

// struct describing phys registers uart
typedef struct _uart_reg {
    volatile uint32_t rbr_thr_dll;
    volatile uint32_t ier_dlh;
    volatile uint32_t iir_fcr;
    volatile uint32_t lcr;
    volatile uint32_t mcr;
    volatile uint32_t lsr;
    volatile uint32_t msr;
    volatile uint32_t scr;
    volatile uint32_t afr;
    volatile uint32_t tx_low;
} uart_reg, *p_uart_reg;
volatile p_uart_reg p_uart[UART_CHANNEL_COUNT];

// Global struct for setting mode channels
typedef struct uart_setting
{
    uint32_t *IrqEnable;
    uint32_t *Mode;
    uint32_t *IrqStatus;
    uint32_t *I2cAddr;
    uint32_t *ShiftDE;
    uint32_t *ShiftTX;
    uint32_t *ShiftRX;

    uint32_t *IrqStatusL_V1;
    uint32_t *IrqStatusH_V1;
} uart_setting, *p_uart_setting;
static volatile uart_setting uart_set;

// struct for describing tty
typedef struct channel_uart_t
{
    volatile p_uart_reg ptr_u;
    unsigned char *tx_buf;
    unsigned char *rx_buf;
    int tx_offset;
    int rx_offset;
    struct
    {
        int baud;
        bool mode;       // 1 - half-duplex
        int data_bits;   // 5, 6, 7 or 8
        int stop_bit;    // 1, 2
        int par;
    } config;
    callback_attr_t attr;
} channel_uart;
channel_uart channel[UART_CHANNEL_COUNT];

// for data protection in channel
pthread_spinlock_t  fifo_spinlock[UART_CHANNEL_COUNT];

iofunc_attr_t sample_attrs[UART_CHANNEL_COUNT];


int io_open (resmgr_context_t *ctp, io_open_t  *msg, RESMGR_HANDLE_T *handle, void *extra);
int io_close (resmgr_context_t *ctp, void *reserved, RESMGR_OCB_T *ocb);
int io_read (resmgr_context_t *ctp, io_read_t  *msg, RESMGR_OCB_T *ocb);
int io_write(resmgr_context_t *ctp, io_write_t *msg, RESMGR_OCB_T *ocb);
int io_unblock(resmgr_context_t *ctp, io_pulse_t *msg, RESMGR_OCB_T *ocb);

int  pci_init           (void);
void port_init          (void * base_addr);
void *interrupt_thread (void * data);
void from_config        (int i);

const struct sigevent *isr_handler (void * area, int id);
#endif
