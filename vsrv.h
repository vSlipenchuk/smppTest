#ifndef VSRV_H
#define VSRV_H
#include "vos.h"

#define logf printf

extern char *srv_name;
extern char *srv_dir;

extern int service_mode;



extern int srv_main(int npar,char **par);

extern int    srv_abort;
int    srv_init(char *par);
int    srv_run(int npar,char **par, void *srvMain, void *srvOnStop);

int    srv_start(char *service_name,int time_out);
int    srv_stop(char *service_name);
int    srv_delete(char *service_name);
void   *srv_create(char *service_name,char *service_file,char *dsc);
int    srv_restart(char *name);
int srv_params(int npar,char **par);


extern void *srv_instance;
int wsrv_printf(char *fmt,...);

//int srv_set(int (*on_stop_proc)()); // Set a service add procs...

//Windows section


/*
log_init();
DWORD WINAPI log_run(void* p);
log_create(t_log *s,void *hInst,char *szTitle,int parts,...);
log_puts(t_log *s,int mode,char *buf);
log_putsf(t_log *s,int mode,char *fmt,...);
log_clear(t_log *s);

#define log_font(A,NAME,SIZE) SendMessage((A)->hMemo,WM_SETFONT,(int)CreateFont(SIZE,0,0,0,0,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,0,0,FIXED_PITCH,NAME),1)
*/



#endif
