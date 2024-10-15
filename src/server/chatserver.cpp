#include "chatserver.hpp"
#include "chatservice.hpp"
#include "json.hpp"
#include<functional>
using namespace std;
using namespace placeholders;
using json = nlohmann::json;

//初始化聊天服务器对象
ChatServer::ChatServer(EventLoop* loop,
                const InetAddress& listenAddr,
                const string& nameArg)
                :_server(loop,listenAddr,nameArg),_loop(loop)
    {
        //注册链接回调
        _server.setConnectionCallback(std::bind(&ChatServer::onconnection,this,_1));

        //注册消息回调
        _server.setMessageCallback(std::bind(&ChatServer::onmessage,this,_1,_2,_3));

        //设置线程数量
        _server.setThreadNum(4);
    }

//启动服务
void ChatServer::start()
{
    _server.start();
}


//上报相关链接信息的回调函数
void ChatServer::onconnection(const TcpConnectionPtr &conn)
{
    //用户断开连接
    if(!conn->connected()){
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }

}

//上报读写事件相关信息的回调函数
void ChatServer::onmessage(const TcpConnectionPtr &conn,
                Buffer *bufffer,
                Timestamp time)
{
    string buf=bufffer->retrieveAllAsString();
    json js=json::parse(buf);
    //目的：完全解耦网络模块的代码和业务模块的代码

    //通过js["msgid"]获取业务处理器handler
    auto msgHandler =  ChatService::instance()->getHandler(js["msgid"].get<int>());
    //回调消息绑定好的事件处理器
    msgHandler(conn,js,time);

}