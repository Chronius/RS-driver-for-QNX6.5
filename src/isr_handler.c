/*
 * isr_handler.c
 *
 *  Created on: 31.08.2017
 *      Author: abuzarov_bv
 */


#include <proto.h>

struct sigevent event;

void *interrupt_thread(void *data) {
    int id, ch_num, i;

    memset(&event, 0, sizeof(event));
    memset((void *)&dev_l, 0, sizeof(dev_l));
    event.sigev_notify = SIGEV_INTR;

    id = pci_init();

    for (ch_num = 0; ch_num < UART_CHANNEL_COUNT; ch_num++) {
        memset(&fifo_spinlock[ch_num], 0, sizeof(fifo_spinlock[ch_num]));
    }

    *(uart_set.IrqEnable) |= (0xFFFFFFFF);
    while (1) {
        InterruptWait(0, NULL);
        *(uart_set.IrqEnable) &= ~(0xFFFFFFFF);
        for (ch_num = 0; ch_num < UART_CHANNEL_COUNT; ch_num++) {
            // read for clear
            pthread_spin_lock(&fifo_spinlock[ch_num]);
            // Check open device
            if ((1 << ch_num) & dev_l.dev_open)
            {
                //if Rx
                if (LSR_DR_Get(p_uart[ch_num]->lsr) && IER_RxD_Get(p_uart[ch_num]->ier_dlh)) {
                    int count = 0;
                    int offset = channel[ch_num].rx_offset;
                    while (LSR_DR_Get(p_uart[ch_num]->lsr) && (offset + count < channel[ch_num].attr.nbytes) ) {
                        channel[ch_num].rx_buf[offset + count] =
                                RBR_Get(p_uart[ch_num]->rbr_thr_dll);
                        count++;
                    }
                    int msr = p_uart[ch_num]->msr;
                    int lsr = p_uart[ch_num]->lsr;
                    printf("msr:%x lsr:%x\n", msr, lsr);
                    ClearBit(p_uart[ch_num]->ier_dlh, 0);
                    MsgReply(channel[ch_num].attr.client_id, count, channel[ch_num].rx_buf + offset, count);
                    free(channel[ch_num].rx_buf);
                    offset += count;
                    channel[ch_num].rx_offset = offset;
                }
                //if Tx
                if (IIR_Get(p_uart[ch_num]->iir_fcr) == TXFIFO_EMPTY) {
                    uint32_t lsr = p_uart[ch_num]->lsr;
                    int offset = channel[ch_num].tx_offset;
                    int count;
                    if (LSR_THRE_Get(lsr) || LSR_TEMPT_Get(lsr)) {
                        count = min(channel[ch_num].attr.nbytes, ALTERA_FIFO_SIZE);
                        for (i = 0; i < count; i++) {
                            p_uart[ch_num]->rbr_thr_dll = channel[ch_num].tx_buf[offset + i];
                        }
                        channel[ch_num].attr.nbytes -= count;
                        channel[ch_num].tx_offset += count;
                        if (channel[ch_num].attr.nbytes <= 0) {
                            // Disable TX fifo interrupt//
                            ClearBit(p_uart[ch_num]->ier_dlh, 1);
                            // and free pointer on user buffer
                            free(channel[ch_num].tx_buf);
                            MsgReply(channel[ch_num].attr.client_id, EOK, NULL, count);
                        }
                    }
                }
            }
            pthread_spin_unlock(&fifo_spinlock[ch_num]);
        }
//        *(uart_set.IrqEnable) &= ~(0xFFFFFFFF);
        *(uart_set.IrqEnable) |= (0xFFFFFFFF);
        InterruptUnmask(17, id);
    }
}

void port_init(void *base_addr) {
    uart_set.IrqEnable = (uint32_t *) (base_addr + 0x2000120);
    uart_set.IrqStatus = (uint32_t *) (base_addr + 0x2000100);
    uart_set.Mode = (uint32_t *) (base_addr + 0x2000060);

    /*half duplex*/
    *(uart_set.Mode) = 0xFFFFFFFF;
    /*full*/
//    *(uart_set.Mode) &= ~(0xFFFFFFFF);

    *(uart_set.IrqEnable) &= ~(0xFFFFFFFF);

    uart_set.I2cAddr = (uint32_t *) (base_addr + 0x2000400);
    uart_set.ShiftDE = (uint32_t *) (base_addr + 0x2000200);
    uart_set.ShiftTX = (uint32_t *) (base_addr + 0x2000210);
    uart_set.ShiftRX = (uint32_t *) (base_addr + 0x2000220);

    uart_set.IrqStatusH_V1 = (uint32_t *) (base_addr + RTS485_STATUS_IRQH_V1);
    uart_set.IrqStatusL_V1 = (uint32_t *) (base_addr + RTS485_STATUS_IRQL_V1);

    int ch_num;
    for (ch_num = 0; ch_num < UART_CHANNEL_COUNT; ch_num++) {
        uint32_t LCR = 0;
        LCR = p_uart[ch_num]->lcr;
        p_uart[ch_num]->lcr = LCR | LCR_DLAB_Set;
        uint32_t DIV = CPU_CLOCK_MHZ / (B115200 * 16);
        p_uart[ch_num]->rbr_thr_dll = DLL_Set(DIV);
        p_uart[ch_num]->ier_dlh = DLH_Set(DIV >> 8);
        LCR = p_uart[ch_num]->lcr;
        LCR &= ~LCR_DLAB_Set;
        p_uart[ch_num]->lcr = LCR | LCR_SDB_Set(LCR_SDB_MODE_08);
        //      p_uart[ch_num]->mcr = 1 << 4;
        p_uart[ch_num]->mcr = 0;
        p_uart[ch_num]->iir_fcr = FCR_FIFO_ENABLE_Set | FCR_RESETRF_Set
                | FCR_RESETTF_Set;
        //      p_uart[ch_num]->ier_dlh = IER_RxD_Set;
        p_uart[ch_num]->ier_dlh = 0;
        channel[ch_num].config.mode = true;
        channel[ch_num].config.baud = B115200;
        channel[ch_num].config.data_bits = MODE_08_TO_CS(LCR_SDB_MODE_08);
        from_config(ch_num);
    }
}

const struct sigevent *isr_handler(void *area, int id) {
    ((dev_list *) area)->status = *(uart_set.IrqStatus);
    ((dev_list *) area)->enable = *(uart_set.IrqEnable);
//    int i;
//    for (i = 0; i < UART_CHANNEL_COUNT; i++) {
//        if (CheckBit(p_uart[i]->iir_fcr, 1)) {
//            ClearBit(*(uart_set.IrqEnable), i);
//            SetNbit(*(uart_set.IrqEnable), i);
//        }
//    }
    if (*(uart_set.IrqStatus) == 0) {
        return (NULL);
    }
    return (&event);
}


void from_config(int ch_num) {
    int Baud;
    Baud = channel[ch_num].config.baud;
    int Mode;
    Mode = *(uart_set.Mode);

    if (channel[ch_num].config.mode) {
        SetNbit(Mode, (19 - ch_num));
    } else {
        ClearBit(Mode, (19 - ch_num));
    }
    *(uart_set.Mode) = Mode;

    Baud = channel[ch_num].config.baud;

    uint32_t LCR = 0;
    LCR = p_uart[ch_num]->lcr;
    p_uart[ch_num]->lcr = LCR | LCR_DLAB_Set;
    uint32_t DIV = CPU_CLOCK_MHZ / (Baud * 16);
    p_uart[ch_num]->rbr_thr_dll = DLL_Set(DIV);
    p_uart[ch_num]->ier_dlh = DLH_Set(DIV >> 8);
    LCR = p_uart[ch_num]->lcr;
    LCR &= ~LCR_DLAB_Set;
    p_uart[ch_num]->lcr = LCR
            | LCR_SDB_Set(MODE_CS_TO_(channel[ch_num].config.data_bits));
    p_uart[ch_num]->mcr = 0;
    p_uart[ch_num]->iir_fcr = FCR_FIFO_ENABLE_Set | FCR_RESETRF_Set
            | FCR_RESETTF_Set;
    p_uart[ch_num]->ier_dlh = 0;
}
