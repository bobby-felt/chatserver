#include "offlinemessagemodel.hpp"
#include "db.h"



//存储用户的离线消息
void offlineMsgModel::insert(int userid,string msg)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql,"insert into offlinemessage values(%d,'%s')",userid,msg.c_str());   //这里只能存储一条离线消息,把离线消息表的主键去掉解决了

    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }

}
//删除用户的离线消息
void offlineMsgModel::remove(int userid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql,"delete from offlinemessage where userid=%d",userid);

    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}
//查询用户的离线消息
vector<string> offlineMsgModel::query(int userid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql,"select message from offlinemessage where userid = %d ",userid);
    vector<string> v;
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)
        {
            //把userid用户的所有离线消息放入vector
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res))!=nullptr)
            {
                v.push_back(row[0]);
            }
            //释放资源，防止内存泄漏
            mysql_free_result(res);
            
        }
    }
    return v;
}