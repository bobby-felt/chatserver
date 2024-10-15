#include"redis.hpp"
#include<iostream>
using namespace std;

redis::redis():_publish_context(nullptr),_subscribe_context(nullptr)
{

}

redis::~redis()
{
    if(_publish_context!=nullptr) redisFree(_publish_context);
    if(_subscribe_context!=nullptr) redisFree(_subscribe_context);
}

//链接redis服务器
bool redis::connect()
{
    //fuzepublish发布消息的上下文链接
    _publish_context=redisConnect("127.0.0.1",6379);
    if(_publish_context==nullptr)
    {
        cerr<<"connect redis failed!"<<endl;
        return false;
    }

    //负责subscribe订阅消息的上下文链接
    _subscribe_context=redisConnect("127.0.0.1",6379);
    if(_subscribe_context==nullptr)
    {
        cerr<<"connect redis failed!"<<endl;
        return false;
    }

    //在单独的线程中，监听通道上的事件，有消息给业务层上报
    thread t([&](){
        observe_channel_message();
    });
    t.detach();

    cout<<"connect redis-server success!"<<endl;
    return true;
}

//向redis指定的通道channel发布消息
bool redis::publish(int channel,string msg)
{
    redisReply *reply=(redisReply *)redisCommand(_publish_context,"PUBLISH %d %s",channel,msg.c_str());
    if(nullptr==reply)
    {
        cerr<<"publish command failed!"<<endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

//向redis指定的通道subscribe订阅消息
bool redis::subscribe(int channel)
{
    //直接用redisCommand()会造成通道阻塞，redisCommand()包含了redisAppendCommand(),redisBufferWrite(),redisGetR eply()三个函数，最后一个属于等待阻塞函数
    if(REDIS_ERR==redisAppendCommand(this->_subscribe_context,"SUBSCRIBE %d",channel))//把命令写到本地缓存
    {
        cerr<<"subscribe command failed!"<<endl;
        return false;
    }
    //redisBufferWrite把命令从缓存区，发送到redisserver
    int done=0;
    while(!done)
    {
        if(REDIS_ERR==redisBufferWrite(this->_subscribe_context,&done))
        {
            cerr<<"subscribe command failed!"<<endl;
            return false;
        }
    }

    return true;
}

//向redis指定的通道unsubscribe取消订阅消息
bool redis::unsubscribe(int channel)
{
    //直接用redisCommand()会造成通道阻塞，redisCommand()包含了redisAppendCommand(),redisBufferWrite(),redisGetR eply()三个函数，最后一个属于等待阻塞函数
    if(REDIS_ERR==redisAppendCommand(this->_subscribe_context,"SUBSCRIBE %d",channel))//把命令写到本地缓存
    {
        cerr<<"unsubscribe command failed!"<<endl;
        return false;
    }
    //redisBufferWrite把命令从缓存区，发送到redisserver
    int done=0;
    while(!done)
    {
        if(REDIS_ERR==redisBufferWrite(this->_subscribe_context,&done))
        {
            cerr<<"unsubscribe command failed!"<<endl;
            return false;
        }
    }

    return true;
}

//在独立线程中接受订阅通道中的消息
void redis::observe_channel_message()
{
    redisReply *reply=nullptr;
    while(REDIS_OK==redisGetReply(this->_subscribe_context,(void **)&reply))
    {
        //订阅收到的消息是一个三个元素的数组
        if(reply!=nullptr&&reply->element[2]!=nullptr&&reply->element[2]->str!=nullptr)
        {
            //给业务层上报通道上收到的消息
            _notify_message_handler(atoi(reply->element[1]->str),reply->element[2]->str);
        }
        freeReplyObject(reply);
    }

    cerr<<">>>>>>>>>>>>>>>>>>observe_channel_message quit<<<<<<<<<<<<<<<<<<<<<<<<<"<<endl;
}

//初始化向业务层上报通道消息的回调对象
void redis::init_notify_handler(function<void(int,string)> fn)
{
    this->_notify_message_handler=fn;
}