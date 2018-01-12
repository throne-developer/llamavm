#include <stdio.h>
#include "gram.h"
#include "base/state.h"

Grammar::Grammar() {
  lstate = NULL;
  prevLine = 0;
}

Grammar::~Grammar() {
}

ObjectProto* Grammar::parse(LlamaState* ls, const char* filename) {
  lstate = ls;

  /*源码运行在一个隐式的函数内*/
  FuncState fs;
  open_func(fs, NULL);
  fs.proto->filename.set_str(filename);

  /*整个源码文件作为一个chunk处理，chunk表示代码块，等同于block。代码块可以嵌套，比如while嵌套while*/
  if (lexState.open_file(filename)) {
    try {
      next();
      chunk(fs);
    }
    catch (RuntimeError& e) {
      printf("============= parse exception =============\n");
      printf("[FILE] %s:%d, [ERROR] %s\n", filename, e.line, e.msg.data);
      fs.proto->codes.clear();
      fs.proto->lines.clear();
    }
  }

  lexState.close_file();
  close_func(fs);
  return fs.proto;
}

void Grammar::open_func(FuncState& fs, FuncState* prev) {
  int argNum = 0;
  fs.proto = lstate->new_proto(FUNC_NAME);
  fs.prev = prev;
  code_arg(fs, argNum);
}

/*函数解析结束，proto之外的所有成员都要释放*/
void Grammar::close_func(FuncState& fs) {
  code_op(fs, END_CODE);
}

/* chunk -> { stat [`;`] } */
void Grammar::chunk(FuncState& fs) {
  while (stat(fs)) {
    optional(TK_SEMICOLON);
  }
}

/*解析语句，返回false表示语句块结束*/
bool Grammar::stat(FuncState& fs) {
  bool chunkCont = true;

  switch (currToken.tt) {
  case TK_NAME:
    name_stat(fs);
    break;
  case TK_IF:  
    if_stat(fs);  
    break;
  case TK_WHILE:  
    while_stat(fs);  
    break;
  case TK_FOR:
    for_stat(fs);
    break;
  case TK_LOCAL:  
    local_stat(fs);  
    break;
  case TK_FUNCTION:  
    func_stat(fs);  
    break;
  case TK_RETURN:  
    return_stat(fs);  
    chunkCont = false;  /*最后一条语句*/
    break;
  case TK_BREAK:
  case TK_CONTINUE:
    break_continue_stat(fs, TK_BREAK == currToken.tt);
    chunkCont = false;
    break;
  case TK_DO:
    next();
    block(fs);
    check_next(TK_END);
    break;

  case TK_SEMICOLON:  /*单独分号，会停止stat解析；分号只能用于语句后面*/
  case TK_END:  /*end结束语句块*/
  case TK_ELSE:  /*上一个if或else语句块的结束*/
  case TK_ELSEIF:
  case TK_EOS:  /*源码块结束*/
    chunkCont = false;
    break;

  default:
    error("unexpected token");
    chunkCont = false;
    break;
  }
  return chunkCont;
}

void Grammar::name_stat(FuncState& fs) {
  VarDesc v;
  var_or_func(fs, v);
  
  if (VAR_EXPR == v.vt) {  /* stat -> func(...) */
    if (0 == v.info) {
      error("not function call");
    }
    fix_func_returns(fs, v.info, 0);  /*不需要返回值*/
  }
  else {  /* stat -> assignment */
    int leftExtra = assignment(fs, v, 1);
    adjust_stack(fs, -leftExtra);
  }
}

void Grammar::var_or_func(FuncState& fs, VarDesc& v) {
  search_var(fs, v);
  var_or_func_suffix(fs, v);
}

void Grammar::var_or_func_suffix(FuncState& fs, VarDesc& v) {
  while (true) {
    switch (currToken.tt) {
    case TK_L_PAREN: {
      code_push_var(fs, v);
      v.as_func_expr(func_call(fs));
      break;
    }
    case TK_L_SQUARE: {
      next();
      code_push_var(fs, v);
      expr_code_var(fs);
      check_next(TK_R_SQUARE);
      v.as_indexed();
      break;
    }
    case TK_POINT: {
      next();
      code_push_var(fs, v);
      v.as_dot(name_constant(fs));
      break;
    }
    default:
      return;
    }
  }
}

/*多变量赋值
（1）表达式列表 按照 从左到右 顺序生成指令
（2）变量列表 按照 从右到左 顺序生成指令
例如 a,b=1,2; 指令为： push 1 -> push 2 -> set b -> set a */
int Grammar::assignment(FuncState& fs, VarDesc& v, int vars) {
  int left = 0;

  if (VAR_DOT == v.vt) {
    code_op_arg(fs, OP_PUSH_CONSTANT, v.info);
    v.as_indexed();
  }

  if (TK_COMMA == currToken.tt) {  /*多变量赋值*/
    VarDesc v2;
    next();
    var_or_func(fs, v2);
    if (VAR_EXPR == v2.vt) {
      error("var syntax error");
    }
    left = assignment(fs, v2, vars + 1);  /*多变量依次压入函数栈*/
  }
  else {  /*解析=右边的表达式列表，生成n组指令*/
    ExplistDesc desc;
    check_next(TK_ASSIGN);
    explist1(fs, desc);
    adjust_multi_assign(fs, vars, desc);
  }

  if (VAR_INDEXED == v.vt) {
    if (0 == left && 1 == vars) {
      code_store_var(fs, v);
    }
    else {
      int stackDistance = vars - 1 + left;
      code_op_arg(fs, OP_SET_TABLE, stackDistance);
      left += 2;
    }
  }
  else {
    code_store_var(fs, v);
  }
  return left;
}

/* localstat -> LOCAL varlist [`=` explist1] */
void Grammar::local_stat(FuncState& fs) {
  ExplistDesc desc;
  int vars = 0;
  SymbolString name;

  /*多个局部变量存入 FuncState::localVars，如 local a,b,c */
  do {
    next();
    fetch_name(fs, name);
    save_localvar(fs, name, vars++);
  } while (TK_COMMA == currToken.tt);

  if (optional(TK_ASSIGN)) {
    explist1(fs, desc);
  }

  fs.localNum += vars;
  adjust_multi_assign(fs, vars, desc);
}

void Grammar::fetch_name(FuncState& fs, SymbolString& name) {
  check(TK_NAME);
  name = currToken.str;
  next();
}

int Grammar::name_constant(FuncState& fs) {
  check(TK_NAME);
  int index = string_constant(fs, currToken.str);
  next();
  return index;
}

int Grammar::string_constant(FuncState& fs, const char* name) {
  SymbolString strName = name;
  std::map<SymbolString, int>::iterator iter = fs.constStringIndexs.find(strName);
  if (iter != fs.constStringIndexs.end())
    return iter->second;

  int index = next_constant(fs);
  fs.constStringIndexs[strName] = index;

  lstate->new_string(fs.proto->constants.data[index], name, 0, FUNC_NAME, GCSTATE_FIXED);
  return index;
}

int Grammar::number_constant(FuncState& fs, Number n) {
  for (int i = 0; i < fs.proto->constants.size(); i++) {
    TObject& obj = fs.proto->constants.data[i];
    if (OBJECT_NUMBER == obj.ot && NUM_VAL(obj) == n) {
      return i;
    }
  }

  int index = next_constant(fs);
  fs.proto->constants.data[index].as_number(n);
  return index;
}

int Grammar::next_constant(FuncState& fs) {
  fs.proto->constants.add(TObject());
  return fs.proto->constants.size() - 1;
}

void Grammar::next() {
  prevLine = currToken.line;
  currToken = lexState.nextToken();
}

void Grammar::check(TokenType tt) {
  if (currToken.tt != tt) {
    error("not expected token");
  }
}

void Grammar::check_next(TokenType tt) {
  check(tt);
  next();
}

bool Grammar::optional(TokenType tt) {
  if (currToken.tt == tt) {
    next();
    return true;
  }
  return false;
}

void Grammar::error(const char* msg) {
  lstate->error(msg, currToken.line);
}

void Grammar::search_var(FuncState& fs, VarDesc& v) {
  SymbolString name;
  fetch_name(fs, name);

  /*查找局部变量*/
  int localIndex = -1;
  for (int i = fs.localNum - 1; i >= 0; i--) {
    if (fs.localVars[i] == name) {
      localIndex = i;
      break;
    }
  }

  if (-1 != localIndex) {
    v.as_local(localIndex);
  }
  else {
    v.as_global(string_constant(fs, name.data));
  }
}

void Grammar::save_localvar(FuncState& fs, const SymbolString& name, int varIndex) {
  if (fs.localNum + varIndex + 1 >= GRAM_LOCAL_VARS) {
    error("too many local variables");
  }
  fs.localVars[fs.localNum + varIndex] = name;
}

void Grammar::code_store_var(FuncState& fs, VarDesc& v) {
  switch (v.vt) {
  case VAR_LOCAL:
    code_op_arg(fs, OP_SET_LOCAL, v.info);
    break;
  case VAR_GLOBAL:
    code_op_arg(fs, OP_SET_GLOBAL, v.info);
    break;
  case VAR_INDEXED:
    code_op(fs, OP_SET_TABLE_AND_POP);
    break;
  default:
    break;
  }
}

void Grammar::code_push_var(FuncState& fs, VarDesc& v) {
  switch (v.vt) {
  case VAR_LOCAL:
    code_op_arg(fs, OP_PUSH_LOCAL, v.info);
    break;
  case VAR_GLOBAL:
    code_op_arg(fs, OP_GET_GLOBAL, v.info);
    break;
  case VAR_DOT:
    code_op_arg(fs, OP_TABLE_DOT_GET, v.info);
    break;
  case VAR_INDEXED:
    code_op(fs, OP_TABLE_INDEXED_GET);
    break;
  case VAR_EXPR:  /*函数调用若作为中间表达式，返回值数量强制设置为1*/
    fix_func_returns(fs, v.info, 1);
    break;
  default:
    break;
  }
  v.reset();
}

/* 函数返回值数量自适应规则
f();               -- adjusted to 0
g(x, f());         -- f() is adjusted to 1
a,b,c = f(), x;    -- f() is adjusted to 1 result (and c gets nil)
a,b,c = x, f();    -- f() is adjusted to 2
a,b,c = f();       -- f() is adjusted to 3
return f();        -- returns all values returned by f()
*/
void Grammar::fix_func_returns(FuncState&fs, int callPc, int results) {
  if (callPc > 0) {
    fs.proto->codes.data[callPc + 1] = results;
  }
}

/*在多重赋值表达式里，函数调用若为最后一个表达式，返回值数量动态适应*/
void Grammar::adjust_multi_assign(FuncState& fs, int vars, ExplistDesc& desc) {
  int need = vars - desc.expNum;
  if (desc.callPc > 0) {  /*最后一个表达式为函数调用，函数调用本身占用1个值*/
    need += 1;
    if (need >= 0) {
      fix_func_returns(fs, desc.callPc, need);
    }
    else {  /*右值太多，不需要函数返回值，且弹出多余的右值*/
      fix_func_returns(fs, desc.callPc, 0);
      adjust_stack(fs, need);
    }
  }
  else {
    adjust_stack(fs, need);  /*没有函数调用，调整左右值的数量*/
  }
}

void Grammar::adjust_stack(FuncState& fs, int need) {
  if (need > 0) {  /*表达式数量不足，填充多个nil*/
    code_op_arg(fs, OP_PUSH_NIL, need);
  }
  else if (need < 0) {  /*表达式数量超过varlist数量，弹出多余的表达式； 或者弹出超出作用域的局部变量*/
    code_op_arg(fs, OP_POP, -need);
  }
}

void Grammar::pop_local_vars(FuncState& fs, int n) {
  adjust_stack(fs, -n);
}

void Grammar::code_op(FuncState& fs, OpCode op) {
  fs.proto->add_op(op, prevLine);
}

void Grammar::code_arg(FuncState& fs, int arg) {
  fs.proto->codes.add(arg);
}

/*返回指令pc*/
int Grammar::code_op_arg(FuncState& fs, OpCode op, int arg) {
  fs.proto->add_op(op, prevLine);
  fs.proto->codes.add(arg);
  return fs.proto->codes.size() - 2;
}

/*JMP指令跳到下一个pc
[0 ]
...
[9 ] JMP
[10] offset
...
[12] jmp to here (code size=13, offset=2)
*/
void Grammar::fix_jmp_to_next(FuncState&fs, int jmpPc) {
  fix_jmp_dest(fs, jmpPc, fs.proto->codes.size());
}

void Grammar::fix_jmp_dest(FuncState&fs, int jmpPc, int destPc) {
  int offset = destPc - (jmpPc + 2);
  fs.proto->codes.data[jmpPc + 1] = offset;
}

void Grammar::fix_op_arg(FuncState& fs, int pc, int arg) {
  fs.proto->codes.data[pc] = arg;
}
