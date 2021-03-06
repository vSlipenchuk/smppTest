/*

������� �������� - ������ ��� ������������������ ������.
����� �������:
 1. � ���������� �������� ������� ��������� � ��������� ����������;
 2. ��� �������� ���������� ���������� �� �����;

*/

#include <stdio.h>
//#include <conio.h>
#include "gearSMPP.h"

#ifdef smppTestMain
#include "vs0.c"
#include "vss.c"
#include "sock.c"
#include "std_sock.c"
#include "coders.c"
#include "vdb.c" // for scanUINT!!!
#include "ini.c"
#include "vos.c"
//#include "vdb2.c"
#include "logger.c"
#include "exe.c"

//#include "httpSrv.c"
#include "smpp.c"
#include "smppSrv.c"
//#include "../util/httpTest.c"
#include "strutil.c"
#include "vos_linux_kbhit.c"

#endif

#ifdef WIN32
unsigned long os_ticks() { return GetTickCount();}
#endif

int testReplayMode = 1; // 1 - just accept & print, 0 - do Nothing, 2 -replay with message, -1 decline???


void *gets_buf2(char *buf,int size) {
gets(buf);
}

Counter TestReadCounter = {1000000}; // PPS

int onSmppConsoleMessageStatus(smppMsg *msg, smppCommand *cmd, smppSocket *sock) { // When status changes
if (cmd->status==0) { // OK, out message sent!!!
    printf(" >> + message#%d accepted by server cmdnum=%x\n",msg->num,cmd->num);
    } else {
    printf(" >> -FAIL message#%d declined with status=%x,cmdnum=%x\n",msg->num,cmd->status,cmd->num);
    }
onSmppMessageStatus(msg,cmd,sock); // Remove it from a queue
return 1; // OK
}


// ���, ������ �� ����� ���� ����������... �������� - ������???



int CounterPPS(Counter *c) { return (c->pValue+c->ppValue)/2; }

int smppConsoleClient2(char *host, void *onNewMessage,char **cmd, int len) { // Create a client connection & run it???
char szHost[100];
smppSocket *sm = smppSocketNew();
Socket *sock =  &sm->sock; // Already here & "created"
sock->parent = 0 ; // No Parent -)))
sm->logLevel = 5;
strNcpy(szHost,host); host=szHost;
if (!sm->log) sm->log = logOpen("smppTest.log");
static SocketPool TMP_POOL; TMP_POOL.log = sm->log; TMP_POOL.logLevel = sm->logLevel;
sm->sock.pool = &TMP_POOL;
sm->onNewMessage = onNewMessage;
CLOG(sm,1,"...smppClient: connecting <%s> onMessage=%p",host,onNewMessage);
//printf("BeginConnect\n");
sm->sendMode=1; // submit
smppClientConnect(sm,host); // Try Connect -)))
//printf("Resulted connect\n");
//CLOG(sm,1,"...2smppClient: connecting <%s>",host);
if (sock->connectStatus!=connectApp) {
    CLOG(sm,1,"-smppClient: connect failed");
    smppSocketClear(&sm);
    return 0;
    }
printf("DoneConnect...\n");
printf("+smppClient connected '%s'. Type a command for:\n"
       " send message: 'sms <phone> text'\n"
       " send binary : 'bin <phone> hexstring\n"
       " set send_sm command (submit|delivr|data_sm): 'm<1|2|3>'"
       " exit program: 'exit'\n",host);
sock->readPacket = &TestReadCounter;
//printf("!!! -- !!! OK - auth done - can do everything???\n");
// ������ ������ ����� ����� ��� -- ����� �������� � �������� ��������� � ����� ������ - ���� ������?

objAddRef(sock); // ! When disconnected - socket will be destroyed!!!

while(sock->state) { // While Iam connected make a loop
    char buf[280];
    TimeUpdate();
    //printf(">>> ---------- SRUN\n");
    if (NeedReport()) Reportf("PacketPerSecond = %d Time:%s, pVal=%d",CounterPPS(&TestReadCounter),szTimeNow,TestReadCounter.pValue);
    RunSleep( SocketRun(sock));

    //printf(">>> -----------SDONE\n");

    if (len>0) {
       strNcpy(buf,cmd[0]); cmd++;
       len--;
       } else  {
        if (!kbhit()) continue;
        gets_buf2(buf,sizeof(buf));
       }
//    gets(buf);
    if (!buf[0] || strcmp(buf,"exit")==0) break; // Aborted ???
    //if (memcmp(buf,"db",3)==0) {        gearSMPPcheckDb(sm);        }
    smppConsole((void*)sock, buf);
    // mk sms & send???
    }
CLOG(sm,1,"+smppClient: done");
return 0;
}


int smppConsoleClient(char *host,void *onNewMessage) {
return smppConsoleClient2(host,onNewMessage,0,0);
}

void smppTestHelp() {
printf("Usage:\n"
       " [smppClient]:  smppTest.exe <[smppp://][user[#sysId]/pass@]host[:port]> [commands]\n"
       " [smppSevrver]: smppTest.exe -p[port] -r[replayMode:1] -C[runClient:0]\n"
       );
}

int onTestSmppBind(smppSocket *sock,int mode,uchar *u,uchar *p,uchar *sysId) { // Always bind???
CLOG(sock,2,"bindRequest: mode:%d,user:'%s',pass:'%s',sysId:'%s'\n",mode,u,p,sysId);
 strNcpy(sock->user,u);  strNcpy(sock->pass,p);  strNcpy(sock->sysID,sysId);  // Copy OK
return ESME_ROK; // OK?
}


int onSmppConsoleNewMessage(smppSocket *smpp,smppCommand *cmd) { // We have a command???
printf(" (ESME_ROK) console message accepted {a:'%s',b:'%s',t:'%s'}\n",cmd->src_addr,cmd->dst_addr,cmd->text);
return ESME_ROK;
}


int onTestSmppMessage(smppSocket *smpp,smppCommand *cmd) { // We have a command???
smppMsg *msg;
printf(" (ESME_ROK) server message accepted {a:'%s',b:'%s',t:'%s'} replay:%d msgid:'%s'\n",cmd->src_addr,cmd->dst_addr,cmd->text,
 testReplayMode,cmd->msgid);
switch(testReplayMode) {
    case -1: return ESME_RSYSERR; // SystemError
    case 0:  return SMPP_RES_ASYNC; // Do Nothing - async
    case 1:  return ESME_ROK; // OK, accept & print
    case 2:
             // Push a new message back with a same code
             printf("Server is replaing to Message....\n");
             msg = smppSocketSendText(smpp,cmd->dst_addr,cmd->src_addr,cmd->text,onSmppConsoleMessageStatus); // Ret It Back
             printf("Replay Message posted: %d\n",msg->num);
             return ESME_ROK;
    case 3: // Replay By UserID
           {
           smppSrv *srv = smpp->sock.pool; // HereIs Server pool
           smppSocket *dst=0; int i;
           for(i=0;i<arrLength(srv->srv.sock);i++) {
               dst = (smppSocket*)srv->srv.sock[i];
               if (strcmp(dst->user,cmd->dst_addr)==0) break;
               }
           if (i==arrLength(srv->srv.sock)) return ESME_RINVDSTADR; // Invalid Addr
           msg = smppSocketSendText(dst,cmd->src_addr,cmd->dst_addr,cmd->text,onSmppConsoleMessageStatus); // Ret It Back
           printf("Forward Message posted: %d\n",msg->num);
           return ESME_ROK; // OK message here...
           }
    }
return ESME_RSYSERR; // Some Error on it
}



int smppConsoleTestClient(char *host) { // for thread_create
return smppConsoleClient(host, onTestSmppMessage );
}

int smppTestServer(int npar,char **par) {
int port = SMPP_PORT_DEF;
 int i,logLevel=2,CLIENT=0;
for(i=1;i<npar;i++) {
    uchar *cmd = par[i];
    if (*cmd!='-') continue;
    cmd++;
    if (*cmd=='p') sscanf(cmd+1,"%d",&port);
     else if (*cmd=='d') sscanf(cmd+1,"%d",&logLevel);
       else if (*cmd=='r') sscanf(cmd+1,"%d",&testReplayMode);
         else if (*cmd=='C') sscanf(cmd+1,"%d",&CLIENT);

    }
smppSrv *smpp = smppSrvCreate(port,onTestSmppBind,onTestSmppMessage);
if (!smpp) {
    printf("FailStart smppServer on port:%d\n",port); return 1; // ERROR
    }
smpp->master->log = logOpen("smppTest.log"); // Here me logger
smpp->master->logLevel = logLevel; //def loglevel???

smpp->srv.log = smpp->master->log;
smpp->srv.logLevel = smpp->master->logLevel;
smpp->master->onNewMessage = onTestSmppMessage; // When new message here -???
smpp->master->sock.readPacket = &TestReadCounter;

Logf("smppServer started on port:%d,replayMode:%d",port,testReplayMode);
if (CLIENT) { // RunClient here
   printf("Creating console client for this server...\n");
   static char szHost[80]; sprintf(szHost,"user/pass@localhost:%d",port);
      thread_create( smppConsoleTestClient, szHost);
   }
   //sleep(5);
//printf("RunMe...\n");
while(!aborted) { // Do A Job on every socket???
    int cnt=0;
    TimeUpdate(); // Sets TimeNow
    for(i=0;i<arrLength(smpp->srv.sock);i++) {
    smppSocket *Sock = (void*)smpp->srv.sock[i];
    //printf("Run = %d..\n",i);
    cnt+=SocketRun(&Sock->sock);  // ��� ������ - ������� smppSocket???
    if (NeedReport()) Reportf("PacketPerSecond = %d Time:%s, pVal=%d",CounterPPS(&TestReadCounter),szTimeNow,TestReadCounter.pValue);
    //if (Sock->gear) cnt+=httpClientRun(Sock->gear);
    }
    if (cnt==0) msleep(10);
    //RunSleep(cnt); // If Nothing here???
  }
// Whell - we need some job???
Logf("smppServer done...\n");
return 0; // OK
}

int smppTestMain(int npar,char **par) { //
uchar *host=0;
int i;
net_init();
if (npar<2) { smppTestHelp(); return 1;} // Error
if (strncmp(par[1],"-p",2)==0) return smppTestServer(npar,par);
host = par[1]; if (strncmp(host,"smpp://",7)==0) host+=7;
// - do a parameters (logLevel???)
for(i=2;i<npar;i++) {
    uchar *cmd = par[i];
    if (!*cmd=='-') continue;
    cmd++;
    if (*cmd=='r') sscanf(cmd+1,"%d",&testReplayMode); // MyReplayMode???
    }
printf("CreateConsole, host:'%s',replayMode:%d\n",host,testReplayMode);
smppConsoleClient2(host,onTestSmppMessage,par+2,npar-2); // Run as console
return 0; // OK
}

