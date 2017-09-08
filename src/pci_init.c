/*
 * pci_init.c
 *
 *  Created on: 31.08.2017
 *      Author: abuzarov_bv
 */

#include <proto.h>

int pci_init()
{
	uint32_t irq;
    void *handler = NULL, *ptr = NULL, *ptr_to_set = NULL;
    int phdl = 0;
    int id, err, i;
    struct pci_dev_info inf;

    ThreadCtl(_NTO_TCTL_IO, 0);

    uint16_t VendorId = 0x1172;
    uint16_t DeviceId = 0xe001;

    phdl = pci_attach(0);
    if (phdl == -1) {
        fprintf(stderr, "Unable to initialize PCI\n");
        return EXIT_FAILURE;
    }


    /* Initialize the pci_dev_info structure */
    memset(&inf, 0, sizeof(inf));
    int pidx = 0;
    inf.VendorId = VendorId;
    inf.DeviceId = DeviceId;

    handler = pci_attach_device(NULL, PCI_SHARE | PCI_PERSIST, pidx, &inf);

    if (handler == NULL) {
        fprintf(stderr, "Unable to locate adapter\n");
        return -1;
    }
    fprintf(stderr, "Found adapter. Current IRQ is 0x%d h.\n", inf.Irq);
    irq = 17;
    /*
     * Bug in pci_read_config8 and pci_attach_device -
     * when read PCI space bar pci_irq return
     * hexadecimal value in decimal,
     *  for example:
     *      return int 11 when irq = 0x11 or int 17
     * so, since we know this value we assign its manually
     *
         uint8_t pci_irq = 0;
         pci_read_config8(inf.BusNumber,inf.DevFunc, 0x3c, 1, &pci_irq);
         fprintf(stderr,"\n base address 1=%x", pci_irq);
     */

    ptr = mmap_device_memory(NULL, inf.BaseAddressSize[0], PROT_READ
            | PROT_WRITE | PROT_NOCACHE, 0, inf.PciBaseAddress[0]);
    if (ptr == MAP_FAILED) {
        printf("Failed to map device memory thanks to error\n");
        exit(EXIT_FAILURE);
    } else {
        ptr_to_set = ptr;
        channel[0].ptr_u = p_uart[0] = (p_uart_reg *)(ptr_to_set + 0x1000000 + 0x2000 + 0x600);
        channel[1].ptr_u = p_uart[1] = (p_uart_reg *)(ptr_to_set + 0x1000000 + 0x2000 + 0x400);
        channel[2].ptr_u = p_uart[2] = (p_uart_reg *)(ptr_to_set + 0x1000000 + 0x2000 + 0x200);
        channel[3].ptr_u = p_uart[3] = (p_uart_reg *)(ptr_to_set + 0x1000000 + 0x2000 + 0x000);

        channel[4].ptr_u = p_uart[4] = (p_uart_reg *)(ptr_to_set + 0x1000000 + 0x1800 + 0x600);
        channel[5].ptr_u = p_uart[5] = (p_uart_reg *)(ptr_to_set + 0x1000000 + 0x1800 + 0x400);
        channel[6].ptr_u = p_uart[6] = (p_uart_reg *)(ptr_to_set + 0x1000000 + 0x1800 + 0x200);
        channel[7].ptr_u = p_uart[7] = (p_uart_reg *)(ptr_to_set + 0x1000000 + 0x1800 + 0x000);

        channel[8].ptr_u = p_uart[8] = (p_uart_reg *)(ptr_to_set + 0x1000000 + 0x1000 + 0x600);
        channel[9].ptr_u = p_uart[9] = (p_uart_reg *)(ptr_to_set + 0x1000000 + 0x1000 + 0x400);
        channel[10].ptr_u = p_uart[10] = (p_uart_reg *)(ptr_to_set + 0x1000000 + 0x1000 + 0x200);
        channel[11].ptr_u = p_uart[11] = (p_uart_reg *)(ptr_to_set + 0x1000000 + 0x1000 + 0x000);

        channel[12].ptr_u = p_uart[12] = (p_uart_reg *)(ptr_to_set + 0x1000000 + 0x0800 + 0x600);
        channel[13].ptr_u = p_uart[13] = (p_uart_reg *)(ptr_to_set + 0x1000000 + 0x0800 + 0x400);
        channel[14].ptr_u = p_uart[14] = (p_uart_reg *)(ptr_to_set + 0x1000000 + 0x0800 + 0x200);
        channel[15].ptr_u = p_uart[15] = (p_uart_reg *)(ptr_to_set + 0x1000000 + 0x0800 + 0x000);

        channel[16].ptr_u = p_uart[16] = (p_uart_reg *)(ptr_to_set + 0x1000000 + 0x0000 + 0x600);
        channel[17].ptr_u = p_uart[17] = (p_uart_reg *)(ptr_to_set + 0x1000000 + 0x0000 + 0x400);
        channel[18].ptr_u = p_uart[18] = (p_uart_reg *)(ptr_to_set + 0x1000000 + 0x0000 + 0x200);
        channel[19].ptr_u = p_uart[19] = (p_uart_reg *)(ptr_to_set + 0x1000000 + 0x0000 + 0x000);
        /*-------------------------------------------------------------------------------------*/
        channel[20].ptr_u = p_uart[20] = (p_uart_reg *)(ptr_to_set + 0x0000000 + 0x1000 + 0x600);
        channel[21].ptr_u = p_uart[21] = (p_uart_reg *)(ptr_to_set + 0x0000000 + 0x1000 + 0x400);
        channel[22].ptr_u = p_uart[22] = (p_uart_reg *)(ptr_to_set + 0x0000000 + 0x1000 + 0x200);
        channel[23].ptr_u = p_uart[23] = (p_uart_reg *)(ptr_to_set + 0x0000000 + 0x1000 + 0x000);

        channel[24].ptr_u = p_uart[24] = (p_uart_reg *)(ptr_to_set + 0x0000000 + 0x0800 + 0x600);
        channel[25].ptr_u = p_uart[25] = (p_uart_reg *)(ptr_to_set + 0x0000000 + 0x0800 + 0x400);
        channel[26].ptr_u = p_uart[26] = (p_uart_reg *)(ptr_to_set + 0x0000000 + 0x0800 + 0x200);
        channel[27].ptr_u = p_uart[27] = (p_uart_reg *)(ptr_to_set + 0x0000000 + 0x0800 + 0x000);

        channel[28].ptr_u = p_uart[28] = (p_uart_reg *)(ptr_to_set + 0x0000000 + 0x0000 + 0x600);
        channel[29].ptr_u = p_uart[29] = (p_uart_reg *)(ptr_to_set + 0x0000000 + 0x0000 + 0x400);
        channel[30].ptr_u = p_uart[30] = (p_uart_reg *)(ptr_to_set + 0x0000000 + 0x0000 + 0x200);
        channel[31].ptr_u = p_uart[31] = (p_uart_reg *)(ptr_to_set + 0x0000000 + 0x0000 + 0x000);
        port_init(ptr_to_set);
    }

    if ((id = InterruptAttach(irq, isr_handler, (void*) &dev_l, sizeof(dev_l),
            0)) == -1) {
        printf("Can`t interrupt attach\n");
        exit(EXIT_FAILURE);
    }

    if ((err = InterruptUnmask(irq, id)) == -1) {
        printf("Can`t interrupt unmask\n");
        exit(EXIT_FAILURE);
    }

    return id;
}
