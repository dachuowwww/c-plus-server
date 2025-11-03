#include <sys/socket.h>
#include <arpa/inet.h>

//EPOLL_CLOEXEC
struct sockaddr_in serv_addr;  
struct sockaddr_in clnt_addr;
socklen_t clnt_addr_len = sizeof(clnt_addr);

int main(){
    bzero(&serv_addr, sizeof(serv_addr));
    bzero(&clnt_addr, sizeof(clnt_addr));

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    errif(sockfd == -1, "socket create error");
    serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    serv_addr.sin_port = htons(8888);
    errif(bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1, "socket bind error");

    errif(listen(sockfd, SOMAXCONN) == -1, "socket listen error");
    int clnt_sockfd = accept(sockfd, (sockaddr*)&clnt_addr, &clnt_addr_len);
    errif(clnt_sockfd == -1, "socket accept error");
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clnt_addr.sin_addr, ip_str, sizeof(ip_str));
    cout << "new client fd " << clnt_sockfd << "! IP: " << ip_str <<" Port:" << ntohs(clnt_addr.sin_port) << endl;
    while (true) {
        char buf[1024];     //定义缓冲区
        bzero(&buf, sizeof(buf));       //清空缓冲区
        ssize_t read_bytes = read(clnt_sockfd, buf, sizeof(buf)); //从客户端socket读到缓冲区，返回已读数据大小
        if(read_bytes > 0){
            cout<<"message from client fd "<< clnt_sockfd<<": "<<buf<<endl ;  
            write(clnt_sockfd, buf, sizeof(buf));           //将相同的数据写回到客户端
        } else if(read_bytes == 0){             //read返回0，表示EOF
            cout<<"client fd "<<clnt_sockfd <<"disconnected"<<endl ;
            close(clnt_sockfd);
            break;
        } else if(read_bytes == -1){        //read返回-1，表示发生错误，按照上文方法进行错误处理
            close(clnt_sockfd);
            errif(true, "socket read error");
        }
    }
    return 0;
}

