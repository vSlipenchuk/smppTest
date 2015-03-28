#include "gearSMPP.h"
#include "smpp.h"
#include <stdio.h>
//#include <conio.h>




// client functions???



int gearSMPPcheckDb(smppSocket *smpp) { // Счас быстренько разберемся в БД???
//database *db = smpp->db;
// check a new messages (if any???) for delivery, except thouse we already have in a deliver ???
return 0;
}



/*
  -- NCC - 195.98.63.23:4834
       User: smticc
       Pass: smticc
       Sme: 9888

       smticc#9888/smticc@195.98.63.23:4834
*/

// Главная структура - специфика работы с БД ???

VS0OBJ0(gearSMPP,0);

gearSMPP *Main; // Main SMPP service

#ifdef SMPPDB
int smsdb_newOut(smppSocket *smpp, uchar *src_addr,uchar *dst_addr,uchar *text) { // reg it!!!
gearSMPP *Main = smpp->parent;
database *db = Main->db;
return smsdb_new(db,src_addr,dst_addr,text, Main->phone,Main->usr,1,2,0,-1); // JustReg
}
#endif

typedef struct { // Структурка
    uchar num_a[40];
    uchar num_b[40];
    } gearSmppCmd;


int emptyCall() {return 0;}

int gearOnSmppReply(httpCmd *cmd) { // Вызывается, когда гир окончил работу по реальной команде...
httpClient *cli = cmd->cli;
vss txt; gearSmppCmd *c = (void*)&cmd->buf;
smppSocket *sock = cmd->handle; // Мой сокет...
char buf[512];
int code=0;
printf("onSmppReply res=%d cmd=%p cli=%p sock=%p\n",cmd->res_code,cmd,cli,sock); //VSS(cli->res.U));
if (cli->res.U.len>0) code=atoi(cli->res.U.data); // GetACode
if (code>=400 && code<500) {
    CLOG(cli,0,"-GEAR - AuthFailed, expect 200 OK",code);
    aborted=1;
    return 0;
    }
if (code!=200) {
    CLOG(cli,0,"-GEAR - WrongAnswer: %d, expect 200 OK",code);
    return 0;
    }
txt = cli->res.B; vssTrim(&txt);
if (txt.len>140) txt.len=140; strncpy(buf,txt.data,txt.len); buf[txt.len]=0;
//printf("?\n");
printf("gear returns: '%s', send SMS back (from=%s to %s)\n",buf,c->num_b,c->num_a);
//exit(999); // Пока так-)))
if (!buf[0]) return 0;
smppSocketSendText(sock,c->num_b,c->num_a,buf,emptyCall); // Try to push back (whell-happy???)
printf("DoneSend???\n");
return 0;
}

int onGearDisconnect() {
Logf("FATAL - gearConnection broked. Abort");
aborted=1;
return 0;
}

int onSmeDie(smppSocket *sock) {
Logf("FATAL - sme '%s' died. Abort",sock->host);
aborted=1;
return 0;
}


//url_decode

int url_encode(uchar *dst,uchar *src,int len) { // OK?
int cnt=0;
if (len<0) len = strlen(src);
while(len>0) { // JUST COPY
    int i;
    for(i=0;i<len && src[i]>32 && src[i]<=128 && !strchr("+&=?# '\"",src[i]);i++); // no spec...
    if (i>0) { if (dst) {memcpy(dst,src,i); dst+=i;} cnt+=i; src+=i; len-=i; continue;} // Flash them
    if (dst) {sprintf(dst,"%c%01X%01X",'%',*src>>4,*src&0xF); dst+=3; } // CodeIt
    src++; len--; cnt+=3;
    }
if (dst) *dst=0;
return cnt;
}

void urlcat(uchar *str,uchar *name,uchar *val) { // ZU!
strcat(str,"&"); // RestOf
strcat(str,name); strcat(str,"=");
url_encode(str+strlen(str),val,-1); // Adds a data
}

int gearOnSmpp(httpClient *gear,smppSocket *sock,uchar *num_a,uchar *num_b,uchar *txt) {
httpCmd *cmd; uchar URL[1024];
gearSmppCmd *c;
//printf("gearOnSmpp -- started na=%s,nb=%s,txt=%s...\n",num_a,num_b,txt); // Whell
// Нужно уметь закодить параметры HTML
URL[0]=0;
urlcat(URL,"num_a",num_a); urlcat(URL,"num_b",num_b); urlcat(URL,"txt",txt); // CopyIt !!!
cmd = httpClientGet(gear,URL);
cmd->onDone = gearOnSmppReply; // When commands done???
cmd->handle = sock;  // Запомнили текущий сокет?
c = (void*)&cmd->buf[0];
strNcpy(c->num_a,num_a);
strNcpy(c->num_b,num_b); // CopyThem back...
//printf("Done gearOnSmpp cmd=%p cli=%p cmd->cli=%p\n",cmd,gear),cmd->cli;
return 0;
}


int smppSMS2DB(smppSocket *smpp,smppCommand *cmd) { // We have a command???
gearSMPP *Main = smpp->parent;
httpClient *gear = smpp->gear;
if (gear) { // if we have a gear -- CALL it!!!
     gearOnSmpp(gear,smpp,cmd->src_addr,cmd->dst_addr,cmd->text); // Do It
     return ESME_ROK; // Отсылаем сразу ОК? - некозяво надо бы подождать ответ чоль?
     }
if (!Main->db) {
    CLOG(smpp,3,"+SMS2DB  no database defined");
    return 1; // ERROR?
    }
int smsn =
   smsdb_new(Main->db,cmd->src_addr,cmd->dst_addr,cmd->text, Main->phone, Main->usr,
       -1, 1, // recv incoming SMS
       cmd->data_coding,cmd->sm_default_msg_id);
if (!smsn) { CLOG(smpp,3,"-SMS2DB regIN error: %s",Main->db->error);  return ESME_RSYSERR; }
CLOG(smpp,3,"+SMS2DB  regIN#%d a:'%s',b:'%s',text:'%s'",smsn, cmd->src_addr,cmd->dst_addr, cmd->text);
return ESME_ROK;
}

int smsDbSetStatus(database *db, int n, int status) { // Update Status In A DB
int ok;
ok = db_compilef(db,"update sms set status=%d,modified=%s,r=r+1 where n=%d",
     status,db_sysdate(db),n) && db_exec(db) && db_commit(db);
if (!ok) {
    //printf("smsDbSet Status Failed %s\n",db->error);
    aborted = 1;
    return 0;
    }
return 1; // OK
}

int onDbSentStatus(smppMsg *msg, smppCommand *cmd, smppSocket *sock) { // When status changes
int idx;
gearSMPP *Main = sock->parent;
database *db = Main->db;
//printf("onDbMessageStatus: message %d -> changed state, new status=%x\n",msg->cmd.num,cmd->status);
if (cmd->status==0) { // OK, out message sent!!!
    //printf("DbMessage Sent OK, remove from a queue...\n");
    smsDbSetStatus(db,msg->N, -1); // Send OK
    } else {
    //printf("DbMessage SEND FAILED, remove anyway...\n");
    smsDbSetStatus(db,msg->N, -2); // Send Error
    }
// do a remove???
idx = msg-sock->out;
//printf("Remove Index %d, len=%d\n",idx,arrLength(sock->in));
arrDelN((void**)&sock->out,idx,1); // Remove???
//printf("Index removed, newlen=%d\n",arrLength(sock->in));
return 1; // OK
}



int smppDB2SMS(smppSocket *sock) { // Check New SMS in a database
gearSMPP *srv  = sock->parent;
//httpClient *gear = sock->gear;
database *db = srv->db;
int cnt = 0, ok, cnt_now;
cnt_now = arrLength(sock->out);// Это у меня на текущем сокете или где???
if (cnt_now>=MAX_SMS_OUT) return 0; // No More -)))
if (!db) return 0;
CLOG(sock,5,"check new messages in a db for phone:%d,maxN:%d",srv->phone,srv->outN);
ok = db_selectf(db,"select N,NUM_A,NUM_B,NAME from sms where r_phone=%d and status=1 "
 " and N>%d and dir=2 ", srv->phone, srv->outN);
if (!ok) {
    CLOG(sock,5,"-FATAL db_check_new: %s",db->error);
    aborted = 1;
    return 0;
    }
while(db_fetch(db) && cnt+cnt_now<MAX_SMS_OUT) { // Process Them
    smppMsg *msg;
    db_col *c = db->out.cols;
    int n = db_int(c); c++;
    uchar *na = db_text(c); c++;
    uchar *nb = db_text(c); c++;
    uchar *text = db_text(c); c++;
    if (!*na) na=sock->sysID; // Default - gets from a socket???
    msg = smppSocketSendText(sock,na,nb,text,onDbSentStatus); // ZU - if no OK???
    msg->N = n; // For updater...
    if (n>srv->outN) srv->outN = n; // Fix Max N
    cnt++;
    }
srv->outChecked = TimeNow;
if (cnt==0 && cnt_now) { // Fix It
    srv->outN = 0; // Start with again???
    return 0;
    }
// ok - now do smth with it???
return 1; // OK
}

int gearSMPP_ini(gearSMPP *main, uchar *strINI) { // Config from a ini-file
vss INI = vssCreate(strINI,-1),m,s;
httpClient *gear = 0; // SomeGearConnection???
uchar buf[1024];
if (!iniFindSection(INI,"main",&m)) return 0; // No Main Here
if (!main->log) main->log = logOpen("gearSMPP.log"); // Opens a file
strcpy(main->name,"gear"); // SetLogName
log2stdout = iniFindIntDef(m,"log2stdout",log2stdout); // Печать лога на экран?
main->logLevel = iniFindIntDef(m,"logLevel",1); // DefaultLogLevel
main->usr = iniFindIntDef(m,"usr",0); // MainUser
main->phone = iniFindIntDef(m,"phone",0); // MainPhone
if (iniFindStringBuf(INI,"database",buf,sizeof(buf))) { // We have a db???
    CLOG(main,3,"...try db_connect :%s",buf);
    main->db = db_new();
    if (!db_connect_string(main->db,buf)) {
        CLOG(main,0,"-Failed dbconnect :%s\n",main->db->error);
        return 0; // ERROR!!!
        }
    db_config_db(main->db); // Confiure NextN from a db
    if (!smsdb_check(main->db)) { // FATAL!!!
        CLOG(main,0,"-Failed check smsdb:%s\n",main->db->error);
        return 0;
        }
    // ZU - db_config???
    CLOG(main,3,"+smsdb connected & configured OK");
    }
if (iniFindSection(INI,"gear",&s)) { // Server SME here ->>>
    //int port;
    gear = httpClientNew();
    if (!iniFindStringBuf(s,"cs",buf,sizeof(buf)))
     strcpy(buf,"smpp/smpp@localhost:81/cls/call?cls=sms&fun=onSmppMessage?");
    gear->log = main->log;
    gear->logLevel = iniFindIntDef(s,"logLevel",main->logLevel); // DefaultLogLevel
    CLOG(main,3,"...try connect gear '%s'",buf);
    if (httpClientInit(gear,buf,0)<=0) { // Fail connect to gear ???
        CLOG(main,0,"-FAIL connect gear '%s'",buf);
        }
    CLOG(main,1,"...gear connected OK");
    gear->onDisconnect = onGearDisconnect; // JustAbort it...
    }
if (iniFindStringBuf(INI,"sme",buf,sizeof(buf))) { // Client SME here!!!
    CLOG(main,3,"...try connect client SME:'%s'",buf);
    smppSocket *smpp;
    smpp =  smppSocketNew();
    smpp->log = main->log; smpp->logLevel = main->logLevel; // CopyLogInfo
    if (smppClientConnect(smpp,buf)<=0) return 0; // Error already reported
    smpp->onNewMessage = smppSMS2DB; // Register a new messages in a database
    smpp->sock.onDie = onSmeDie;
    smpp->parent = Main; // Remember a parent -)))
    Socket2Pool((void*)smpp,&main->srv.srv); // AddToSocket Pool
    smpp->gear =gear;
    }
if (iniFindSection(INI,"server",&s)) { // Server SME here ->>>
    int port;
    port = iniFindIntDef(s,"port",SMPP_PORT_DEF); // MainPort
    CLOG(main,3,"...try start server on port:%d",port);
    smppSocket *smpp;
    smpp =  smppSocketNew();
    sprintf(smpp->name,"SMPP#%d",port);
    smpp->log = main->log; smpp->logLevel = main->logLevel; // CopyLogInfo
    //smpp->phone iniFindIntDef(s,"phone",7789); // Нужно уметь заменять phone для этого соединения !!!
    if (!SocketListen(&smpp->sock,port)) {
        CLOG(main,1,"-FAIL Listen socket smpp-port %d",port);
        return 0;
        }
    iniFindStringBuf(INI,"user",smpp->user,sizeof(smpp->user));
    iniFindStringBuf(INI,"pass",smpp->pass,sizeof(smpp->pass));
    CLOG(main,1,"+SMPP server started, port:%d,user:'%s',pass:'%s'",port,smpp->user,smpp->pass);
    smpp->onNewMessage = smppSMS2DB; // Register a new messages in a database
    smpp->parent = Main; // Remember a parent -)))
    Socket2Pool((void*)smpp,&main->srv.srv); // AddToSocket Pool - as server....
    }
CLOG(main,1,"smpp ini done OK");
return 1; // OK ???
}


int gearSMPP_run(gearSMPP *srv) {
smppSrv *smpp = &srv->srv;
int i,cnt=0;
TimeUpdate(); // Sets TimeNow
for(i=0;i<arrLength(smpp->srv.sock);i++) {
    smppSocket *Sock = (void*)smpp->srv.sock[i];
    cnt+=SocketRun(&Sock->sock);  // Все сокеты - реально smppSocket???
    if (Sock->gear) cnt+=httpClientRun(Sock->gear);
    }
if (TimeNow > srv->outChecked + SMS_OUT_CHECK) { // Have To Do???
    for(i=0;i<arrLength(smpp->srv.sock);i++) {
      smppSocket *Sock = (void*)smpp->srv.sock[i];
      smppDB2SMS(Sock); // Need To Do...
      //cnt+=SocketRun(&Sock->sock);  // Все сокеты - реально smppSocket???
      }
    }
if (cnt) return cnt; // OK -???
if (kbhit()) {
    char buf[1024];
    gets(buf);
    if (buf[0]==0) { aborted=1; return 1; }  // Done!!!
    smppConsole((void*)smpp->srv.sock[0],buf); // Run On First socket...
    }
return 0; // Done...
}

int gearSMPPmain(int npar,char **par) { // First - init global SMPP server...

return smppTestMain(npar,par) ; // ZUZUKA

    Main = gearSMPPNew(); // Create a new service -)))
    uchar *INI;
    if (npar>2 && strcmp(par[1],"-c")==0) { // Spec - a client ???
        return smppConsoleClient(par[2],onTestSmppMessage);
        //onSmppConsoleNewMessage);
        }
    sleepTime = 10;
    INI = strLoad("gearSMPP.ini"); // Try Load Ini File
    if (INI) { // get [main] section
        if (gearSMPP_ini(Main,INI)<=0) return 0; // Stop - fail start INI
        CLOG(Main,0,"ini Found & loaded - process messages, <Enter> to abort");
        while(!aborted) RunSleep( gearSMPP_run(Main) ); // Loop It
        CLOG(Main,0,"service shutdown, aborted:%d",aborted);
        return 0;
        } ; //else alertf("NoIniFound in dir='%s'!");
    return 0;
}





