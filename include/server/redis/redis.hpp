#ifndef REDIS_H
#define REDIS_H

#include<hiredis/hiredis.h>
#include<thread>
#include<functional>
using namespace std;

class redis
{
private:
    //hiredis同步上下文对象，负责publish消息
    redisContext *_publish_context;

    //hiredis同步上下文对象，负责subscribe消息          留有两个上下文是因为subscribe后，该上下文占用，不能发消息，需要另外一个空闲的上下文publish
    redisContext *_subscribe_context;

    //回调操作，收到订阅的消息，给service层上报
    function<void(int,string)> _notify_message_handler;
public:
    redis();
    ~redis();

    //链接redis服务器
    bool connect();

    //向redis指定的通道channel发布消息
    bool publish(int channel,string msg);

    //向redis指定的通道subscribe订阅消息
    bool subscribe(int channel);

    //向redis指定的通道unsubscribe取消订阅消息
    bool unsubscribe(int channel);

    //在独立线程中接受订阅通道中的消息
    void observe_channel_message();

    //初始化向业务层上报通道消息的回调对象
    void init_notify_handler(function<void(int,string)> fn);
};





#endif