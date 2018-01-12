#include "gram.h"
#include "base/const.h"

/*return����ֵ�б���func�����б�explist����Ϊ���б�explist1������Ϊ��*/
void Grammar::explist(FuncState& fs, ExplistDesc& desc) {
  switch (currToken.tt) {  /*��Ϊ�ֺţ��������ţ���Ϊ���б�*/
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

expr���壺
��1��expr����Ϊȫ�ֱ������ֲ���������������ʽ
��2����Ϊȫ�ֱ������ֲ�������ֱ���� VarDesc��ʾ
��3����Ϊ�������ñ��ʽ����Ҫ����Ӧ��������ֵ������Ҳ������ VarDesc��ʾ
��4����Ϊ�Ǻ������ñ��ʽ��VarDesc ������
��5������exprΪ������ʽ�������ֻ������һ��ֵ */
void Grammar::explist1(FuncState& fs, ExplistDesc& desc) {
  desc.expNum = 1;
  VarDesc v;
  expr(fs, v);

  while (TK_COMMA == currToken.tt) {  /*����ʽ*/
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

and��or�����ִ�ж�·���㣬andȡ��һ��false���ʽ��orȡ��һ��true���ʽ
�� v = a and b and c or d����������ָ��
[1 ] GET_GLOBAL    a

[3 ] ON_FALSE_JMP  2      <-- ��a��Ϊfalse������a�� ��aΪfalse������ָ��7���൱��a and b����a
[5 ] GET_GLOBAL    b      <-- ��ִ�е�ָ��5���൱��a and b����b

[7 ] ON_FALSE_JMP  2
[9 ] GET_GLOBAL    c

[11] ON_TRUE_JMP   2
[13] GET_GLOBAL    d

[15] SET_GLOBAL    v

�ܽ᣺��ѹ���һ��ֵ����ȡ��һ��ֵ��ִ��JMP�����򵯳���һ��ֵ��ѹ��ڶ���ֵ */
void Grammar::expr(FuncState& fs, VarDesc& v) {
  binop_expr(fs, v);

  while (TK_AND == currToken.tt || TK_OR == currToken.tt) {
    code_push_var(fs, v);
    int jmpPc = code_op_arg(fs, TK_AND == currToken.tt ? OP_ON_FALSE_JMP : OP_ON_TRUE_JMP, 0);
    next();
    binop_expr(fs, v);
    code_push_var(fs, v);
    fix_jmp_to_next(fs, jmpPc);  /*������תλ��*/
  }
}

void Grammar::expr_code_var(FuncState& fs) {
  VarDesc v;
  expr(fs, v);
  code_push_var(fs, v);
}

/* binop_expr -> unary_expr { binop unary_expr } 

���ʽ��ֵ��ָ���б��൱������ջ��OpStack��Ϊ�����ջ��ԭ��ͬ�沨�����ʽ
��1�����ݰ�˳��д������ջ������ָ�
��2��ÿ��ѹ�������ǰ���Ƚ�ջ���ĸ����ȼ������������������ָ��
��3�����������ţ������ڱ��ʽ��Ϊ�µ�expr����

�� a+b*c-d����ջ״̬�仯Ϊ��ѭ��һ��Ϊһ����
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

  if (sk.top > 0) {  /*���һ���������ջ*/
    code_push_var(fs, v);
    code_higher_op(fs, sk, 0);
  }
}

/* unary_expr -> { not | `-` } simple_exp */
void Grammar::unary_expr(FuncState& fs, VarDesc& v, OpStack& sk) {
  while (UnaryOps[currToken.tt].priority > 0) {  /*һԪ�����ѹջ�����ȼ����ڶ�Ԫ*/
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
  case TK_NUMBER: {  /*����Գ������֡��ַ�������ָ��*/
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
  case TK_NAME: {  /*array��hash*/
    VarDesc v;
    expr(fs, v);
    if (TK_ASSIGN == currToken.tt) {
      switch (v.vt) {  /*TK_NAME�Ƚ���Ϊ�������ٻ�ȡ����������*/
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