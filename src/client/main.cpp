#include"json.hpp"
#include<iostream>
#include<string>
#include<thread>
#include<vector>
#include<chrono>
#include<ctime>
using namespace std;
using json=nlohmann::json;

#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include"group.hpp"
#include"user.hpp"
#include"public.hpp"

//记录当前系统登录的用户信息
User g_currentUser;
//记录当前登录用户的好友列表
vector<User> g_currentUserFriendList;
//记录当前登录用户的群组列表信息
vector<group> g_currentUserGroupList;
//显示当前登录用户的基本信息
void showCurrentUserData();
//控制主菜单页面
bool isMainMenuRunning = false;

//接受线程
void readTaskHandler(int clientfd);
//获取系统时间（聊天需要有时间信息）
string getCurrentTime();
//主聊天页面程序
void mainmenu(int clientfd);

//聊天客户端程序实现，main线程用作发送线程，子线程用作接受线程
int main(int argc,char **argv)                                                  //这两个东西是什么意思
{
    if(argc<3)
    {
        cerr<<"command invalid! example: ./ChatClient 127.0.0.1 6000"<<endl;
        exit(-1);
    }

    //解析通过命令行参数传递的ip和port
    char *ip=argv[1];
    uint16_t port = atoi(argv[2]);

    //创建client端的socket
    int clientfd = socket(AF_INET,SOCK_STREAM,0);
    if(-1==clientfd)
    {
        cerr<<"socket create error"<<endl;
        exit(-1);
    }

    //填写client需要连接的server信息ip+port
    sockaddr_in server;
    memset(&server,0,sizeof(sockaddr_in));

    server.sin_family=AF_INET;
    server.sin_port=htons(port);
    server.sin_addr.s_addr=inet_addr(ip);

    //client和server进行连接
    if(-1==connect(clientfd,(sockaddr *)&server,sizeof(sockaddr_in)))
    {
        cerr<<"connect server error"<<endl;
        close(clientfd);
        exit(-1);
    }

    //main线程用于接收用户输入，负责发送数据
    for(;;)
    {
        //显示页面菜单、登录注册退出
        cout<<"========================"<<endl;
        cout<<"1.login"<<endl;
        cout<<"2.register"<<endl;
        cout<<"3.quit"<<endl;
        cout<<"========================"<<endl;
        cout<<"choice:";
        int choice=0;
        cin>>choice;
        cin.get();  //读掉缓冲区残留的回车

        switch (choice)
        {
        case 1://登录业务
        {
            int id=-1;
            char pwd[50]={0};
            cout<<"userid:";
            cin>>id;
            cin.get();  //读掉缓冲区的回车
            cout<<"userpassword:";
            cin.getline(pwd,50);

            json js;
            js["msgid"]=LOGIN_MSG;
            js["id"]=id;
            js["password"]=pwd;
            string request=js.dump();
            int len=send(clientfd,request.c_str(),strlen(request.c_str())+1,0);
            if(len==-1)
            {
                cerr<<"send login msg error:"<<request<<endl;
            }
            else
            {
                char buffer[1024]={0};
                len=recv(clientfd,buffer,1024,0);
                if(-1==len)
                {
                    cerr<<"recv login response error"<<endl;
                }
                else
                {
                    json responsejs=json::parse(buffer);
                    if(0!=responsejs["errno"])//登录失败
                    {
                        cerr<<responsejs["errmsg"] <<endl;
                    }
                    else    //登陆成功
                    {
                        //记录当前用户的id和name
                        g_currentUser.setid(responsejs["id"]);
                        g_currentUser.setname(responsejs["name"]);

                        cout<<"记录信息"<<endl;

                        //记录当前用户的好友列表信息
                        if(responsejs.contains("friends"))
                        {
                            //初始化
                            g_currentUserFriendList.clear();

                            vector<string>vec=responsejs["friends"];
                            for(string &s:vec)
                            {
                                json js=json::parse(s);
                                User user;
                                user.setid(js["id"]);
                                user.setname(js["name"]);
                                user.setstate(js["state"]);
                                g_currentUserFriendList.push_back(user);
                            }
                        }

                        //记录当前用户的群组列表信息
                        if(responsejs.contains("groups"))
                        {
                            //初始化
                            g_currentUserGroupList.clear();

                            vector<string>vec1=responsejs["groups"];
                            for(string &s:vec1)
                            {
                                json gjs=json::parse(s);
                                group g;
                                g.setid(gjs["id"]);
                                g.setname(gjs["name"]);
                                g.setdesc(gjs["desc"]);

                                vector<string>vec2=gjs["users"];
                                //if(vec2.empty()) cout<<"组成员为空"<<endl;
                                for(string &s2:vec2)
                                {
                                    json ujs=json::parse(s2);
                                    groupuser u;
                                    u.setid(ujs["id"]);
                                    u.setname(ujs["name"]);
                                    u.setstate(ujs["state"]);
                                    u.setrole(ujs["role"]);
                                    g.getusers().push_back(u);
                                    //cout<<"有群组员"<<endl;
                                }

                                g_currentUserGroupList.push_back(g);
                            }
                        }

                        //显示登录用户的基本信息
                        showCurrentUserData();

                        //显示当前用户的离线消息，个人聊天信息或者群组消息
                        if(responsejs.contains("offlinemsg"))
                        {
                            vector<string> vec=responsejs["offlinemsg"];
                            for(string &s:vec)
                            {
                                json js=json::parse(s);
                                //cout<<js["time"]<<"["<<js["id"]<<"]"<<js["name"]<<" said: "<<js["msg"]<<endl;
                                int msgtype=js["msgid"];
                                if(msgtype==ONE_CHAT_MSG)
                                {
                                    cout<<js["time"]<<" ["<<js["id"]<<"] "<<js["name"]<<" said: "<<js["msg"]<<endl;
                                }
                                else if(msgtype==GROUP_CHAT_MSG)
                                {
                                    cout<<js["time"]<<"群号：["<<js["groupid"]<<"] "<<" ["<<js["id"]<<"] "<<js["name"]<<" said: "<<js["msg"]<<endl;
                                }
                            }
                        }

                        //登陆成功，启动接收线程负责接收数据,该线程只启动一次
                        static int readthreadnumber=0;
                        if(readthreadnumber==0)
                        {
                            std::thread readTask(readTaskHandler,clientfd); //pthread_create
                            readTask.detach();      //pthread_detach
                            readthreadnumber++;
                        }
                        

                        //进入聊天菜单页面
                        isMainMenuRunning=true;
                        mainmenu(clientfd);


                    }
                }

            }
            break;
        }
        case 2://注册业务
        {
            char name[50];
            char pwd[50];
            cout<<"name:";
            cin.getline(name,50);  //用cin>>读取的话名字里面不能有空格回车
            cout<<"password:";
            cin.getline(pwd,50);

            json js;
            js["msgid"]=REG_MSG;
            js["name"]=name;
            js["password"]=pwd;
            string request=js.dump();

            int len=send(clientfd,request.c_str(),strlen(request.c_str())+1,0);
            if(-1==len)
            {
                cerr<<"send reg msg error:"<<request<<endl;
            }
            else
            {
                char buffer[1024]={0};
                len=recv(clientfd,buffer,1024,0);
                if(-1==len)
                {
                    cerr<<"recv reg response error"<<endl;
                }
                else
                {
                    json responsejs=json::parse(buffer);
                    if(0!=responsejs["errno"])  //注册失败
                    {
                        cerr<<name<<" is already exist,reigster error!"<<endl;
                    }
                    else    //  注册成功
                    {
                        cout<<name<<" register success,userid is :"<<responsejs["id"]<<",do not forget it."<<endl;
                         
                    }

                }
            }

            break;
        }
        case 3://退出登录
            close(clientfd);
            exit(0);
            break;
        
        default:
            cout<<"invalid input!"<<endl;
            break;
        }
    }
    return 0;
}


//接受线程
void readTaskHandler(int clientfd)
{
    for(;;)
    {
        char buffer[1024]={0};
        int len = recv(clientfd,buffer,1024,0);
        if(-1==len||0==len)
        {
            close(clientfd);
            exit(-1);
        }

        //接收ChatServer转发的数据，反序列生成json数据对象
        json js=json::parse(buffer);
        int msgtype=js["msgid"];
        if(msgtype==ONE_CHAT_MSG)
        {
            cout<<js["time"]<<" ["<<js["id"]<<"] "<<js["name"]<<" said: "<<js["msg"]<<endl;
            continue;
        }
        else if(msgtype==GROUP_CHAT_MSG)
        {
            cout<<js["time"]<<"群号：["<<js["groupid"]<<"] "<<" ["<<js["id"]<<"] "<<js["name"]<<" said: "<<js["msg"]<<endl;
            continue;
        }
    }

}

//获取系统时间（聊天需要有时间信息）
string getCurrentTime()
{
    auto tt=std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm=localtime(&tt);
    char date[60]={0};
    sprintf(date,"%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year+1900,(int)ptm->tm_mon+1,(int)ptm->tm_mday,
            (int)ptm->tm_hour,(int)ptm->tm_min,(int)ptm->tm_sec);
    return std::string(date);
}

//command handler
void help(int fd=0,string str="");
void chat(int,string);
void addfriend(int,string);
void creategroup(int,string);
void addgroup(int,string);
void groupchat(int,string);
void loginout(int,string);

//系统支持的客户端命令列表
unordered_map<string,string> commandMap = {
    {"help","显示所有支持的命令,格式help"},
    {"chat","一对一聊天,格式chat:friendid:message"},
    {"addfriend","添加好友,格式addfriend:friendid"},
    {"creategroup","创建群组,格式creategroup:groupname:groupdesc"},
    {"addgroup","加入群组,格式addgroup:groupid"},
    {"groupchat","群聊,格式groupchat:groupid:message"},
    {"loginout","注销,格式loginout"}
};

//注册系统支持的客户端命令
unordered_map<string,function<void(int,string)>> commandHandlerMap = {
    {"help",help},
    {"chat",chat},
    {"addfriend",addfriend},
    {"creategroup",creategroup},
    {"addgroup",addgroup},
    {"groupchat",groupchat},
    {"loginout",loginout}
};

//主聊天页面程序
void mainmenu(int clientfd)
{
    help();
    char buffer[1024]={0};
    while(isMainMenuRunning)
    {
        cin.getline(buffer,1024);
        string commandbuf(buffer);
        string command; //存储命令
        int idx = commandbuf.find(":");
        if(-1==idx)
        {
            command=commandbuf;
        }
        else
        {
            command=commandbuf.substr(0,idx);
        }
        auto it=commandHandlerMap.find(command);
        if(it==commandHandlerMap.end())
        {
            cerr<<"invalid inout command!"<<endl;
            continue;
        }

        //调用相应的命令处理回调，mainmenu对修改封闭，添加新功能不需要修改该函数
        it->second(clientfd,commandbuf.substr(idx+1,commandbuf.size()-idx)); //调用命令

    }
}

//command handler
void help(int,string)
{
    cout<<"show command list:"<<endl;
    for(auto &s:commandMap)
    {
        cout<<s.first<<":"<<s.second<<endl;
    }
    cout<<endl;
}
void chat(int clientfd,string str)
{
    int idx=str.find(":");
    if(-1==idx)
    {
        cerr<<"chat command invalid!"<<endl;
        return;
    }
    int friendid=atoi(str.substr(0,idx).c_str());
    string msg=str.substr(idx+1,str.size()-idx);

    json js;
    js["msgid"]=ONE_CHAT_MSG;
    js["id"]=g_currentUser.getid();
    js["name"]=g_currentUser.getname();
    js["to"]=friendid;
    js["msg"]=msg;
    js["time"]=getCurrentTime();
    string buffer=js.dump();

    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1==len)
    {
        cerr<<"send chat msg error -> "<<buffer<<endl;
    }

}
void addfriend(int clientfd,string str)
{
    int friendid=atoi(str.c_str());
    json js;
    js["msgid"]=ADD_FRIEND_MSG;
    js["id"]=g_currentUser.getid();
    js["friendid"]=friendid;
    string buffer=js.dump();

    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1==len)
    {
        cerr<<"send addfriend msg error -> "<<buffer<<endl;
    }
}
void creategroup(int clientfd,string str)
{
    int idx=str.find(":");
    if(-1==idx)
    {
        cerr<<"creategroup command invalid!"<<endl;
        return;
    }
    string groupname=str.substr(0,idx);
    string groupdesc=str.substr(idx+1,str.size()-idx);
    json js;
    js["msgid"]=CREAT_GROUP_MSG;
    js["id"]=g_currentUser.getid();
    js["name"]=groupname;
    js["desc"]=groupdesc;
    string buffer=js.dump();

    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1==len)
    {
        cerr<<"send creategroup msg error -> "<<buffer<<endl;
    }
}
void addgroup(int clientfd,string str)
{
    int groupid=atoi(str.c_str());
    json js;
    js["msgid"]=ADD_GROUP_MSG;
    js["id"]=g_currentUser.getid();
    js["groupid"]=groupid;
    string buffer=js.dump();

    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1==len)
    {
        cerr<<"send addgroup msg error -> "<<buffer<<endl;
    }
}
void groupchat(int clientfd,string str)
{
    int idx=str.find(":");
    if(-1==idx)
    {
        cerr<<"groupchat command invalid!"<<endl;
        return;
    }
    int groupid=atoi(str.substr(0,idx).c_str());
    string msg=str.substr(idx+1,str.size()-idx);
    json js;
    js["msgid"]=GROUP_CHAT_MSG;
    js["id"]=g_currentUser.getid();
    js["name"]=g_currentUser.getname();
    js["groupid"]=groupid;
    js["msg"]=msg;
    js["time"]=getCurrentTime();
    string buffer=js.dump();

    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1==len)
    {
        cerr<<"send groupchat msg error -> "<<buffer<<endl;
    }
}
void loginout(int clientfd,string str)
{
    json js;
    js["msgid"]=LOGINOUT_MSG;
    js["id"]=g_currentUser.getid();
    string buffer=js.dump();

    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1==len)
    {
        cerr<<"send loginout msg error"<<buffer<<endl;
    }
    else
    {
        isMainMenuRunning=false;
    }
}

//显示当前登录用户的基本信息
void showCurrentUserData()
{
    cout<<"================login user================"<<endl;
    cout<<"current user => id:"<<g_currentUser.getid()<<" name: "<<g_currentUser.getname()<<endl;
    cout<<"----------------friend list--------------------"<<endl;
    if(!g_currentUserFriendList.empty())
    {
        for(User &user:g_currentUserFriendList)
        {
            cout<<user.getid()<<" "<<user.getname()<<" "<<user.getstate()<<endl;
        }
    }
    cout<<"----------------group list--------------------"<<endl;
    if(!g_currentUserGroupList.empty())
    {
        for(group &g:g_currentUserGroupList)
        {
            cout<<g.getid()<<" "<<g.getname()<<" "<<g.getdesc()<<endl;
            for(groupuser &user:g.getusers())
            {
                cout<<user.getid()<<" "<<user.getname()<<" "<<user.getstate()<<" "<<user.getrole()<<endl;
            }
            cout<<endl;
        }
    }
    cout<<"====================================="<<endl;
}