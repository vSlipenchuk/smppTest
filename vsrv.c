
#include <stdio.h>
#include <stdlib.h>

#include "vtypes.h"
#include "logger.h"
#include "io.h"
#include "vsrv.h"


char *srv_name=NULL;
char *srv_dir=".";
char *srv_startDir=0; // Start Directory
char *srv_file=NULL;
void *srv_instance;
int service_mode = 0; // not a service&&&

int (*on_stop)() = (void*) sig_done; // default StopService Handler...

SERVICE_TABLE_ENTRY   srv_dispatch_table[2];
SERVICE_STATUS        srv_status;
HANDLE                srv_main_thread;
SERVICE_STATUS_HANDLE srv_hstatus;

int srv_init(char *cmd) {
  int i;
  char *p;
  //printf("Start init with cmd='%s'\n");
  //srv_instance=LoadLibrary(cmd);
  srv_startDir = malloc(MAX_PATH+1);
  GetCurrentDirectory(MAX_PATH+1,srv_startDir); // StartedDirectory
  for(i=0,p=0; cmd[i]; i++)  if(cmd[i]=='\\' || cmd[i]=='/') p=cmd+i;
  if(!p) {
    srv_dir=malloc(MAX_PATH+1);
    GetCurrentDirectory(MAX_PATH+1,srv_dir);
    }
  else   {    *p=0;     srv_dir=cmd;     cmd=p+1;  } // GetFrom a here -)))
  for(i=0,p=0;cmd[i];i++) if (cmd[i]=='.') p=cmd+i;
  if(p) *p=0;
  if(!srv_name) srv_name=cmd;
  srv_file=cmd;
  printf("srv_dir:'%s',srv_file:'%s',srv_name:'%s'\n",srv_dir,srv_file,srv_name);
  return 1;
}

/*
  Проходит по пути и создает или изменяет строковый ключ
*/

int reg_write_str(void *root,uchar *path,uchar *val) { // Вытаскивает если надо столько сколько надо
int lev=0;
//printf("reg_key_open: root=%p path='%s'\n",root,path);
if (!root) { // Проверяем начало на HKLM
    if (strncmp(path,"/HKLM/",6)==0) {root=HKEY_LOCAL_MACHINE; path+=6;}
    else if (strncmp(path,"/HKUSR/",7)==0) {root=HKEY_USERS; path+=7;}
     else root=HKEY_LOCAL_MACHINE; // by default
    }
// Now - check - if we have other '/'?
while(1) { // PathByPath
uchar *name;
while(*path=='/') path++;  // Remove a '/'
name = strchr(path,'/'); if (!name) break;
int ipos = name-path; // length of a name
    uchar szPath[128]; // new name
    HKEY res; DWORD dw;
    strNcpy(szPath,path); if (ipos>120) ipos=120; szPath[ipos]=0; // Copy It
    //printf("OpenKey, root=%p path='%s', rest='%s'\n",root,szPath,name);
    if (RegCreateKeyEx(root,szPath,0,0,REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS ,0,&res,&dw)!=ERROR_SUCCESS) {
        if (lev) RegCloseKey(root); return 0;  }
    if (lev) RegCloseKey(root); // ClosePrev Key
    root = res; lev++; path=name+1;
}
//printf("Yes Name='%s', lev=%d!\n",path,lev);
int ok = RegSetValueEx(root,path,0,REG_SZ,val,strlen(val));
if (lev) RegCloseKey(root);
return ok == ERROR_SUCCESS;
}

int srv_set_dsc(char *name,char *dsc) {
char path[1024]; if (!dsc) dsc="";
if (!name) name=srv_name;
sprintf(path,"/SYSTEM/CurrentControlSet/Services/%s/Description",name);
return reg_write_str(0,path,dsc);
}

void *srv_create(char *service_name,char *service_file,char *dsc) {
  void *serv_manager;
  serv_manager=OpenSCManager(0,0,SC_MANAGER_CREATE_SERVICE|SC_MANAGER_CONNECT);
  if(!serv_manager) return 0;
  if (!CreateService(serv_manager,service_name,service_name,
    SERVICE_START|SERVICE_STOP|GENERIC_EXECUTE,
    SERVICE_WIN32_OWN_PROCESS|SERVICE_INTERACTIVE_PROCESS,
    SERVICE_DEMAND_START,
    SERVICE_ERROR_IGNORE,
    service_file,0,0,0,0,0)) return 0;
  if (dsc && *dsc) srv_set_dsc(service_name,dsc);
  // how to set DSC???
  return (void*)1; //OK
}


int srv_delete(char *service_name) {
  void *serv_manager,*serv_handle;
  serv_manager=OpenSCManager(0,0,SC_MANAGER_CREATE_SERVICE|SC_MANAGER_CONNECT);
  if(!serv_manager) return 0;
  serv_handle=OpenService(serv_manager,service_name,DELETE);
  if(!serv_handle) return 0;
  if(!DeleteService(serv_handle)) return 0;
  return 1;
}


int srv_stop(char *service_name) {
  void *serv_manager,*serv_handle;
  if (service_name==0) {
      if (srv_name && srv_name[0]) service_name = srv_name;
	  else return 0;
  }
  serv_manager=OpenSCManager(0,0,SC_MANAGER_ALL_ACCESS|SC_MANAGER_CONNECT);
  if(!serv_manager) return 1;
  serv_handle=OpenService(serv_manager,service_name,SERVICE_STOP);
  if(!serv_handle) return 2;
  if(ControlService(serv_handle,SERVICE_CONTROL_STOP,&srv_status)==0) return 3;
  return 0;
}

int srv_start(char *service_name, int time_out) {
  SERVICE_STATUS SS;
  int i;
  void *serv_manager,*serv_handle;
  serv_manager=OpenSCManager(0,0,SC_MANAGER_CREATE_SERVICE|SC_MANAGER_CONNECT);
  if(!serv_manager) return 0;
  serv_handle=OpenService(serv_manager,service_name,SERVICE_ALL_ACCESS);
  if(!serv_handle) return 0;
  if(!StartService(serv_handle,0,0)) return 0;
  for(i=0; i<time_out; i++) {
    if(QueryServiceStatus(serv_handle,&SS) &&
       SS.dwCurrentState==SERVICE_RUNNING) return 1;
    Sleep(1000);
   if(i==2) fprintf(stderr,"%s starting:",srv_name);
   if(i>2)  fprintf(stderr,"*");
  }
  if(i==0) i=1;
  if(i==time_out) i=0;
  if(i>2) fprintf(stderr,"\n");
  return i;
}


int srv_set(int (*on_stop_proc)()) {
on_stop = on_stop_proc;
return 0;
}


VOID WINAPI srv_run_proc(DWORD argc,LPTSTR *argv);

int srv_params(int npar,char **par) {
  int e;  char *p =par[1];
  srv_init(par[0]);
  //printf("Here %s with srv_name=%s\n",par[1],srv_name);
int i;
for(i=1;i<npar;i++) {
    uchar *p = par[i];
    if (*p!='-') continue;
    p++;
    //alertf("PAR='%s'",p);
    if (p[0]=='@') {
        srv_name=p+1; // Change Service Name
        service_mode = 1; // OK
        }
   else if (p[0]=='w') { // ChangeWorkingDir...
        p++;
        //alertf("ChDir to '%s'",p);
        //int ok =
        chdir(p); // Change a dir...
        //alertf("Res=%d",ok);
        }
    }
  if( npar>1 && (*p='-'|| *p=='/' ) ) {
    p++; if (*p=='-') p++;
    if(strnicmp(p,"install",7)==0 || strnicmp(p,"create",6)==0)     {
      uchar szbuf[MAX_PATH*2], *dsc = 0, *params="";
      if (npar>2) { // other name !!!
          uchar *n = par[2];
          dsc = strchr(n,'|'); if (dsc) {*dsc=0; dsc++;} // Dsc By Default???
          if (*n) srv_name = n ; // Reset StrName
          }
      if (!dsc) dsc = szDescription; // Default Module Desc
      if (npar>3) { // run parameters !!!
          params = par[3];
          }
      sprintf(szbuf,"%s\\%s.exe %s -w%s -@%s",srv_dir,srv_file,params,srv_startDir,srv_name); // Intalls it
      //sprintf(szbuf,"%s\\%s.exe -w%s -@%s",srv_dir,srv_file,srv_startDir,srv_name); // Intalls it
      srv_stop(srv_name); srv_delete(srv_name); // Try Remove -???
      if(srv_create(srv_name,szbuf,dsc))
           fprintf(stderr,"%s: created in '%s'.\n",srv_name,szbuf);
      else fprintf(stderr,"%s: error creating...\n",srv_name);
      return 0;
    }
    if(stricmp(p,"uninstall")==0 || stricmp(p,"remove")==0)     {
      if(npar>2) srv_name=par[2];
      srv_stop(srv_name);
      if(srv_delete(srv_name))
           fprintf(stderr,"%s: deleted Ok.\n",srv_name);
      else fprintf(stderr,"%s: error  delete...\n",srv_name);
      return 0;
    }
    if(stricmp(p,"restart")==0)     {
      if(npar>2) srv_name=par[2];
      e = srv_stop(srv_name);
      if(e)  fprintf(stderr,"%s: error#%d stopping...\n",srv_name,e);
        else fprintf(stderr,"%s: stopped Ok.\n",srv_name);
      if(srv_start(srv_name,300))
           fprintf(stderr,"%s: started Ok.\n",srv_name);
      else fprintf(stderr,"%s: error starting...\n",srv_name);
      return 0;
    }
    if(stricmp(p,"stop")==0)     {
      if(npar>2) srv_name=par[2];
      e = srv_stop(srv_name);
      if(e)  fprintf(stderr,"%s: error#%d stopping...\n",srv_name,e);
        else fprintf(stderr,"%s: stopped Ok.\n",srv_name);
      return 0;
    }
    if(stricmp(p,"start")==0)     {
      if(npar>2) srv_name=par[2];
      if(srv_start(srv_name,300))
           fprintf(stderr,"%s: started Ok.\n",srv_name);
      else fprintf(stderr,"%s: error starting...\n",srv_name);
      return 0;
    }
   }
return -1;
}

static int (*srvMainProc)();
static char **srvParams;
static int srvParamCount;

int srv_main(int npar,char **par) { // Будет вызвана из srv_run_proc
  srvMainProc(srvParamCount,srvParams);
  srv_stop(srv_name); // Автоматически останавливает сервис, когда все закончилось....
  return 0;
}


int srv_run(int npar,char **par, void *srvMain, void *srvOnStop) {
  srvMainProc = srvMain;
  on_stop = srvOnStop;  srvParams = par; srvParamCount = npar;
  srv_dispatch_table[0].lpServiceName=srv_name;
  srv_dispatch_table[0].lpServiceProc=srv_run_proc;
  srv_dispatch_table[1].lpServiceName=0;
  srv_dispatch_table[1].lpServiceProc=0;
  if (!StartServiceCtrlDispatcher(srv_dispatch_table)) {
    logf("%s: Unable start dispatch procedure!",srv_name);
    return -1;
  }
  return 0;
}


int srv_restart(char *name) {
  char szcmd[1024];
  if((!name) || *name==0) name=srv_name;
  sprintf(szcmd,"srestart.exe %s",name);
  return WinExec(szcmd,SW_SHOW);
}


VOID WINAPI srv_run_handler(DWORD Control) {
  switch(Control) {
    case SERVICE_CONTROL_STOP:
      srv_status.dwCheckPoint=0;
      srv_status.dwCurrentState = SERVICE_STOPPED;
      srv_status.dwWaitHint = 0;
      //sig_done(); // Call Stop Handler...
      if (on_stop) on_stop();
      CloseHandle(srv_main_thread);
      break;

   case SERVICE_CONTROL_INTERROGATE:
     break;

   default: Logf("VSRV:Error: Unhandled control function %d",Control);
  }
  if(!SetServiceStatus(srv_hstatus,&srv_status))
    Logf("VSRV: Set service status failed");
}


VOID WINAPI srv_run_proc(DWORD argc,LPTSTR *argv) {
  srv_status.dwServiceType      = SERVICE_WIN32;
  srv_status.dwCurrentState     = SERVICE_START_PENDING;
  srv_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
  srv_status.dwCheckPoint       = 0;
  srv_hstatus = RegisterServiceCtrlHandler(srv_name,srv_run_handler);
  if(!srv_hstatus) {
    int code=GetLastError();
    if(code==ERROR_INVALID_NAME)
      Logf("VSRV: %s: Fail to register Handle. ServiceName invalid.",srv_name);
    else
      if (code==ERROR_SERVICE_DOES_NOT_EXIST) Logf("VSRV: %s: Fail to register Handle. service does not exists.",srv_name);
      else Logf("VSRV: %s: ERROR #%d: Fail to register Handle.",srv_name,code);
    return ;
  }
  srv_status.dwCurrentState=SERVICE_RUNNING;
  SetServiceStatus(srv_hstatus,&srv_status);

  srv_main_thread=(HANDLE)thread_create(srv_main,argc,(void*)argv);
  if(!srv_main_thread) {
    Logf("VSRV: %s ERROR main run thread failed with code %d",srv_name,GetLastError());
    srv_status.dwCurrentState=SERVICE_STOPPED;
    srv_status.dwServiceSpecificExitCode=0;
    srv_status.dwWin32ExitCode=1;
    SetServiceStatus(srv_hstatus,&srv_status);
    return;
  }

}

