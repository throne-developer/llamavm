/*
** 语法解析、指令生成
** 输入：代码文件
** 输出：ObjectProto对象，包含指令列表和常量对象
** 相关知识：Lua语法、上下文无关文法、递归下降分析、逆波兰表达式、基于堆栈的虚拟机指令、指令回填
** 参考代码：Lua3.2、Lua5.1
*/
#ifndef grammar_h__
#define grammar_h__

#include <map>
#include "lex.h"
#include "base/lobject.h"

class Grammar {
public:
  struct LoopLabel;

  /*函数对象*/
  struct FuncState {
    FuncState() : proto(0), prev(0), breakLabel(0), contLabel(0), localNum(0) {}

    ObjectProto* proto;
    FuncState* prev;  /*父函数*/

    /*函数内常量字符串列表，value为常量字符串在 ObjectProto::constants 数组索引*/
    std::map<SymbolString, int> constStringIndexs;
    LoopLabel* breakLabel;  /*break、continue跳转信息*/
    LoopLabel* contLabel;

    int localNum;  /*有效局部变量数*/
    SymbolString localVars[GRAM_LOCAL_VARS];  /*局部变量名称，其下标对应运行栈位置*/
  };

  /*值类型*/
  enum VarType {
    VAR_GLOBAL = 0,  /*全局变量，如 a */
    VAR_LOCAL,  /*局部变量，如 local b */
    VAR_DOT,  /*table成员，如 a.b */
    VAR_INDEXED,  /*table成员，如 a[b] */
    VAR_EXPR,  /*表达式，若info大于0表示函数表达式，如func(...)，0表示常量 */
  };

  /*值描述*/
  struct VarDesc {
    VarDesc() { reset(); }
    void reset() { vt = VAR_EXPR; info = 0; }
    
    void as_global(int constIndex) { vt = VAR_GLOBAL; info = constIndex; }
    void as_local(int localIndex) { vt = VAR_LOCAL; info = localIndex; }
    void as_func_expr(int callpc) { vt = VAR_EXPR; info = callpc; }
    void as_dot(int constIndex) { vt = VAR_DOT; info = constIndex; }
    void as_indexed() { vt = VAR_INDEXED; info = 0; }

    VarType vt;
    int info;  /*global为变量名的常量索引，local为栈位置，expr为call指令pc，dot为常量索引*/
  };

  /*表达式列表描述，如 1,"aaa",func() */
  struct ExplistDesc {
    ExplistDesc(){ reset(); }
    void reset() { expNum = 0; callPc = 0; }

    int expNum;
    int callPc;  /*若最后一个表达式为函数调用，记录call指令pc*/
  };

  /*运算符堆栈*/
  struct OpStack {
    OpPriority oplist[GRAM_OP_STACK_NUM];
    int top;
  };

  enum FieldType {
    FIELD_NONE = 0,
    FIELD_HASH,
    FIELD_ARRAY,
  };

  /*table构造描述*/
  struct TablePartDesc {
    TablePartDesc() : ft(FIELD_NONE), fields(0) {}

    FieldType ft;
    int fields;
  };

  /*循环体内跳转信息*/
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
