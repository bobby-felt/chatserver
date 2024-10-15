#include"friendmodel.hpp"
#include"db.h"

//添加好友关系
void friendmodel::insert(int userid,int friendid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql,"insert into friend values(%d,%d)",userid,friendid);

    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
            
}

//返回用户好友列表
vector<User> friendmodel::query(int userid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql,"select a.id,a.name,a.state from user a inner join friend b on b.friendid = a.id where b.userid = %d ",userid);
    vector<User> v;
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res))!=nullptr)
            {
                User user;
                user.setid(atoi(row[0]));
                user.setname(row[1]);
                user.setstate(row[2]);
                v.push_back(user);
            }
            //释放资源，防止内存泄漏
            mysql_free_result(res);
            
        }
    }

    sprintf(sql,"select a.id,a.name,a.state from user a inner join friend b on b.userid = a.id where b.friendid = %d ",userid);
    MySQL mysql2;
    if(mysql2.connect())
    {
        MYSQL_RES *res = mysql2.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res))!=nullptr)
            {
                User user;
                user.setid(atoi(row[0]));
                user.setname(row[1]);
                user.setstate(row[2]);
                v.push_back(user);
            }
            //释放资源，防止内存泄漏
            mysql_free_result(res);
            
        }
    }


    return v;
}