#include "skiplist.hpp"
#include "match.hpp"
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <ctime>

using namespace std;

class MatchManager:public BaseMatchManager<std::string,unsigned int>{
public:
    MatchManager(unsigned int matchNum,unsigned int eleLimit):BaseMatchManager(matchNum,eleLimit){
    }

    bool onSuccess(std::vector<std::string>& allKeys){
        cout << "Match Success:" << allKeys.size() << "个 ";
        for( unsigned int i=0;i<allKeys.size();++i ){
            cout << allKeys[i] << " ";
        }
        cout << endl;
    }
};

unsigned int id=0;

void genTeam(std::string& teamName,unsigned int& num,unsigned int& value){
    char temp[100]={0};
    num=std::rand()%5+1;
    sprintf(temp,"%s%d_%d_%d","team",id,value,num);
    teamName=temp;

    id+=1;
}

unsigned int defualtUpLimitFun(const unsigned int& v){
    return v+1000;
}

unsigned int  defualtLowLimitFun(const unsigned int& v){
    return v>1000?v-1000:0;
}

struct genPair{
    std::string teamName;
    unsigned int num;
    unsigned int randVal;
};

int main(){
    MatchManager mm(10,5);
    unsigned int allTeams=2000000;

    std::vector<genPair> genDatas;
    std::string teamName;
    unsigned int num;
    for(unsigned int i=0;i<allTeams;++i){
        unsigned int value=(unsigned int)(std::rand()%200000+1);
        genTeam(teamName,num,value);
        genPair gp={teamName,num,value};
        genDatas.push_back(gp);
    }

    unsigned long long beginTime=std::time(0);
    for(unsigned int i=0;i<allTeams;++i){
        mm.enterMatch(genDatas[i].teamName,genDatas[i].randVal,genDatas[i].num,&defualtUpLimitFun,&defualtLowLimitFun);
    }
    unsigned long long endTime=std::time(0);
    cout << "matchRate:" << 100-mm.getRemainNum()*100.0/allTeams << " cost:" << endTime-beginTime;
}
