#ifndef GROUP_H
#define GROUP_H

#include"groupuser.hpp"
#include<vector>

using namespace std;

class group
{
private:
    int id;
    string name;
    string desc;
    vector<groupuser> users;
public:
    group(int id=-1,string name="",string desc="")
    {
        this->id=id;
        this->name=name;
        this->desc=desc;
    }

    void setid(int id) {this->id=id;}
    void setname(string name) {this->name=name;}
    void setdesc(string desc) {this->desc=desc;}

    int getid() {return this->id;}
    string getname() {return this->name;}
    string getdesc() {return this->desc;}
    vector<groupuser>& getusers() {return this->users;}
};












#endif