#include <stdio.h>
#include <string.h>

int hexprintf(unsigned char *c, unsigned int len)
{

  int i,j;
  char tmp[16];

  while(len){
    for(i = 0; i < 16; i++){
      len--;
      printf("%02x ",*c);
      tmp[i] = *c;
      c++;
      /*if(i == 7){
	printf("  ");
	}*/

      if(!len){
	break;
      }
    }

    for(j = 0; j < 15 -i; j++){
      printf("   ");
    }

    printf("  |");
    for(j = 0; j < i; j++){
      if((tmp[j] > 32) && (tmp[j] < 128)){
	printf("%c",tmp[j]);
      } else {
	printf(".");
      }
    }
    for(j = 0; j < 16-i; j++){
      printf(" ");
    }
    printf("|\n");
  }
}


