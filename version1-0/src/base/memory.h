/*
** �ڴ����ͼ�أ�
��1��ʹ��Allocator����ֶ�new��delete�����ڼ���ڴ�й¶
��2��BlockMonitor��¼������ͷŵ��ڴ��
��3��Arrayʵ�ֶ�̬����
��4��GCNode��¼����gcstate���Լ����������nextָ��
��5��GCLinkedList�������п�gc����
��6��HashSet�������г��ַ����������ַ����ڻ���gc����
��7��ShortString������ַ���
*/
#ifndef memory_h__
#define memory_h__

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define array_grow(_arr) { if (_arr.top>=_arr.capacity) { _arr.grow(__FUNCTION__); } }


class BlockMonitor {
public:
  static void reset();
  static void add(void* ptr, int size, const char* func, const char* prevFunc);
  static void del(void* ptr);
  static void dump();
};

template<class T>
class Allocator {
public:
  static T* new_one(const char* func, const char* prevFunc) {
    T* obj = new T();
    BlockMonitor::add(obj, sizeof(T), func, prevFunc);
    return obj;
  }

  static T* new_array(int size, const char* func, const char* prevFunc) {
    T* obj = new T[size];
    BlockMonitor::add(obj, sizeof(T)*(size), func, prevFunc);
    return obj;
  }

  static void del_one(T*& ptr) {
    if (ptr) {
      BlockMonitor::del(ptr);
      delete ptr;
      ptr = 0;
    }
  }

  static void del_array(T*& ptr) {
    if (ptr) {
      BlockMonitor::del(ptr);
      delete[] ptr;
      ptr = 0;
    }
  }
};

template<class T>
class Array {
public:
  Array() : data(0), capacity(0), top(0) {}

  ~Array() {
    Allocator<T>::del_array(data);
  }

private:
  Array(const Array&);  /*���ø��ƹ��죬�� Array a=b; */
  Array& operator=(const Array&);  /*���ø�ֵ�������� Array a; a=b; */

public:
  void init(int cap, const char* prevFunc) {
    data = Allocator<T>::new_array(cap, __FUNCTION__, prevFunc);
    capacity = cap;
    top = 0;
  }

  void grow(const char* prevFunc) {
    int newsize = capacity * 2;
    T* newData = Allocator<T>::new_array(newsize, __FUNCTION__, prevFunc);
    memcpy(newData, data, capacity*sizeof(T));
    Allocator<T>::del_array(data);

    capacity = newsize;
    data = newData;
  }

  void add(T obj) {
    if (top >= capacity) {
      grow(__FUNCTION__);
    }
    data[top++] = obj;
  }

  void clear() { top = 0; }

  int size() const { return top; }

public:
  T* data;
  int capacity;
  int top;
};

typedef Array<char> LongString;

enum GCState {
  GCSTATE_NO_MARK = 0,
  GCSTATE_MARKED,
  GCSTATE_FIXED,
};

template<class T>
struct GCNode {
  GCNode() : next(0), state(GCSTATE_NO_MARK) {}

  T* next;
  GCState state;
};

template<class T>
class GCLinkedList {
public:
  GCLinkedList() : head(0), blocks(0) {}

  void insert_head(T* data) {
    data->gc.next = head;
    head = data;
    blocks++;
  }

  void collect_unmarked(GCLinkedList& ulist) {
    T* prev = 0;
    T* curr = head;

    while (curr) {
      T* next = curr->gc.next;

      if (GCSTATE_NO_MARK == curr->gc.state) {  /*δ��ǣ�ȡ��*/
        ulist.insert_head(curr);
        blocks--;

        if (!prev) {  /*ͷ���*/
          head = next;
        }
        else {  /*�м���*/
          prev->gc.next = next;
        }
      }
      else {  /* �ѱ�ǣ��������������� */
        if (GCSTATE_MARKED == curr->gc.state) {
          curr->gc.state = GCSTATE_NO_MARK;
        }
        prev = curr;
      }

      curr = next;
    }
  }

  void free() {
    T* curr = head;
    while (curr) {
      T* next = curr->gc.next;
      Allocator<T>::del_one(curr);
      curr = next;
    }

    head = 0;
    blocks = 0;
  }

public:
  T* head;
  int blocks;
};

/*����T���ṩ hash��gc�ֶΣ�equal����*/
template<class T>
class HashSet {
public:
  typedef GCLinkedList<T> GCList;
  HashSet() : datas(0), used(0), capacity(0), minsize(0) {}

  ~HashSet() {
    Allocator<GCList>::del_array(datas);
  }

  void init(int minsz) {
    resize(minsz);
    minsize = minsz;
  }

  void add(T* value) {
    datas[value->hash % capacity].insert_head(value);
    if (++used > capacity) {
      resize(capacity * 2);  /*����*/
    }
  }

  void shrink() {
    if (used < capacity / 4 && capacity > minsize * 3) {
      resize(capacity / 2);  /*����*/
    }
  }

  T* find(const T& value) const {
    T* curr = datas[value.hash % capacity].head;
    while (curr) {
      if (value.equal(*curr)) {
        return curr;
      }
      curr = curr->gc.next;
    }
    return 0;
  }

  void collect_unmarked(GCList& ulist) {
    for (int i = 0; i < capacity; i++) {
      datas[i].collect_unmarked(ulist);
    }
    used -= ulist.blocks;
  }

  int used_num() const { return used; }

private:
  void resize(int newsize) {
    if (0 == newsize)
      return;

    GCList* newDatas = Allocator<GCList>::new_array(newsize, __FUNCTION__, __FUNCTION__);

    for (int i = 0; i < capacity; i++) {
      T* curr = datas[i].head;
      while (curr) {
        T* next = curr->gc.next;
        newDatas[curr->hash % newsize].insert_head(curr);
        curr = next;
      }
    }
    Allocator<GCList>::del_array(datas);
    
    capacity = newsize;
    datas = newDatas;
  }

private:
  GCList* datas;
  int used;
  int capacity;
  int minsize;
};

template<int BUFF_SIZE>
struct StaticString {
  StaticString() {
    length = 0;
    data[length] = '\0';
  }

  StaticString(const char* str) {
    length = 0;
    data[length] = '\0';
    set_str(str);
  }

  void clear() { length = 0; }

  void set_str(const char* str) {
    if (str) {
      length = strlen(str);
      if (length >= BUFF_SIZE) {
        length = BUFF_SIZE - 1;
      }
      strncpy(data, str, length);
      data[length] = '\0';
    }
  }

  void append(char c) {
    if (length < BUFF_SIZE) {
      data[length++] = c;
    }
  }

  void format(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    length = vsnprintf(data, BUFF_SIZE - 1, fmt, ap);
    va_end(ap);
    data[length] = '\0';
  }

  bool operator ==(const StaticString& other) const {
    return length == other.length && strncmp(data, other.data, length) == 0;
  }

  bool operator <(const StaticString& other) const {
    if (length != other.length) {
      return length < other.length;
    }
    return (strncmp(data, other.data, length) < 0 ? true : false);
  }

public:
  char data[BUFF_SIZE];
  int length;
};

typedef StaticString<128> SymbolString;
typedef StaticString<1024> ShortString;

#endif // memory_h__
