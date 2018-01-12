/*
** 内存管理规则：
（1）语法分析器的输出是一个proto对象，除此之外的其他对象由语法分析器自己回收
（2）proto对象包含的常量列表、指令列表、子proto对象，停止虚拟机时统一回收，不参与gc
（3）虚拟机运行过程中，动态创建的字符串、table对象由gc自动回收

** 虚拟机构成
（1）Proto字节码指令
（2）运行栈
（3）全局对象表
（4）动态对象链表

** 注意事项
（1）通过stack_xxx宏管理堆栈，减少函数调用次数
（2）堆栈在运行时会自增长，导致栈上元素地址发生变化。
  在堆栈可能自增长时，应避免通过TObject&、TObject*引用栈上元素，直接使用TObject最佳。
*/
#ifndef state_h__
#define state_h__

#include "lobject.h"

#define stack_top(_s)             (_s->sk.top)
#define stack_at(_s, _i)          (_s->sk.data[_i])
#define stack_reverse(_s, _i)     (_s->sk.data[_s->sk.top+(_i)])
#define stack_back(_s)            (_s->sk.data[_s->sk.top-1])
#define stack_pop(_s)             (--_s->sk.top)
#define stack_pop_n(_s,_n)        (_s->sk.top-=(_n))

#define stack_push_object(_s, _o) { array_grow(_s->sk); _s->sk.data[_s->sk.top++] = _o; }
#define stack_push_number(_s, _n) { array_grow(_s->sk); _s->sk.data[_s->sk.top].ot = OBJECT_NUMBER;  _s->sk.data[_s->sk.top++].value.num = _n; }
#define stack_push_nil(_s)        { array_grow(_s->sk); _s->sk.data[_s->sk.top++].ot = OBJECT_NIL; }

struct LlamaState {
  LlamaState();
  ~LlamaState();

  void execute_file(const char* filename);
  void execute_file_with_debug(const char* filename);

  void new_string(TObject& obj, const char* str, int length, const char* prevFunc, GCState state);
  ObjectProto* new_proto(const char* prevFunc);
  ObjectTable* new_table(int arrays, int hashs, const char* prevFunc, GCState state);

  void push_string(const char* s);
  void push_number(Number n);
  void push_object(TObject obj);
  void push_nil();
  void set_global(const TObject& key, const TObject& value);

  static void error(ShortString msg, int line = 0);
  static void add_buff(LongString& buff, const char* s, int length);

private:
  void open_state();
  void close_state();

public:
  Array<TObject> sk;
  Table globals;
  HashSet<ObjectString> strings;
  GCLinkedList<ObjectTable> tables;
  CStack cstack;
  int currPc;
};

#endif // state_h__
