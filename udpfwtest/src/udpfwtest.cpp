/*
* udpfwtest.cpp
*
*  Created on: 23 sep 2013
*      Author: johan.e.hogdahl@gmail.com
*/


#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>


#define MAXPACKSIZE 35000

int main(int argc, char **argv) {
  struct sockaddr addrfrom;
  struct sockaddr addrto;
  int sock;
  u_char outpack[MAXPACKSIZE];
  struct iphdr *ip;
  struct udphdr *udp;

  int packetsize = 0;
  int datasize;
  struct sockaddr_in *from;
  struct sockaddr_in *to;
  struct protoent *proto;


  if (argc<5) {
      fprintf(stderr,"Usage: %s srcaddr srcport dstaddr dstport [udplength]\n", argv[0]);
      fprintf(stderr,"addr formate xxx.xxx.xxx.xxx\n");
      fprintf(stderr,"Be carefull, wrongly used this program can do a lot of damage\n");
      fprintf(stderr,"Program has to be run by root\n\n");


      return 1;
  }

  const char * srcaddr=argv[1];
  int srcport=atoi(argv[2]);
  const char * destaddr=argv[3];
  int destport=atoi(argv[4]);
  int preflen= (argc > 5) ? atoi(argv[5]) : 0; // prefered packe length, will always be as large as needed to send a packet

  if(preflen > MAXPACKSIZE){
      fprintf(stderr, "Oversized packet, max size is %d", MAXPACKSIZE);
  }

  if(strcmp(srcaddr,destaddr) == 0 && srcport == destport){
      fprintf(stderr, "Spoof warning, srcadd and destaddr, srcport and destport pairs may not all be same\n");
      return 1;
  }

  if(srcport == 7){
      fprintf(stderr, "Spoof warning, echo port may not be used as source\n");
      return 1;
  }


  if (!(proto = getprotobyname("udp"))) {
      fprintf(stderr,"getprotobyname failed\n");
      return 1;
  }

  //printf("proto is %d", proto->p_proto);


  memset(&addrfrom, 0, sizeof(struct sockaddr));
  from = (struct sockaddr_in *)&addrfrom;
  from->sin_family = AF_INET;
  from->sin_port=htons(srcport);
  if (!inet_aton(srcaddr, &from->sin_addr)) {
      fprintf(stderr,"Bad srcaddr: %s\n",srcaddr);
      return 1;
  }

  memset(&addrto, 0, sizeof(struct sockaddr));
  to = (struct sockaddr_in *)&addrto;
  to->sin_family = AF_INET;
  to->sin_port=htons(destport);
  if (!inet_aton(destaddr, &to->sin_addr)) {
      fprintf(stderr,"Bad destaddr: %s\n",srcaddr);
      return 1;
  }

  if (!(proto = getprotobyname("udp"))) {
      fprintf(stderr,"getprotobyname(udp)");
      return(2);
  }

  if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
      fprintf(stderr,"socket failed\n");
      return 1;
  }

  // create UDP Packet in outpack
  ip=(struct iphdr *)outpack;
  ip->version=4;
  ip->ihl=5;  // header size
  ip->tos=0;
  ip->id=0x0;
  ip->frag_off=0x40;
  ip->ttl=0x40;
  ip->protocol=proto->p_proto; // 0x11
  ip->check=0;
  ip->saddr=from->sin_addr.s_addr;
  ip->daddr=to->sin_addr.s_addr;
  packetsize+=ip->ihl<<2;

  int udphdsize = sizeof(struct udphdr) - 2;
  packetsize+= udphdsize;

  // a visible message
  sprintf((char*)outpack + packetsize,"%s",argv[0]);
  datasize=strlen(argv[0])+1;
  packetsize+=datasize;

  if(packetsize < preflen){
      packetsize = preflen;
      datasize = packetsize - (ip->ihl<<2) - udphdsize;
  }

  // udp header
  udp=(struct udphdr *)((char *)outpack + (ip->ihl<<2));
  udp->source=htons(srcport);
  udp->dest=htons(destport);
  udp->len=htons(udphdsize+datasize);
  udp->check=0; // does not seem to be used ...
  ip->tot_len=htons(packetsize);

  if (sendto(sock, (char *)outpack, packetsize, 0, &addrto, sizeof(struct sockaddr)) < 0) {
      fprintf(stderr,"sendto failed\n");
      return 1;
  }

  if(packetsize < 100){
      int tot = 0;
      int i , j;
      for(j = 0 ; j < 10 && tot < packetsize; j++){
          int hid = tot;
          printf("    %04x:",hid);
          for(i = 0 ; i < 8 && tot < packetsize ; i++){
              printf("%02x", outpack[tot++]);
              if(tot < packetsize){
                  printf("%02x ", outpack[tot++]);
              }
          }
          printf("\n");
      }
  }


  printf("sent %s %d bytes\n",argv[0], packetsize);
  close(sock);
  return(0);
}




