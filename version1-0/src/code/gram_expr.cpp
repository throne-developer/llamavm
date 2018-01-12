#include "gram.h"
#include "base/const.h"

/*return返回值列表，或func参数列表，explist可以为空列表，explist1不可以为空*/
void Grammar::explist(FuncState& fs, ExplistDesc& desc) {
  switch (currToken.tt) {  /*若为分号，或右括号，作为空列表*/
  case TK_SEMICOLON: case TK_R_PAREN: case TK_EOS:
  case TK_ELSE: case TK_ELSEIF: case TK_END:
    desc.reset();
    break;

  default:
    explist1(fs, desc);
    break;
  }
}

/* explist1 -> expr {`,` expr} 

expr含义：
（1）expr可以为全局变量、局部变量、运算符表达式
（2）若为全局变量、局部变量可直接用 VarDesc表示
（3）若为函数调用表达式，需要自适应函数返回值数量，也可以用 VarDesc表示
（4）若为非函数调用表达式，VarDesc 无意义
（5）不管expr为何种形式，计算后只会生成一个值 */
void Grammar::explist1(FuncState& fs, ExplistDesc& desc) {
  desc.expNum = 1;
  VarDesc v;
  expr(fs, v);

  while (TK_COMMA == currToken.tt) {  /*多表达式*/
    desc.expNum++;
    code_push_var(fs, v);
    next();
    expr(fs, v);
  }

  if (VAR_EXPR == v.vt) {
    desc.callPc = v.info;
  }
  else {
    code_push_var(fs, v);
    desc.callPc = 0;
  }
}

/* expr -> binop_expr { [AND | OR] binop_expr }

and、or运算符执行短路计算，and取第一个false表达式，or取第一个true表达式
如 v = a and b and c or d，生成如下指令
[1 ] GET_GLOBAL    a

[3 ] ON_FALSE_JMP  2      <-- 若a不为false，弹出a； 若a为false，跳到指令7，相当于a and b等于a
[5 ] GET_GLOBAL    b      <-- 若执行到指令5，相当于a and b等于b

[7 ] ON_FALSE_JMP  2
[9 ] GET_GLOBAL    c

[11] ON_TRUE_JMP   2
[13] GET_GLOBAL    d

[15] SET_GLOBAL    v

总结：先压入第一个值，若取第一个值就执行JMP，否则弹出第一个值，压入第二个值 */
void Grammar::expr(FuncState& fs, VarDesc& v) {
  binop_expr(fs, v);

  while (TK_AND == currToken.tt || TK_OR == currToken.tt) {
    code_push_var(fs, v);
    int jmpPc = code_op_arg(fs, TK_AND == currToken.tt ? OP_ON_FALSE_JMP : OP_ON_TRUE_JMP, 0);
    next();
    binop_expr(fs, v);
    code_push_var(fs, v);
    fix_jmp_to_next(fs, jmpPc);  /*回填跳转位置*/
  }
}

void Grammar::expr_code_var(FuncState& fs) {
  VarDesc v;
  expr(fs, v);
  code_push_var(fs, v);
}

/* binop_expr -> unary_expr { binop unary_expr } 

表达式求值，指令列表相当于数据栈，OpStack作为运算符栈，原理同逆波兰表达式
（1）数据按顺序写入数据栈（生成指令）
（2）每次压入运算符前，先将栈顶的高优先级的运算符弹出且生成指令
（3）若遇到括号，括号内表达式作为新的expr处理

如 a+b*c-d，堆栈状态变化为（循环一次为一步）
1> [a b]             [+]
2> [a b c]           [+ *]
3> [a b c * + d]     [-]
4> [a b c * + d -]   []  */
void Grammar::binop_expr(FuncState& fs, VarDesc& v) {
  OpStack sk;
  sk.top = 0;
  unary_expr(fs, v, sk);

  while (BinOps[currToken.tt].priority > 0) {
    code_push_var(fs, v);
    code_higher_op(fs, sk, BinOps[currToken.tt].priority);
    push_op(sk, BinOps[currToken.tt]);

    next();
    unary_expr(fs, v, sk);
    code_push_var(fs, v);
  }

  if (sk.top > 0) {  /*最后一个运算符出栈*/
    code_push_var(fs, v);
    code_higher_op(fs, sk, 0);
  }
}

/* unary_expr -> { not | `-` } simple_exp */
void Grammar::unary_expr(FuncState& fs, VarDesc& v, OpStack& sk) {
  while (UnaryOps[currToken.tt].priority > 0) {  /*一元运算符压栈，优先级高于二元*/
    push_op(sk, UnaryOps[currToken.tt]);
    next();
  }
  simple_exp(fs, v);
}

void Grammar::push_op(OpStack& sk, OpPriority op) {
  if (sk.top >= GRAM_OP_STACK_NUM) {
    error("too many operator");
  }
  sk.oplist[sk.top++] = op;
}

void Grammar::code_higher_op(FuncState& fs, OpStack& sk, int priority) {
  while (sk.top > 0 && sk.oplist[sk.top - 1].priority >= priority) {
    code_op(fs, sk.oplist[sk.top - 1].opcode);
    sk.top--;
  }
}

/* simple_exp -> NUMBER | STRING | NAME | `(` expr `)` */
void Grammar::simple_exp(FuncState& fs, VarDesc& v) {
  switch (currToken.tt) {
  case TK_NUMBER: {  /*仅针对常量数字、字符串生成指令*/
    code_op_arg(fs, OP_PUSH_CONSTANT, number_constant(fs, atof(currToken.str)));
    next();
    break;
  }
  case TK_STRING: {
    code_op_arg(fs, OP_PUSH_CONSTANT, string_constant(fs, currToken.str));
    next();
    break;
  }
  case TK_NIL: {
    code_op_arg(fs, OP_PUSH_NIL, 1);
    next();
    break;
  }
  case TK_NAME: {
    var_or_func(fs, v);
    return;
  }
  case TK_L_PAREN: {
    next();
    expr(fs, v);
    check_next(TK_R_PAREN);
    return;
  }
  case TK_L_BRACES: {
    table_construct(fs);
    break;
  }
  default:
    break;
  }
  v.reset();
}

void Grammar::table_construct(FuncState& fs) {
  TablePartDesc desc, nextDesc;
  int arrays = 0, hashs = 0, pc = 0;

  pc = code_op_arg(fs, OP_NEW_TABLE, arrays);
  code_arg(fs, hashs);

  check_next(TK_L_BRACES);
  table_part(fs, desc);

  if (TK_SEMICOLON == currToken.tt) {
    next();
    table_part(fs, nextDesc);
    if (FIELD_NONE != desc.ft && desc.ft == nextDesc.ft) {
      error("invalid table construct");
    }
  }
  check_next(TK_R_BRACES);

  arrays = (FIELD_ARRAY == desc.ft ? desc.fields : 0);
  arrays += (FIELD_ARRAY == nextDesc.ft ? nextDesc.fields : 0);
  fix_op_arg(fs, pc + 1, arrays);

  hashs = (FIELD_HASH == desc.ft ? desc.fields : 0);
  hashs += (FIELD_HASH == nextDesc.ft ? nextDesc.fields : 0);
  fix_op_arg(fs, pc + 2, hashs);
}

void Grammar::table_part(FuncState& fs, TablePartDesc& desc) {
  FieldType ft = FIELD_NONE;

  switch (currToken.tt) {
  case TK_SEMICOLON:
  case TK_R_BRACES: {
    break;
  }
  case TK_NAME: {  /*array或hash*/
    VarDesc v;
    expr(fs, v);
    if (TK_ASSIGN == currToken.tt) {
      switch (v.vt) {  /*TK_NAME先解析为变量，再获取变量的名字*/
      case VAR_GLOBAL:
        code_op_arg(fs, OP_PUSH_CONSTANT, v.info);
        break;
      case VAR_LOCAL: {
        int constIndex = string_constant(fs, fs.localVars[v.info].data);
        code_op_arg(fs, OP_PUSH_CONSTANT, constIndex);
        break;
      }
      default:
        error("unexpect token");
        break;
      }

      next();
      expr_code_var(fs);
      ft = FIELD_HASH;
    }
    else {
      code_push_var(fs, v);
      ft = FIELD_ARRAY;
    }
    break;
  }
  case TK_L_SQUARE: {
    hash_field(fs);
    ft = FIELD_HASH;
    break;
  }
  default: {
    expr_code_var(fs);
    ft = FIELD_ARRAY;
    break;
  }
  }

  desc.fields = (FIELD_NONE == ft ? 0 : field_list(fs, ft));
  desc.ft = ft;
}

int Grammar::field_list(FuncState& fs, FieldType ft) {
  int fields = 1;
  while (TK_COMMA == currToken.tt) {
    next();
    if (TK_SEMICOLON == currToken.tt || TK_R_BRACES == currToken.tt) {
      break;
    }
    fields++;
    (FIELD_ARRAY == ft) ? expr_code_var(fs) : hash_field(fs);
  }

  if (FIELD_ARRAY == ft) {
    code_op_arg(fs, OP_SET_ARRAY, fields);
  }
  else {
    code_op_arg(fs, OP_SET_HASH, fields);
  }
  return fields;
}

void Grammar::hash_field(FuncState& fs) {
  switch (currToken.tt) {
  case TK_NAME:
    code_op_arg(fs, OP_PUSH_CONSTANT, name_constant(fs));
    break;
  case TK_L_SQUARE:
    next();
    expr_code_var(fs);
    check_next(TK_R_SQUARE);
    break;
  default:
    error("hash field error");
    break;
  }

  check_next(TK_ASSIGN);
  expr_code_var(fs);
}