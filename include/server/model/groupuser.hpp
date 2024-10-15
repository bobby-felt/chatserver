#ifndef GROUPUSER_H
#define GROUPUSER_H

#include"user.hpp"

//群组用户多了一个role，直接从user继承
class groupuser:public User
{
private:
    string role;
public:
    void setrole(string role){this->role=role;}
    string getrole(){return this->role;}
};








#endif