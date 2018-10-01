//
// Created by Xiao Ma on 8/29/18.
//
#define BUFSIZE 1000
#define FNSIZE 1100
#ifndef ARCHIVESERVER_TCPCONNECTION_H
#define ARCHIVESERVER_TCPCONNECTION_H

#endif //ARCHIVESERVER_TCPCONNECTION_H
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>

class tcpsocket{
private:
    int sockfd;
    struct sockaddr_in addr;
    socklen_t addrlen;
    int opt;
    bool initialized;
    int portno;
public:
    tcpsocket();
    int initial(int port);
    int accecptrequest();
    void closesocket();
};
class filearchive{
private:
    FILE *f;
    char filename[FNSIZE];
    int sfd;
    /*state transition tables：
     * 0: started file not created
     * 1: file created
     * 2: file loading underway
     * 3: file loading finished(or interrupted)*/
    int state;

    char filebuf[BUFSIZE];
public:
    filearchive(int sock);
    /* */
    int createfile();
    int recvfile();
    int readfilename();
    int readheader();
    int processheader();
    ~filearchive();
};