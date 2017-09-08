/*
 * write.c
 *
 *  Created on: 31.08.2017
 *      Author: abuzarov_bv
 */
#include <proto.h>
/*
 *  io_write
 *
 *  At this point, the client has called the library write()
 *  function, and expects that our resource manager will write
 *  the number of bytes that have been specified to the device.
 */
#define FIFO_DEV_SIZE 127

int io_write(resmgr_context_t *ctp, io_write_t *msg, RESMGR_OCB_T *ocb) {
    int status;
    int off;
    int start_data_offset;
    struct _xtype_offset *xoffset;

    /* Check the access permissions of the client */
    if ((status = iofunc_write_verify(ctp, msg, ocb, NULL)) != EOK) {
        return (status);
    }

    /* Check if pwrite() or normal write() */
    int xtype = msg->i.xtype & _IO_XTYPE_MASK;
    if (xtype == _IO_XTYPE_OFFSET) {
        xoffset = (struct _xtype_offset*) (&msg->i + 1);
        start_data_offset = sizeof(msg->i) + sizeof(*xoffset);
        off = xoffset->offset;
    } else if (xtype == _IO_XTYPE_NONE) {
        off = ocb->offset;
        start_data_offset = sizeof(msg->i);
    } else {
        return (ENOSYS);
    }

    unsigned char *buffer;
    int nbytes;
    nbytes = msg->i.nbytes;
    if ((buffer = (unsigned char *) malloc(nbytes)) == NULL) {
        return (ENOMEM);
    }

    //	MsgRead()
    if (resmgr_msgread(ctp, buffer, nbytes, start_data_offset) == -1) {
        free(buffer);
        return (errno);
    }

    channel[ctp->id].attr.client_id = ctp->rcvid;
    channel[ctp->id].attr.offset = 0;
    channel[ctp->id].attr.nbytes = nbytes;
    channel[ctp->id].tx_buf = buffer;
    channel[ctp->id].tx_offset = 0;

    if (nbytes)
        ocb->attr->flags |= IOFUNC_ATTR_MTIME | IOFUNC_ATTR_CTIME;

    p_uart[ctp->id]->ier_dlh |= IER_TxD_Set;
    return _RESMGR_NOREPLY;
}
