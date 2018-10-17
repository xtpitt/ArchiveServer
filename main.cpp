#include <iostream>
#include <cstring>
#include <thread>
#include <csignal>
#include <cstdio>
#include "tcpconnection.h"
#define SERVERPORT 50625

volatile sig_atomic_t work = 1;

void signalbreak(int sig){
    work=0;
    printf("Break Signal Received.\n");
    //FIXME: HOW TO MARK INCOMPLETED FILE TRANSFER??
    exit(0);
}
int filexsferhandler(filearchive* fa){

    char fn[FNSIZE];
    memset((char*)fn,0,FNSIZE);
    if(fa->readfilename()<0){
        printf("Error obtaining filename\n");
        delete fa;
        return -1;
    }
    if(fa->createfile()<0){//FIXME: IN CASE FUTURE ERRORS
        printf("Error creating local file\n");
        delete fa;
        return -1;
    }
    if(fa->readheader()<0){
        printf("Error reading first header\n");
        delete fa;
        return -1;
    }
    if(fa->processheader()<0){
        printf("Error processing frame header\n");
        delete fa;
        return -1;
    }
    if(fa->recvfile()<0){
        printf("Error writing local filename\n");
        delete fa;
        return -1;
    }
    delete fa;
    return 0;
}
int main() {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT,signalbreak);
    tcpsocket server;
    if(server.initial(SERVERPORT)<0)
        return -1;
    int newfd;
    //multithread approach
    while(work){
        newfd=server.accecptrequest();
        if(newfd>0){
            filearchive *fa=new filearchive(newfd);
            std::thread fxsferthread(filexsferhandler, fa);
            fxsferthread.detach();
        }
    }
    server.closesocket();
    return 0;

    //static approach, tested successful
    /*while(work) {
        newfd = server.accecptrequest();
        if (newfd > 1) {
            filearchive fa(newfd);
            fa.readfilename();
            fa.createfile();
            fa.readheader();
            fa.processheader();
            fa.recvfile();
        }
    }
     */


}

