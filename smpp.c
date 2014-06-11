/*
  Модуль поддержки smpp соединения и сервера...
  Тут должны быть только кодеры - без всяких функций сокетов...
*/


#include "smpp.h"
#include "coders.h"


int smppPackString(smppCommand *cmd, char *str, int max) {
int l; char *d = cmd->body+cmd->len-16; // Point of copy
if (max<0) return 0; // Invalid pack
if (!str) str="";
l = strlen(str); if (l>max) l = max;
//printf("PackStr='%s', cmd->len=%d max=%d l=%d\n",str,cmd->len,max,l);
if (cmd->len+l+1>=sizeof(cmd->body)) return 0; // overflow
//printf("l=%d\n",l);
if (l>0) memcpy(d,str,l); // Copy Data
//printf("set?\n");
//memset(d+l,0,max-l); // Rest is zeros
d[l]=0;
//printf("Done!\n");
cmd->len+=l+1; // Len+zero
//printf("DonePackStr, len=%d\n",cmd->len);
return 1; // OK
}

int smppPackFixed(smppCommand *cmd,char *str,int max) {
if (!str || !*str) return smppPackString(cmd,0,0); // NULL
return smppPackString(cmd,str,max);
}

int smppPackBytes(smppCommand *cmd,void *val,int len) {
unsigned char *d = cmd->body+cmd->len-16;
memcpy(d,val,len);
cmd->len+=len;
return 1;
}


int smppPackByte(smppCommand *cmd,unsigned char val) { return smppPackBytes(cmd,&val,1);}
int smppPackWord(smppCommand *cmd,unsigned short val) { val=HTONS(val); return smppPackBytes(cmd,&val,2);}

int smppPackBind(smppCommand *cmd,int id, int num, char *system_id, char *password, char *system_type) {
int ok;
cmd->len = 16; cmd->id = id; cmd->status=0; cmd->num = num;
ok = smppPackString(cmd,system_id,16) // Max 15 + NULL
  && smppPackString(cmd,password,9)
  && smppPackString(cmd,system_type,13)
  && smppPackByte(cmd, SMPP_VERSION) // 34 - version? smppdoc 5.2.5
  && smppPackByte(cmd, 0) // AddrTon
  && smppPackByte(cmd, 0) // AddrNPI
  && smppPackString(cmd,"",41);
if (!ok) return 0; // Fail
ok = cmd->len; // real packet length
cmd->len = NTOHL(cmd->len); // Convert to network
return ok; // real length
}

int smppPackAddr(smppCommand *cmd,uchar *num) {
int ton=tonNational,npi=npiISDN;
if (!num) num="";
if (num[0]=='+') {ton=tonInternational; num++;}
return smppPackByte(cmd,ton) && smppPackByte(cmd,npi) && smppPackString(cmd,num,65);
}

int smppPackTLV(smppCommand *cmd,short id,short len,void *data) {
return smppPackWord(cmd, id) && smppPackWord(cmd,len) && smppPackBytes(cmd,data,len);
}


int smppPacketReady(uchar *data,int len) {
int ilen;
if (len<4*4) return 0; // Not Yet Header
ilen = HTONL(*((int*)data)); // MyNew Len ?
if (len<4*4 || ilen>SMPP_MAX_DATA) {
    printf("smppPacketReady: invalid smpp cmdlen=%d from %x\n",ilen,*(int*)data);
    return -1;
    }
if (ilen>len) return 0; // Not Yet Body
//printf("smppPacket here len=%d\n",ilen);
return ilen; // ok - packet here
}

void hexdumpcb(uchar *msg,uchar *data,int len,void (*fun)(),void *hfun) {
uchar buf[1024];
sprintf(buf,"hexdump %s len=%d\n",msg,len); fun(hfun,buf,strlen(buf));
while(len>0) {
    int i; uchar *p = buf;
    for(i=0;i<16;i++) {
        if (i<len) sprintf(p,"%02x ",data[i]);
           else sprintf(p,"   "); // fill a rest
        p+=3;
        }
    sprintf(p,"\n"); p++; // New line
    fun(hfun,buf,strlen(buf));
    len-=16; data+=16;
    }
}

void prnstr(void *handle,uchar *buf,int len) { printf("%*.*s",len,len,buf);}
void hexdump(uchar *msg,void *data,int len) { hexdumpcb(msg,data,len, prnstr,0); }

void smppHead(int *buf,int cmd,int status,int ref) {
buf[0]=HTONL(16); // len
buf[1]=cmd;
buf[2]=status;
buf[3]=ref;
}

int smppPackResp(smppCommand *cmd, int cmd_id, int cmd_status, int seq_number, char *msg_id) {
int *L;
smppHead((void*)cmd->body,cmd_id, HTONL(cmd_status), seq_number); cmd->len = 16; // Just a header
if (msg_id) { // Add C- Strin here ...
    smppPackString(cmd,msg_id,255); // JustIt
    }
cmd->dlen = cmd->len;
L = (void*) cmd->body;
*L = HTONL(cmd->dlen); // Convert a length
return cmd->dlen; // Total Convertred Lenth
}

int smppPack_DELIVER_SM_RESP(smppCommand *cmd, int status, char *msg) {
return smppPackResp(cmd,smpp_deliver_sm_resp, status, cmd->num, msg); // Pack It
}

int smppPack_SUBMIT_SM_RESP(smppCommand *cmd, int status, char *msg) {
return smppPackResp(cmd,smpp_submit_sm_resp, status, cmd->num, msg); // Pack It
}

int smppPack_DATA_SM_RESP(smppCommand *cmd, int status, char *msg) {
return smppPackResp(cmd,smpp_data_sm_resp, status, cmd->num, msg); // Pack It
}


int smppPopString(smppCommand *cmd, int *pos, uchar **u) { // Extracts a string
int len,i = *pos;
char *b=cmd->body+(*pos); //  Gets a begin
*u=b;
if (i>cmd->dlen) return 0; // Over
for(len=0;i+len<cmd->dlen && b[len];len++); // def len
(*pos)+=len+1;
return len+1; // OK
}

int smppPopByte(smppCommand *cmd,int *pos,unsigned char *val) {
if (*pos>=cmd->dlen) return 0; // Over
*val = cmd->body[*pos]; *pos+=1;
return 1; // Just Revert
}

int smppPopAddr(smppCommand *cmd,int *pos,char *addr) { // Gets addr to a buffer?
int ok; uchar ton,npi,*num;
ok  = smppPopByte(cmd,pos,&ton) && smppPopByte(cmd,pos,&npi) && smppPopString(cmd,pos,&num);
if (!ok) return 0;
if (ton==tonInternational) {
    sprintf(addr,"+%s",num);
    }
else { // Just Ignore???
    sprintf(addr,"%s",num);
    }
return 1; // OK
}



int smppPackSubmit(smppCommand *cmd,int num,char *service_type,char *src_addr,char *dst_addr,
  int esm_class,
   int registered_delivery,int pid, int data_coding,char *data,int len) { // Payload Here
int  ok;
cmd->len = 16; cmd->id = smpp_submit_sm; cmd->status=0; cmd->num = num;
if (len<0) len = strlen(data);
ok = smppPackString(cmd,service_type,6) // Max 15 + NULL
  && smppPackAddr(cmd,src_addr)
  && smppPackAddr(cmd,dst_addr)
  && smppPackByte(cmd,esm_class) // 0=Store&Forward, 1=DATAGRAM, 2=Forward
  && smppPackByte(cmd,pid) // PID
  && smppPackByte(cmd,0) // Priority
  && smppPackString(cmd,"",0) // ShedTime
  && smppPackString(cmd,"",0) // VP
  && smppPackByte(cmd,registered_delivery) // delivery reports???
  && smppPackByte(cmd,1) // replaceIfpresent !!!
  && smppPackByte(cmd,data_coding)
  && smppPackByte(cmd,0) // Def message ID
  && smppPackByte(cmd,len) // DataLength
  && smppPackBytes(cmd,data,len); // MessagePayload - TLV 5.3.2.32
if (!ok) return 0;
cmd->dlen = cmd->len; cmd->len = HTONL(cmd->len);
return cmd->dlen;
}


int smppPackDataSM(smppCommand *cmd,int num,char *service_type,char *src_addr,char *dst_addr,int esm_class,
   int registered_delivery,int pid, int data_coding,char *data,int len) { // Payload Here
int  ok;
cmd->len = 16; cmd->id = smpp_data_sm; cmd->status=0; cmd->num = num;
if (len<0) len = strlen(data);
ok = smppPackString(cmd,service_type,6) // Max 15 + NULL
  && smppPackAddr(cmd,src_addr)
  && smppPackAddr(cmd,dst_addr)
  && smppPackByte(cmd,esm_class) // 0=Store&Forward, 1=DATAGRAM, 2=Forward
  && smppPackByte(cmd,registered_delivery) // delivery reports???
  && smppPackByte(cmd,data_coding)
  && smppPackTLV(cmd,smpp_message_payload,len,data); // MessagePayload - TLV 5.3.2.32
if (!ok) return 0;
cmd->dlen = cmd->len; cmd->len = HTONL(cmd->len);
return cmd->dlen;
}

int smppPackDataOK(smppCommand *cmd, int id, int num, char *message_id) {
int ok;
cmd->len = 16; cmd->id = id; cmd->status=0; cmd->num = num;
ok = smppPackString(cmd,message_id,65); // on OK
if (!ok) return 0;
ok = cmd->len; cmd->len = HTONL(cmd->len);
return ok; // Yes!
//delivery_failure_reasona TLV Include to indicate reason for
//delivery failure.
//5.3.2.33
}

int smppDecodeBind(smppCommand *cmd, uchar **u,uchar **p,uchar **sysID) {
int pos=0;
return smppPopString(cmd,&pos,u) && smppPopString(cmd,&pos,p) && smppPopString(cmd,&pos,sysID);
}

int smppPackBindResp(smppCommand *cmd, int mode, int status) {
int id = smpp_bind_transceiver_resp;
if (mode == 1) id = smpp_bind_receiver_resp;
 else if (mode == 2) id = smpp_bind_transmitter_resp;
cmd->id = id; cmd->status=HTONL(status); cmd->dlen = 16;
cmd->len = HTONL(cmd->dlen);
return cmd->dlen; // Yes!
}

int smppPackDelivr(smppCommand *cmd,int num,char *service_type,char *src_addr,char *dst_addr,
  int esm_class,
   int registered_delivery,int pid, int data_coding,char *data,int len) { // Payload Here
int  ok;
//printf("PackDelivr... src_add='%s', srv_type='%s' dst_adr='%s' text='%s' len=%d\n",src_addr,service_type,dst_addr,data,len);
cmd->len = 16; cmd->id = smpp_deliver_sm; cmd->status=0; cmd->num = num;
if (len<0) len = strlen(data);
ok = smppPackString(cmd,service_type,6) // Max 15 + NULL
  && smppPackAddr(cmd,src_addr)
  && smppPackAddr(cmd,dst_addr)
  && smppPackByte(cmd,esm_class) // 0=Store&Forward, 1=DATAGRAM, 2=Forward
  && smppPackByte(cmd,pid) // PID
  && smppPackByte(cmd,0) // Priority

  && smppPackString(cmd,"",0) // ShedTime

  && smppPackString(cmd,"",0) // VP

  && smppPackByte(cmd,registered_delivery) // delivery reports???
  && smppPackByte(cmd,1) // ReplaceIFPresentdelivery reports???
  && smppPackByte(cmd,data_coding)
  && smppPackByte(cmd,0) // Def message ID
  && smppPackByte(cmd,len) // DataLength
  && smppPackBytes(cmd,data,len); // MessagePayload - TLV 5.3.2.32

  ;
//printf("Res:%d\n",ok);
if (!ok) return 0;
ok = cmd->len; cmd->len = HTONL(cmd->len);
//printf("Done?\n");
return ok;
}



int smppDecodeDelivr(smppCommand *cmd) {
int pos=0,ok;
//hexdump("DecodeDelivr:",cmd->body,cmd->dlen);
ok = smppPopString(cmd,&pos,&cmd->service_type)
     && smppPopAddr(cmd,&pos,cmd->src_addr)
     && smppPopAddr(cmd,&pos,cmd->dst_addr)
     && smppPopByte(cmd,&pos,&cmd->esm_class)
     && smppPopByte(cmd,&pos,&cmd->pid)
     && smppPopByte(cmd,&pos,&cmd->priority)
     && smppPopString(cmd,&pos,&cmd->shed_time)
     && smppPopString(cmd,&pos,&cmd->vp)
     && smppPopByte(cmd,&pos,&cmd->registred_delivery)
     && smppPopByte(cmd,&pos,&cmd->replace_if_present)
     && smppPopByte(cmd,&pos,&cmd->data_coding)
     && smppPopByte(cmd,&pos,&cmd->sm_default_msg_id)
     && smppPopByte(cmd,&pos,&cmd->sm_length);
if (!ok) return 0; // Decode error???
cmd->text = cmd->body+pos; cmd->text[cmd->sm_length]=0; // Zero Term anyway -)))
if ((cmd->data_coding&0xF) == 8) { // Unicode !!!
    //gsm2win(cmd->text,cmd->text,cmd->sm_length); // Decode It
    //gsm2utf(); // ZU?
    }
//printf("HERE IN SMS (Delivr) from='%s' to '%s' dcs=%d text=%s\n",
  // cmd->src_addr,cmd->dst_addr, cmd->data_coding,cmd->text);
return 1; // OK
}


int smppUnpackTLV(smppCommand *cmd, int pos) { // Retrive important commands...
int id,  len, LEN; uchar *data;
data = cmd->body + pos; LEN = cmd->dlen - pos;
while(LEN> 4+4) { // Have Smth
 id = HTONS( *(unsigned short*)data); data+=2; len = HTONS(*(unsigned short*)data); data+=2; LEN-=4;
 //printf("TLV - ID=%d LEN=%d RLEN=%d",id,len,LEN); // Here
 switch(id) {
 case smpp_message_payload: cmd->text=data; cmd->tlen = len; break;// SetTextHere
 }
 if (len>LEN || len<0) break; // ERROR!!!
 data+=len; LEN-=len; // again???
 }
return 1; // OK - done
}


int smppDecodeSubmit(smppCommand *cmd) {
int ok,pos=0;
ok = smppPopString(cmd,&pos,&cmd->service_type)
     && smppPopAddr(cmd,&pos,cmd->src_addr)
     && smppPopAddr(cmd,&pos,cmd->dst_addr)
     && smppPopByte(cmd,&pos,&cmd->esm_class)
     && smppPopByte(cmd,&pos,&cmd->pid)
     && smppPopByte(cmd,&pos,&cmd->priority)
     && smppPopString(cmd,&pos,&cmd->shed_time)
     && smppPopString(cmd,&pos,&cmd->vp)
     && smppPopByte(cmd,&pos,&cmd->registred_delivery)
     && smppPopByte(cmd,&pos,&cmd->replace_if_present)
     && smppPopByte(cmd,&pos,&cmd->data_coding)
     && smppPopByte(cmd,&pos,&cmd->sm_default_msg_id)
     && smppPopByte(cmd,&pos,&cmd->sm_length);
if (!ok) return 0; // Decode error???
cmd->text = cmd->body+pos; cmd->text[cmd->sm_length]=0; // Zero Term anyway -)))
if ((cmd->data_coding&0xF) == 8) { // Unicode !!!
    //gsm2win(cmd->text,cmd->text,cmd->sm_length); // Decode It
    }
return 1; // ok
}


int smppDecodeDataSM(smppCommand *cmd) {
int pos=0,ok;
//hexdump("DecodeDelivr:",cmd->body,cmd->dlen);
cmd->text = 0;
ok = smppPopString(cmd,&pos,&cmd->service_type)
     && smppPopAddr(cmd,&pos,cmd->src_addr)
     && smppPopAddr(cmd,&pos,cmd->dst_addr)
     && smppPopByte(cmd,&pos,&cmd->esm_class)
     //&& smppPopByte(cmd,&pos,&cmd->pid)
     //&& smppPopByte(cmd,&pos,&cmd->priority)
     //&& smppPopString(cmd,&pos,&cmd->shed_time)
     //&& smppPopString(cmd,&pos,&cmd->vp)
     && smppPopByte(cmd,&pos,&cmd->registred_delivery)
     //&& smppPopByte(cmd,&pos,&cmd->replace_if_present)
     && smppPopByte(cmd,&pos,&cmd->data_coding)
     //&& smppPopByte(cmd,&pos,&cmd->sm_default_msg_id)
     //&& smppPopByte(cmd,&pos,&cmd->sm_length);
     && smppUnpackTLV(cmd,pos);
if (!ok) return 0; // Decode error???
if (!cmd->text) cmd->text=""; // Empty Message Payload???
cmd->text[cmd->tlen]=0; // SetTextLength
//cmd->text = cmd->body+pos; cmd->text[cmd->sm_length]=0; // Zero Term anyway -)))
if ((cmd->data_coding&0xF) == 8) { // Unicode !!!
    //gsm2win(cmd->text,cmd->text,cmd->tlen); // Decode It
    }
//printf("HERE IN SMS (Delivr) from='%s' to '%s' dcs=%d text=%s\n",
  // cmd->src_addr,cmd->dst_addr, cmd->data_coding,cmd->text);
return 1; // OK
}


