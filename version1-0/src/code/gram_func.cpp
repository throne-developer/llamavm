#include "gram.h"

/*函数调用，依次将函数proto、参数列表压栈，生成CALL指令，返回call指令pc

如 function add(a,b) return a+b end;  c=add(100,200);  生成如下指令
[01] PUSH_CONSTANT   proto(add)
[03] SET_GLOBAL      'add'
[05] GET_GLOBAL      'add'
[07] PUSH_NUMBER     100
[09] PUSH_NUMBER     200
[11] OP_CALL         1 2
[14] SET_GLOBAL      c

function add() 等同于 add = function() ... */
int Grammar::func_call(FuncState& fs) {
  int params = 1, results = 0;

  switch (currToken.tt) {
  case TK_L_PAREN: {
    ExplistDesc desc;
    next();
    explist(fs, desc);
    check_next(TK_R_PAREN);

    params = desc.expNum;
    fix_func_returns(fs, desc.callPc, 1);  /*若有参数为函数调用，返回值数量设为1*/
    break;
  }

  default:
    break;
  }

  code_op(fs, OP_CALL);
  code_arg(fs, results);  /*返回值数量由接收方决定，先设置为0*/
  code_arg(fs, params);

  return fs.proto->codes.size() - 3;
}

/* func_stat -> FUNCTION funcname body */
void Grammar::func_stat(FuncState& fs) {
  VarDesc v;
  next();
  func_name(fs, v);
  body(fs);
  code_store_var(fs, v);  /*关联函数名和proto对象*/
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

  /*proto对象存入父级proto的constant列表，并生成入栈指令，以便绑定函数名和proto*/
  TObject obj;
  obj.as_proto(subfs.proto);
  fs.proto->constants.add(obj);

  code_op_arg(fs, OP_PUSH_CONSTANT, fs.proto->constants.size() - 1);
}

/* parlist -> { NAME [`,`] } */
void Grammar::parlist(FuncState& fs) {
  int params = 0;
  SymbolString name;

  while (TK_NAME == currToken.tt) {  /*参数作为局部变量*/
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

  if (desc.callPc > 0) {  /*返回值最后一个表达式为函数调用*/
    fs.proto->codes.data[desc.callPc] = OP_TAILCALL;
    fs.proto->codes.data[desc.callPc + 1] = fs.localNum;
  }
  else {
    code_op_arg(fs, OP_RETURN, fs.localNum);
  }
  optional(TK_SEMICOLON);
}