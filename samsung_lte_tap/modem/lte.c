#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <libusb.h>





#define	SAMSUNG_VENDOR_ID		0x04e8
#define	SAMSUNG_STORAGE_ID		0x689a
#define	SAMSUNG_GTB3730_ID		0x6889

#define	STORAGE_INTERFACE		0
#define	STORAGE_ENDPOINT_OUT		0x6
#define	STORAGE_ENDPOINT_IN		0x85

#define	CONTROL_INTERFACE		0
#define	CONTROL_ENDPOINT_OUT		0x2
#define	CONTROL_ENDPOINT_IN		0x81

#define	MODEM_INTERFACE			1
#define MODEM_ENDPOINT_OUT		0x4
#define	MODEM_ENDPOINT_IN		0x83

#define MAX_PACKET_LEN 0x4000 /*4000*/

static unsigned char read_buffer_modem[MAX_PACKET_LEN];
static unsigned char read_buffer_ctrl[MAX_PACKET_LEN];

static struct libusb_transfer *req_transfer_modem = NULL;
static struct libusb_transfer *req_transfer_ctrl  = NULL;

static struct libusb_device_handle *devh = NULL;



#define CHECK_NEGATIVE(x) {if((r = (x)) < 0) return r;}
#define CHECK_DISCONNECTED(x) {if((r = (x)) == LIBUSB_ERROR_NO_DEVICE) exit_release_resources(0);}


static int tap_fd = -1;
static char tap_dev[20] = "c2xx";
static int tap_if_up = 0;

static int device_disconnected = 0;

int isconn=0;

static nfds_t nfds;
static struct pollfd* fds = NULL;

libusb_context *context=NULL;
struct libusb_transfer *trans=NULL;

int process_response(void *dev, const unsigned char *buf, int len);


libusb_device_handle *init_modem(int debug)
{
	int r, received, i;
	static const unsigned char modem_init1_out[] = {0x57, 0x50, 0x04, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00};
	static const unsigned char modem_init2_out[] = {0x57, 0x50, 0x04, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0xf4, 0x00, 0x00};
	static const char init_msg_1[] = 
	  {  0x57, 0x43, 0x1f, 0x00, 0x15, 0x02, 0x00, 0x00,
	     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	     0x00, 0x00, 0x15, 0x02, 0x7f, 0x0f, 0x00, 0x00,
	     0x0c, 0x00, 0x00, 0xff, 0xa0, 0x00, 0x10, 0x14,
	     0x10, 0xc9, 0xa6, 0xff, 0x7e, 0x2b, 0x00, 0x00};
	static const char init_msg_2[] = 
	  {  0x57, 0x43, 0x1f, 0x00, 0x15, 0x02, 0x00, 0x00,
	     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	     0x00, 0x00, 0x15, 0x02, 0x7f, 0x0f, 0x00, 0x00,
	     0x0c, 0x00, 0x01, 0xff, 0xa0, 0x00, 0x20, 0x22,
	     0xb2, 0xd9, 0xa6, 0xff, 0x7e, 0x72, 0x42, 0x00};
	static const char init_msg_3[] = 
	  {  0x57, 0x43, 0x1a, 0x00, 0x15, 0x02, 0x00, 0x00,
	     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	     0x00, 0x00, 0x15, 0x02, 0x7f, 0x0a, 0x00, 0x00,
	     0x07, 0x00, 0x02, 0xff, 0xa0, 0x00, 0x00, 0x7e};

	unsigned char buf[1000];
	libusb_device_handle *devhb;
	
	for(i = 60; (devhb = libusb_open_device_with_vid_pid(context, SAMSUNG_VENDOR_ID, SAMSUNG_GTB3730_ID)) == NULL && i > 0; i--) {
	  printf("%.4x:%.4x not found, will retry %d more times\n", SAMSUNG_VENDOR_ID, SAMSUNG_GTB3730_ID, i);
	  sleep(1);
	}
	printf("Device Openned in Comercial mode!\n");

	if(libusb_kernel_driver_active(devhb, MODEM_INTERFACE)){
	  libusb_detach_kernel_driver(devhb, MODEM_INTERFACE);
	  printf("Detaching kernel from MODEM\n");
	}
	if(r = libusb_claim_interface(devhb, MODEM_INTERFACE)) {
	  printf("claim modem: %d\n", r);
	  return NULL;
	}
	if(libusb_kernel_driver_active(devhb, CONTROL_INTERFACE)){
	  libusb_detach_kernel_driver(devhb, CONTROL_INTERFACE);
	  printf("Detaching kernel !\n");
	}
	if(r = libusb_claim_interface(devhb, CONTROL_INTERFACE)) {
	  printf("claim ctrl: %d\n", r);
	  return NULL;
	}
	printf("Device claimed\n");
	if(r = libusb_bulk_transfer(devhb, CONTROL_ENDPOINT_OUT, (unsigned char*)modem_init1_out, sizeof(modem_init1_out), &received, 10)) {
	  printf("sent: %d\n", received);
	}
	/*
	if(r = libusb_bulk_transfer(devhb, CONTROL_ENDPOINT_IN, (unsigned char*)buf, sizeof(buf), &received, 10)) {
	  printf("received: %d\n", received);
	  }*/

	
	if(r = libusb_bulk_transfer(devhb, CONTROL_ENDPOINT_OUT, (unsigned char*)modem_init2_out, sizeof(modem_init2_out), &received, 10)) {
	  printf("sent: %d\n", received);
	}
	
	if(debug){
	  if(r = libusb_bulk_transfer(devhb, CONTROL_ENDPOINT_OUT, (unsigned char*)init_msg_1, sizeof(init_msg_1), &received, 10)) {
	    printf("sent: %d\n", received);
	  }
	  
	  if(r = libusb_bulk_transfer(devhb, CONTROL_ENDPOINT_OUT, (unsigned char*)init_msg_2, sizeof(init_msg_2), &received, 10)) {
	    printf("sent: %d\n", received);
	  }
	  
	  if(r = libusb_bulk_transfer(devhb, CONTROL_ENDPOINT_OUT, (unsigned char*)init_msg_3, sizeof(init_msg_3), &received, 10)) {
	    printf("sent: %d\n", received);
	  }
	}


	/*
	if(r = libusb_bulk_transfer(devhb, CONTROL_ENDPOINT_IN, (unsigned char*)buf, sizeof(buf), &received, 10)) {
	  printf("received: %d\n", received);
	  }*/

	/*
	printf("MAC??\n");
	hexprintf(buf,received);
	*/
	printf("Done init\n");
	return devhb;
       
}	

void connected()
{
  isconn=1;
}

int lte_send_modem(const unsigned char *buf)
{
  int r, received, i;
  if(r = libusb_bulk_transfer(devh, MODEM_ENDPOINT_OUT, (unsigned char*)buf,strlen(buf)  , &received, 0)) {
    printf("sent: %d\n", received);
  }
}


int lte_send_ctrl(const unsigned char *buf, int len)
{
  int r, received, i;

  if(isconn){
    //printf("sending ctrl:\n");
    //hexprintf(buf,len);
  if(r = libusb_bulk_transfer(devh, CONTROL_ENDPOINT_OUT, (unsigned char*)buf, len , &received, 100)) {
    printf("sent: %d\n", received);
  }
}
}

void release_usb_device(libusb_device_handle *devhb) 
{
  libusb_release_interface(devhb, MODEM_INTERFACE);
  libusb_release_interface(devhb, CONTROL_INTERFACE);
  libusb_attach_kernel_driver(devhb, MODEM_INTERFACE);
  libusb_attach_kernel_driver(devhb, CONTROL_INTERFACE);
  //libusb_reset_device(devh);
  libusb_close( devhb );
  libusb_exit(context);
}




static int read_tap()
{

  tap_read();
  
  return 0;
}


static int process_events_once(int timeout)
{
  struct timeval tv = {0, 0};
  int r;
  int libusb_delay;
  int delay;
  unsigned int i;
  char process_libusb = 0;

  r = libusb_get_next_timeout(context, &tv);
  if (r == 1 && tv.tv_sec == 0 && tv.tv_usec == 0)
    {
      r = libusb_handle_events_timeout(context, &tv);
    }
  delay = libusb_delay = tv.tv_sec * 1000 + tv.tv_usec;
  if (delay <= 0 || delay > timeout)
    {
      delay = timeout;
    }
  CHECK_NEGATIVE(poll(fds, nfds, delay));
  process_libusb = (r == 0 && delay == libusb_delay);
  for (i = 0; i < nfds; ++i)
    {
      if (fds[i].fd == tap_fd) {
	if (fds[i].revents)
	  {
	    CHECK_NEGATIVE(read_tap());
	  }
	continue;
      }
      process_libusb |= fds[i].revents;
    }
  if (process_libusb)
    {
      struct timeval tv = {.tv_sec = 0, .tv_usec = 0};
      CHECK_NEGATIVE(libusb_handle_events_timeout(context, &tv));
    }
  return 0;
}


/* set close-on-exec flag on the file descriptor */
int set_coe(int fd)
{
  int flags;

  flags = fcntl(fd, F_GETFD);
  if (flags == -1)
    {
      printf("failed to set close-on-exec flag on fd %d\n", fd);
      return -1;
    }
  flags |= FD_CLOEXEC;
  if (fcntl(fd, F_SETFD, flags) == -1)
    {
      printf("failed to set close-on-exec flag on fd %d\n", fd);
      return -1;
    }

  return 0;
}


/* brings interface up and runs a user-supplied script */
static int if_create()
{
  tap_fd = create_tap(tap_dev);
  if (tap_fd < 0) {
    printf("failed to allocate tap interface\n");
    printf(
	      "You should have TUN/TAP driver compiled in the kernel or as a kernel module.\n"
	      "If 'modprobe tun' doesn't help then recompile your kernel.");
    //exit_release_resources(1);
  }
  
  //tap_set_hwaddr(tap_fd, tap_dev, wd_status.mac);
  tap_set_mtu(tap_fd, tap_dev, 2360/*1386*/);
  set_coe(tap_fd);
  return 0;
}

/* brings interface up and runs a user-supplied script */
static int if_up()
{
  //tap_bring_up(tap_fd, tap_dev);
  //wmlog_msg(2, "Starting if-up script...");
  //raise_event("if-up");
  tap_if_up = 1;
  //return 0;
}



int alloc_fds()
{
  int i;
  const struct libusb_pollfd **usb_fds = libusb_get_pollfds(context);

  if (!usb_fds)
    {
      return -1;
    }
  nfds = 0;
  while (usb_fds[nfds])
    {
      nfds++;
    }
  if (tap_fd != -1) {
    nfds++;
  }
  if(fds != NULL) {
    free(fds);
  }
  fds = (struct pollfd*)calloc(nfds, sizeof(struct pollfd));
  for (i = 0; usb_fds[i]; ++i)
    {
      fds[i].fd = usb_fds[i]->fd;
      fds[i].events = usb_fds[i]->events;
      set_coe(usb_fds[i]->fd);
    }
  if (tap_fd != -1) {
    fds[i].fd = tap_fd;
    fds[i].events = POLLIN;
    fds[i].revents = 0;
  }
  free(usb_fds);

  return 0;
}

void cb_add_pollfd(int fd, short events, void *user_data)
{
  alloc_fds();
}

void cb_remove_pollfd(int fd, void *user_data)
{
  alloc_fds();
}




static void cb_req_modem(struct libusb_transfer *transfer)
{
  if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
    printf("async bulk read error %d\n", transfer->status);
    if (transfer->status == LIBUSB_TRANSFER_NO_DEVICE) {
      device_disconnected = 1;
      return;
    }
  } else {
    modem_response(transfer->buffer, transfer->actual_length);
  }
  if (libusb_submit_transfer(req_transfer_modem) < 0) {
    printf("async read transfer sumbit failed\n");
  }
}

static void cb_req_ctrl(struct libusb_transfer *transfer)
{
  if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
    printf("async bulk read error %d\n", transfer->status);
    if (transfer->status == LIBUSB_TRANSFER_NO_DEVICE) {
      device_disconnected = 1;
      return;
    }
  } else {
    process_response(NULL, transfer->buffer, transfer->actual_length);
    //hexprintf(transfer->buffer, transfer->actual_length);
  }
  if (libusb_submit_transfer(req_transfer_ctrl) < 0) {
    printf("async read transfer sumbit failed\n");
  }
}


static int alloc_transfers(void)
{
  req_transfer_modem = libusb_alloc_transfer(0);
  if (!req_transfer_modem)
    return -ENOMEM;

  req_transfer_ctrl = libusb_alloc_transfer(0);
  if (!req_transfer_ctrl)
    return -ENOMEM;

  libusb_fill_bulk_transfer(req_transfer_modem, devh, MODEM_ENDPOINT_IN, read_buffer_modem,
			    sizeof(read_buffer_modem), cb_req_modem, NULL, 0);

  libusb_fill_bulk_transfer(req_transfer_ctrl, devh, CONTROL_ENDPOINT_IN, read_buffer_ctrl,
			    sizeof(read_buffer_ctrl), cb_req_ctrl, NULL, 0);

  return 0;
}


int main(int argc, char **argv)
{
  int i;
  char toto;

  const struct libusb_pollfd **fds;
  struct event *ev1;
  char buf[1000];
  char cmd[100];
  int received,r;
  char *debug_ip = NULL;
  char *apn=NULL;
  int c;
  int debug=0;

  opterr = 0;
  while ((c = getopt (argc, argv, "va:d:")) != -1){
    switch (c){
    case 'v':
      break;
    case 'd':
      debug_ip = optarg;
      debug=1;
      break;
    case 'a':
      apn = optarg;
      break;
    }
  }
  
  if(!apn){
    printf("Usage: ./lte -a APN_NAME [-d GSM_TAP_IP]\nex: ./lte -a orange.fr [-d 192.168.0.1]\n");
    return -1;
  }

  modem_at_init();

  libusb_init(&context);
  libusb_set_debug(context, 3);
  devh = init_modem(debug);


  sleep(1);
  
  if_create();

  alloc_fds();
  libusb_set_pollfd_notifiers(context, cb_add_pollfd, cb_remove_pollfd, NULL);

  alloc_transfers();
  libusb_submit_transfer(req_transfer_modem);  
  libusb_submit_transfer(req_transfer_ctrl);  

  if(debug){
    gsmtap_open(debug_ip);
  }
  while(1){
    process_events_once(100);
    //  printf(".\n");
    modem_process(apn);
  }

  release_usb_device(devh);
  return 0;

}

