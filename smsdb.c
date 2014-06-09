/*
 ƒл€ работы нужна база данных.
 ¬ базе должны присутствовать таблицы:
  sms - собственно таблица дл€ работы с смсками.

*/

#include "gearSMPP.h"
#include <vdb.h>


db_col *db_colByName(database *db, uchar *name, int nl) {
db_col *c = db->out.cols; int i;
if (nl<0) nl = strlen(name);
for(i=0;i<db->out.count;i++,c++) if (strlen(c->name)==nl && strnicmp(c->name,name,nl)==0) return c;
snprintf(db->error,sizeof(db->error),"colNotFound: %*.*s",nl,nl,name);
return 0; // Not Found
}


int db_table_check(database *db, uchar *tabname, uchar *fld) {
char buf[2048];
vss COLS,NAME,OPT;
if (!tabname || !fld) return 1; // Everything is OK
while(1) {
if (!db_selectf(db,"select * from %s",tabname)) { // New - need to create !!!
    // Ќужно создать предложение create table name ("
    sprintf(buf,"create table %s(%s)",tabname,fld);
    if (!db_exec_once(db,buf)) { // Fail???
        printf("-CREATE TABLE: %s, ON %s\n",db->error,buf);
        db_rollback(db);  return 0; // Fail
        }
    printf("+TABLE CREATED OK>%s\n",buf);
    db_commit(db);  return 1; // OK
    }
COLS = vssCreate(fld,-1);  // Ќужно проверить - вдруг есть новые колонки???
int cnt=0;
while(COLS.len>0) {
    OPT = vssGetTillStr(&COLS,",",1,1);
    vssTrim(&OPT);
    NAME = vssGetWord(&OPT); vssTrim(&NAME); vssTrim(&OPT);
    if (!NAME.len) continue; // ???
    // “еперь - ищем эту колонку внутри текущей
    if (db_colByName(db,NAME.data,NAME.len)) continue; // We Have It
    // Need To Create new column!!!
    snprintf(buf,sizeof(buf),"alter table %s add %*.*s %*.*s",tabname,VSS(NAME),VSS(OPT));
    if (!db_exec_once(db,buf)) { // Fail???
        printf("-ALTER TABLE: %s, ON %s\n",db->error,buf);
        db_rollback(db);
        return 0; // Fail
        }
    printf("+TABLE ALTERED OK>%s\n",buf);
    db_commit(db); // ZU - надо выходить???
    cnt++; break;
    }
if (cnt==0) break; // Everything is done!!!
  }
return 1; // Everything is done???
}

int db_table_check2(database *db, uchar *tabname, uchar *fld,uchar *fld2) { // Check common & other fields???
return db_table_check(db,tabname,fld) && db_table_check(db,tabname,fld2);
}

uchar *gearSYSfld="N integer not null primary key,UP integer default 0,TYP integer default 0,R integer default 0,ORD integer default 0,"
   "NAME varchar(20),R_ACC integer,USR integer,STATUS integer,"
   "ADJUST varchar(80),CREATED date default sysdate,MODIFIED date default sysdate";

int gear_table_check(database *db,uchar *tabname,uchar *fld) {
return db_table_check2(db,tabname,fld,gearSYSfld);
}

//--- SMS db staring ....

int smsdb_check(database *db) {
return gear_table_check(db,"sms",
       "N integer not null primary key,NUM_A varchar(80),DIR integer,NUM_B varchar(80),NAME vacrhar(2048),STATUS integer,MREF integer,PID integer,RD integer,"
       "DCS integer,R_PHONE integer,SENT date,DELIVERED date,UDH varchar(512),F_DT date,T_DT date");
}

int smsdb_new(database *db, char *src_addr, char *dst_addr,char *text, int r_phone,
  int usr, int status, int dir, int data_coding,int mref) {
int smsn,ok;
smsn = db_nextN(db,"sms");
ok = db_compilef(db,"insert into sms(N,UP,R,DIR,MREF,STATUS,CREATED,MODIFIED,TYP,NAME,"
  " DCS, NUM_A,NUM_B,R_PHONE,USR) values(%d,0,1,%d,%d,%d,%s,%s,1,:name,%d,'%s','%s',%d, %d)",
    smsn, dir, mref, status, db_sysdate(db), db_sysdate(db), data_coding, src_addr,
        dst_addr,r_phone, usr)
     && db_bind(db,"name",dbChar,0,text,strlen(text))
     && db_exec(db) && db_commit(db);
//printf("reg SMS=%d code=%d, err=%s, sysdate=%s\n",smsn,ok,db->error,db_sysdate(db));
if (!ok) return 0;
return smsn;
}
