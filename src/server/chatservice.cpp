#include "chatservice.hpp"
#include"public.hpp"
#include<muduo/base/Logging.h>
#include<vector>
using namespace muduo;
using namespace std;
//#include<iostream>


//获取单例对象的接口函数
ChatService* ChatService::instance(){
    static ChatService service;
    return &service;
}

//注册消息以及对应的Handler回调操作
ChatService::ChatService()
{
    _msgHandlermap.insert({LOGIN_MSG,std::bind(&ChatService::login,this,_1,_2,_3)});
    _msgHandlermap.insert({REG_MSG,std::bind(&ChatService::reg,this,_1,_2,_3)});
    _msgHandlermap.insert({ONE_CHAT_MSG,std::bind(&ChatService::onechat,this,_1,_2,_3)});
    _msgHandlermap.insert({ADD_FRIEND_MSG,std::bind(&ChatService::addfriend,this,_1,_2,_3)});
    _msgHandlermap.insert({CREAT_GROUP_MSG,std::bind(&ChatService::creatGroup,this,_1,_2,_3)});
    _msgHandlermap.insert({ADD_GROUP_MSG,std::bind(&ChatService::addGroup,this,_1,_2,_3)});
    _msgHandlermap.insert({GROUP_CHAT_MSG,std::bind(&ChatService::groupChat,this,_1,_2,_3)});
    _msgHandlermap.insert({LOGINOUT_MSG,std::bind(&ChatService::loginout,this,_1,_2,_3)});

    //链接redis服务器
    if(_redis.connect())
    {
        //设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage,this,_1,_2));
    }
}

//获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    //记录错误日志，msgid没有对应的事件处理回调
    auto it=_msgHandlermap.find(msgid);
    if(it==_msgHandlermap.end())
    {
        //返回一个默认的处理器，空操作
        return [=](const TcpConnectionPtr &conn,json &js,Timestamp time)
        {
            LOG_ERROR <<"msgid:"<<msgid<<"can not find handler!";
        };
    }
    else
    {
        return _msgHandlermap[msgid]; 
    }   
}

//处理登录业务   id   pwd
void ChatService::login(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int id=js["id"];
    string pwd = js["password"];
    User user = _usermodel.query(id);
    if(user.getid()!=-1 && user.getpwd()==pwd)
    {
        if(user.getstate()=="online")
        {
            //该用户已登录，禁止重复登录
            //登录失败
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2; 
            response["errmsg"] = "重复登录，登录失败";
            conn->send(response.dump());
        }
        else
        {
            //登陆成功，更新用户状态信息
            user.setstate("online");
            _usermodel.update(user);

            {
            lock_guard<mutex> lock(_connMutex);
            _userConnMap.insert({id,conn});     //记录用户的连接信息
            }

            //id用户登录成功后，向redis订阅channel(id)
            _redis.subscribe(id);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getid();
            response["name"] = user.getname();
            //查询用户是否有离线消息
            vector<string> vec=_offlinemsgmodel.query(id);
            if(!vec.empty())
            {
                response["offlinemsg"] = vec;
                _offlinemsgmodel.remove(id);    //读取离线消息后，将该用户的离线消息删除
            }
            //查询该用户的好友信息并返回
            vector<User> frivec=_friendmodel.query(id);
            if(!frivec.empty())
            {
                vector<string> vec2;
                for(User &user:frivec)
                {
                    json js;
                    js["id"]=user.getid();
                    js["name"]=user.getname();
                    js["state"]=user.getstate();
                    vec2.push_back(js.dump());
                }
                response["friends"]=vec2;
            }

            //查询用户的群组消息
            vector<group> groupvec=_groupmodel.queryGroups(id);
            if(!groupvec.empty())
            {
                vector<string> groupV;
                for(auto &g:groupvec)
                {
                    json groupjs;
                    groupjs["id"]=g.getid();
                    groupjs["name"]=g.getname();
                    groupjs["desc"]=g.getdesc();
                    vector<string> userV;
                    //if(g.getusers().empty()) cout<<"群里面没人"<<endl;
                    for(auto &user:g.getusers())
                    {
                        
                        json js;
                        js["id"]=user.getid();
                        js["name"]=user.getname();
                        js["state"]=user.getstate();
                        js["role"]=user.getrole();
                        userV.push_back(js.dump());
                    }
                    groupjs["users"]=userV;
                    groupV.push_back(groupjs.dump());
                }
                response["groups"]=groupV;
            }

            conn->send(response.dump());
        }
    }
    else
    {
        //登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1; 
        response["errmsg"] = "用户名或密码错误";
        conn->send(response.dump());
    }
}

//处理注册业务  name  password
void ChatService::reg(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];
    User user;
    user.setname(name);
    user.setpwd(pwd);
    bool ref = _usermodel.insert(user);
    if(ref)
    {
        //注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getid();
        conn->send(response.dump());
    }
    else
    {
        //注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        //response["id"] = user.getid();
        conn->send(response.dump());
    }
}

//添加好友业务  msgid  id  friendid
void ChatService::addfriend(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid=js["id"];
    int friendid=js["friendid"];

    //存储好友信息
    _friendmodel.insert(userid,friendid);

}

//服务器异常后，用户状态重置方法
void ChatService::reset()
{
    //把online用户的状态设置为offline
    _usermodel.resetuserstate();
}

//处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
    lock_guard<mutex> lock(_connMutex);
    for(auto it=_userConnMap.begin();it!=_userConnMap.end();it++)
    {
        if(it->second==conn)
        {
            //map表删除用户的连接信息
            user.setid(it->first);
            _userConnMap.erase(it);
            break;
        }
    }
    }

    //在redis中取消订阅通道
    _redis.unsubscribe(user.getid());

    //更新用户的状态信息
    _usermodel.update(user);
}

//处理注销业务
void ChatService::loginout(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid=js["id"];
    {
    lock_guard<mutex> lock(_connMutex);
    auto it=_userConnMap.find(userid);
    if(it!=_userConnMap.end())
    {
        _userConnMap.erase(it);
    }
    }

    //在redis中取消订阅通道
    _redis.unsubscribe(userid);

    //更新用户的状态信息
    User user;
    user.setid(userid);
    _usermodel.update(user);
}

//一对一聊天业务
void ChatService::onechat(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int toid=js["to"];

    {           //此作用域加锁
        lock_guard<mutex> lock(_connMutex);
        auto it=_userConnMap.find(toid);
        if(it!=_userConnMap.end())
        {
            //toid在线，转发消息
            it->second->send(js.dump());
            return;
        }  
    }

    //查询toid是否在线
    User user=_usermodel.query(toid);
    if(user.getstate()=="online")
    {
        _redis.publish(toid,js.dump());
        return;
    }
    
    //toid不在线，存储离线消息
    _offlinemsgmodel.insert(toid,js.dump());
}

//创建群组业务
void ChatService::creatGroup(const TcpConnectionPtr &conn,json &js,Timestamp time)
{   
    int userid=js["id"];
    string groupname=js["name"];
    string groupdesc=js["desc"];
    group g;
    g.setdesc(groupdesc);
    g.setname(groupname);
    _groupmodel.createGroup(g);
    int groupid=g.getid();
    _groupmodel.addGroup(userid,groupid,"creator");
}

//加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid=js["id"];
    int groupid=js["groupid"];
    _groupmodel.addGroup(userid,groupid,"normal");
}

//群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid=js["id"];
    int groupid=js["groupid"];
    vector<int> toid=_groupmodel.queryGroupUsers(userid,groupid);

    {
    lock_guard<mutex> lock(_connMutex);
    for(int id:toid)
    {
        auto it=_userConnMap.find(id);
        if(it!=_userConnMap.end())
        {
            //对方在线且在同一服务器下
            it->second->send(js.dump());
        }
        else
        {
            //对方在线但不在同一服务器下
            User user=_usermodel.query(id);
            if(user.getstate()=="online")
            {
                _redis.publish(id,js.dump());
                return;
            }
            else 
            {//对方不在线，存储离线消息

                _offlinemsgmodel.insert(id,js.dump());
            }
        }
    }
    }
}

//从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid,string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it=_userConnMap.find(userid);
    if(it!=_userConnMap.end())
    {
        it->second->send(msg);
        return;
    }    

    //存储该用户的离线消息
    _offlinemsgmodel.insert(userid,msg);
}