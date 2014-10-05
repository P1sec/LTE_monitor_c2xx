#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int process_config_response(void *dev, const unsigned char *buf, int len)
{
  //printf("Config:\n");
  //  hexprintf(buf,len);
  gsmtap_send(buf,len);
  return 0;
}

int process_data_response(void *dev, const unsigned char *buf, int len)
{
  //printf("Data:\n");
  //  hexprintf(buf,len);
  tap_write(buf+6, len - 6);
  return 0;
}

int process_P_response(void *dev, const unsigned char *buf, int len)
{
  printf("P!:\n");
  unsigned char macaddr[6];
  int i;
  char resp[] = {0x57, 0x50, 0x04, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x01, 0x52, 0x00};
  switch(buf[2]){
  case 0x12:
    memcpy(macaddr,buf + 10, 6);
    for(i = 0; i < 6; i++){
      printf("%02x:",macaddr[i]);
    }
    printf("\n");
    tap_set_macaddr(macaddr);
    break;
  case 0x0e:
    hexprintf(buf,len);
    //lte_send_ctrl(resp,sizeof(resp));
    break;
  default:
    printf("UNKNOWN!\n");
    hexprintf(buf,len);
    break;  
  }

  return 0;
}



int process_response(void *dev, const unsigned char *thebuf, int len)
{
  int check_len;
  unsigned char *buf;

  buf = (unsigned char *)thebuf;
  if(len < 4) {
    printf( "short read");
    return -1;
  }
  //hexprintf(buf,len); 
  while(buf < thebuf + len){
    if(buf[0] != 0x57) {
      printf( "bad header\n");
      return -1;
    }

    check_len = 4 + buf[2] + (buf[3] << 8);
    if(buf[1] == 0x43 || buf[1] == 0x44 || buf[1] == 0x50) {
      check_len += 2;
    }
    //printf("check_len %d\n", check_len);

    switch (buf[1]) {
    case 0x43:
      process_config_response(dev, buf, check_len);
      break;
    case 0x44:
      //return process_data_response(dev, buf, len);
      //printf("got one\n");
      //hexprintf(buf,check_len);
      process_data_response(dev, buf, check_len);
      break;
    case 0x50:
      process_P_response(dev, buf, check_len);
      break;
    default:
      printf("bad response type: %02x\n", buf[1]);
    }
    buf += check_len; 
  }
}
