#ifndef IM_
#define IM_


int codeCommandPacket(char *buf, char tar[3], int len ) { //  GSM 03.48
len+=13+1; // Add CommandPacketHeaderLen + CHL(1)
buf[0]=0x2, buf[1]=0x70, buf[2]=0x0; buf+=3;      //  STD UDHL+UDH: 02 + 70 00
buf[0] = (len>>8)&0xFF; buf[1]=len&0xFF; buf+=2;  //  CPL (2)-rest packlen (CHL+CommandHeader+SecuredData)
buf[0] = 0xD; buf++;  // CHL - header length (from SPI to RCC inc) - 13bytes
buf[0]=0; buf[1]=0; buf+=2; // 00 00 // SPI: NO-RC,NO-ENC,NO-COUNTER  NO-POR
buf[0]=0; buf[1]=0; buf+=2; // UNUSED: KIC KID
buf[0]=tar[0]; buf[1]=tar[1]; buf[2]=tar[2]; buf+=3; // 01 1A 00  // TAR(3)
memset(buf,0,5+1); // UNUSED COUNTER(5)+PCNTR
return 16+3; //
}

// im coders

typedef struct {
  uchar code;
  uchar flags;
  short num;
  short len;
  } im_cmd;

typedef struct {
 uchar nslot;
 uchar dur;
 uchar ttl;
 uchar page; // codepage
 uchar len;
 } im_slot;

int im_code_livetext(char *buf, int slot, int dur /*show sec*/, int ttl /* in hour*/, char *text) { // koi8
int  len = strlen(text);
if (len>24) len = 24; // no more
int  dlen = len+5;


im_cmd *c = (void*)buf;
c->code = 0x5;
c->flags = 0x0; // звук не требуется - но все равно играет.?
c->num = htons(1);
c->len = htons(dlen);
buf+=6; im_slot *s = (void*)buf;

s->nslot = slot;
s->dur = dur;
s->ttl = ttl;
s->page = 0x83 ; // koi8r
s->len = len;

buf+=5; memcpy(buf,text,len); // copy data


return len + 5/*SLOT*/+ 6/*CMD*/;
}



#endif
