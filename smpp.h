#ifndef SMPP_H
#define SMPP_H

#define uint32 unsigned int
#define uint16 unsigned short

#define HTONL(A) ((((uint32)(A) & 0xff000000) >> 24) | \
(((uint32)(A) & 0x00ff0000) >> 8) | \
(((uint32)(A) & 0x0000ff00) << 8) | \
(((uint32)(A) & 0x000000ff) << 24))
#define HTONS(A) ((((uint16)(A) & 0xff00) >> 8) | \
                    (((uint16)(A) & 0x00ff) << 8))

#define NTOHL HTONL
#define NTOHS HTONS


#define SMPPCMD(A,CODE) A = HTONL(CODE)
//#define SMPP_VERSION 0x34
#define SMPP_VERSION 0x50
#include <vtypes.h>

#define SMPP_PORT_DEF 2775 /* Default SMPP Port ??? */
#define SMPP_MAX_DATA 1024 /* Max Size of SMPP packet */
#define SMPP_RES_ASYNC -1 /* When no need to send default answer to the command */

/* service_type
УУ (NULL) Default
УCMTФ Cellular Messaging
УCPTФ Cellular Paging
УVMNФ Voice Mail Notification
УVMAФ Voice Mail Alerting
УWAPФ Wireless Application Protocol
УUSSDФ Unstructured Supplementary Services Data
*/

/* message-state
ENROUTE 1 The message is in enroute
state.
DELIVERED 2 Message is delivered to
destination
EXPIRED 3 Message validity period has
expired.
DELETED 4 Message has been deleted.
UNDELIVERABLE 5 Message is undeliverable
ACCEPTED 6 Message is in accepted state
(i.e. has been manually read
on behalf of the subscriber by
customer service)
UNKNOWN 7 Message is in invalid state
REJECTED 8 Message is in a rejected state

*/

enum { // smpp optional parameters - to do it all
    /*
  dest_addr_subunit=0x0005, // GSM
  dest_network_type=0x0006, // Generic
  dest_bearer_type=0x0007, // Generic
  dest_telematics_id=0x0008, // GSM
  source_addr_subunit=0x000D, // GSM
  source_network_type 0x000E Generic
source_bearer_type 0x000F Generic
source_telematics_id 0x0010 GSM
qos_time_to_live 0x0017 Generic
payload_type 0x0019 Generic
additional_status_info_text 0x001D Generic
receipted_message_id 0x001E Generic
ms_msg_wait_facilities 0x0030 GSM
privacy_indicator 0x0201 CDMA, TDMA
source_subaddress 0x0202 CDMA, TDMA
dest_subaddress 0x0203 CDMA, TDMA
user_message_reference 0x0204 Generic
user_response_code 0x0205 CDMA, TDMA
source_port 0x020A Generic
destination_port 0x020B Generic
sar_msg_ref_num 0x020C Generic
language_indicator 0x020D CDMA, TDMA
sar_total_segments 0x020E Generic
sar_segment_seqnum 0x020F Generic
SC_interface_version 0x0210 Generic
callback_num_pres_ind 0x0302 TDMA
callback_num_atag 0x0303 TDMA
number_of_messages 0x0304 CDMA
callback_num 0x0381 CDMA, TDMA, GSM, iDEN
dpf_result 0x0420 Generic
set_dpf 0x0421 Generic
ms_availability_status 0x0422 Generic
network_error_code 0x0423 Generic
*/
  smpp_message_payload=0x0424, // Generic
  /*
delivery_failure_reason 0x0425 Generic
more_messages_to_send 0x0426 GSM
message_state 0x0427 Generic
ussd_service_op 0x0501 GSM (USSD)
display_time 0x1201 CDMA, TDMA
sms_signal 0x1203 TDMA
ms_validity 0x1204 CDMA, TDMA
alert_on_message_delivery 0x130C CDMA
its_reply_type 0x1380 CDMA
its_session_info 0x1383 CDMA
*/
};


enum { // SMPP ton codes
    tonUnknown = 0,
    tonInternational = 1,
    tonNational = 2,
    tonNetworkSpecific = 3,
    tonSubscriberNumber = 4,
    tonAlphanumeric = 5,
    tonAbbreviated = 6,
    //All other values reserved
    };

enum { // SMPP npi codes
    npiUnknown=0,
    npiISDN = 1, // (E163/E164)
    npiData = 2, //(X.121)
    npiTelex =3, //(F.69)
    npiLandMobile  = 6, //(E.212)
    npiNational = 8 , // 00001000
    npiPrivate = 9, //00001001
    npiERMES = 10, //00001010
    npiInternet = 14, // Internet (IP) 00001110
    npiWAP = 18, //Client Id (to bedefined by WAP Forum) 00010010
    //All other values reserved
    };

enum { //
    esmStoreAndForward = 0, // Default...
    esmDatagramm = 1,
    esmForward = 2,
      // flags
    esmUDHI =  0x40, // user data header indication
    };

enum {
    // flags
     dcsIndOff =0xC0, // MWI
     dcsIndOn  =0xC8, //indVms=0, indFax=1, indEmail=2, indOther=3, // indicators

     dcsFlash = 0xF0
     };

enum { // smpp command id
    SMPPCMD(smpp_bind_receiver,0x00000001),
    SMPPCMD(smpp_bind_transmitter, 0x00000002),
    SMPPCMD(smpp_query_sm,0x00000003),
    SMPPCMD(smpp_submit_sm,0x00000004),
    SMPPCMD(smpp_deliver_sm,0x00000005),
    SMPPCMD(smpp_unbind,0x00000006),
    SMPPCMD(smpp_replace_sm,0x00000007),
    SMPPCMD(smpp_cancel_sm, 0x00000008),
    SMPPCMD(smpp_bind_transceiver,0x00000009),
    SMPPCMD(smpp_outbind,0x0000000B),
    SMPPCMD(smpp_enquire_link,0x00000015),
    SMPPCMD(smpp_submit_multi,0x00000021),
    SMPPCMD(smpp_alert_notification,0x00000102),
    SMPPCMD(smpp_data_sm,0x00000103),
    SMPPCMD(smpp_broadcast_sm,0x00000111),
    SMPPCMD(smpp_query_broadcast_sm,0x00000112),
    SMPPCMD(smpp_cancel_broadcast_sm, 0x00000113),
    SMPPCMD(smpp_generic_nack, 0x80000000),
    SMPPCMD(smpp_bind_receiver_resp, 0x80000001),
    SMPPCMD(smpp_bind_transmitter_resp, 0x80000002),
    SMPPCMD(smpp_query_sm_resp, 0x80000003),
    SMPPCMD(smpp_submit_sm_resp, 0x80000004),
    SMPPCMD(smpp_deliver_sm_resp, 0x80000005),
    SMPPCMD(smpp_unbind_resp, 0x80000006),
    SMPPCMD(smpp_replace_sm_resp, 0x80000007),
    SMPPCMD(smpp_cancel_sm_resp, 0x80000008),
    SMPPCMD(smpp_bind_transceiver_resp, 0x80000009),
    SMPPCMD(smpp_enquire_link_resp, 0x80000015),
    SMPPCMD(smpp_submit_multi_resp, 0x80000021),
    SMPPCMD(smpp_data_sm_resp, 0x80000103),
    SMPPCMD(smpp_broadcast_sm_resp, 0x80000111),
    SMPPCMD(smpp_query_broadcast_sm_resp, 0x80000112),
    SMPPCMD(smpp_cancel_broadcast_sm_resp, 0x80000113),
    //Reserved for MC Vendor 0x00010200 - 0x000102FF, 0x80010200 - 0x800102FF
    };

typedef struct {
    int len,id,status,num; // Length,ID,ErrorStatus,SequenceNumber
    uchar body[1024]; // Then body follows, page 37 pack types -)))
    uchar src_addr[80],dst_addr[80]; // Copy Of Addresses
    uchar *data; int dlen; // Data & datalen of a message
    uchar *text,*vp,*shed_time,*service_type; // Ref to text
    int tlen; // Same as sm_length or more -???
    uchar data_coding,pid,priority,esm_class,registred_delivery,replace_if_present,sm_default_msg_id,sm_length;
    } smppCommand;

enum {
    ESME_ROK=0x00000000, // No Error
    ESME_RINVMSGLEN=0x00000001, // Message Length is invalid
    ESME_RINVCMDLEN=0x00000002,  // Command Length is invalid
    ESME_RINVCMDID=0x00000003, //Invalid Command ID
    ESME_RINVBNDSTS=0x00000004, // Incorrect BIND Status for given command
    ESME_RALYBND=0x00000005, // ESME Already in Bound State
    ESME_RINVPRTFLG=0x00000006, // Invalid Priority Flag
    ESME_RINVREGDLVFLG = 0x00000007, // Invalid Registered Delivery Flag
    ESME_RSYSERR =0x00000008, // System Error
//Reserved 0x00000009 Reserved
    ESME_RINVSRCADR=0x0000000A, // Invalid Source Address
    ESME_RINVDSTADR=0x0000000B, // Invalid Dest Addr
    ESME_RINVMSGID=0x0000000C, // Message ID is invalid
    ESME_RBINDFAIL=0x0000000D, // Bind Failed
    ESME_RINVPASWD=0x0000000E, // Invalid Password
    ESME_RINVSYSID=0x0000000F, // Invalid System ID
//Reserved 0x00000010 Reserved
    ESME_RCANCELFAIL =0x00000011, // Cancel SM Failed
//Reserved 0x00000012 Reserved
    ESME_RREPLACEFAIL = 0x00000013, // Replace SM Failed
    ESME_RMSGQFUL =0x00000014, // Message Queue Full
    ESME_RINVSERTYP =0x00000015, // Invalid Service Type
//Reserved 0x00000016-0x00000032 Reserved
    ESME_RINVNUMDESTS = 0x00000033, // Invalid number of destinations
    ESME_RINVDLNAME = 0x00000034, // Invalid Distribution List name
//Reserved 0x00000035-0x0000003FReserved
    ESME_RINVDESTFLAG =0x00000040, // Destination flag is invalid
//(submit_multi) Reserved 0x00000041 Reserved
    ESME_RINVSUBREP = 0x00000042, // Invalid Сsubmit with replaceТ request
//(i.e. submit_sm with replace_if_present_flag set)
    ESME_RINVESMCLASS =0x00000043, // Invalid esm_class field data
    ESME_RCNTSUBDL = 0x00000044, // Cannot Submit to Distribution List
    ESME_RSUBMITFAIL = 0x00000045, // submit_sm or submit_multi failed
//Reserved 0x00000046-0x00000047 Reserved
    ESME_RINVSRCTON =0x00000048, // Invalid Source address TON
    ESME_RINVSRCNPI =0x00000049, // Invalid Source address NPI
    ESME_RINVDSTTON =0x00000050, // Invalid Destination address TON
    ESME_RINVDSTNPI =0x00000051, // Invalid Destination address NPI
//Reserved 0x00000052 Reserved
    ESME_RINVSYSTYP = 0x00000053, // Invalid system_type field
    ESME_RINVREPFLAG = 0x00000054, // Invalid replace_if_present flag
    ESME_RINVNUMMSGS =0x00000055, // Invalid number of messages
//Reserved 0x00000056- 0x00000057 Reserved
    ESME_RTHROTTLED =0x00000058, // Throttling error (ESME has exceeded
//allowed message limits) Reserved 0x00000059-  0x00000060 Reserved
    ESME_RINVSCHED = 0x00000061, //Invalid Scheduled Delivery Time
    ESME_RINVEXPIRY = 0x00000062, // Invalid message validity period
//(Expiry time)
    ESME_RINVDFTMSGID = 0x00000063, // Predefined Message Invalid or Not Found
    /*
ESME_RX_T_APPN 0x00000064 ESME Receiver Temporary App
Error Code
ESME_RX_P_APPN 0x00000065 ESME Receiver Permanent App Error
Code
ESME_RX_R_APPN 0x00000066 ESME Receiver Reject Message Error
Code
ESME_RQUERYFAIL 0x00000067 query_sm request failed
Reserved 0x00000068
-
0x000000BF
Reserved
ESME_RINVOPTPARSTREAM 0x000000C0 Error in the optional part of the PDU
Body.
ESME_ROPTPARNOTALLWD 0x000000C1 Optional Parameter not allowed
ESME_RINVPARLEN 0x000000C2 Invalid Parameter Length.
ESME_RMISSINGOPTPARAM 0x000000C3 Expected Optional Parameter missing
ESME_RINVOPTPARAMVAL 0x000000C4 Invalid Optional Parameter Value
ESME_RDELIVERYFAILURE 0x000000FE Delivery Failure (used for
data_sm_resp)
ESME_RUNKNOWNERR 0x000000FF Unknown Error
Reserved for SMPP extension 0x00000100-
0x000003FF
Reserved for SMPP extension
Reserved for SMSC vendor
specific errors
0x00000400-
0x000004FF
Reserved for SMSC vendor specific
errors
Reserved 0x00000500-
0xFFFFFFFF
Reserved
*/
};

int smppPacketReady(uchar *data,int len);
int smppPackBind(smppCommand *cmd,int id, int num, char *system_id, char *password, char *system_type);
int smppPackDataSM(smppCommand *cmd,int num,char *service_type,char *src_addr,char *dst_addr,int esm,
      int registred_delivery,int pid, int data_coding,char *data,int len); // Payload Here

int smppPackSubmit(smppCommand *cmd,int num,char *service_type,char *src_addr,char *dst_addr,int esm_class,
   int registered_delivery,int pid, int data_coding,char *data,int len) ; // Payload Here
int smppPackDelivr(smppCommand *cmd,int num,char *service_type,char *src_addr,char *dst_addr,
  int esm_class,
   int registered_delivery,int pid, int data_coding,char *data,int len); // Payload Here



int smppPack_DELIVER_SM_RESP(smppCommand *cmd, int status, char *msg);
int smppPack_SUBMIT_SM_RESP(smppCommand *cmd, int status, char *msg);
int smppPack_DATA_SM_RESP(smppCommand *cmd, int status, char *msg);


/*
#include "smpp.h" // SMPP 5.0 & 3.4 declarations

typedef struct {
    SocketPool srv; // “ут все сокеты - коннектор и подсоединени€
    int port;
    Socket *master; // —лушающий сокет
    int (*onSmppBind)(); // ¬ызываетс€, когда приходит авторизационный пакет
    int (*onSmppPacket)(); // ¬ызываетс€, когда готов пакет (кроме авторизации)
    int (*onSmppDone)(); // ¬ызываетс€ при отключении (сокета или unbind)
    int  enquireLink;    //  „ерез сколько запрашивать проверку св€зи (0 - не запрашивать)
    } smppSrv;

VS0OBJH(smppSrv); // «апуск сервиса -)))

smppSrv *smppSrvCreate(int port, void *onSmppBind, void *onSmppPacket,void *onSmppDone); // »нициализаци€ и запуск сервера
*/


// Command decoders
int smppPopString(smppCommand *cmd, int *pos, uchar **u) ; // Extracts a string
int smppPopAddr(smppCommand *cmd,int *pos,char *addr) ;// Gets addr to a buffer?
int smppPopByte(smppCommand *cmd,int *pos,unsigned char *val);
void smppHead(int *buf,int cmd,int status,int ref);
int smppPackDataOK(smppCommand *cmd, int id, int num, char *message_id);

int smppDecodeDelivr(smppCommand *cmd);
int smppDecodeSubmit(smppCommand *cmd);
int smppDecodeDataSM(smppCommand *cmd);


int smppDecodeBind(smppCommand *cmd, uchar **u,uchar **p,uchar **sysID);
int smppPackBindResp(smppCommand *cmd, int mode, int status);


#endif //SMPP_H
