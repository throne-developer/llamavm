#include "gram.h"

/*�������ã����ν�����proto�������б�ѹջ������CALLָ�����callָ��pc

�� function add(a,b) return a+b end;  c=add(100,200);  ��������ָ��
[01] PUSH_CONSTANT   proto(add)
[03] SET_GLOBAL      'add'
[05] GET_GLOBAL      'add'
[07] PUSH_NUMBER     100
[09] PUSH_NUMBER     200
[11] OP_CALL         1 2
[14] SET_GLOBAL      c

function add() ��ͬ�� add = function() ... */
int Grammar::func_call(FuncState& fs) {
  int params = 1, results = 0;

  switch (currToken.tt) {
  case TK_L_PAREN: {
    ExplistDesc desc;
    next();
    explist(fs, desc);
    check_next(TK_R_PAREN);

    params = desc.expNum;
    fix_func_returns(fs, desc.callPc, 1);  /*���в���Ϊ�������ã�����ֵ������Ϊ1*/
    break;
  }

  default:
    break;
  }

  code_op(fs, OP_CALL);
  code_arg(fs, results);  /*����ֵ�����ɽ��շ�������������Ϊ0*/
  code_arg(fs, params);

  return fs.proto->codes.size() - 3;
}

/* func_stat -> FUNCTION funcname body */
void Grammar::func_stat(FuncState& fs) {
  VarDesc v;
  next();
  func_name(fs, v);
  body(fs);
  code_store_var(fs, v);  /*������������proto����*/
}

void Grammar::func_name(FuncState& fs, VarDesc& v) {
  search_var(fs, v);
}

/* body -> `(` parlist `)` chunk END */
void Grammar::body(FuncState& fs) {
  FuncState subfs;
  open_func(subfs, &fs);

  check_next(TK_L_PAREN);
  parlist(subfs);
  check_next(TK_R_PAREN);

  chunk(subfs);
  check_next(TK_END);
  close_func(subfs);

  /*proto������븸��proto��constant�б���������ջָ��Ա�󶨺�������proto*/
  TObject obj;
  obj.as_proto(subfs.proto);
  fs.proto->constants.add(obj);

  code_op_arg(fs, OP_PUSH_CONSTANT, fs.proto->constants.size() - 1);
}

/* parlist -> { NAME [`,`] } */
void Grammar::parlist(FuncState& fs) {
  int params = 0;
  SymbolString name;

  while (TK_NAME == currToken.tt) {  /*������Ϊ�ֲ�����*/
    fetch_name(fs, name);
    save_localvar(fs, name, params++);
    if (!optional(TK_COMMA)) {
      break;
    }
  }
  fs.localNum += params;
  fs.proto->codes.data[GRAM_CODE_ARG_NUM_POS] = params;
}

/* return_stat -> RETURN explist */
void Grammar::return_stat(FuncState& fs) {
  ExplistDesc desc;

  next();
  explist(fs, desc);

  if (desc.callPc > 0) {  /*����ֵ���һ�����ʽΪ��������*/
    fs.proto->codes.data[desc.callPc] = OP_TAILCALL;
    fs.proto->codes.data[desc.callPc + 1] = fs.localNum;
  }
  else {
    code_op_arg(fs, OP_RETURN, fs.localNum);
  }
  optional(TK_SEMICOLON);
}