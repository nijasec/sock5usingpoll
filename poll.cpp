
/*
 * Sock5 server implementain using poll api 
 * Created By Nijas
 * 
 * */
#include <stdio.h>

#include <iostream>

#include <thread>

#include <sys/socket.h>

#include <stdlib.h>

#include <netinet/in.h>

#include <string.h>

#include <unistd.h>

#include <arpa/inet.h>

#include <pthread.h>

#include <signal.h>

#include <chrono>

#include <netdb.h> 
#include <poll.h>

#define BUFSIZE 65536

//socks types
enum socks {
  RESERVED = 0x00,
    VERSION4 = 0x04,
    VERSION5 = 0x05
};

enum socks_auth_methods {
  NOAUTH = 0x00,
    USERPASS = 0x02,
    NOMETHOD = 0xff
};

enum socks_auth_userpass {
  AUTH_OK = 0x00,
    AUTH_VERSION = 0x01,
    AUTH_FAIL = 0xff
};

enum socks_command {
  CONNECT = 0x01
};

enum socks_command_type {
  IP = 0x01,
    DOMAIN = 0x03
};

enum socks_status {
  OK = 0x00,
    FAILED = 0x05
};

int readn(int fd, char * buf, int n) {
  int nread, left = n;
  while (left > 0) {
    if ((nread = read(fd, buf, left)) == -1) {
      if (errno == EINTR || errno == EAGAIN) {
        continue;
      }
    } else {
      if (nread == 0) {
        return 0;
      } else {
        left -= nread;
        buf = buf + nread;
      }
    }
  }
  return n;
}

int writen(int fd, char * buf, int n) {
  int nwrite, left = n;
  while (left > 0) {
    if ((nwrite = write(fd, buf, left)) == -1) {
      if (errno == EINTR || errno == EAGAIN) {
        continue;
      }
    } else {
      if (nwrite == n) {
        return 0;
      } else {
        left -= nwrite;
        buf = buf + nwrite;
      }
    }
  }
  return n;
}

struct sockets {
  int sock1;
  int sock2;
  /* declare as many members as desired, but the entire structure size must be known to the compiler. */
};
using namespace std;

pthread_mutex_t lock;
//class  for client handle
void outs(const char * msg) {
  pthread_mutex_lock( & lock);
  cout << msg << endl;
  pthread_mutex_unlock( & lock);
}

class HandleClient {

  public:

    void handleClient(int client) {
      if (getByte(client) == VERSION5) //checking version
      {
        getByte(client); //skiping no of methode
        if (getByte(client) == NOAUTH) //No authetication
        {
          char REPLY[2];
          int d = 0;
          bool connected = false;
          outs("No autheticatiopn req from client");
          REPLY[0] = VERSION5;
          REPLY[1] = OK;
          d = sendtoClient(client, REPLY, 2);
          outs("sending first reply");

          connected = getClientCommand(client);

        }
      }

    }
    
  void clientToRemote(int c, int r) {
    int dlen = 0;
    int SIZE = 4096;
    char buffer[SIZE];
    bool isAlive = true;

    while (isAlive) {
      try {
        dlen = read(c, buffer, SIZE); //read client data

      } catch (char * msg) {
        cout << "error";
      }
      // cout<<"read: c to r"<<dlen<<endl;
      if (dlen < 0) {
        isAlive = false;
        outs("diedi");
      }
      if (dlen > 0)
        dlen = sendToServer(r, buffer, dlen);
      if (dlen < 0) {
        isAlive = false;
        outs("writing faild diedi");
        break;
      }
      if (dlen == 0) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        // outs("sleeping c to r");
      }
      //  std::this_thread::yield();   

    }

  }

  void remoteToClient(int r, int c) {
    int dlen = 0;
    int SIZE = 4096;
    char buffer[SIZE];
    bool isAlive = true;

    while (isAlive) {
      try {
        dlen = read(r, buffer, SIZE); //read client data
      } catch (char * msg) {
        cout << "error";
      }
      // cout<<"read:r to c"<<dlen<<endl;
      if (dlen < 0) {
        isAlive = false;
        cout << "diedi" << endl;
        break;
      }
      if (dlen > 0) {
        dlen = sendtoClient(c, buffer, dlen);
        //cout<<"r:"<<dlen<<endl;
        if (dlen < 0) {
          isAlive = false;
          outs("wrirting diedi");
          break;

        }
      }
      if (dlen == 0)
        std::this_thread::sleep_for(std::chrono::seconds(1));
      //  outs("sleeping r to c");
      // std::this_thread::yield(); 
    }
  }

  void relay_usingpoll(int fd0, int fd1) {
    int timeout;

    int nfds = 2, current_size = 0, i, j, len;
    struct pollfd fds[2];
    int rc, readerfd, writerfd,conn_close=0;
    char buffer[BUFSIZE];
    
    //initalising poll
    memset(fds, 0, sizeof(fds));
    fds[0].fd = fd0;
    fds[0].events = POLLIN;
    fds[1].fd = fd1;
    fds[1].events = POLLIN;
    timeout = (3 * 60 * 1000);
    do {
      // outs("waiting on poll");
      rc = poll(fds, nfds, timeout);
      if (rc < 0) {
        perror("  poll() failed");
        break;
      }
      if (rc == 0) {
        outs("  poll() timed out.  End program.\n");
        break;
      }

      if (fds[0].revents == 0) {
        readerfd = 1;
        writerfd = 0;
        //  outs("read from server");
      } else {
        readerfd = 0;
        writerfd = 1;
        // outs("read from clienr");
      }

      rc = read(fds[readerfd].fd, buffer, sizeof(buffer));
      //  rc=recv(fds[readerfd].fd,buffer,sizeof(buffer),0);    
      if (rc < 0) {
        if (errno != EWOULDBLOCK) {

          outs("  rcv failed\n");
          conn_close=1; 
        //  close(fds[readerfd].fd);
          //close_conn = TRUE;
        }
        break;
      }
      if (rc == 0) {
        outs("  Connection closed\n");
        //close_conn = TRUE;
       conn_close=1;
        break;
      }

      /*****************************************************/
      /* Data was received                                 */
      /*****************************************************/
      len = rc;
      //  printf("  %d bytes received\n", len);

      /*****************************************************/
      /* Echo the data back to the client                  */
      /*****************************************************/
      rc = write(fds[writerfd].fd, buffer, len);
      // rc = send(fds[writerfd].fd, buffer, len, 0);
      if (rc < 0) {
        outs("  send() failed");
        close(fds[writerfd].fd);
        conn_close = 1;
        break;
      }

    } while (1);
    if(conn_close){
   close(fds[readerfd].fd);
   close(fds[writerfd].fd);
    
    }

  }
  void relay_pipe(int fd0, int fd1) {
    int maxfd, ret;
    fd_set rd_set;
    size_t nread;
    char buffer_r[BUFSIZE];

    outs("Connecting two sockets");
    //init
    maxfd = (fd0 > fd1) ? fd0 : fd1;
    while (1) {
      FD_ZERO( & rd_set);
      FD_SET(fd0, & rd_set);
      FD_SET(fd1, & rd_set);
      ret = select(maxfd + 1, & rd_set, NULL, NULL, NULL);

      if (ret < 0 && errno == EINTR) {
        continue;
      }

      if (FD_ISSET(fd0, & rd_set)) {
        nread = recv(fd0, buffer_r, BUFSIZE, 0);
        if (nread <= 0)
          break;
        send(fd1, (const void * ) buffer_r, nread, 0);
      }

      if (FD_ISSET(fd1, & rd_set)) {
        nread = recv(fd1, buffer_r, BUFSIZE, 0);
        if (nread <= 0)
          break;
        send(fd0, (const void * ) buffer_r, nread, 0);
      }
    }
  }

  void relay(int client, int remote) {
    relay_usingpoll(client, remote);
    // relay_pipe(client, remote);
    return;

    try {
      std::thread t1 = clientToRemoteT(client, remote);

      // cout<<"t1 detached"<<endl;
      std::thread t2 = remoteToClientT(remote, client);
      t1.detach();
      t2.detach();
      outs("tbot detached");
      //cout<<"t2 detached"<<endl;

    } catch (std::exception & m) {
      cout << "error at realy" << endl;
    }

  }
  int sendToServer(int socket, char * buffer, int len) {
    int n = 0;
    try {
      //   cout<<"writing to "<<socket<<" data length"<<len;
      n = write(socket, buffer, len);

    } catch (std::exception & msg) {
      cout << "error";
    }
    return n;
  }

  char getByte(int socket) {
    char buf[1], n;
    if (socket > 0) {
      n = read(socket, buf, 1);
      //cout<<"no. of bytes read"<<n<<std::endl;
      if (n > 0)
        return buf[0];
      else
        return 0x00;
    }
  }
  int sendtoClient(int socket, char * buffer, int len) {
    int n = 0;
    try {
      //cout<<"writing to cl "<<socket<<" data length"<<len;
      n = write(socket, buffer, len);
    } catch (char * msg) {
      cout << "error cli";
    }
    //cout<<"wrote to "<<socket<<" data length"<<n;
    return n;
  }
  bool getClientCommand(int client) {
    bool flag = false;
    char DST_Addr[256];
    char IP[8];
    char DST_Port[2];
    int addr_Len, n;
    int ADDR_Size[] = {
      -1,
      4,
      -1,
      -1,
      16
    };
    char SOCKS_Version = getByte(client);
    char socksCommand = getByte(client);
    char RSV = getByte(client);
    char ATYP = getByte(client);

    int remotesocket = 0;
    addr_Len = ADDR_Size[ATYP];
    DST_Addr[0] = getByte(client);
    //outs("get addre");
    if (ATYP == 0x03) {
      addr_Len = DST_Addr[0] + 1;
      //outs("DOMAIN");
      
    }
    int i;
    char buff[256];
    memset( & buff, 0, sizeof(buff));
    for (i = 1; i < addr_Len; i++) {
      DST_Addr[i] = getByte(client);
      buff[i-1]=DST_Addr[i];
    //  printf("%c",DST_Addr[i]);
      
      
    }
    

    DST_Port[0] = getByte(client);
    DST_Port[1] = getByte(client);
   // outs("port copied ");
    if (SOCKS_Version != 0x05) {

      refuseCommand(client, 0xFF);
      return false;
    }
    if (socksCommand < 0x01 || socksCommand > 0x03) {
      refuseCommand(client, 0x07);
      return false;
    }
    if (ATYP == 0x04) {
      refuseCommand(client, 0x08);
      return false;
    }
    if ((ATYP >= 0x04) || (ATYP <= 0)) {

      refuseCommand(client, 0x08);
      return false;
    }
  if(ATYP!=0x03)
        calculateIP(client, IP, DST_Addr,ATYP);
        else
            calculateIP(client, IP, buff,ATYP);
    
    if (socksCommand == 0x03) {
      outs("its a udp req");

      createUDP(IP, calcPort(DST_Port[0], DST_Port[1]));

    }

    remotesocket = connectToServer(IP, calcPort(DST_Port[0], DST_Port[1]));
    if (remotesocket > 0) {
      // cout<<"Remote socket connected"<<endl;
      //replying to client
      char r[10];
      r[0] = 0x05;
      r[1] = 0x00;
      r[2] = 0x00;
      r[3] = 0x01;
      r[4] = 0x00;
      r[5] = 0x00;
      r[6] = 0x00;
      r[7] = 0x00;
      r[8] = (char)((1081 & 0xFF00) >> 8); // Port High
      r[9] = (char)(1081 & 0x00FF); // Port Low
      n = sendtoClient(client, r, sizeof(r));
      // cout<<"relpied writes"<<n<<endl;
      relay(client, remotesocket);
      return true;
    }
    return false;

  }
  
  
  bool createUDP(char * ip, int port) {
    int udpsockfd, remoteudpsock;
    char buffer[2048];
    int len;
   
    struct sockaddr_in servaddr, remoteaddr, foo;

    // Creating socket file descriptor 
    if ((udpsockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
      perror("socket creation failed");
      return false;
      //  exit(EXIT_FAILURE); 
    }
    int tryvalue = 100, i = 0;
    memset( & servaddr, 0, sizeof(servaddr));
    // memset(&cliaddr, 0, sizeof(cliaddr)); 

    // Filling server information 
    servaddr.sin_family = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = INADDR_ANY;

    srand(time(NULL));
    int number;
    while (i < tryvalue) {

      number = ((rand() % 65535) + 1);
      servaddr.sin_port = htons(number);
      //struct sockaddr_in to;
      //  memset(&to, 0, sizeof(to));
      //  to.sin_family = AF_INET;
      //to.sin_addr   = inet_pton();
      /* if(inet_pton(AF_INET, "129.5.24.1", &to.sin_addr)<=0)  
          { 
              printf("\nInvalid address/ Address not supported \n"); 
              return -1; 
          }*/
      /*to.sin_port   = htons(80);
      int st=sendto(udpsockfd, data_sent, sizeof(data_sent), 0,
                 (struct sockaddr*)&to, sizeof(to));*/
      i++;
      //printf("randome number:=%d ", number);

      // Bind the socket with the server address 
      if (bind(udpsockfd, (const struct sockaddr * ) & servaddr,
          sizeof(servaddr)) < 0) {
        perror("bind failed");
        outs("udp bind failed let me try again");
        continue;
        //  exit(EXIT_FAILURE); 
      } else
        break;
      // struct sockaddr foo;

    }
  }
  //void processUdp(int sock,sockaddr_in client,)
  int connectToServer(char * ip, int port) {
    int sock;
    char * p;
   // outs(ip);
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      printf("\n Socket creation error \n");
      return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    //printf("port=%d", port);
    //outs(p);

    // Convert IPv4 and IPv6 addresses from text to binary form 
    if (inet_pton(AF_INET, ip, & serv_addr.sin_addr) <= 0) {
      printf("\nInvalid address/ Address not supported \n");
      return -1;
    }
    try {
      if (connect(sock, (struct sockaddr * ) & serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
      }

    } catch (char * msg) {
      cout << "error";
    }
    //cout << "connected" << endl;
    return sock;

  }
  int byte2int(char b) {
    int res = b;
    if (res < 0) res = (int)(0x100 + res);
    return res;
  }
  int calcPort(char Hi, char Lo) {

    return ((byte2int(Hi) << 8) | byte2int(Lo));
  }
 void calculateIP(int client, char * p, char * addr,int type) {
      
      
    int i;
    struct hostent *host_entry; 
    char *hostbuffer; 
    
    
    if(type==DOMAIN)
    {
    
     host_entry=   gethostbyname(addr);
     if(host_entry==NULL)
         outs("host entry null");
         else
    hostbuffer=inet_ntoa(*((struct in_addr*) 
                           host_entry->h_addr_list[0])); 
                           
         
                    sprintf(p,"%s",hostbuffer);
                           
     // outs(p);
// return p;
        
    }else
    {
    try {
      sprintf(p, "%d.%d.%d.%d", byte2int(addr[0]), byte2int(addr[1]), byte2int(addr[2]), byte2int(addr[3]));
     // std::cout << "IP:" << p;
    } catch (char * m) {
      std::cout << "error calucip" << endl;
    }
    }
    
  }

  void refuseCommand(int socket, char code) {
    char buf[10];
    buf[0] = 0x05;
    buf[1] = code;

    write(socket, buf, 10);
  }
  std::thread handleThread(int c) {
    return std::thread([ = ] {
      handleClient(c);
    });
  }
  std::thread clientToRemoteT(int c, int r) {
    return std::thread([ = ] {
      clientToRemote(c, r);
    });
  }
  std::thread remoteToClientT(int r, int c) {
    return std::thread([ = ] {
      remoteToClient(r, c);
    });
  }

};

//class for main server
class ServerListener {

  public:
    bool isValid;
  void runServer(int port) {
    int i, server_fd, conn_num = 0;
    struct sockaddr_in address;

    int addrlen = sizeof(address);

    // Creating socket file descriptor 
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
      perror("socket failed");
      exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080 
    /* if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                   &opt, sizeof(opt))) 
     { 
         perror("setsockopt"); 
         exit(EXIT_FAILURE); 
     } */
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Forcefully attaching socket to the port 8080 
    if (bind(server_fd, (struct sockaddr * ) & address,
        sizeof(address)) < 0) {
      perror("bind failed");
      exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 5) < 0) {
      perror("listen");
      cout << "listen error";
      exit(EXIT_FAILURE);
    }
    isValid = true;
    std::cout << "....Sock5 Server.... " << std::endl;
    while (isValid) {

      int new_socket;
      if ((new_socket = accept(server_fd, (struct sockaddr * ) & address,
          (socklen_t * ) & addrlen)) < 0) {
        perror("accept error");
        cout << "accept error";
        exit(EXIT_FAILURE);
      }
      HandleClient clientHandler;
      std::thread t = clientHandler.handleThread(new_socket);
      t.detach();
      // cout<<"connection number"<<conn_num++<<endl;

    }

    outs("server loop ending");
    try {
      close(server_fd);
    } catch (char * msg) {
      std::cout << "Error closing sockewt" << msg;
    }

  }
  std::thread mainThread(int port) {
    return std::thread([ = ] {
      runServer(port);
    });
  }

};

void segfault(int signal, siginfo_t * si, void * arg) {
  printf("caught");
  exit(0);
}
void sig_handler(int signum) {
  std::cerr << "error=" << signum;
}
int main(int argc, char ** argv) {
  struct sigaction sa;

  try {
    int x = 0;
    int port = 9050;
    sockets sokc = {
      12,
      13
    };
    char q;
    ServerListener server;
    //memset(&sa,0,sizeof(struct sigaction))
    //  sig
    printf("Sock5 server implementation\n");
    /* this variable is our reference to the second thread */
    //SIG_IGN
    signal(SIGPIPE, sig_handler);
    // server.runServer();
    std::thread mainThread = server.mainThread(port);
    mainThread.detach();
    while (q != 'q') {
      cin >> q;

    }
    std::cout << "program exitiing" << endl;
    server.isValid = false;

  } catch (std::exception & e) {
    cout << "unknmmnwdw error";
  }
  return 0;
}