/*
** Table数据结构：
（1）包含array和hash两部分，hash基于开放地址法实现
（2）key为除nil之外的任意类型
（3）key类型对应的索引计算方法
[number ]  array部分使用int索引，hash部分使用number的hash值
[string ]  字符串hash值
[func   ]  指针hash值
[proto  ]  指针hash值
[table  ]  指针hash值

** 新增元素规则
（1）key为number，且在arraySize范围内，存入array； 否则都存入hash
（2）若hash无空闲结点，触发rehash
*/
#ifndef table_h__
#define table_h__

#include "base/memory.h"
#include "base/basedef.h"

struct Table {
  friend class gc;

private:
  struct Node {
    Node() : next(0) {}
    bool used() { return NOT_NIL(value) && NOT_NIL(key); }

    TObject key;
    TObject value;
    Node* next;  /*下一个冲突结点*/
  };
  enum { INT_BITS = 26, };

public:
  Table() : arraySize(0), hashSize(0), arrayData(0), hashData(0), last_free(-1) {}
  ~Table() {
    Allocator<TObject>::del_array(arrayData);
    Allocator<Node>::del_array(hashData);
  }

  void init(int arrays, int hashs) { resize(arrays, hashs); }
  int getn();
  TObject next_key(const TObject& key);
  
  void set(const TObject& key, const TObject& value);
  void set_num(int n, const TObject& value);
  TObject get(const TObject& key);
  TObject get_num(int n);

private:
  int nkey(const TObject& key);
  Node* search_node(const TObject& key);
  Node* main_position(const TObject& key);
  Node* get_last_free_pos();
  Node* adjust_pos(Node* mainPos, Node* freePos, Node* currActualPos);
  int get_pos(const TObject& key);

  void rehash(const TObject& key);
  void array_stat(int sections[], int& nkeys, int& keys);
  void hash_stat(int sections[], int& nkeys, int& keys);
  bool count_int_key(const TObject& key, int sections[]);
  int compute_array_size(int sections[], int nkeys);
  void resize(int newArraySize, int newHashSize);

private:
  TObject* arrayData;
  int arraySize;
  Node* hashData;
  int hashSize;
  int last_free;
};

#endif // table_h__