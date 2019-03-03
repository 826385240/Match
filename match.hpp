#include <set>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>
#include <iostream>
#include <stdio.h>
#include "skiplist.hpp"

using namespace std;

struct CombineNode{
    std::string key;
    std::map<unsigned int,unsigned int>   eleNum;       // 元素值 --> 元素个数
};

class FastCombine{
public:
    FastCombine(int sum,int ele):combineVal(sum),eleValLimit(ele){
        _combine(combineVal);

        //for(std::map<unsigned int, std::vector<CombineNode> >::iterator it =combines.begin();it != combines.end();++it){
        //    cout << "个数为" << it->first << ":" <<endl;
        //    for(int i=0;i < it->second.size();++i){
        //        CombineNode& cn=it->second[i];
        //        cout << cn.key << endl;
        //    }
        //}
    }

    const std::vector<CombineNode>& getCombinesByKey(unsigned int key){
        static std::vector<CombineNode> defaultCombines;
        std::map<unsigned int, std::vector<CombineNode> >::iterator it=combines.find(key);
        if( it != combines.end() ){
            return it->second;
        }
        return defaultCombines;
    }

private:
    bool _isExistCombines(unsigned int key){
        return combines.find(key) != combines.end();
    }

    bool _popVec(std::vector<unsigned int>& vec,unsigned int& popVal){
        if(!vec.empty()){
            popVal=vec[vec.size()-1];
            vec.erase(vec.end()-1);
            return true;
        }
        return false;
    }

    bool _saveVec(std::vector<unsigned int> vec){
        std::sort(vec.begin(),vec.end());
        CombineNode cn;

        char temp[1024]={0};
        for(int i=0; i < vec.size();++i){
            sprintf(temp,"%s%d",temp,vec[i]);
            cn.eleNum[vec[i]]++;
        }
        cn.key=temp;

        if(validCombines.insert(cn.key).second){
            for(  std::map<unsigned int,unsigned int>::iterator it=cn.eleNum.begin() ; it != cn.eleNum.end() ; ++it){
                combines[it->first].push_back(cn);
            }
        }
    }

    void _combine(int sum){
        std::vector<unsigned int> vec;
        unsigned int next=1,curSum=0,popVal=0;
        do{
            if (next > eleValLimit){
                if (!_popVec(vec,popVal)){
                    break;
                }

                curSum-=popVal;
                next=popVal+1;
            }else if(curSum==sum){
                _saveVec(vec);
                if (!_popVec(vec,popVal)){
                    break;
                }

                curSum-=popVal;
                next=popVal+1;
            }else if(curSum+next>combineVal){
                if (!_popVec(vec,popVal)){
                    break;
                }

                curSum-=popVal;
                next=popVal+1;
            }else{
                vec.push_back(next);
                curSum+=next;
                next=1;
            }
        }while(true);
	}
private:
    int combineVal;
    int eleValLimit;
    std::set<std::string> validCombines;

    //combineVal的所有组合    < 元素值 , <组合集,组合信息> >
    std::map<unsigned int, std::vector<CombineNode> > combines;
};

class Combine{
public:
    Combine(int sum,int ele):combineVal(sum),eleValLimit(ele){
        unsigned int* comArray=new unsigned int[sum];
        _combine(comArray , sum);
        delete [] comArray;

        for(std::map<unsigned int, std::vector<CombineNode> >::iterator it =combines.begin();it != combines.end();++it){
            cout << "个数为" << it->first << ":" <<endl;
            for(int i=0;i < it->second.size();++i){
                CombineNode& cn=it->second[i];
                cout << cn.key << endl;
            }
        }
    }

    const std::vector<CombineNode>& getCombinesByKey(unsigned int key){
        static std::vector<CombineNode> defaultCombines;
        std::map<unsigned int, std::vector<CombineNode> >::iterator it=combines.find(key);
        if( it != combines.end() ){
            return it->second;
        }
        return defaultCombines;
    }

private:
    bool _isExistCombines(unsigned int key){
        return combines.find(key) != combines.end();
    }

    bool _saveVec(unsigned int array[]){
        CombineNode cn;

        char temp[1024]={0};
        for(int i=0; i < combineVal ;++i){
            for(int j=0; j < array[i] ; ++j)
            {
                sprintf(temp,"%s%d",temp,i+1);
            }
            cn.eleNum[i+1]=array[i];
        }
        cn.key=temp;

        if(validCombines.insert(cn.key).second){
            for(  std::map<unsigned int,unsigned int>::iterator it=cn.eleNum.begin() ; it != cn.eleNum.end() ; ++it){
                combines[it->first].push_back(cn);
            }
        }
    }

    void _combine(unsigned int array[] , int leftSum){
        if( leftSum == 0 )
        {
            _saveVec(array);
            return;
        }

        unsigned times=std::min(leftSum,eleValLimit);
        for(unsigned int i=1;i <= times; ++i)
        {
            ++array[i-1];
            _combine(array,leftSum-i);
            --array[i-1];
        }
	}
private:
    int combineVal;
    int eleValLimit;
    std::set<std::string> validCombines;

    //combineVal的所有组合    < 元素值 , <组合集,组合信息> >
    std::map<unsigned int, std::vector<CombineNode> > combines;
};

/*
 * Key:需要实现<操作符重载
 * Value:可以保存Key相关的信息及权重信息,使用默认比较函数_defValueCmp需要实现<操作符重载
 */
template<typename Key,typename Value>
class BaseMatchManager{
public:
    //记录类型,半开区间 [lowLimit , upLimit)
    struct RecordType{
        RecordType():upLimit(-1),lowLimit(-1){
        }

        int lowLimit;
        int upLimit;
        bool updateRecord(unsigned int num,bool succ){
            if( succ ){
                if( num > lowLimit )
                    lowLimit = num;
            } else {
                if( num < upLimit )
                    upLimit = num;
            }
        }
    };
public:
    typedef Value(*pRangeFun)(const Value&);
    typedef typename skiplist<Key,Value>::pcompare pCompareFun;

    BaseMatchManager(unsigned int matchNum,unsigned int eleLimit, pCompareFun cmp=&BaseMatchManager::_defValueCmp):combines(matchNum,eleLimit){
        for(unsigned int i=1 ; i <= matchNum && i <= eleLimit; ++i ){
            waitQueues.insert(std::make_pair(i,skiplist<Key,Value>(cmp)));
        }
    }
public:
    virtual bool onSuccess(std::vector<Key>& allKeys) = 0;
public:
    bool enterMatch(Key& k , Value& v , unsigned int num, pRangeFun upLimitFun=&BaseMatchManager::_defUpLimitFun,pRangeFun lowLimitFun=&BaseMatchManager::_defLowLimitFun){
        const std::vector<CombineNode>& nodes=combines.getCombinesByKey(num);
        if( nodes.empty() )
            return false;

        std::map<unsigned int,RecordType> records;
        for(unsigned int i=nodes.size() ; i>0 ; --i){
            //遍历能容下当前队伍的所有组合
            bool success=true;
            const CombineNode& node=nodes[i-1];
            for(std::map<unsigned int,unsigned int>::const_iterator it=node.eleNum.begin() ; it != node.eleNum.end() ; ++it){
                unsigned int configNum=it->first;

                //检查当前组合的人数是否满足匹配条件
                skiplist<Key,Value>& sl=waitQueues[configNum];
                int searchNum=(configNum==num?it->second-1:it->second);
                RecordType& rt=records[configNum];
                if ( searchNum <= rt.lowLimit ){                                //在确定满足当前组合的范围内
                    continue;
                } else if ( rt.upLimit != -1 && searchNum >= rt.upLimit){       //在确定不满足当前组合的范围内
                    success=false;
                    break;
                } else {                                                        //不确定满足当前组合的范围
                    if (!searchNum || sl.exist_n_by_score_range(lowLimitFun(v), upLimitFun(v), searchNum)){
                        if ( searchNum > 0 )
                            rt.updateRecord(searchNum , true);
                    } else {
                        rt.updateRecord(searchNum , false);

                        success=false;
                        break;
                    }
                }
            }

            if(success) return _onMatchSuccess(node,k,v,num , upLimitFun , lowLimitFun );
        }

        //匹配失败,放入等待队列
        waitQueues[num].insert(k,v);
    }

    unsigned int getRemainNum(){
        unsigned int sum = 0;

        for(typename std::map<unsigned int,skiplist<Key,Value> >::iterator it = waitQueues.begin() ; it != waitQueues.end() ; ++it){
            sum += it->second.size();
            cout <<  "人数:" << it->first << " 队伍数:" << it->second.size() << endl;
        }
        return sum;
    }
private:
    bool _onMatchSuccess(const CombineNode& node , Key k , Value v , unsigned int num, pRangeFun upLimitFun,pRangeFun lowLimitFun){
        std::vector<Key> allKeys;
        for(std::map<unsigned int,unsigned int>::const_iterator it=node.eleNum.begin() ; it != node.eleNum.end() ; ++it){
            //检查当前组合的人数是否满足匹配条件
            skiplist<Key,Value>& sl=waitQueues[it->first];

            std::vector<Key> output_ids;
            if (!sl.get_mid_n_by_score_range(lowLimitFun(v), v , upLimitFun(v), it->first==num?it->second-1:it->second, output_ids)){
                cout << "失败" << sl.size() << ":" << (it->first==num?it->second-1:it->second) << endl;
                return false;
            }
            sl.skiplist_delete(output_ids);
            allKeys.insert(allKeys.end(),output_ids.begin(),output_ids.end());
        }
        allKeys.push_back(k);
        onSuccess(allKeys);
    }

    static Value _defUpLimitFun(const Value& v){
        return v;
    }

    static Value _defLowLimitFun(const Value& v){
        return v;
    }

    //注意:比较函数最好包括等于:比如 >= 或 <=
    static bool _defValueCmp(const Key& k1,const Value& v1, const Key& k2,const Value& v2){
        return v1 >= v2;
    }
private:
    FastCombine combines;
    std::map<unsigned int,skiplist<Key,Value> > waitQueues;
};


template<typename Key,typename Value>
class BaseLargeMatchManager
{
    struct GroupNode{
        GroupNode() : memSum(0){
        }

        GroupNode(unsigned int num,Value& v) : value(v), memSum(num){
        }
        Value value;            //组内value值
        unsigned int memSum;    //当前总人数
        std::map<unsigned int,std::map<Key,Value> > allMems; //队伍人数 --> <Key , Value>

        operator Value(){
            return value;
        }
    };
public:
    typedef Value(*pRangeFun)(const Value&);
    typedef typename skiplist<Key,Value>::pcompare pCompareFun;

    BaseLargeMatchManager(unsigned int m,unsigned int e, pCompareFun cmp=&BaseLargeMatchManager::_defValueCmp):
        matchNum(m),
        eleLimit(e),
        waitQueues(cmp)
    {
    }
public:
    virtual bool onSuccess(std::vector<Key>& allKeys) = 0;
public:
    bool enterMatch(Key& k , Value& v , unsigned int num, pRangeFun upLimitFun=&BaseLargeMatchManager::_defUpLimitFun,pRangeFun lowLimitFun=&BaseLargeMatchManager::_defLowLimitFun){
        GroupNode gn(num,v);
        skiplist<Key,GroupNode>::pvalue_node node=waitQueues,find_near_node(k,v);
    }
private:
    bool checkFull(skiplist<Key,GroupNode>::pvalue_node node, Key& k, Value& v, Key& kick_key, Value& kick_value)
    {
        GroupNode& gn=node->second;
        if( gn.memSum + sum == matchNum )
        {
            gn.memSum = matchNum;
            gn.allMems.insert(std::make_pair(k , v));
            return true;
        }

        if( gn.memSum + sum > matchNum )
        {
            unsigned int kicknum=gn.memSum + sum - matchNum;

        }
    }
    //注意:比较函数最好包括等于:比如 >= 或 <=
    static bool _defValueCmp(const Key& k1,const Value& v1, const Key& k2,const Value& v2){
        return v1 >= v2;
    }

    static Value _defUpLimitFun(const Value& v){
        return v;
    }

    static Value _defLowLimitFun(const Value& v){
        return v;
    }
private:
    unsigned int matchNum;
    unsigned int eleLimit;
    skiplist<Key,GroupNode> waitQueues;     // 创建人Key --> 组内成员
};
