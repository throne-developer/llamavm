#include "gram.h"

/* ifstat -> cond THEN block { ELSEIF ifstat } [ ELSE block ] END

����1...
IF_FALSE ��ת�� [��һ��elseif �� else �� ����]
block1ָ���б�...
JMP [����]

����2...
IF_FALSE ��ת�� [��һ��elseif �� else �� ����]
block2ָ���б�...
JMP [����]

else
block3ָ���б�...

����λ��  */
void Grammar::if_stat(FuncState& fs) {
  int falseJmpPc = 0, jmpPc = 0;

  next();
  condition(fs);
  falseJmpPc = code_op_arg(fs, OP_IF_FALSE_JMP, 0);
  check_next(TK_THEN);
  block(fs);

  /*���elseif��else������jmpָ���������*/
  if (TK_ELSE == currToken.tt || TK_ELSEIF == currToken.tt) {
    jmpPc = code_op_arg(fs, OP_JMP, 0);
  }

  fix_jmp_to_next(fs, falseJmpPc);  /*false cond ��������*/
  if (TK_ELSEIF == currToken.tt) {
    if_stat(fs);  /*���if_statѹջ����*/
  }
  else {
    if (optional(TK_ELSE)) {
      block(fs);
    }
    check_next(TK_END);
  }

  /*if��������һ��λ��Ϊ���ڣ�����ÿ��if_stat��jmpλ��*/
  if (jmpPc > 0) {
    fix_jmp_to_next(fs, jmpPc);
  }
}

void Grammar::condition(FuncState& fs) {
  expr_code_var(fs);
}

void Grammar::block(FuncState& fs) {
  int locals = fs.localNum;
  chunk(fs);
  pop_local_vars(fs, fs.localNum - locals);
  fs.localNum = locals;
}

/* whilestat -> WHILE cond DO block END

����...
IF_FALSE ��ת�� [����]
blockָ���б�...
JMP [����]
����  */
void Grammar::while_stat(FuncState& fs) {
  int enterPc = fs.proto->codes.size(), exitPc = 0, falseJmpPc = 0;
  LoopLabel breakLabel, contLabel;

  enter_loop(fs, breakLabel, contLabel);
  next();
  condition(fs);
  falseJmpPc = code_op_arg(fs, OP_IF_FALSE_JMP, 0);
  check_next(TK_DO);
  block(fs);
  check_next(TK_END);

  int jmpBackPc = code_op_arg(fs, OP_JMP, 0);
  fix_jmp_dest(fs, jmpBackPc, enterPc);  /*���ص�����ָ��*/

  exitPc = fs.proto->codes.size();
  fix_jmp_dest(fs, falseJmpPc, exitPc);  /*false cond ��������*/

  breakLabel.destPc = exitPc;
  contLabel.destPc = enterPc;
  leave_loop(fs, breakLabel, contLabel);
}

void Grammar::enter_loop(FuncState& fs, LoopLabel& bl, LoopLabel& cl) {
  bl.localNum = fs.localNum;
  bl.prev = fs.breakLabel;
  fs.breakLabel = &bl;

  cl.localNum = fs.localNum;
  cl.prev = fs.contLabel;
  fs.contLabel = &cl;
}

void Grammar::leave_loop(FuncState& fs, LoopLabel& bl, LoopLabel& cl) {
  for (int i = 0; i < bl.jmpNum; i++) {
    fix_jmp_dest(fs, bl.jmpPc[i], bl.destPc);
  }
  fs.breakLabel = bl.prev;

  for (int i = 0; i < cl.jmpNum; i++) {
    fix_jmp_dest(fs, cl.jmpPc[i], cl.destPc);
  }
  fs.contLabel = cl.prev;
}

void Grammar::break_continue_stat(FuncState& fs, bool isbreak) {
  LoopLabel* lb = isbreak ? fs.breakLabel : fs.contLabel;
  if (NULL == lb) {
    error("no loop to break or continue");
  }
  if (lb->jmpNum >= GRAM_LOOP_JMP_NUM) {
    error("too many break or continue");
  }

  next();
  pop_local_vars(fs, fs.localNum - lb->localNum);
  lb->jmpPc[lb->jmpNum++] = code_op_arg(fs, OP_JMP, 0);
}

/* forstat -> FOR NAME = expr, expr [,expr] DO block END */
void Grammar::for_stat(FuncState& fs) {
  LoopLabel bl, cl;
  enter_loop(fs, bl, cl);

  int vars = for_init(fs);
  fs.localNum += vars;
  cl.localNum += vars;

  int initPc = code_op_arg(fs, OP_FOR_INIT, 0);
  int enterPc = fs.proto->codes.size();
  check_next(TK_DO);
  block(fs);
  check_next(TK_END);

  int loopPc = code_op_arg(fs, OP_FOR_LOOP, 0);
  fix_jmp_dest(fs, loopPc, enterPc);

  int exitPc = fs.proto->codes.size();
  fix_jmp_dest(fs, initPc, exitPc);

  bl.destPc = exitPc;
  cl.destPc = loopPc;
  leave_loop(fs, bl, cl);

  fs.localNum -= vars;
}

int Grammar::for_init(FuncState& fs) {
  SymbolString name;

  next();
  fetch_name(fs, name);
  check_next(TK_ASSIGN);

  expr_code_var(fs);
  check_next(TK_COMMA);
  expr_code_var(fs);
  if (optional(TK_COMMA)) {
    expr_code_var(fs);
  }
  else {
    code_op_arg(fs, OP_PUSH_CONSTANT, number_constant(fs, 1));
  }

  int vars = 0;
  save_localvar(fs, name, vars++);
  name = "(for-limit)";
  save_localvar(fs, name, vars++);
  name = "(for-step)";
  save_localvar(fs, name, vars++);
  return vars;
}