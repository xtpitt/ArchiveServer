//
// Created by Xiao Ma on 8/29/18.
//
#define BUFSIZE 16400 //FIXME: theoretical maximum value of a frame
#define FNSIZE 256
#ifndef ARCHIVESERVER_TCPCONNECTION_H
#define ARCHIVESERVER_TCPCONNECTION_H


#endif //ARCHIVESERVER_TCPCONNECTION_H
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <unordered_map>

using namespace std;
extern unordered_map<string, int> record_state;
extern unordered_map<string, int> record_validpivot;

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
     * FIXME: no record of header exchange states
     * 0: started file not created
     * 1: file created
     * 2: file header written
     * 3: JSON header written
     * 4: frame dumping started
     * 5: frame dumping finished(or interrupted)*/
    int state;
    int validpivot;
    char filebuf[BUFSIZE];
    static unordered_map<string, int> record_state;
    static unordered_map<string, int> record_validpivot;

public:
    filearchive(int sock);
    int createfile();
    int recvfile();
    int readfilename();
    int readheader();
    int processheader();
    void updatectrlstatus();
    /*
    * Read size 8 frameheader "MEETECHO"
    * Read size 2 framelength uint
    * Read frame
    */
    int collectframe();
    ~filearchive();
};