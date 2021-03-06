//
// Created by Xiao Ma on 8/29/18.
//
#define FHEADERLEN 8
#define FRAMESIZEBYTES 2

#include "tcpconnection.h"
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <cstring>
#include <pthread.h>
#include <unordered_map>
#include <string>
using namespace std;



tcpsocket::tcpsocket() {
    sockfd=-1;
    memset((char*)&addr, 0, sizeof(addr));
    addrlen= sizeof(addr);
    opt=1;
    initialized=false;
    portno=0;
}
int tcpsocket::initial(int port) {
    if(initialized){
        perror("This socket has been initialized\n");
        return -1;
    }
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr= htonl(INADDR_ANY);
    addr.sin_port=htons(port);
    portno=port;
    if((sockfd=socket(AF_INET, SOCK_STREAM,0))<0) {
        perror("Unable to start TCP msg speed test socket");
        return -1;
    }
    if(bind(sockfd, (struct sockaddr*)&addr, sizeof(addr))<0){
        perror("Unable to bind speed socket");
        return -1;
    }
    if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&opt, sizeof(opt))){
        perror("Error setting options for TCP socket");
        return -1;
    }
    if(listen(sockfd,40)<0){
        perror("Fail to listen to SPTPORT");
        return -1;
    }
    printf("TCP Port %d is open, ready for incoming requests.\n", port);
    return 0;
}
int tcpsocket::accecptrequest() {
    int sfd;
    if((sfd=accept(sockfd,(struct sockaddr*)&addr,&addrlen))<=0){
        perror("Error Accepting request socket.\n");
        return -1;
    }
    printf("New request accpepted to socket#%d.\n", sfd);
    return sfd;
}
void tcpsocket::closesocket() {
    close(sockfd);
    printf("Server socket at port %d is closed.\n", portno);
}

filearchive::filearchive(int sock) {
    this->sfd=sock;
    this->state=0;
    this->validpivot=0;
}
int updatectrlstatus(){

}
int filearchive::createfile() {
    f=fopen(filename,"r");
    if(f){//file is found
        fclose(f);
        printf("File: %s Found. Assume reconnection, appending at the end...\n", filename);
        f=fopen(filename,"a");
        return 1;
    }
    else{
        f=fopen(filename,"w");
        if(f){
            printf("Creating New file: %s \n", filename);
            this->state=1;
            return 0;
        }
        else{
            printf("File Creation Failure: %s, do you have root privilege?\n", filename);
            return -1;
        }


    }

}
int filearchive::processheader() {
    ssize_t read;
    memset(filebuf,0,BUFSIZE);
    read=recvfrom(sfd, filebuf, sizeof(uint16_t), 0, NULL, NULL);
    if(read<=0){
        perror("Error reading file header size");
        return -1;
    }
    uint16_t len;
    memcpy(&len,filebuf, sizeof(uint16_t));
    len=ntohs(len);
    printf("Json Header length %d\n",len);
    ssize_t offset=sizeof(uint16_t);
    while(offset<len+sizeof(uint16_t)){
        read=recvfrom(sfd, filebuf+offset, len+sizeof(uint16_t)-offset, 0, NULL, NULL);
        if(read<=0){
            perror("Error reading JSON header content");
            return -1;
        }
        offset+=read;
    }
    printf("Json Header Content(raw) %s\n",filebuf+sizeof(uint16_t));
    fwrite(filebuf, sizeof(char),len+sizeof(uint16_t),f);
    if(ferror(f)){
        perror("Error writing header content to destination file");
        return -1;
    }
    this->validpivot+=offset;
    this->state=3;
    return 0;
}

int filearchive::readfilename() {
    ssize_t read;
    int offset=0;
    int headerlen;
    memset(filename, 0, FNSIZE);
    read=recvfrom(sfd, filebuf, sizeof(int), 0, NULL, NULL);
    if(read<0){
        perror("Error reading file name");
        return -1;
    }
    //assume offset is more than 4
    memcpy(&headerlen,filebuf, sizeof(int));
    while(offset<headerlen){
        read=recvfrom(sfd, filebuf+sizeof(int)+offset, headerlen-offset, 0, NULL, NULL);
        if(read<0){
            perror("Error reading file name");
            return -1;
        }
        offset+=read;
    }
    memcpy(filename,filebuf+sizeof(int),headerlen);
    return 0;
}
int filearchive::readheader() {
    ssize_t read;
    int offset=0;
    int headerlen;
    memset(filebuf,0,BUFSIZE);
    read=recvfrom(sfd, filebuf+offset, sizeof(int), 0, NULL, NULL);
    if(read<0){
        perror("Error reading file name size");
        updatectrlstatus();
        return -1;
    }
    offset+=read;
    //assume offset is more than 4
    memcpy(&headerlen,filebuf, sizeof(int));
    while(offset<sizeof(int)+headerlen){
        read=recvfrom(sfd, filebuf+offset, sizeof(int)+headerlen-offset, 0, NULL, NULL);
        if(read<0){
            perror("Error reading file name");
            updatectrlstatus();
            return -1;
        }
        offset+=read;
    }
    printf("Header size %d, content:--%s--\n",headerlen,filebuf+sizeof(int));
    fwrite((char*)filebuf+sizeof(int),1, headerlen,f);
    if(ferror(f)){
        perror("Error writing to file");
        updatectrlstatus();
        return -1;
    }
    this->validpivot+=headerlen;
    this->state=2;
    return 0;
}
int filearchive::recvfile() {
    ssize_t read;
    ssize_t offset=0;
    this->state=4;
    while(1){
        memset(filebuf,0,BUFSIZE);
        read=recvfrom(sfd, filebuf, BUFSIZE, 0, NULL, NULL);
        if(read<0){
            if(errno!=EPIPE)
                printf("%s\n", strerror(errno));
            perror("Error receiving file from socket");
            return -1;
        }
        if(read==0){
            //other side has shutdown
            this->state=5;
            close(sfd);
            fclose(f);
	        printf("File Transfer %s completed, %d bytes received\n",filename, offset);
            return 0;
        }

        size_t writeoff=0;
        size_t write=0;
        while(writeoff<read){
            write=fwrite(filebuf+writeoff, sizeof(char), read-writeoff, f);
            if(ferror(f)){
                perror("Error writing to file");
                close(sfd);
                return -1;
            }
            writeoff+=write;
        }
        offset+=read;
        //printf("%d bytes received\n", offset);
    }

}
int filearchive::collectframe() {
    ssize_t toread=0, offset=0, base=0;
    size_t writeoff=0;
    size_t write=0;
    ssize_t read;
    size_t total=0;
    while(1){
        //collect frame header 8 bytes
        memset(filebuf, 0, BUFSIZE);
        toread=FHEADERLEN;
        offset=0;
        base=0;
        while(offset<toread){
            read=recvfrom(sfd, filebuf+base+offset, toread-offset, 0, NULL, NULL);
            if(read<0){
                perror("Error receiving Frame Header");
                updatectrlstatus();
                return -1;
            }
            else if(read==0){//if remote end disconnects, the transmission ends
                this->state=5;
                close(sfd);
                fclose(f);
                printf("File Transfer %s completed, %d bytes received\n",filename, total);
                return 0;
            }
            offset+=read;
        }
        //collect frame size 2 bytes
        base+=toread;
        offset=0;
        toread=FRAMESIZEBYTES;
        while(offset<toread){
            read=recvfrom(sfd, filebuf+base+offset, toread-offset, 0, NULL, NULL);
            if(read<=0){
                perror("Error receiving Frame Size");
                updatectrlstatus();
                return -1;
            }
            offset+=read;
        }
        uint16_t framesize;
        memcpy(&framesize, filebuf+base, toread);
        framesize=ntohs(framesize);
        //collect frame
        base+=toread;
        offset=0;
        toread=framesize;
        while(offset<toread){
            read=recvfrom(sfd, filebuf+base+offset, toread-offset, 0, NULL, NULL);
            if(read<=0){
                perror("Error receiving Frame Content");
                updatectrlstatus();
                return -1;
            }
            offset+=read;
        }
        base+=toread;
        //write to file
        writeoff=0;
        write=0;
        while(writeoff<base){
            write=fwrite(filebuf+writeoff, sizeof(char), base-writeoff, f);
            if(ferror(f)){
                perror("Error writing to file");
                updatectrlstatus();
                return -1;
            }
            writeoff+=write;
        }
        total+=base;
        this->validpivot+=base;
        //printf("One Frame Processed\n");
    }
}
void filearchive::updatectrlstatus(){
    string s(this->filename);
    if(this->state != 5){
        record_state[s]=this->state;
        record_validpivot[s]=this->validpivot;
        printf("State: %d, validpivot: %d", this->state, this->validpivot);
    }

}
filearchive::~filearchive() {
    printf("Transfer Session for %s ended\n", filename);
    if(this->state!=5){
        close(sfd);
        if(f)
            fclose(f);
        printf("End Status abnormal\n");
    }
    printf("File Closed.\n");
    sfd=-1;
    state=-1;
}
