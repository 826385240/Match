/*
 1.维持一个有序数列，查找效率（平均常数级），删除，查找排名，插入（Olog(N)）
 2.<Key,Value> Value支持指针类型
 3.保证Key唯一，insert（）只支持唯一key,修改，使用update（）更新value
 4.为保证查找效率，比较消耗内存
 5.自定义比较函数，增加对于key的比较
*/

#ifndef SKIPLIST_HPP_
#define SKIPLIST_HPP_

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <map>
#include <set>

#define _LINUX
#define LIB_FREE free
#define LIB_MALLOC malloc

#define SKIPLIST_P 0.25
#define SKIPLIST_MAXLEVEL 32

typedef unsigned int uint32_t;
typedef signed int s_uint32;

// A very simple random number generator.  Not especially good at
// generating truly random bits, but good enough for our needs in this
// package.
class Random {
private:
	uint32_t seed_;
public:
	explicit Random(uint32_t s) : seed_(s & 0x7fffffffu) {
		// Avoid bad seeds.
		if (seed_ == 0 || seed_ == 2147483647L) {
			seed_ = 1;
		}
	}
	uint32_t Next() {
		static const uint32_t M = 2147483647L;   // 2^31-1
		static const uint64_t A = 16807;  // bits 14, 8, 7, 5, 2, 1, 0
		// We are computing
		//       seed_ = (seed_ * A) % M,    where M = 2^31-1
		//
		// seed_ must not be zero or M, or else all subsequent computed values
		// will be zero or M respectively.  For all other values, seed_ will end
		// up cycling through every number in [1,M-1]
		uint64_t product = seed_ * A;

		// Compute (product % M) using the fact that ((x << 31) % M) == x.
		seed_ = static_cast<uint32_t>((product >> 31) + (product & M));
		// The first reduction may overflow by 1 bit, so we may need to
		// repeat.  mod == M is not possible; using > allows the faster
		// sign-bit-based test.
		if (seed_ > M) {
			seed_ -= M;
		}
		return seed_;
	}
	// Returns a uniformly distributed value in the range [0..n-1]
	// REQUIRES: n > 0
	uint32_t Uniform(int n) { return Next() % n; }

	// Randomly returns true ~"1/n" of the time, and false otherwise.
	// REQUIRES: n > 0
	bool OneIn(int n) { return (Next() % n) == 0; }

	// Skewed: pick "base" uniformly from range [0,max_log] and then
	// return "base" random bits.  The effect is to pick a number in the
	// range [0,2^max_log-1] with exponential bias towards smaller numbers.
	uint32_t Skewed(int max_log) {
		return Uniform(1 << Uniform(max_log + 1));
	}
};

template<typename Key,typename Value>
struct Base_Node
{
	Key first;                     //Key
	Value second;           //Value
};

template<typename Key,typename Value>
struct Node : public Base_Node<Key,Value>
{
	unsigned int node_level;
	struct  Level_Node
	{
		  Node* pre;
		  Node* next;
		  unsigned  long long span;
	} level[1];
};

//simple iterator for skiplist
template<typename Key,typename Value>
class skiplist_iterator
{
public:
	typedef Base_Node<Key,Value> base_node;
	typedef base_node* pbase_node;
	typedef Node<Key,Value> node;
	typedef node* pnode;

	skiplist_iterator() : pvalue(NULL),pbase_value(NULL)
	{

	}
	explicit skiplist_iterator(pnode pnode_value) : pvalue(pnode_value),
		pbase_value(dynamic_cast<pbase_node>(pvalue))
	{
	}

	virtual ~skiplist_iterator()
	{
		pvalue = NULL;
		pbase_value = NULL;
	}

public:
	 base_node& operator*()
	{
		return *pbase_value;
	}

	pbase_node operator->()
	{
		return pbase_value;
	}

	pbase_node operator&()
	{
		return pbase_value;
	}

    skiplist_iterator& operator=(pnode pnode_value)
	{
		pvalue = pnode_value;
		pbase_value = dynamic_cast<pbase_node>(pnode_value);
		return *this;
	}

	skiplist_iterator& operator++()
	{
		if( pvalue ) {
			pvalue = pvalue->level[0].next;
			pbase_value = dynamic_cast<pbase_node>(pvalue);
		}
		return *this;
	}

	skiplist_iterator& operator++(int)
	{
		if( pvalue ) {
			pvalue = pvalue->level[0].next;
			pbase_value = dynamic_cast<pbase_node>(pvalue);
		}
		return *this;
	}

     bool operator ==(skiplist_iterator it1)
	{
		return pvalue == it1.pvalue;
	}

	 bool operator !=(skiplist_iterator it1)
	 {
		 return pvalue != it1.pvalue;
	 }

private:
	pbase_node pbase_value;
	pnode pvalue;
};

template<typename Key,typename Value>
class skiplist
{
public:
	typedef bool (*pcompare)(const Key& k1,const Value& v1, const Key& k2,const Value& v2);

#ifdef _LINUX
	typedef  Node<Key,Value> value_node;
	typedef  value_node* pvalue_node;
	typedef  skiplist_iterator<Key,Value> iterator;
	typedef  typename Node<Key,Value>::Level_Node level_node;
#else
	typedef typename Node<Key,Value> value_node;
	typedef typename  value_node* pvalue_node;
	typedef typename skiplist_iterator<Key,Value> iterator;
#endif
	//interface for user
    const int NODE_SIZE=(sizeof(value_node)+SKIPLIST_MAXLEVEL*sizeof(level_node));
public:
	//insert new value with the comparison function
	bool insert(Key& key,Value& value,pcompare pcmp);
	bool insert(Key& key,Value& value);
    //get the ranking data that contains data of the top N
    //void contain_topn(int front_count, std::vector<Value>& output);
	//get the ranking data that contains data of current_key and date of the top N
    void contain_topn(Key& current_key, int front_count, std::vector<Value>& output, bool& extra);
    //get the ranking data that contains data of current_key and date of the top N
    void contain_topn(Key& current_key, int offset, int front_count, std::vector<Value>& output, bool& extra);
	//remove by the key and return the next valid iterator
	iterator skiplist_delete(const Key& key);
	void skiplist_delete(const std::vector<Key>& keys);
	//update the location of the value changed
	void update( iterator& it, Value& value);
	//find the value by key
	const iterator find(const Key& key);
	//get the rank of the key
	unsigned long long get_rank(const Key& key);
	unsigned long long get_rankV(const Value& value);
	//get the skiplist size
	unsigned long long size();
    //find score between
    bool find_n_score_in(Value  value1, Value  value2, s_uint32 n, std::set<Key>& rule_out, std::vector<Key>& ch_ids);
    bool get_by_score_range(Value value_min, Value value_max, std::vector<Key>& output_ids);
    bool get_n_by_score_range(Value value_min, Value value_max, s_uint32 n , std::vector<Key>& output_ids);
    bool get_mid_n_by_score_range(Value value_min,  Value value_mid, Value value_max, s_uint32 n , std::vector<Key>& output_ids);
    bool exist_n_by_score_range(Value value_min, Value value_max, s_uint32 n);

    Node<Key,Value>& find_near_node(Key& key, Value& value);

//deal the simple iterator for skiplist
public:
	iterator begin() { return begin_ = (NULL != head_ ?  (head_->level[0].next) : NULL);}
    iterator end() {return end_;}

public:
	skiplist();
	explicit skiplist(const pcompare& cmp);
	virtual ~skiplist();
	void clear();
	void skiplist_free();

	//init the skiplist
	bool init();

private:
	//creat the new node
	pvalue_node create_node(unsigned int level, Key& key, Value& value);
	pvalue_node create_node(unsigned int level);
	pvalue_node create_node(void* buffer, unsigned int level, Key& key, Value& value);
	void insert(pvalue_node pnode,unsigned int level);
	void remove(pvalue_node pnode);
    //get the level of the node
	unsigned int get_random_level();
	bool equal(pvalue_node pnode1,pvalue_node pnode2);

private:
	//typedef stdext::hash_map<Key,pvalue_node> container_;
	//typedef boost::unordered::unordered_map<Key,pvalue_node> container_;
	typedef std::map<Key,pvalue_node> container_;
	typedef typename container_::iterator map_iterator_;
	container_ node_caches_;

    //pcompare  const secondcompare_;  //compare function for user define
	pcompare secondcompare_;               //compare function for user define
	unsigned int current_level_;                 //the level of skiplist
	unsigned long long length_;                //the length of the list

	//pvalue_node const head_;               //the first node of the list
	pvalue_node  head_;               //the first node of the list
	pvalue_node  tail_;                  //the last node of the list

	iterator begin_;               //point first list node
	iterator end_;                  //point  null
	iterator find_;                 //point the node of the key

	Random rnd_;               // Read/written only by Insert(),随机器，产生随机的level层数
};

template<typename Key,typename Value>
bool skiplist<Key,Value>::insert(Key& key,Value& value,pcompare pcmp)
{
	if( pcmp  && NULL == secondcompare_  ) {
		secondcompare_ = pcmp;
	}
	if( NULL == secondcompare_ ) return false;

	//the key is already exist
	iterator it  = find(key);
	if( it != end() ) return false;

	//random the new level for the new node
	int level = get_random_level();

	//creat the new node
	bool result(false);
	pvalue_node new_node = create_node(level,key,value);
	if( new_node ) {
        insert(new_node, level); result = true;
		node_caches_.insert(std::make_pair(key,new_node));
	}

	return result;
}

template<typename Key,typename Value>
bool skiplist<Key,Value>::insert(Key& key,Value& value)
{
	if( NULL == secondcompare_ ) return false;

	//the key is already exist
	iterator it  = find(key);
	if( it != end() ) return false;

	//random the new level for the new node
	int level = get_random_level();

	//creat the new node
	bool result = false;
	pvalue_node new_node = create_node(level,key,value);
	if( new_node ) {
		insert(new_node,level); result = true;
		node_caches_.insert(std::make_pair(key,new_node));
	}

	return result;
}
template<typename Key,typename Value>
void skiplist<Key,Value>::contain_topn(Key& current_key, int front_count,std::vector<Value>& output, bool& extra)
{
	if( !front_count || NULL == head_ ) return;

    //extra:true current_key在排名范围外[1:offset+front_count]
    extra = true;
    int count(0);
	pvalue_node  current_node = head_;
	while( NULL != current_node->level[0].next && count++ < front_count )
	{
        if ((current_node->level[0].next)->first == current_key) {
            extra = false;
        }
		output.push_back((current_node->level[0].next)->second);
		current_node = (current_node->level[0].next);
	}

    if (extra) {
		map_iterator_ it = node_caches_.find(current_key);
		if( it != node_caches_.end() ) {
			if( (*it).second ) {
				output.push_back( ((*it).second)->second);
			}
		}
	}
}

template<typename Key,typename Value>
void skiplist<Key, Value>::contain_topn(Key& current_key, int offset, int front_count, std::vector<Value>& output, bool& extra)
{
	if( !front_count || NULL == head_ ) return;

    //extra:true current_key在排名范围外（offset:offset+front_count]
    extra = true;
	int count(0); bool symbol(true);
	pvalue_node  current_node = head_;
	while( NULL != current_node->level[0].next && count++ < front_count )
	{
        if (offset < count)
        {
            //在有效范围内找到了该current_key
            if ((current_node->level[0].next)->first == current_key)
            {
                symbol = false; extra = false;
            }
            output.push_back((current_node->level[0].next)->second);
        } else
        if ((current_node->level[0].next)->first == current_key)
        {
            output.push_back((current_node->level[0].next)->second);
            symbol = false;
        }

		current_node = (current_node->level[0].next);
	}

    if (symbol) {
		map_iterator_ it = node_caches_.find(current_key);
		if( it != node_caches_.end() ) {
			if( (*it).second ) {
				output.push_back( ((*it).second)->second);
			}
		}
	}
}


template<typename Key,typename Value>
void skiplist<Key,Value>::skiplist_delete(const std::vector<Key>& keys){
    for(unsigned long long i=0 ; i < keys.size() ; ++i){
        map_iterator_ it = node_caches_.find(keys[i]);
        if( it != node_caches_.end() )
        {
            if( (*it).second ) {

                //remove from the list
                remove((*it).second);

                //release the node;
                //free((*it).second);
                pvalue_node pnode = (*it).second;
                pnode->~value_node();
                LIB_FREE((*it).second);
                node_caches_.erase(it);
            }
        }
    }
}

template<typename Key,typename Value>
typename skiplist<Key,Value>::iterator skiplist<Key,Value>::skiplist_delete(const Key& key)
{
	//clear the find_iterator;
	skiplist<Key, Value>::iterator find_tmp;
	find_tmp = NULL;
	map_iterator_ it = node_caches_.find(key);
	if( it != node_caches_.end() )
	{
		if( (*it).second ) {

			//remove from the list
			remove((*it).second);
			//set the next node for iterator
			find_tmp = (*it).second->level[0].next;

			//release the node;
			//free((*it).second);
            pvalue_node pnode = (*it).second;
            pnode->~value_node();
			LIB_FREE((*it).second);
			node_caches_.erase(it);
		}
	}
	return find_tmp;
}

template<typename Key,typename Value>
void skiplist<Key,Value>::update(iterator& it, Value& value)
{
	if( it == end()) return;

	pvalue_node pcurrent_node = static_cast<pvalue_node>(&it);
	if( pcurrent_node )
	{
		pcurrent_node->second = value;

		//step1,remove from the list
		remove(pcurrent_node);

		//step2,insert the node on the right location
		insert(pcurrent_node,pcurrent_node->node_level);
	}
}

template<typename Key,typename Value>
 const typename skiplist<Key,Value>::iterator skiplist<Key,Value>::find(const Key& key)
{
	pvalue_node pcurrent_node(NULL);
	map_iterator_ it = node_caches_.find(key);
	if( it != node_caches_.end() )
	{
		if( (*it).second )	 {
			pcurrent_node = (*it).second;
		}
	}

	skiplist<Key, Value>::iterator find_tmp;
	find_tmp = pcurrent_node;

	return find_tmp;
}

template<typename Key,typename Value>
unsigned long long skiplist<Key,Value>::get_rank(const Key& key)
{
	unsigned long long rank = 0;
	register pvalue_node current_node(NULL),next_node(NULL);
	map_iterator_ it = node_caches_.find(key);
	if( it != node_caches_.end() )
	{
		if( NULL == (*it).second ) return rank;

		current_node = head_;
		for ( int i = current_level_-1; i >= 0; --i)
		{
			while( NULL != current_node->level[i].next  )
			{
				next_node = current_node ->level[i].next;
				if( key != next_node->first  &&
					(!secondcompare_(key,(*it).second->second,next_node->first,next_node->second)) ) {
					rank += current_node->level[i].span;
					current_node = next_node;
				} else {
					break;
				}
			}


			if( equal(current_node->level[i].next,(*it).second) ) {
				rank += current_node->level[i].span;
				return rank;
			}
		}
	}
	return rank;
}

template<typename Key,typename Value>
unsigned long long skiplist<Key,Value>::get_rankV(const Value& value)
{
    Key key;
	unsigned long long rank = 0;
	register pvalue_node current_node=head_,next_node(NULL);
    for ( int i = current_level_-1; i >= 0; --i)
    {
        while( NULL != current_node->level[i].next  )
        {
            next_node = current_node ->level[i].next;
            if( secondcompare_(next_node->first,next_node->second,key,value) ) {
                rank += current_node->level[i].span;
                current_node = next_node;
            } else {
                break;
            }
        }
    }
	return rank;
}

template<typename Key,typename Value>
unsigned long long skiplist<Key,Value>::size()
{
	return length_;
}

template<typename Key,typename Value>
skiplist<Key,Value>::skiplist() : secondcompare_(NULL),current_level_(1),length_(0),head_(NULL),begin_(NULL),
	end_(NULL), find_(NULL),rnd_(0xdeadbeef)
{
	srand((unsigned)time(NULL));
}

template<typename Key,typename Value>
skiplist<Key,Value>::skiplist(const pcompare& cmp) : secondcompare_(cmp),current_level_(1),length_(0),head_(NULL),
	begin_(NULL),end_(NULL), find_(NULL),rnd_(0xdeadbeef)
{
	srand((unsigned)time(NULL));
}

template<typename Key,typename Value>
skiplist<Key,Value>::~skiplist()
{
	skiplist_free();
}

template<typename Key,typename Value>
void skiplist<Key,Value>::clear()
{
	skiplist_free();
}

template<typename Key,typename Value>
void skiplist<Key,Value>::skiplist_free()
{
	//release the node
    if (head_)  head_->~value_node(); LIB_FREE(head_);//free(head_);
	//if( tail_ ) free(tail_);
	for (map_iterator_ it = node_caches_.begin();
		it != node_caches_.end(); ++it)
	{
		if( it-> second ) {
			//free(it->second);
            it->second->~value_node();
			LIB_FREE(it->second);
		}
	}
	node_caches_.clear();
	head_ = NULL; tail_ = NULL;
}

template<typename Key,typename Value>
bool skiplist<Key,Value>::init()
{
	if( NULL != head_ )  return true;

	//level begin 0
	head_ = create_node(SKIPLIST_MAXLEVEL);
	if( NULL == head_ ) return false;

	//init next array
	for (int i = 0; i < SKIPLIST_MAXLEVEL; ++i) {
		head_ -> level[i].pre = NULL;
		head_ -> level[i].next = NULL;
		head_ -> level[i].span = 0;
	}

	return true;
}

template<typename Key, typename Value>
 typename skiplist<Key,Value>::pvalue_node skiplist<Key,Value>::create_node(unsigned int level,Key& key, Value& value)
 {
	 //create the new node
	 //pvalue_node pnode = (pvalue_node)malloc(sizeof(value_node)+level*sizeof(value_node::Level_Node));
	 pvalue_node pnode =(pvalue_node)LIB_MALLOC(sizeof(value_node)+level*sizeof(typename value_node::Level_Node));
	 if( NULL != pnode )
	 {
         new (pnode)value_node();
		 pnode -> first = key;
		 pnode -> second = value;
		 pnode -> node_level = level;
		 return pnode;
	 }
	 return NULL;
 }

template<typename Key, typename Value>
 typename skiplist<Key,Value>::pvalue_node skiplist<Key,Value>::create_node(void* buffer, unsigned int level,Key& key, Value& value)
 {
	 if( NULL != buffer )
	 {
         pvalue_node pnode=(pvalue_node)buffer;
         new (pnode) value_node();
		 pnode -> first = key;
		 pnode -> second = value;
		 pnode -> node_level = level;
		 return pnode;
	 }
	 return NULL;
 }

 template<typename Key,typename Value>
 typename skiplist<Key,Value>::pvalue_node skiplist<Key,Value>::create_node(unsigned int level)
 {
	  //pvalue_node pnode = (pvalue_node)malloc(sizeof(value_node)+level * sizeof(value_node::Level_Node));
	  pvalue_node pnode = (pvalue_node)LIB_MALLOC(sizeof(value_node)+level*sizeof(typename value_node::Level_Node));
	  if( NULL != pnode)
	  {
          new  (pnode)value_node();
		  pnode -> node_level = level;
		  return pnode;
	  }
	  return NULL;
 }

template<typename Key, typename Value>
void skiplist<Key,Value>::insert(pvalue_node pnode,unsigned int level)
{
		if( NULL == pnode || NULL == secondcompare_ ) return;

		unsigned long long rank[SKIPLIST_MAXLEVEL] = {0};   //record the span of the node by layer to layer
		pvalue_node update[SKIPLIST_MAXLEVEL] = {NULL};
		register pvalue_node current_node(NULL),next_node(NULL);
		//if the head node is not exsit,creat the new one
		if( NULL == head_ ) {
			init();
		}

		//search the right location
		current_node = head_;
		for ( int i = current_level_-1; i >= 0; --i)
		{
			//if the level is the max,set rank[i] zero,if not,set rank[i] the previous rank
			rank[i] = (i == (current_level_-1) ) ? 0 : rank[i+1];
			while( NULL != current_node->level[i].next  )
			{
				next_node = current_node ->level[i].next;
				if( !secondcompare_(pnode->first,pnode->second,next_node->first,next_node->second)) {
					rank[i] += current_node->level[i].span;
					current_node = next_node;
				} else {
					break;
				}
			}
			update[i] = current_node;
		}

		//if the current_level_ of skiplist is bigger than new level,update the current_level_
		if ( level > current_level_  )
		{
			for(unsigned int i = current_level_; i < level; ++i) {
				//update the rank
				rank[i] = 0;
				update[i] = head_;
				update[i]->level[i].span = length_;
			}
			current_level_ = level;
		}

		//insert to the list by layer to layer
		for ( int i = level-1; i >= 0; --i)
		{
			//insert the new node like double linked list
			pnode->level[i].next = update[i]->level[i].next;
			if( NULL != update[i]->level[i].next) {
				(update[i]->level[i].next)->level[i].pre = pnode;
			}
			pnode->level[i].pre = update[i];
			update[i]->level[i].next = pnode;

			//update span covered by update[i] as pnode is inserted here
			pnode->level[i].span = update[i]->level[i].span - (rank[0]-rank[i]);
			update[i]->level[i].span = (rank[0]-rank[i]) + 1;
		}

		//if the level of the node is less than the current level of the list ,
		//update the span that is not inserted on the level
		for (unsigned int i = level; i < current_level_; ++i)
		{
			update[i]->level[i].span++;
		}

		//set the tail point of the list
		if( NULL == pnode->level[0].next ) {
			tail_ = pnode;
		}

		//update the length of the list
		length_++;
 }

template<typename Key, typename Value>
void skiplist<Key,Value>::remove(pvalue_node pnode)
{
	if( NULL == head_  || NULL == pnode) return;

	unsigned int node_level(0);
	pvalue_node update[SKIPLIST_MAXLEVEL] = {NULL};
	register pvalue_node current_node(NULL),find_node(NULL);

	//search the right location
	current_node = pnode;
	for (unsigned i = 0; i < current_level_  ; ++i)
	{
		find_node = ( pnode == current_node ) ?
			current_node->level[i].pre : current_node;

		if( node_level++ >= current_node->node_level - 1
			&& i < current_level_-1 )
		{
			while(head_ != current_node->level[i].pre &&
				(current_node->level[i].pre)->node_level <= current_node->node_level ) {
					current_node = current_node->level[i].pre;
			}

			current_node = current_node->level[i].pre;
		}

		update[i] = find_node;
	}

	//remove from the list ,and update the span by layer to layer
	for ( int i = current_level_-1; i >= 0; --i)
	{
		if( update[i] ) {
			if( update[i]->level[i].next == pnode ) {
				update[i]->level[i].span += pnode->level[i].span - 1;
				update[i]->level[i].next = pnode->level[i].next;
				if( pnode->level[i].next ) {
					(pnode->level[i].next )->level[i].pre = update[i];
				}
			} else {
				update[i]->level[i].span -= 1;
			}
		}
	}

	//deal with the pre point of the node on the last level
	if( NULL == pnode->level[0].next ) {
		tail_ = pnode ->level[0].pre;
		//(pnode->level[0].next)->pre = pnode->pre;
	}

	//if the deleted node is last node on the level,change the level of the list
	for ( int i = current_level_ - 1; i >= 0 && current_level_ > 1; --i)
	{
		if( NULL == head_->level[i].next ) {
			current_level_--;
		}
	}

	//adjust the length of the list
	length_--;
}

template<typename Key, typename Value>
unsigned int skiplist<Key,Value>::get_random_level()
{
	static const unsigned int kBranching = 4;
	unsigned int level = 1;
	while (level < SKIPLIST_MAXLEVEL && ((rnd_.Next() % kBranching) == 0)) {
		level++;
	}
	/*while((rand()&0xFFFF) < (SKIPLIST_P * 0xFFFF))
	{
		level++;
	}
	return (level < SKIPLIST_MAXLEVEL ? level : SKIPLIST_MAXLEVEL);*/
	return level;
}

template<typename Key, typename Value>
bool skiplist<Key,Value>::equal(pvalue_node pnode1,pvalue_node pnode2)
{
	bool result(false);
	if( NULL != pnode1 && NULL != pnode2 ) {
		if( pnode1-> first == pnode2 -> first ) {
			result  = true;
		}
	}
	return result;
}

template<typename Key, typename Value>
bool skiplist<Key, Value>::find_n_score_in(Value  value1, Value  value2, s_uint32 n, std::set<Key>& rule_out, std::vector<Key>&ch_ids)
{
    bool result(false);
    if (NULL == value1 || NULL == secondcompare_) return result;

    unsigned long long rank[SKIPLIST_MAXLEVEL] = { 0 };   //record the span of the node by layer to layer

    register pvalue_node current_node(NULL), next_node(NULL), choice_node(NULL);

    int level = get_random_level();

    //if the head node is not exsit,creat the new one
    if (NULL == head_) {
        return result;
    }
    s_uint32 actor_id = 0;
    //creat the new node
    pvalue_node new_node = create_node(level, actor_id, value1);
    if (new_node) {

        //search the right location
        current_node = head_;
        for (int i = current_level_ - 1; i >= 0; --i)
        {
            //if the level is the max,set rank[i] zero,if not,set rank[i] the previous rank
            rank[i] = (i == (current_level_ - 1)) ? 0 : rank[i + 1];
            while (NULL != current_node->level[i].next)
            {
                next_node = current_node->level[i].next;
                if (!secondcompare_(new_node->first, new_node->second, next_node->first, next_node->second)) {
                    rank[i] += current_node->level[i].span;
                    current_node = next_node;
                }
                else {
                    break;
                }
            }
        }


        choice_node = current_node->level[0].next;
        while (choice_node)
        {
            if (rule_out.find(choice_node->first) == rule_out.end())
            {
                ch_ids.push_back(choice_node->first);
            }
            if (ch_ids.size() >= n)
            {
                break;
            }
            choice_node = (choice_node->level[0].next);
        }
        if (0 == ch_ids.size())
        {
            choice_node = current_node;
            while (head_ != choice_node)
            {
                if (rule_out.find(choice_node->first) == rule_out.end())
                {
                    ch_ids.push_back(choice_node->first);
                }
                if (ch_ids.size() >= n)
                {
                    break;
                }
                choice_node = (choice_node->level[0].pre);
            }
        }
        if (0 < ch_ids.size())
        {
            result = true;
        }
    }
    new_node->~value_node();
    LIB_FREE(new_node);

    return result;
}

//比较函数 >=或<= 时为 [value_min,value_max] ; 比较函数 >或< 时为 (value_min,value_max)
template<typename Key, typename Value>
bool skiplist<Key, Value>::exist_n_by_score_range(Value value_min, Value value_max, s_uint32 n)
{
    if( n == 0 )
        return true;

    if ( NULL == secondcompare_ || NULL == head_)
        return false;

    pvalue_node pcurr_node(NULL);
    pvalue_node pnext_node(NULL);
    pvalue_node pchoice_node(NULL);

    int level = get_random_level();

    Key key_id;
    //creat the new node
    char front_stack[NODE_SIZE],back_stack[NODE_SIZE];
    pvalue_node pnode_front = create_node((void*)front_stack, level, key_id, value_max);
    pvalue_node pnode_back = create_node((void*)back_stack, level, key_id, value_min);
    if (NULL == pnode_front || NULL == pnode_back)
        return false;

    if(!secondcompare_(pnode_front->first, pnode_front->second, pnode_back->first, pnode_back->second)){
        pvalue_node temp = pnode_back;
        pnode_back = pnode_front;
        pnode_front = temp;
    }

    //查找前面的节点
    pcurr_node = head_;
    for (int i = current_level_ - 1; i >= 0; --i)
    {
        while (NULL != pcurr_node->level[i].next)
        {
            pnext_node = pcurr_node->level[i].next;
            if (!secondcompare_(pnode_front->first, pnode_front->second, pnext_node->first, pnext_node->second))
            {
                pcurr_node = pnext_node;
            }
            else
                break;
        }
    }

    //迭代查找积分范围内的节点
    pchoice_node = pcurr_node->level[0].next;
    while ( n && NULL != pchoice_node )
    {
        if (secondcompare_(pchoice_node->first, pchoice_node->second , pnode_back->first, pnode_back->second))
        {
            --n;
            pchoice_node = (pchoice_node->level[0].next);
        }
        else
            break;
    }

    return (n==0);
}

//比较函数 >=或<= 时为 [value_min,value_max] ; 比较函数 >或< 时为 (value_min,value_max)
template<typename Key, typename Value>
bool skiplist<Key, Value>::get_n_by_score_range(Value value_min, Value value_max, s_uint32 n , std::vector<Key>& output_ids)
{
    if( n == 0 )
        return true;

    if ( NULL == secondcompare_ || NULL == head_)
        return false;

    pvalue_node pcurr_node(NULL);
    pvalue_node pnext_node(NULL);
    pvalue_node pchoice_node(NULL);

    int level = get_random_level();

    Key key_id;
    //creat the new node
    char front_stack[NODE_SIZE],back_stack[NODE_SIZE];
    pvalue_node pnode_front = create_node((void*)front_stack, level, key_id, value_max);
    pvalue_node pnode_back = create_node((void*)back_stack, level, key_id, value_min);
    if (NULL == pnode_front || NULL == pnode_back)
        return false;

    if(!secondcompare_(pnode_front->first, pnode_front->second, pnode_back->first, pnode_back->second)){
        pvalue_node temp = pnode_back;
        pnode_back = pnode_front;
        pnode_front = temp;
    }

    //查找前面的节点
    pcurr_node = head_;
    for (int i = current_level_ - 1; i >= 0; --i)
    {
        while (NULL != pcurr_node->level[i].next)
        {
            pnext_node = pcurr_node->level[i].next;
            if (!secondcompare_(pnode_front->first, pnode_front->second, pnext_node->first, pnext_node->second))
            {
                pcurr_node = pnext_node;
            }
            else
                break;
        }
    }

    //迭代查找积分范围内的节点
    pchoice_node = pcurr_node->level[0].next;
    while (NULL != pchoice_node && output_ids.size()<n)
    {
        if (secondcompare_(pchoice_node->first, pchoice_node->second , pnode_back->first, pnode_back->second))
        {
            output_ids.push_back(pchoice_node->first);
            pchoice_node = (pchoice_node->level[0].next);
        }
        else
            break;
    }

    return (output_ids.size()>=n);
}

//比较函数 >=或<= 时为 [value_min,value_max] ; 比较函数 >或< 时为 (value_min,value_max)
template<typename Key, typename Value>
bool skiplist<Key, Value>::get_mid_n_by_score_range(Value value_min,  Value value_mid, Value value_max, s_uint32 n , std::vector<Key>& output_ids)
{
    if( n == 0 )
        return true;

    if ( NULL == secondcompare_ || NULL == head_)
        return false;

    Key key_id;
    int level = get_random_level();
    char mid_stack[NODE_SIZE];
    pvalue_node pnode_mid=create_node((void*)mid_stack, level, key_id, value_mid);
    if(NULL == pnode_mid)
        return false;

    pvalue_node pcurr_node = head_,pnext_node(NULL);
    for (int i = current_level_ - 1; i >= 0; --i)
    {
        while (NULL != pcurr_node->level[i].next)
        {
            pnext_node = pcurr_node->level[i].next;
            if (!secondcompare_(pnode_mid->first, pnode_mid->second, pnext_node->first, pnext_node->second))
            {
                pcurr_node = pnext_node;
            }
            else
                break;
        }
    }

    //creat the new node
    char front_stack[NODE_SIZE],back_stack[NODE_SIZE];
    pvalue_node pnode_front = create_node((void*)front_stack, level, key_id, value_max);
    pvalue_node pnode_back = create_node((void*)back_stack, level, key_id, value_min);
    if (NULL == pnode_front || NULL == pnode_back)
        return false;

    if(!secondcompare_(pnode_front->first, pnode_front->second, pnode_back->first, pnode_back->second)){
        pvalue_node temp = pnode_back;
        pnode_back = pnode_front;
        pnode_front = temp;
    }

    if(!secondcompare_(pnode_front->first,pnode_front->second,pnode_mid->first,pnode_mid->second) || !secondcompare_(pnode_mid->first,pnode_mid->second,pnode_back->first,pnode_back->second))
        return false;

    //迭代查找积分范围内的节点
    bool front_over=false,back_over=false,direct=true;
    pvalue_node temp_back=pcurr_node->level[0].next;
    pvalue_node temp_front=pcurr_node;
    while ((!front_over || !back_over) && output_ids.size()<n)
    {
        if( direct )
        {
            if (temp_back && secondcompare_(temp_back->first, temp_back->second , pnode_back->first, pnode_back->second))
            {
                output_ids.push_back(temp_back->first);
                temp_back = (temp_back->level[0].next);
            }
            else
                back_over = true;

           if( !front_over ) direct = !direct;
        }
        else
        {
            if (temp_front && temp_front != head_ && secondcompare_(pnode_front->first, pnode_front->second , temp_front->first, temp_front->second))
            {
                output_ids.push_back(temp_front->first);
                temp_front = (temp_front->level[0].pre);
            }
            else
                front_over = true;

            if( !back_over ) direct = !direct;
        }
    }

    return (output_ids.size()>=n);
}

//比较函数 >=或<= 时为 [value_min,value_max] ; 比较函数 >或< 时为 (value_min,value_max)
template<typename Key, typename Value>
bool skiplist<Key, Value>::get_by_score_range(Value value_min, Value value_max, std::vector<Key>& output_ids)
{
    if (NULL == value_min || NULL == value_max || NULL == secondcompare_ || NULL == head_)
        return false;

    pvalue_node pcurr_node(NULL);
    pvalue_node pnext_node(NULL);
    pvalue_node pchoice_node(NULL);

    int level = get_random_level();

    Key key_id = 0;
    //creat the new node
    char min_stack[NODE_SIZE],max_stack[NODE_SIZE];
    pvalue_node pnode_max = create_node((void*)max_stack, level, key_id, value_max);
    pvalue_node pnode_min = create_node((void*)min_stack, level, key_id, value_min);
    if (NULL == pnode_min || NULL == pnode_max)
        return false;

    //预处理
    if(!secondcompare_(pnode_min->first, pnode_min->second, pnode_max->first, pnode_max->second)){
        pvalue_node temp = pnode_max;
        pnode_max = pnode_min;
        pnode_min = temp;
    }

    //查找前面的节点
    pcurr_node = head_;
    for (int i = current_level_ - 1; i >= 0; --i)
    {
        while (NULL != pcurr_node->level[i].next)
        {
            pnext_node = pcurr_node->level[i].next;
            if (!secondcompare_(pnode_max->first, pnode_max->second, pnext_node->first, pnext_node->second))
            {
                pcurr_node = pnext_node;
            }
            else
                break;
        }
    }

    //迭代查找积分范围内的节点
    pchoice_node = pcurr_node->level[0].next;
    while (NULL != pchoice_node)
    {
        output_ids.push_back(pchoice_node->first);
        if (!secondcompare_(pnode_min->first, pnode_min->second, pchoice_node->first, pchoice_node->second))
        {
            pchoice_node = (pchoice_node->level[0].next);
        }
        else
            break;
    }

    return !output_ids.empty();
}

template<typename Key, typename Value>
skiplist<Key, Value>::pvalue_node skiplist<Key, Value>::find_near_node(Key& key, Value& value)
{
    if ( NULL == secondcompare_ || NULL == head_)
        return false;

    int level = get_random_level();
    char find_stack[NODE_SIZE];
    pvalue_node pnode_find=create_node((void*)find_stack, level, key, value);
    if(NULL == pnode_find)
        return false;

    pvalue_node pcurr_node = head_;
    for (int i = current_level_ - 1; i >= 0; --i)
    {
        while (NULL != pcurr_node->level[i].next)
        {
            pnext_node = pcurr_node->level[i].next;
            if (!secondcompare_(pnode_find->first, pnode_find->second, pnext_node->first, pnext_node->second))
            {
                pcurr_node = pnext_node;
            }
            else
                break;
        }
    }
    return pcurr_node;
}

#endif
