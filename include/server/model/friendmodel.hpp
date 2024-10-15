#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include<vector>
#include"user.hpp"

//维护好友信息的操作方法
class friendmodel
{
public:
    //添加好友关系
    void insert(int userid,int friendid);

    //返回用户好友列表
    vector<User> query(int userid);
private:
    /* data */

};










#endif