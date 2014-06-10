#include <stdio.h>
#include <stdlib.h>
#include <vtypes.h>

#include "httpSrv.h"
#include "gearSMPP.h"
#include "vsrv.h"
#include <stdarg.h>


int hex_dump(char *msg,uchar *s,int len) {
int i;
 printf("\n%s:",msg);
 for(i=0;i<len;i++) printf("%02x ",s[i]);
 printf("\n");
return 0;
}

//#include "microHttp.c"


//httpClientTest();


//signal(SIGINT,sig_done);

int alertf(char *fmt,...) {
char buf[1024];
BUF_FMT(buf,fmt);
//return MessageBox(0,buf,srv_name,MB_SERVICE_NOTIFICATION)==0;
char buf2[1024]; sprintf(buf2,"notify '%s'",buf); system(buf2); // call to notify
}

char szDescription[1024]; // MyDefault Descritption...
#include "version.h"

int tst() {
    char d[100]; char *p="abcdef.,-+";
hexdump(p,p,strlen(p));
int l = utf2gsm(d,p,-1);
hexdump("out",d, l);
return 0;
}

int main(int npar,char **par) {

    //return tst();

    //printf("Hello world from SMPP!\n");
    sprintf(szDescription,"%s, ver: %d.%d.%d.%d, Build: %s", // sz Desc placed in exe.c...
              Description, Version, "GCC");
    //printf("D:<%s>\n",szDescription);
    //return 0;
    net_init(); // must be done by module -)))
//    signal(SIGINT,sig_done); // ƒл€ консоли - останока по CtrlC
    //srv_init(par[0]); // exe initing ....
    //alertf("srv_starting....",service_mode);


#ifdef MSWIN
// window service part
    if (srv_params(npar,par)==0) return 0; // Init&starts -install - create & other
    if (service_mode) { // ”казываетс€ если есть -@name в параметрах...
        srv_run( npar, par, gearSMPPmain, sig_done ); // Run As Parameters...
        return 0; // By Service Stop???
        }
#endif

    //return httpTestMain(npar,par); // testHttp.exe
    if (npar>1 && par[1][1]=='p') return smppTestServer(npar,par); // MicroHttpServer
    //if (npar>1 && par[1][1]=='p') return MicroHttpMain(npar,par); // MicroHttpServer
    if (npar>1 && strncmp(par[1],"http://",7)==0) return httpTestMain(npar,par);
    if (npar>1 && strncmp(par[1],"smpp://",7)==0) return smppTestMain(npar,par);
    //return simpleHttp();
    return gearSMPPmain(npar,par);
}

