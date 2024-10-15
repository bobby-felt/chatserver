#ifndef USERMUDUO_H
#define USERMUDUO_H

#include"user.hpp"

//User表的数据操作类
class UserModel
{
public:
    //user表的增加方法
    bool insert(User &user);

    //根据用户id查询用户信息
    User query(int id);

    //更新用户状态信息
    bool update(User &user);

    //重置用户的状态信息
    void resetuserstate();

private:


};





#endif