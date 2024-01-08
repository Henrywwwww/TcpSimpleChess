#include <iostream>
#include <sstream>
#include <string>
#include<stdio.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string.h>
#include<unistd.h>
#define BACKLOG 5     //完成三次握手但没有accept的队列的长度
#define CONCURRENT_MAX 8   //应用层同时可以处理的连接
#define SERVER_PORT 11332
#define BUFFER_SIZE 1024
#define QUIT_CMD ".quit"
int client_fds[CONCURRENT_MAX];

//Tic Tac Toe
char chess[3][3] ;

int main(int argc, const char * argv[])
{

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            chess[i][j]='*';
        }
    }

    char input_msg[BUFFER_SIZE];
    char recv_msg[BUFFER_SIZE];
    //本地地址
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    bzero(&(server_addr.sin_zero), 8);
    //创建socket
    int server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_sock_fd == -1)
    {
        perror("socket error");
        return 1;
    }
    //绑定socket
    int bind_result = bind(server_sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(bind_result == -1)
    {
        perror("bind error");
        return 1;
    }
    //listen
    if(listen(server_sock_fd, BACKLOG) == -1)
    {
        perror("listen error");
        return 1;
    }
    //fd_set
    fd_set server_fd_set;
    int max_fd = -1;
    struct timeval tv;  //超时时间设置
    while(1)
    {
        tv.tv_sec = 20;
        tv.tv_usec = 0;
        FD_ZERO(&server_fd_set);
        FD_SET(STDIN_FILENO, &server_fd_set);
        if(max_fd <STDIN_FILENO)
        {
            max_fd = STDIN_FILENO;
        }
        //printf("STDIN_FILENO=%d\n", STDIN_FILENO);
    //服务器端socket
        FD_SET(server_sock_fd, &server_fd_set);
       // printf("server_sock_fd=%d\n", server_sock_fd);
        if(max_fd < server_sock_fd)
        {
            max_fd = server_sock_fd;
        }
    //客户端连接
        for(int i =0; i < CONCURRENT_MAX; i++)
        {
            //printf("client_fds[%d]=%d\n", i, client_fds[i]);
            if(client_fds[i] != 0)
            {
                FD_SET(client_fds[i], &server_fd_set);
                if(max_fd < client_fds[i])
                {
                    max_fd = client_fds[i];
                }
            }
        }
        int ret = select(max_fd + 1, &server_fd_set, NULL, NULL, &tv);
        if(ret < 0)
        {
            perror("select 出错\n");
            continue;
        }
        //else if(ret == 0)
        //{
        //    printf("select 超时\n");
        //    continue;
        //}
        else
        {
            //ret 为未状态发生变化的文件描述符的个数
            if(FD_ISSET(STDIN_FILENO, &server_fd_set))
            {
                printf("发送消息：\n");
                bzero(input_msg, BUFFER_SIZE);
                fgets(input_msg, BUFFER_SIZE, stdin);
                //输入“.quit"则退出服务器
                if(strcmp(input_msg, "quit") == 0)
                {
                    exit(0);
                }
                for(int i = 0; i < CONCURRENT_MAX; i++)
                {
                    if(client_fds[i] != 0)
                    {
                        printf("client_fds[%d]=%d\n", i, client_fds[i]);
                        send(client_fds[i], input_msg, BUFFER_SIZE, 0);
                    }
                }
            }
            if(FD_ISSET(server_sock_fd, &server_fd_set))
            {
                //有新的连接请求
                struct sockaddr_in client_address;
                socklen_t address_len;
                int client_sock_fd = accept(server_sock_fd, (struct sockaddr *)&client_address, &address_len);
                printf("new connection client_sock_fd = %d\n", client_sock_fd);
                if(client_sock_fd > 0)
                {
                    int index = -1;
                    for(int i = 0; i < CONCURRENT_MAX; i++)
                    {
                        if(client_fds[i] == 0)
                        {
                            index = i;
                            client_fds[i] = client_sock_fd;
                            break;
                        }
                    }
                    if(index >= 0)
                    {
                        std::string player="客户端";
                        std::string number =std::to_string(index);
                        std::string combinedMsg = player + "(" + number+")"+"加入成功!";
                        printf("新客户端(%d)加入成功 %s:%d\n", index, inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
                        send(client_sock_fd, combinedMsg.c_str(), combinedMsg.length(), 0);
                    }
                    else
                    {
                        bzero(input_msg, BUFFER_SIZE);
                        strcpy(input_msg, "服务器加入的客户端数达到最大值,无法加入!\n");
                        send(client_sock_fd, input_msg, BUFFER_SIZE, 0);
                        printf("客户端连接数达到最大值，新客户端加入失败 %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
                    }
                }
            }
            for(int i =0; i < CONCURRENT_MAX; i++)
            {
                if(client_fds[i] !=0)
                {
                    if(FD_ISSET(client_fds[i], &server_fd_set))
                    {
                        //处理某个客户端过来的消息
                        bzero(recv_msg, BUFFER_SIZE);
                        long byte_num = recv(client_fds[i], recv_msg, BUFFER_SIZE, 0);
                        if (byte_num > 0)
                        {
                            if(byte_num > BUFFER_SIZE)
                            {
                                byte_num = BUFFER_SIZE;
                            }
                            recv_msg[byte_num] = '\0';
                            printf("客户端(%d):%s\n", i, recv_msg);


                            int a, b;
                            a = recv_msg[0] - '0';
                            b = recv_msg[2] - '0';

                            // 提取逗号前后的数字
                            if ((a>=0&&a<=2)&&(b>=0&&b<=2)) {
                              if(chess[a][b]=='O'||chess[a][b]=='X'){
                                std::cerr << "Failed to extract numbers." << std::endl;
                                std::string wrong;
                                wrong ="Wrong input!!!\n";
                                send(client_fds[i], wrong.c_str(), wrong.length(), 0);
                              }else{
                               if(i==0){
                                 chess[a][b]='O';
                               }else{
                                 chess[a][b]='X';
                               }

                            std::string player;
                            recv_msg[byte_num] = '\0';
                            printf("客户端(%d):%s\n", i, recv_msg);

                            std::string turn;
                            turn ="your turn ,Start inputting!!!!!!\n";
                            std::string ban;
                            ban ="Not your turn ,Stop inputting!!!!!!\n";
                            send(client_fds[i], ban.c_str(), ban.length(), 0);

                            if (i == 1) {
                                //player = "客户端(1)";
                                send(client_fds[0], turn.c_str(), turn.length(), 0);
                            } else {
                                //player = "客户端(0)";
                                send(client_fds[1], turn.c_str(), turn.length(), 0);
                            }

                            // 向其他玩家发送信息
                            for (int j = 0; j < CONCURRENT_MAX; j++) {
                                if (client_fds[j] != 0 ) {
                                    // 将 player 和 recv_msg 连接在一起
                                    std::string combinedMsg = player + " " + recv_msg;
                                    send(client_fds[j], combinedMsg.c_str(), combinedMsg.length(), 0);
                                }
                            }

                            //_______zhe li you wen ti_____________________________________________________________________
                            for (int j = 0; j < CONCURRENT_MAX; j++) {
                                if (client_fds[j] != 0 ) {
                                    for (int x = 0; x < 3; x++) {
                                    std::string clientchess = "\n" +std::string(1,chess[x][0]) + std::string(1,chess[x][1])  +std::string(1,chess[x][2]) + "\n";
                                    send(client_fds[j], clientchess.c_str(), clientchess.length(), 0);
                                    }
                                }
                            }

                              }
                            }else{
                               std::cerr << "Failed to extract numbers." << std::endl;
                               std::string wrong;
                               wrong ="Wrong input!!!\n";
                               send(client_fds[i], wrong.c_str(), wrong.length(), 0);
                            }



                            //打印棋盘测试
                            for (int i = 0; i < 3; ++i) {
                                for (int j = 0; j < 3; ++j) {
                                     std::cout << chess[i][j] << ' ';
                                }
                                std::cout << std::endl;  // 在每行末尾添加换行符
                            }

                            int result=-1;
                            //判断胜利逻辑
                            for(int i=0;i<3;i++){
                               if(chess[i][0]=='O'&&chess[i][1]=='O'&&chess[i][2]=='O'){
                                 result=0;
                               }
                               if(chess[0][i]=='O'&&chess[1][i]=='O'&&chess[2][i]=='O'){
                                 result=0;
                               }

                               if(chess[i][0]=='X'&&chess[i][1]=='X'&&chess[i][2]=='X'){
                                 result=1;
                               }
                               if(chess[0][i]=='X'&&chess[1][i]=='X'&&chess[2][i]=='X'){
                                 result=1;
                               }
                            }
                            //斜向判断条件
                               if(chess[0][0]=='O'&&chess[1][1]=='O'&&chess[2][2]=='O'){
                                 result=0;
                               }
                               if(chess[0][2]=='O'&&chess[1][1]=='O'&&chess[2][0]=='O'){
                                 result=0;
                               }

                               if(chess[0][0]=='X'&&chess[1][1]=='X'&&chess[2][2]=='X'){
                                 result=1;
                               }
                               if(chess[0][2]=='X'&&chess[1][1]=='X'&&chess[2][0]=='X'){
                                 result=1;
                               }


                            if(result==1){
                               char res[]="用户端(1)获取胜利";
                               for (int j = 0; j < CONCURRENT_MAX; j++)
                               {
                                 if (client_fds[j] != 0 )
                                 {
                                  send(client_fds[j],res , strlen(res), 0);
                                 }
                               }
                               exit(0);
                            }
                            if(result==0){
                               char res[]="用户端(0)获取胜利";
                               for (int j = 0; j < CONCURRENT_MAX; j++)
                               {
                                 if (client_fds[j] != 0 )
                                 {
                                  send(client_fds[j],res , strlen(res), 0);
                                 }
                               }
                               exit(0);
                            }

                        }
                        else if(byte_num < 0)
                        {
                            printf("从客户端(%d)接受消息出错.\n", i);
                        }
                        else
                        {
                            FD_CLR(client_fds[i], &server_fd_set);
                            client_fds[i] = 0;
                            printf("客户端(%d)退出了\n", i);
                        }
                    }
                }
            }
        }
    }
    return 0;
}
