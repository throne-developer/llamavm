/*
** �﷨������ָ������
** ���룺�����ļ�
** �����ObjectProto���󣬰���ָ���б�ͳ�������
** ���֪ʶ��Lua�﷨���������޹��ķ����ݹ��½��������沨�����ʽ�����ڶ�ջ�������ָ�ָ�����
** �ο����룺Lua3.2��Lua5.1
*/
#ifndef grammar_h__
#define grammar_h__

#include <map>
#include "lex.h"
#include "base/lobject.h"

class Grammar {
public:
  struct LoopLabel;

  /*��������*/
  struct FuncState {
    FuncState() : proto(0), prev(0), breakLabel(0), contLabel(0), localNum(0) {}

    ObjectProto* proto;
    FuncState* prev;  /*������*/

    /*�����ڳ����ַ����б�valueΪ�����ַ����� ObjectProto::constants ��������*/
    std::map<SymbolString, int> constStringIndexs;
    LoopLabel* breakLabel;  /*break��continue��ת��Ϣ*/
    LoopLabel* contLabel;

    int localNum;  /*��Ч�ֲ�������*/
    SymbolString localVars[GRAM_LOCAL_VARS];  /*�ֲ��������ƣ����±��Ӧ����ջλ��*/
  };

  /*ֵ����*/
  enum VarType {
    VAR_GLOBAL = 0,  /*ȫ�ֱ������� a */
    VAR_LOCAL,  /*�ֲ��������� local b */
    VAR_DOT,  /*table��Ա���� a.b */
    VAR_INDEXED,  /*table��Ա���� a[b] */
    VAR_EXPR,  /*���ʽ����info����0��ʾ�������ʽ����func(...)��0��ʾ���� */
  };

  /*ֵ����*/
  struct VarDesc {
    VarDesc() { reset(); }
    void reset() { vt = VAR_EXPR; info = 0; }
    
    void as_global(int constIndex) { vt = VAR_GLOBAL; info = constIndex; }
    void as_local(int localIndex) { vt = VAR_LOCAL; info = localIndex; }
    void as_func_expr(int callpc) { vt = VAR_EXPR; info = callpc; }
    void as_dot(int constIndex) { vt = VAR_DOT; info = constIndex; }
    void as_indexed() { vt = VAR_INDEXED; info = 0; }

    VarType vt;
    int info;  /*globalΪ�������ĳ���������localΪջλ�ã�exprΪcallָ��pc��dotΪ��������*/
  };

  /*���ʽ�б��������� 1,"aaa",func() */
  struct ExplistDesc {
    ExplistDesc(){ reset(); }
    void reset() { expNum = 0; callPc = 0; }

    int expNum;
    int callPc;  /*�����һ�����ʽΪ�������ã���¼callָ��pc*/
  };

  /*�������ջ*/
  struct OpStack {
    OpPriority oplist[GRAM_OP_STACK_NUM];
    int top;
  };

  enum FieldType {
    FIELD_NONE = 0,
    FIELD_HASH,
    FIELD_ARRAY,
  };

  /*table��������*/
  struct TablePartDesc {
    TablePartDesc() : ft(FIELD_NONE), fields(0) {}

    FieldType ft;
    int fields;
  };

  /*ѭ��������ת��Ϣ*/
  struct LoopLabel {
    LoopLabel() : prev(0), jmpNum(0), destPc(0), localNum(0) {}

    int jmpPc[GRAM_LOOP_JMP_NUM];
    int jmpNum;
    int destPc;
    int localNum;
    LoopLabel* prev;
  };

public:
  Grammar();
  ~Grammar();

  ObjectProto* parse(LlamaState* ls, const char* filename);

private:
  void open_func(FuncState& fs, FuncState* prev);
  void close_func(FuncState& fs);

  void chunk(FuncState& fs);
  bool stat(FuncState& fs);

  void name_stat(FuncState& fs);
  void var_or_func(FuncState& fs, VarDesc& v);
  void var_or_func_suffix(FuncState& fs, VarDesc& v);

  int func_call(FuncState& fs);
  int assignment(FuncState& fs, VarDesc& v, int vars);
  void explist(FuncState& fs, ExplistDesc& desc);
  void explist1(FuncState& fs, ExplistDesc& desc);
  void expr(FuncState& fs, VarDesc& v);
  void expr_code_var(FuncState& fs);
  void binop_expr(FuncState& fs, VarDesc& v);
  void unary_expr(FuncState& fs, VarDesc& v, OpStack& opStack);
  void simple_exp(FuncState& fs, VarDesc& v);

  void table_construct(FuncState& fs);
  void table_part(FuncState& fs, TablePartDesc& desc);
  int field_list(FuncState& fs, FieldType ft);
  void hash_field(FuncState& fs);

  void if_stat(FuncState& fs);
  void while_stat(FuncState& fs);
  void break_continue_stat(FuncState& fs, bool isbreak);
  void condition(FuncState& fs);
  void block(FuncState& fs);
  void enter_loop(FuncState& fs, LoopLabel& bl, LoopLabel& cl);
  void leave_loop(FuncState& fs, LoopLabel& bl, LoopLabel& cl);

  void for_stat(FuncState& fs);
  int for_init(FuncState& fs);

  void local_stat(FuncState& fs);
  void save_localvar(FuncState& fs, const SymbolString& name, int varIndex);

  void func_stat(FuncState& fs);
  void func_name(FuncState& fs, VarDesc& v);
  void body(FuncState& fs);
  void parlist(FuncState& fs);
  void return_stat(FuncState& fs);

private:
  void code_op(FuncState& fs, OpCode op);
  void code_arg(FuncState& fs, int arg);
  int code_op_arg(FuncState& fs, OpCode op, int arg);
  void fix_jmp_to_next(FuncState&fs, int jmpPc);
  void fix_jmp_dest(FuncState&fs, int jmpPc, int destPc);
  void fix_op_arg(FuncState& fs, int pc, int arg);
  void code_push_var(FuncState& fs, VarDesc& v);
  void code_store_var(FuncState& fs, VarDesc& v);

private:
  void fetch_name(FuncState& fs, SymbolString& name);
  int name_constant(FuncState& fs);
  int string_constant(FuncState& fs, const char* name);
  int number_constant(FuncState& fs, Number n);
  int next_constant(FuncState& fs);

  void push_op(OpStack&sk, OpPriority op);
  void code_higher_op(FuncState& fs, OpStack& sk, int priority);

  void search_var(FuncState& fs, VarDesc& v);
  void fix_func_returns(FuncState&fs, int callPc, int results);
  void adjust_multi_assign(FuncState& fs, int nvars, ExplistDesc& desc);
  void adjust_stack(FuncState& fs, int need);
  void pop_local_vars(FuncState& fs, int n);
  
  void next();
  void check(TokenType tt);
  void check_next(TokenType tt);
  bool optional(TokenType tt);
  void error(const char* msg);

private:
  LlamaState* lstate;
  Lex lexState;
  Token currToken;
  int prevLine;
};

#endif // grammar_h__
