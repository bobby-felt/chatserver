#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include"group.hpp"
#include<string>
#include<vector>
using namespace std;

class groupmodel
{

public:
    //创建群组
    bool createGroup(group &group);
    //加入群组
    void addGroup(int userid,int groupid,string role);
    //查询用户所在群组信息
    vector<group> queryGroups(int userid);
    //根据指定的groupid查询群组用户id列表，除了userid自己，主要用户群聊业务给群组其他成员群发消息
    vector<int> queryGroupUsers(int userid,int groupid);
};





#endif