#include"chatserver.hpp"
#include"chatservice.hpp"
#include<iostream>
#include<signal.h>

using namespace std;

//处理服务器ctrl+c结束后，重置user的状态信息
void resetHandler(int)
{
    ChatService::instance()->reset();
}

int main(int argc,char **argv){
    signal(SIGINT,resetHandler);        //看看什么意思signal(SIGINT，这两个   signal(),接收一个信号，做另外的一个处理    SIGINT：ctrl+c信号

    if(argc<3)
    {
        cerr<<"command invalid! example: ./ChatServer 127.0.0.1 6000"<<endl;
        exit(-1);
    }

    //解析通过命令行参数传递的ip和port
    char *ip=argv[1];
    uint16_t port = atoi(argv[2]);

    EventLoop loop;
    InetAddress addr(ip,port);
    ChatServer server(&loop,addr,"chatserver");

    server.start();
    loop.loop();

    return 0;
}