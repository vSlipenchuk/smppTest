all: smppTest

LANG="C"


clean:
	rm *.exe



dep:
	#(cd ..; (git clone git@github.com:vSlipenchuk/vos.git)
	(cd ..; git clone git@github.com:vSlipenchuk/vdb.git)

smppTest: smppTest.c smpp.c smppSrv.c
	$(CC) -Os -D smppTestMain=main -D smppTestCommon -I ../vos -I ../vdb \
	   smppTest.c -ldl -lpthread -o smppTest

smppTest.exe: smppTest.c smpp.c smppSrv.c
	$(CC) -Os -D smppTestMain=main -I../i smppTest.c -lws2_32 -lpsapi -o smppTest.exe


httpTest.exe: httpTest.c
	$(CC) -Os -D httpTestMain=main -I../i httpTest.c -lws2_32 -lpsapi -o httpTest.exe


