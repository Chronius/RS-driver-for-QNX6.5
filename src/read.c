/*
 * read.c
 *
 *  Created on: 31.08.2017
 *      Author: abuzarov_bv
 */
#include <proto.h>
/*
 *  io_read
 *
 * The message that we received can be accessed via the
 * pointer *msg. A pointer to the OCB that belongs to this
 * read is the *ocb. The *ctp pointer points to a context
 * structure that is used by the resource manager framework
 * to determine whom to reply to, and more. */


int io_read(resmgr_context_t *ctp, io_read_t *msg, RESMGR_OCB_T *ocb) {
    int nbytes;
    int nparts;
    int status;
    if ((status = iofunc_read_verify(ctp, msg, ocb, NULL)) != EOK)
        return (status);
    if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE)
        return (ENOSYS);

    nbytes = msg->i.nbytes;

    if (nbytes > 0) {
        unsigned char *buffer;
        if ((buffer = (unsigned char *) malloc(nbytes)) == NULL) {
            return (ENOMEM);
        }

        if (status == -1) {
            printf("XXXXXX: Can't gain i/o access permissions");
            exit(EXIT_FAILURE);
        }
        //to save context info for reply when receive data
        channel[ctp->id].attr.client_id = ctp->rcvid;
        channel[ctp->id].attr.nbytes = nbytes;
        channel[ctp->id].attr.offset = 0;
        channel[ctp->id].rx_buf = buffer;
        channel[ctp->id].rx_offset = 0;

        ocb->attr->flags |= IOFUNC_ATTR_ATIME;
       //enable interrupt when received data
        p_uart[ctp->id]->ier_dlh |= IER_RxD_Set;

    } else {
        _IO_SET_READ_NBYTES (ctp, 0);
        nparts = 0;
        return (_RESMGR_NPARTS (nparts));
    }

    return (_RESMGR_NOREPLY);
}
