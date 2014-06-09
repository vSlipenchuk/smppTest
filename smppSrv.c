#include "smpp.h"
#include "gearSMPP.h"

void smppSocketDone(smppSocket *sock) {
SocketDone(&sock->sock); // Clear Socket if any
// Queue - done???
}

VS0OBJ0(smppSocket,smppSocketDone);
VS0OBJ0(smppMsg,0); //Socket,smppSocketDone);

VS0OBJ0(smppSrv,0); // �������� ������� "������"


smppMsg* smppMsgFindByNum(smppMsg *msg,int num) { // Find ???
int i;
for(i=0;i<arrLength(msg);i++) {
    //printf("check num = %d\n",msg[i].cmd.num);
    if (msg[i].cmd.num==num) return msg+i;
    }
//printf("Message num=%d not found in %d ignore...\n",num,arrLength(msg));
return 0; //
}

void smppSendResp(Socket *sock,int cmd, int status, int ref) { // Generic Nack send
int ok; uchar buf[1024];
//printf("HereBind smpp connection\n");
ok = 1; // OK, authorized or 0 if no....
smppHead((void*)buf,cmd,status,ref); // OK responce
// do a send -->
strCat(&sock->out,buf,16); // Push it & Forget???
sock->state = sockSend;
}

void smppSendNak(Socket *sock,int ref) { smppSendResp(sock,smpp_generic_nack,0,ref);}// Generic Nack send

/*
int smppOnMessage(Socket *sock,smppCommand *cmd,smppSrv *srv) { // OK - do a message !!!
int pos = 0; int len; smppCommand CMD;
uchar *service_type,src_addr[80],dst_addr[80],esm_class,registred_delivery,data_coding;
//smppCommand CMD;
// DataSM here ...
hexdump("DecodeDataSM",cmd,cmd->len);
smppPopString(cmd,&pos,&service_type);
smppPopAddr(cmd,&pos,src_addr); // It Must be a buffer???
smppPopAddr(cmd,&pos,dst_addr);
smppPopByte(cmd,&pos,&esm_class);
smppPopByte(cmd,&pos,&registred_delivery);
smppPopByte(cmd,&pos,&data_coding);
// rest = is payload!!!
printf("HERE IN DATA_SMS from='%s' to '%s' esm:%d reg:%d num=%d\n",
   src_addr,dst_addr,   esm_class,registred_delivery,cmd->num);
// ok - send data_sm resp???
len = smppPackDataOK(&CMD,smpp_data_sm_resp,cmd->num,"HelloID"); // Just This Hello ID
if (len<=0) {
    printf("CodePAckDataOK error!!!\n"); return 0;
    }
strCat(&sock->out,(void*)&CMD,len); // Push it & Forget???
sock->state = sockSend;
return 0;
}
*/

/*
int smppSrvDataReady(Socket *sock,uchar *data,int len,smppSrv *srv) {
SocketPool *sp = &srv->srv;
smppCommand *cmd = (void*)data;
printf("smppDataReady on sock=%d\n",sock->sock); // ��� ������ ������ - �� ???
hexdump("smppData",data,len); // DumpThisMessage
uchar *out=0;
IFLOG(sp,3,"smppSrvDataReady: %s",out);
strClear(&out);
printf("cmd1=%x bind_transeiver=%x\n",cmd->id,smpp_bind_transceiver);
cmd->len = NTOHL(cmd->len); // Reverse It
switch(cmd->id) {
case smpp_bind_transceiver:   smppOnBind(sock,3,cmd,srv);  break; // Inout
case smpp_bind_receiver:      smppOnBind(sock,1,cmd,srv);  break; // in only
case smpp_bind_transmitter:   smppOnBind(sock,2,cmd,srv);  break; // out only
case smpp_unbind:
   printf("Here unbind...\n");
   smppSendResp(sock,smpp_unbind_resp,0,cmd->num); // Simple Resp
   //sock->onSend = ...
   break;
case smpp_generic_nack:
   printf("Nack here.. ignore\n");
   break;
case smpp_data_sm:
   printf("DataMessageHere...\n"); // ok - start decode & print a message???
   smppOnMessage(sock,cmd,srv);
   break;
default: // message unknown
  smppSendNak(sock,cmd->num); // GenericNak send -)))
  IFLOG(sp,3,"unkouwn message");
  return len; // Close a connection -)))
}
return len; // ok - remove from a connect -)))
}
*/

int smppAcceptConnection(smppSocket *master, int handle, int ip) { // ��������� TCP ����������, ������� ����� smppSocket � ��������� � ���
smppSocket *sock; SocketPool *srv=master->sock.pool;
printf("Accept a new connection???\n");
sock = smppSocketNew();
sock->sock.sock = handle; ip2szbuf(ip,sock->sock.szip); sock->sock.state = sockConnected; // Default properties
sock->sock.checkPacket = master->sock.checkPacket;
sock->onNewMessage = master->onNewMessage;
sock->onBind = master->onBind;
sock->log = master->log; sock->logLevel = master->logLevel; // CopyLogInfo

sock->sock.N = ++srv->connects; // first is 1
sock->sock.state = sockConnected;
sock->sock.connectStatus = connectTCP;
sock->sock.modified = TimeNow;
sock->sock.readPackets = master->sock.readPackets;
ip2szbuf(ip,sock->sock.szip);

// ��������� � ��� ����������
Socket2Pool(&sock->sock,master->sock.pool);
printf("Accepted!\n");
return 1; // OK
}

smppSrv *smppSrvCreate(int port, void *onBind, void *onMessage) { // ������������� � ������ �������
smppSrv *srv;
smppSocket *sock;
srv = smppSrvNew();
srv->port = port;
//srv->onSmppBind = onSmppBind;
//srv->onSmppPacket = onSmppPacket;
//srv->onSmppDone = onSmppDone;
sock = smppSocketNew(); // Crete Socket Object
sock->onBind = onBind;
sock->onNewMessage = onMessage;
sock->sock.checkPacket = onSMPPClientPacket;
//sock->onConnect = smppNewClient; // -- auto accept every new connection...
srv->master = sock; sock->sock.pool = srv; // Remember a pool for accept -)))
// Have to add a pool???
Socket2Pool(&sock->sock,&srv->srv);
sock->sock.onConnect = (void*) smppAcceptConnection; // default = Accept a connection???
if (!SocketListen(&sock->sock,srv->port)) {
        //printf("Fail Listen socket smpp-port %d\n",port);
        smppSrvClear(&srv);
        return 0;
     }
SocketPush(&srv->srv.sock,&sock->sock); // Register it in a pool
return srv;
}

/// -- common utils

int onSmppMessageStatus(smppMsg *msg, smppCommand *cmd, smppSocket *sock) { // When status changes
int idx;
//printf("onMessageStatus: message %d -> changed state, new status=%x\n",msg->cmd.num,cmd->status);
if (cmd->status==0) { // OK, out message sent!!!
    //printf("Message Sent OK, remove from a queue...\n");
    } else {
    //printf("Message SEND FAILED, remove anyway...\n");
    }
// do a remove???
idx = msg-sock->out;
//printf("Remove Index %d, len=%d\n",idx,arrLength(sock->in));
arrDelN((void**)&sock->out,idx,1); // Remove???
//printf("Index removed, newlen=%d\n",arrLength(sock->in));
return 1; // OK
}

smppMsg *smppSocketSendText(smppSocket *sock,char *from,char *to,uchar *data, void *onMessage) {
smppMsg *msg;
    int len,dcs=0,dlen,rus=0;
char buf[1024];
if (!sock->out) sock->out = smppMsgArray(10); // New Array Here
msg = smppMsgAdd(&sock->out,0); // Add new message to end...
msg->onChangeState = onMessage;
msg->num  = ++sock->num; // New Socket Number???
if (!from || !from[0]) from=sock->src_addr;
dcs=0,rus=0;
for(len=0;data[len];len++) if (data[len]>128) rus++; // Count Rus Letters
if (rus>0) dcs=8; // Unicode
if (dcs==0) {
    len = strlen(data);
    memcpy(buf,data,len);
   } else {
    len = dlen = win2gsm(buf,data,strlen(data)); //unsigned char *d, unsigned char *s, int len) {
    }
printf("sending %d bytes dcs=%d\n",len,dcs);
//sock->sendMode = 2; // OVER!!!
switch(sock->sendMode) {
case 1: len = smppPackSubmit(&msg->cmd,msg->num,sock->sysID,from,to,esmStoreAndForward,
        1 /* request delivery_report */, dcs, buf,len); // Just Do IT
        break;
case 2: len = smppPackDelivr(&msg->cmd,msg->num,sock->sysID,from,to,esmStoreAndForward,
        1 /* request delivery_report */, dcs, buf,len); // Just Do IT
        break;
default: len = smppPackDataSM(&msg->cmd,msg->num,sock->sysID,from,to,esmStoreAndForward,
        1 /* request delivery_report */, dcs, buf,len); // Just Do IT
}
if (len<=0) {
    CLOG(sock,1,"-SendText: PackFailed!");
    return 0;
    }
DLOG(sock,5,(void*)&msg->cmd,len,"SendMessage");
SocketSendDataNow(&sock->sock,&msg->cmd,len); // Push IT
return msg; // New Number returns
}




/// -- client connection

int smppDoBind(smppSocket *sock,int mode, smppCommand *cmd) { // Process bind operation
int res = ESME_RBINDFAIL,len; // BindFailed
uchar *u,*p,*sysId;
if (sock->onBind && smppDecodeBind(cmd,&u,&p,&sysId)) {
    //CLOG(sock,2,"bindRequest: mode:%d,user:'%s',pass:'%s',sysId:'%s'\n",mode,u,p,sysId);
    res=sock->onBind(sock,mode,u,p,sysId); //
    }
if (res==SMPP_RES_ASYNC) return 0; // Async here
// pack back ...
//printf("BindOK - decode a result %d...\n",res);
len = smppPackBindResp(cmd,mode,res); // pack it
//printf("Push back %d bytes or res %d for handle=%d\n",len,res,sock->sock.sock);
//hex_dump("bind_pack",(void*)cmd,len); cmd->len=HTONL(-1); // ProtocolError!!!
SocketSend(&sock->sock,(void*)cmd,len);
return res;
}

void  smppDoOnMessage( smppSocket *sock, int mode, smppCommand *cmd) { // When a new command here???
int res = ESME_ROK;
CLOG(sock,2,"IncomingMessage:{a:'%s',b:'%s',t:'%s'}",cmd->src_addr,cmd->dst_addr,cmd->text);
//printf("Call On Message :%p, smppTest=%p\n",sock->onNewMessage,onTestSmppMessage);
if (sock->onNewMessage) res = sock->onNewMessage(sock,cmd); // ������ ���
if (res == SMPP_RES_ASYNC) return ;
switch(mode) {
    case 1:    smppPack_SUBMIT_SM_RESP(cmd, res, 0); break; //smppCommand *cmd, int status, char *msg);
    case 2:    smppPack_DELIVER_SM_RESP(cmd, res, 0); break; //smppCommand *cmd, int status, char *msg);
    default:   smppPack_DATA_SM_RESP(cmd, res, 0); break; //smppCommand *cmd, int status, char *msg);
    }
SocketSendDataNow(&sock->sock,cmd->body,cmd->dlen); // Push IT - to send...
//printf("Done, replied...\n");
}

int onSMPPClientPacket(uchar *data,int len,smppSocket *sock) {
smppCommand *cmd = (void*)data,CMD;
smppMsg *msg;
len = smppPacketReady(data,len);
if (len<=0) return len; // Error or not ready
memset(&CMD,0,sizeof(CMD)); // Clear CMD
memcpy(&CMD,data,len); // Len has other???
cmd = &CMD;
cmd->dlen  = len;  // Decode My Length ???
DLOG(sock,6,data,len,"onSMPP_Packet_ready"); // PrintIt (in a file???)
switch(cmd->id) {
case smpp_bind_transceiver:   smppDoBind(sock,3,cmd);  break; // Inout
case smpp_bind_receiver:      smppDoBind(sock,1,cmd);  break; // in only (deliver or submit - depends on server-client???)
case smpp_bind_transmitter:   smppDoBind(sock,2,cmd);  break; // out only

case smpp_bind_receiver_resp:
case smpp_bind_transmitter_resp:
case smpp_bind_transceiver_resp:
    //printf("bind resp: error=%d\n",cmd->status);
    if (cmd->status==0) {
        CLOG(sock,2,"smpp_bind OK");
        sock->sock.connectStatus = connectApp; // Whell
    } else {
        CLOG(sock,1,"smpp_bind ERROR: %x",cmd->status);
        SocketDie(&sock->sock,"smpp: bind error");
        }
    break;
case smpp_submit_sm: // Incoming message ??? ZU? - print on a screen ???
    printf("Submit_SM here - decode\n");
    if (smppDecodeSubmit(cmd)<=0) { // Try Do It & print..
      aborted = 1;
      CLOG(sock,0,"-FATAL smppDecodeDataSm error");
      return 0; // Fail?
      }
    smppDoOnMessage(sock,1,cmd);
    break;
case smpp_deliver_sm: // Incoming message ??? ZU? - print on a screen ???
    printf("Deliver_SM here - decode\n");
    if (smppDecodeDelivr(cmd)<=0) { // Try Do It & print..
      aborted = 1;
      CLOG(sock,0,"-FATAL smppDecodeDataSm error");
      return 0; // Fail?
      }
    smppDoOnMessage(sock,2,cmd);
    break;
case smpp_data_sm: // Incoming message ??? ZU? - print on a screen ???
    printf("Data_SM here - decode\n");
    if (smppDecodeDataSM(cmd)<=0) { // Try Do It & print..
      aborted = 1;
      CLOG(sock,0,"-FATAL smppDecodeDataSm error");
      return 0; // Fail?
      }
    smppDoOnMessage(sock,3,cmd);
    break;
case smpp_submit_sm_resp: // When my message is out OK
    msg = smppMsgFindByNum(sock->out,cmd->num); // Try?
    if (msg) { // Have to notify ...
       CLOG(sock,2,"submit_sm_resp: status:%x,num:%d",cmd->status,cmd->num,msg);
       msg->onChangeState(msg,cmd,sock); // CallIt
       arrDelN((void*)&sock->out, msg-sock->out,1 ); // RemoveByPos
       } else  {
        CLOG(sock,2,"- unknown submit_sm_resp: status:%x,num:%d",cmd->status,cmd->num);
        }
    break;
case smpp_deliver_sm_resp: // When my message is out OK
    msg = smppMsgFindByNum(sock->out,cmd->num); // Try?
    if (msg) { // Have to notify ...
       CLOG(sock,2,"deliver_sm_resp: status:%x,num:%d",cmd->status,cmd->num,msg);
       msg->onChangeState(msg,cmd,sock); // CallIt
       arrDelN((void*)&sock->out, msg-sock->out,1 ); // RemoveByPos
       } else  {
        CLOG(sock,2,"- unknown submit_sm_resp: status:%x,num:%d",cmd->status,cmd->num);
        }
    break;
case smpp_data_sm_resp: // response on a data sm???
   // first - try found it
   msg = smppMsgFindByNum(sock->out,cmd->num); // Try?
   //printf("smpp_data_sm_resp here for msg=%p\n",msg);
   if (msg) {
       CLOG(sock,3,"smpp_data_sm_resp,status:%x,num:%d",cmd->status,cmd->num);
       msg->onChangeState(msg,cmd,sock); // CallIt
       // remove it!!!
       arrDelN((void*)&sock->out, msg-sock->out,1 ); // RemoveByPos
       }
   break;
case smpp_generic_nack:
   CLOG(sock,2,"-!! Nack here, ignore,num:%d, status=%x",cmd->num,cmd->id);
   break;
default:
   DLOG(sock,2,&cmd,cmd->dlen,"-smpp UNKNOWN CMD_ID=%x",cmd->id); // send generic nack???
   }
return len; // Anyway...
}



int smppClientConnect(smppSocket *sm, uchar *host) { // Connect to host (sync)
char buf[80],*u,*p,*sysID,*src_addr;
time_t Started;
smppCommand cmd; int len;
//printf("1\n");
Socket *sock =  &sm->sock; // Already here & "created"
sock->checkPacket = onSMPPClientPacket;

strNcpy(buf,host); host=buf; u=p=sysID = src_addr = 0;
u = strchr(host,'@'); if (u) { host=u+1; *u=0; u=buf; // User
   p = strchr(u,'/'); if (p) { *p=0; p++;} // Password
   } else u="";
//printf("2.5\n");
src_addr = strchr(u,':'); if (src_addr) { *src_addr=0; src_addr++;} else src_addr="";
sysID = strchr(src_addr,':'); if (sysID) { *sysID=0; sysID++;}
if (!sysID) sysID=""; // Undefined
if (!u) u="";
if (!p) p="";
//printf("2\n");
snprintf(sm->name,sizeof(sm->name),"%s@%s",u,host); // UserHost
CLOG(sm,3,"... tcp connecting host:%s",host);
if (!socketConnectNow(sock,host,SMPP_PORT_DEF)) {
    CLOG(sm,1,"-FAIL contact host :%s",host);
    SocketDone(sock);
    return 0;
    }
strNcpy(sm->user,u);
strNcpy(sm->pass,p);
strNcpy(sm->sysID,sysID); // My System ID - copy ???
strNcpy(sm->src_addr,src_addr);
CLOG(sm,3,"... host contacted, bind transeiver {user:'%s',pass:'%s',src_addr:'%s',sysId:'%s'}",u,p,sm->src_addr,sm->sysID);
len = smppPackBind(&cmd,smpp_bind_transceiver,0,u,p,sysID);
if (len<0) {
    CLOG(sm,1,"-FAIL pack bind",host);
    return 0;
    }
SocketSendDataNow(sock,&cmd,len); // Push IT
time(&Started);
while(1) {
    TimeUpdate();
    //time_t now;
    //time(&TimeNow); // Get It
    //now = os_time();
    //printf("SockRun\n");
    RunSleep( SocketRun(sock)) ;
    //printf("SockRun - done\n");
    if (sock->connectStatus==0) {
        CLOG(sm,1,"-Sock disconnected while bind operation");
        SocketDone(sock);
        return 0;
        }
    if (sock->connectStatus == connectApp) break;
    if (TimeNow-Started > SMPP_MAX_BIND ) {
        CLOG(sm,1,"-TimeOut on bind operation %d seconds",SMPP_MAX_BIND);
        SocketDone(sock);
        return 0;
        }
    }
CLOG(sm,3,"smpp session authorized OK");
return 1;
}


/// -- smppConsole

int smppConsole(smppSocket *sock,uchar *buf) { // Text command process on a socket
if (*buf=='m') {
    sscanf(buf+1,"%d",&sock->sendMode);
    printf("+sendMode=%d now\n",sock->sendMode);
    return 1;
    }

if (*buf=='d') {
    sscanf(buf+1,"%d",&sock->logLevel);
    printf("+logLevel=%d now\n",sock->logLevel);
    return 1;
    }
if (memcmp(buf,"sms",3)==0) { // Do It
        uchar *sms = buf+3;
        uchar *phone=0;
        while(*sms && *sms<=32) sms++;
        if (*sms) { // Phone Is Here Word...
            phone = sms;
            while(*sms && *sms>32) sms++;
            if (*sms) {*sms=0; sms++;} // Rest is a text
            while(*sms && *sms<=32) sms++; // ltrim
            }
        //if (!phone || !*phone) phone=sock->addr_to;
        if (!*phone) {
            printf("-syntax> 'sms <phone> [text]'\n");
            return 0;
            }
        //dos2win(sms,sms,-1);
        int n  = 0;
        n = (int)smppSocketSendText(sock,"",phone,sms,onSmppConsoleMessageStatus);
        //printf("SEND TO '%s' TEXT '%s'\n",phone,sms);
        //exit(1);
        //if (!n) n = smsdb_newOut(sock,sock->sysID,phone,sms); // reg it!!! ZU - num_a must be here!!!
        if (!n) {
            CLOG(sock,0,"-FAIL register message");
            } else {
            CLOG(sock,0,"+OK, new message:%d",n);
            }
        }
return 1;
}
