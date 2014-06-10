#ifndef GEAR_SMPP
#define GEAR_SMPP

#include <vs0.h>
//#include <vdb.h> // Database
#include <vss.h>
#include <logger.h>
#include "vdb.h"
#include "sock.h" // Socket - service functions
#include "smpp.h" // SMPP 5.0 & 3.4 declarations
#include "httpSrv.h" // Для доступа к гиру???

#define MAX_SMS_OUT 10  /* Maximnum SMS in out buffer (send &&&) */
#define SMS_OUT_CHECK 5 /* Через какое время проверять БД на предмет новых СМС*/

#define SMPP_MAX_BIND 30 /* Максимальное время ответа на bind */

typedef struct { // Мессаджи - лежат в моем пуле - их надо кудато-девать???
    time_t modified; // When this data received or sent
    int num; // External Num (in-out reference)
    smppCommand cmd; // Кодированное сообщение
    int (*onChangeState)(); // Callback notification (if any)
    int N; void *handle; // Some Handlers here -)))
    } smppMsg;


typedef struct {
    SocketPool srv; // Тут все сокеты - коннектор и подсоединения
    int port;
    struct _smppSocket *master; // Обычно - это первый сокет в пуле
    int  enquireLink;    //  Через сколько запрашивать проверку связи (0 - не запрашивать)
    } smppSrv;

VS0OBJH(smppSrv); // Запуск сервиса -)))

smppSrv *smppSrvCreate(int port, void *onBind, void *onMessage) ; // Инициализация и запуск сервера



typedef struct { // SMPP сервис - клиентский (с БД) или без???
    smppSrv srv; // Прокатит - прокатило ???
    char name[80];     int logLevel;     logger *log; // for logging
//    database *db; // Прямое соединение с базой
    smppCommand cmd; // Last Decoded Incoming message (buffer)
    //smppSrv  *srv; // Список серверов обслуживания (т.е. - се рвисных пулов)
    smppMsg **in,**out; // Очереди сообщений - отправляемые и принимаемые
    int usr,phone; // Main User & Main Phone for run with gear tables (sms, phones)
    int outN; // Last N accessed from a db for this phone -> dir=out
    time_t outChecked; // Time than last out sms checked ...
    database *db;
    }  gearSMPP;

#define dcsAccept4 4 /* if present - send win1251 text to it */

typedef struct _smppSocket { // ShortMessageEntity - definitions here
    Socket sock; // Simple TCP Socket
    int num; // reference number for out messages (starts with zero)
    int bindMode,dcsMode; // Are we a bound to smth or not ???
    char name[80]; int logLevel; logger *log; // MyName = user@host
    char host[80],user[80],pass[80],sysID[80]; // AuthInfo here
    char src_addr[80],addr_to[80]; // default sender & recv address
    smppMsg *in,*out; // Queue for incomming & outgoing messages
//    database *db; // Личная БД для данного сокета ...
    int (*onBind)(); // Вызывается при bind. Нужно только в сервере???
    int (*onNewMessage)(); // Возвращает код ошибки (ESME_OK если все ОК) - пакуется в ответ
    void *parent; // Just ref to geearSMPP???
    int maxN; // Last N for a send (db process)
    int sendMode; // 1 - submit, 2 - delivr, 3 data_sm
    httpClient *gear; // Соединение с гиром - если есть - то оно!!!
    } smppSocket;

VS0OBJH(smppMsg);
VS0OBJH(smppSocket);

int smppClientConnect(smppSocket *sm, uchar *host);
int smppConsoleClient(char *host, void *onNewMessage) ; // Test Console here
int onTestSmppMessage(smppSocket *smpp,smppCommand *cmd) ; // We have a command???


int onSmppMessageStatus(smppMsg *msg, smppCommand *cmd, smppSocket *sock);  // Default Handler - print & removes
int onSmppConsoleMessageStatus(smppMsg *msg, smppCommand *cmd, smppSocket *sock) ; // PrintResult

int gearSMPPmain(int npar,char **par); // Main Function, defined in gearSMPP

smppMsg *smppSocketSendBin(smppSocket *sock,char *from,char *to,int esm, int pid, int dcs, uchar *buf, int len, void *onMessage);
smppMsg *smppSocketSendText(smppSocket *sock,char *from,char *to,uchar *data, void *onMessage);



 int smppDB2SMS(smppSocket *sock) ; // Проверяет новые СМС по БД

// Utils
smppMsg* smppMsgFindByNum(smppMsg *msg,int num) ; // Find by a number in array...

int smppOnBind(Socket *sock,int mode, smppCommand *cmd,smppSrv *srv);
int smppConsole(smppSocket *sock,uchar *buf) ; // Text command process on a socket

#ifdef SMPPDB
// Работа с базой данных
int smsdb_check(database *db);
int smsdb_new(database *db, char *src_addr, char *dst_addr,char *text, int r_phone,
  int usr, int status, int dir, int data_coding,int mref);

#endif

int smppTestMain(int npar,char **par); // Тестер (клиент и сервер)
int smppTestServer(int npar,char **par); // Сервер + клиент???

int onSMPPClientPacket(uchar *data,int len,smppSocket *sock); // Default SMPP message checker-handler...


#endif // GEAR_SMPP
