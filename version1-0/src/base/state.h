/*
** �ڴ�������
��1���﷨�������������һ��proto���󣬳���֮��������������﷨�������Լ�����
��2��proto��������ĳ����б�ָ���б���proto����ֹͣ�����ʱͳһ���գ�������gc
��3����������й����У���̬�������ַ�����table������gc�Զ�����

** ���������
��1��Proto�ֽ���ָ��
��2������ջ
��3��ȫ�ֶ����
��4����̬��������

** ע������
��1��ͨ��stack_xxx������ջ�����ٺ������ô���
��2����ջ������ʱ��������������ջ��Ԫ�ص�ַ�����仯��
  �ڶ�ջ����������ʱ��Ӧ����ͨ��TObject&��TObject*����ջ��Ԫ�أ�ֱ��ʹ��TObject��ѡ�
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
