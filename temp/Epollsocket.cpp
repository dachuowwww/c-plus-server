#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "include/error.h"
using namespace std;

//EPOLL_CLOEXEC
struct sockaddr_in serv_addr;  
struct sockaddr_in clnt_addr;
socklen_t clnt_addr_len = sizeof(clnt_addr);
const int MAX_EVENTS = 10;

int main(){
    bzero(&serv_addr, sizeof(serv_addr));
    bzero(&clnt_addr, sizeof(clnt_addr));

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    errif(sockfd == -1, "socket create error");
    serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    serv_addr.sin_port = htons(8888);
    
    errif(bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1, "socket bind error");

    int epfd = epoll_create1(0);
    struct epoll_event events[MAX_EVENTS], ev;

    ev.events = EPOLLIN;    //在代码中使用了ET模式，且未处理错误，在day12进行了修复，实际上接受连接最好不要用ET模式
    ev.data.fd = sockfd;    //该IO口为服务器socket fd
    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);    //将服务器socket fd添加到epoll
    while(true){    // 不断监听epoll上的事件并处理
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);   //有nfds个fd发生事件
        for(int i = 0; i < nfds; ++i){  //处理这nfds个事件
            if(events[i].data.fd == sockfd){    //发生事件的fd是服务器socket fd，表示有新客户端连接
                int clnt_sockfd = accept(sockfd, (sockaddr*)&clnt_addr, &clnt_addr_len);
                errif(clnt_sockfd == -1, "socket accept error");
                char ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &clnt_addr.sin_addr, ip_str, sizeof(ip_str));
                cout << "new client fd " << clnt_sockfd << "! IP: " << ip_str <<" Port:" << ntohs(clnt_addr.sin_port) << endl;
                ev.data.fd = clnt_sockfd; 
                ev.events = EPOLLIN | EPOLLET;  //对于客户端连接，使用ET模式，可以让epoll更加高效，支持更多并发
                fcntl(clnt_sockfd, F_SETFL, fcntl(clnt_sockfd, F_GETFL) | O_NONBLOCK);
                epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sockfd, &ev);   //将该客户端的socket fd添加到epoll
            } else if(events[i].events & EPOLLIN){      //发生事件的是客户端，并且是可读事件（EPOLLIN）
                char buf[1024]; 
                while(true){    //由于使用非阻塞IO，需要不断读取，直到全部读取完毕
                    ssize_t bytes_read = read(events[i].data.fd, buf, sizeof(buf));
                    if(bytes_read > 0){
                        cout<<"client fd "<<events[i].data.fd<<"message : "<<buf<<endl ;  
                        write(events[i].data.fd, buf, sizeof(buf));           //将相同的数据写回到客户端
                    } else if(bytes_read == -1 && errno == EINTR){  //客户端正常中断、继续读取
                        continue;
                    } else if(bytes_read == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))){//非阻塞IO，这个条件表示数据全部读取完毕
                        //该fd上数据读取完毕
                        break;
                    } else if(bytes_read == 0){  //EOF事件，一般表示客户端断开连接
                        cout<<"client fd "<<events[i].data.fd <<"disconnected"<<endl ;
                        close(events[i].data.fd);   //关闭socket会自动将文件描述符从epoll树上移除
                        break;
                    }else if(bytes_read == -1){  //EOF事件，一般表示客户端断开连接
                        errif(true, "socket read error");
                        close(events[i].data.fd);   //关闭socket会自动将文件描述符从epoll树上移除
                        break;
                    }
                }
            }
         }
    }
    return 0;
 }
    