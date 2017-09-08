/* R E S O U R C E   M A N A G E R S  - THE "PCI RS DRIVER */

/* Project Name: "PCI RS driver" */
/*
 * main.c
 *
 *      Author: abuzarov_bv
 */
#include <proto.h>
void options (int argc, char **argv);

/* A resource manager mainly consists of callbacks for POSIX
 * functions a client could call. In the example, we have
 * callbacks for the open(), read() and write() calls. More are
 * possible. If we don't supply own functions (e.g. for stat(),
 * seek(), etc.), the resource manager framework will use default
 * system functions, which in most cases return with an error
 * code to indicate that this resource manager doesn't support
 * this function.*/



/*
 * Our connect and I/O functions - we supply two tables
 * which will be filled with pointers to callback functions
 * for each POSIX function. The connect functions are all
 * functions that take a path, e.g. open(), while the I/O
 * functions are those functions that are used with a file
 * descriptor (fd), e.g. read().
 */

resmgr_connect_funcs_t  connect_funcs;
resmgr_io_funcs_t       io_funcs;

/*
 * Our dispatch, resource manager, and iofunc variables
 * are declared here. These are some small administrative things
 * for our resource manager.
 */

dispatch_t              *dpp;
resmgr_attr_t           rattr;
dispatch_context_t      *ctp;
iofunc_attr_t           ioattr;

char    *progname = "null";
int     optv;                               // -v for verbose operation

int main (int argc, char **argv)
{
	int status;

    status = ThreadCtl(_NTO_TCTL_IO, 0);

    if (status == -1) {
        printf("XXXXXX: Can't gain i/o access permissions");
        exit(EXIT_FAILURE);
    }

	printf ("%s:  starting...\n", progname);

	/* Check for command line options (-v) */
	options (argc, argv);

	/* Allocate and initialize a dispatch structure for use
	 * by our main loop. This is for the resource manager
	 * framework to use. It will receive messages for us,
	 * analyze the message type integer and call the matching
	 * handler callback function (i.e. io_open, io_read, etc.) */
	dpp = dispatch_create ();
	if (dpp == NULL) {
		fprintf (stderr, "%s:  couldn't dispatch_create: %s\n",
				progname, strerror (errno));
		exit (1);
	}

	/* Set up the resource manager attributes structure. We'll
	 * use this as a way of passing information to
	 * resmgr_attach(). The attributes are used to specify
	 * the maximum message length to be received at once,
	 * and the number of message fragments (iov's) that
	 * are possible for the reply.
	 * For now, we'll just use defaults by setting the
	 * attribute structure to zeroes. */
	memset (&rattr, 0, sizeof (rattr));

	/* Now, let's intialize the tables of connect functions and
	 * I/O functions to their defaults (system fallback
	 * routines) and then override the defaults with the
	 * functions that we are providing. */
	iofunc_func_init (_RESMGR_CONNECT_NFUNCS, &connect_funcs,
			_RESMGR_IO_NFUNCS, &io_funcs);
	/* Now we override the default function pointers with
	 * some of our own coded functions: */
	connect_funcs.open = io_open;
	io_funcs.close_ocb = io_close;
	io_funcs.read = io_read;
	io_funcs.write = io_write;

	io_funcs.unblock = io_unblock;

	/* Initialize the device attributes for the particular
	 * device name we are going to register. It consists of
	 * permissions, type of device, owner and group ID */
	iofunc_attr_init (&ioattr, S_IFCHR | 0666, NULL, NULL);

	/* Next we call resmgr_attach() to register our device name
	 * with the process manager, and also to let it know about
	 * our connect and I/O functions. */
    int i;
    char name[PATH_MAX];
    for (i = 0; i < UART_CHANNEL_COUNT; i++)
    {
        snprintf(name, PATH_MAX, "/dev/rts/RS%d", i);
        iofunc_attr_init (&sample_attrs[i], S_IFNAM | 0666, NULL, NULL);
        resmgr_attach (dpp, &rattr, name,
                    _FTYPE_ANY, 0, &connect_funcs, &io_funcs, &sample_attrs[i]);
    }

    pthread_create (NULL, NULL, interrupt_thread, NULL);

	/* Now we allocate some memory for the dispatch context
	 * structure, which will later be used when we receive
	 * messages. */
	ctp = dispatch_context_alloc (dpp);

	/* Done! We can now go into our "receive loop" and wait
	 * for messages. The dispatch_block() function is calling
	 * MsgReceive() under the covers, and receives for us.
	 * The dispatch_handler() function analyzes the message
	 * for us and calls the appropriate callback function. */
	while (1) {
		if ((ctp = dispatch_block (ctp)) == NULL) {
			fprintf (stderr, "%s:  dispatch_block failed: %s\n",
					progname, strerror (errno));
			exit (1);
		}
		/* Call the correct callback function for the message
		 * received. This is a single-threaded resource manager,
		 * so the next request will be handled only when this
		 * call returns. Consult our documentation if you want
		 * to create a multi-threaded resource manager. */
		dispatch_handler (ctp);
	}
}

/*
 *  io_open
 *
 * We are called here when the client does an open().
 * In this simple example, we just call the default routine
 * (which would be called anyway if we did not supply our own
 * callback), which creates an OCB (Open Context Block) for us.
 * In more complex resource managers, you will want to check if
 * the hardware is available, for example.
 */

int io_open(resmgr_context_t *ctp, io_open_t *msg, RESMGR_HANDLE_T *handle,
        void *extra) {
    if (optv) {
        printf("%s:  in io_open\n", progname);
    }

    if (CheckBit(dev_l.dev_open, ctp->id)) {
        return EBUSY;
    } else {
        from_config(ctp->id);
        dev_l.dev_open |= (1 << ctp->id);
//        p_uart[ctp->id]->ier_dlh = p_uart[ctp->id]->ier_dlh | IER_RxD_Set;
        return (iofunc_open_default(ctp, msg, handle, extra));
    }
}

/*
 *  io_close
 *
 * We are called here when the client does an close().
 * Clear Rx and Tx interrupt, and mark in bitmask dev/rts/RS* as closed
 */

int io_close(resmgr_context_t *ctp, void *reserved, RESMGR_OCB_T *ocb) {

    dev_l.dev_open &= ~(1 << ctp->id);

    uint32_t irq_enable = 0;
    irq_enable &= ~(1 << ctp->id);
    p_uart[ctp->id]->ier_dlh = 0;

    return (close(ctp->id));
}

int io_unblock(resmgr_context_t *ctp, io_pulse_t *msg, RESMGR_OCB_T *ocb)
{
    return EOK;
}


/*
 *  options
 *
 *  This routine handles the command-line options.
 *  For our simple /dev/Null, we support:
 *      -v      verbose operation
 */

	void
options (int argc, char **argv)
{
	int     opt;
	int     i;

	optv = 0;

	i = 0;
	while (optind < argc) {
		while ((opt = getopt (argc, argv, "v")) != -1) {
			switch (opt) {
				case 'v':
					optv = 1;
					break;
			}
		}
	}
}
