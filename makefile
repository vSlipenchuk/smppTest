all: smppTest.exe       

clean:
	rm *.exe


smppTest.exe: smppTest.c smpp.c smppSrv.c
	$(CC) -Os -D smppTestMain=main -I../i smppTest.c -lws2_32 -lpsapi -o smppTest.exe


httpTest.exe: httpTest.c
	$(CC) -Os -D httpTestMain=main -I../i httpTest.c -lws2_32 -lpsapi -o httpTest.exe


