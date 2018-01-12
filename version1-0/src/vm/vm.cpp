#include <stdio.h> 
#include "vm.h"
#include "gc.h"
#include "base/common.h"
#include "base/state.h"

int Vm::run(LlamaState* ls, ObjectProto* proto) {
  try {
    int base = 0;
    execute(ls, proto, base);
  }
  catch (RuntimeError& e) {
    printf("============= execute exception =============\n");
    printf("[FILE] %s:%d, [ERROR] %s\n", proto->filename.data, e.line, e.msg.data);
  }
  return 0;
}

/*执行函数指令
base：函数调用基址，对应函数第一个参数位置
返回值：第一个返回值的位置 */
int Vm::execute(LlamaState* ls, ObjectProto* proto, int base) {
  Instruction arg = 0, op = 0;
  bool stop = false;
  int firstResult = base;
  int codeSize = proto->codes.size();
  Array<Instruction>& codes = proto->codes;

  LLAMA_TRY;
  for (int pc = VM_CODE_START; pc < codeSize && !stop; pc++) {
    op = codes.data[pc];
    ls->currPc = pc;

    switch (op) {
    case OP_PUSH_NIL: {
      arg = codes.data[++pc];
      while (arg--) { stack_push_nil(ls); }
      break;
    }

    case OP_POP: {
      arg = codes.data[++pc];
      stack_pop_n(ls, arg);
      break;
    }

    case OP_PUSH_CONSTANT: {
      arg = codes.data[++pc];
      stack_push_object(ls, proto->constants.data[arg]);
      break;
    }

    case OP_PUSH_LOCAL: { /*函数体局部变量，从base开始计数*/
      arg = codes.data[++pc];
      stack_push_object(ls, stack_at(ls, base + arg));
      break;
    }

    case OP_SET_LOCAL: {
      arg = codes.data[++pc];
      stack_at(ls, base + arg) = stack_back(ls);
      stack_pop(ls);
      break;
    }

    case OP_SET_GLOBAL: {
      arg = codes.data[++pc];
      ls->globals.set(proto->constants.data[arg], stack_back(ls));
      stack_pop(ls);
      break;
    }

    case OP_GET_GLOBAL: {
      arg = codes.data[++pc];
      stack_push_object(ls, ls->globals.get(proto->constants.data[arg]));
      break;
    }

    case OP_ADD: case OP_SUB: case OP_MUL: case OP_DIV: {
      TObject& rval = stack_reverse(ls, -1);
      TObject& lval = stack_reverse(ls, -2);

      if (rval.ot == OBJECT_NUMBER && lval.ot == OBJECT_NUMBER) {
        Number result = calc(NUM_VAL(lval), NUM_VAL(rval), (OpCode)op);
        stack_pop(ls);
        NUM_VAL(stack_back(ls)) = result;
      }
      else {
        ls->error("calc operand isn't number");
      }
      break;
    }

    case OP_GT: case OP_GE: case OP_LT: case OP_LE: case OP_EQ: case OP_NE: {
      TObject& rval = stack_reverse(ls, -1);
      TObject& lval = stack_reverse(ls, -2);

      if (rval.ot == OBJECT_NUMBER && lval.ot == OBJECT_NUMBER) {
        bool result = compare(NUM_VAL(lval), NUM_VAL(rval), (OpCode)op);
        stack_pop(ls);
        stack_back(ls).ot = (result ? OBJECT_NUMBER : OBJECT_NIL);
        NUM_VAL(stack_back(ls)) = 1;
      }
      else {
        ls->error("compare operand isn't number");
      }
      break;
    }

    case OP_MINUS: {
      TObject& val = stack_back(ls);
      if (OBJECT_NUMBER == val.ot) {
        NUM_VAL(val) = -1 * NUM_VAL(val);
      }
      else {
        ls->error("minus operand isn't number");
      }
      break;
    }

    case OP_NOT: {
      TObject& val = stack_back(ls);
      val.ot = (OBJECT_NIL == val.ot ? OBJECT_NUMBER : OBJECT_NIL);
      NUM_VAL(val) = 1;
      break;
    }

    case OP_CONCAT: {
      TObject& rval = stack_reverse(ls, -1);
      TObject& lval = stack_reverse(ls, -2);

      if (OBJECT_STRING == rval.ot && OBJECT_STRING == lval.ot) {
        TObject obj;
        concat(ls, lval, rval, obj);
        stack_pop(ls);
        stack_back(ls) = obj;
      }
      else {
        ls->error("concat operand isn't string");
      }
      gc::check_gc(ls);
      break;
    }

    case OP_NEW_TABLE: {
      int arrays = codes.data[++pc];
      int hashs = codes.data[++pc];

      TObject obj;
      obj.as_table(ls->new_table(arrays, hashs, FUNC_NAME, GCSTATE_NO_MARK));
      stack_push_object(ls, obj);
      gc::check_gc(ls);
      break;
    }

    case OP_SET_ARRAY: {
      arg = codes.data[++pc];
      int tbIndex = stack_top(ls) - arg - 1;
      ObjectTable* table = stack_at(ls, tbIndex).value.table;

      int ikey = 1;
      for (int i = tbIndex + 1; i < stack_top(ls); i++) {
        table->t.set_num(ikey++, stack_at(ls, i));
      }
      stack_pop_n(ls, arg);
      break;
    }

    case OP_SET_HASH: {
      arg = codes.data[++pc];
      int tbIndex = stack_top(ls) - 2 * arg - 1;
      ObjectTable* table = stack_at(ls, tbIndex).value.table;

      for (int i = tbIndex + 1; i < stack_top(ls); i += 2) {
        table->t.set(stack_at(ls, i), stack_at(ls, i + 1));
      }
      stack_pop_n(ls, 2 * arg);
      break;
    }

    case OP_SET_TABLE_AND_POP: case OP_SET_TABLE: {
      arg = 0;
      if (OP_SET_TABLE == op) {
        arg = codes.data[++pc];
      }

      TObject& value = stack_back(ls);
      TObject& table = stack_reverse(ls, -3 - arg);
      TObject& key = stack_reverse(ls, -2 - arg);

      if (OBJECT_TABLE == table.ot) {
        Table& t = TABLE_VAL(table);
        t.set(key, value);

        stack_pop(ls);
        if (OP_SET_TABLE_AND_POP == op) {
          stack_pop_n(ls, 2);
        }
      }
      else {
        ls->error("get table failed");
      }
      break;
    }

    case OP_TABLE_DOT_GET: case OP_TABLE_INDEXED_GET: {
      if (OP_TABLE_DOT_GET == op) {
        arg = codes.data[++pc];
        stack_push_object(ls, proto->constants.data[arg]);
      }

      TObject& key = stack_reverse(ls, -1);
      TObject& table = stack_reverse(ls, -2);

      if (OBJECT_TABLE == table.ot) {
        TObject result = TABLE_VAL(table).get(key);
        stack_pop(ls);
        stack_back(ls) = result;
      }
      else {
        ls->error("get table failed");
      }
      break;
    }

    case OP_ON_FALSE_JMP: case OP_ON_TRUE_JMP: {
      arg = codes.data[++pc];

      TObject& val = stack_back(ls);
      int b = (op == OP_ON_FALSE_JMP ? 0 : 1);
      if (b == BOOL_VAL(val)) {
        pc += arg;  /*满足执行跳转，不满足弹出栈顶元素*/
      }
      else {
        stack_pop(ls);
      }
      break;
    }

    case OP_IF_FALSE_JMP: {
      arg = codes.data[++pc];

      TObject& val = stack_back(ls);
      if (0 == BOOL_VAL(val)){
        pc += arg;  /*满足执行跳转*/
      }
      stack_pop(ls);
      break;
    }

    case OP_JMP: {
      arg = codes.data[++pc];
      pc += arg;
      break;
    }

    case OP_FOR_INIT:
    case OP_FOR_LOOP: {
      arg = codes.data[++pc];
      bool isloop = (OP_FOR_LOOP == op);

      TObject& step = stack_reverse(ls, -1);
      TObject& limit = stack_reverse(ls, -2);
      TObject& counter = stack_reverse(ls, -3);
      if (OBJECT_NUMBER == step.ot && OBJECT_NUMBER == limit.ot && OBJECT_NUMBER == counter.ot) {
        if (isloop) {
          NUM_VAL(counter) += NUM_VAL(step);
        }
        bool cond = NUM_VAL(step) > 0 ? NUM_VAL(counter)<=NUM_VAL(limit) : NUM_VAL(counter)>=NUM_VAL(limit);
        if (cond) {
          pc += isloop ? arg : 0;;
        }
        else {
          stack_pop_n(ls, 3);
          pc += isloop ? 0 : arg;
        }
      }
      else {
        ls->error("for initial value isn't number");
      }
      break;
    }

    case OP_CALL: {
      int results = codes.data[++pc];
      int params = codes.data[++pc];
      call(ls, params, results);
      break;
    }

    case OP_TAILCALL: {
      int localvars = codes.data[++pc];
      int params = codes.data[++pc];
      call(ls, params, VM_MULTI_RETURN);  /*任意返回值数*/
      firstResult = base + localvars;
      stop = true;
      break;
    }

    case OP_RETURN: {
      int localvars = codes.data[++pc];
      firstResult = base + localvars;
      stop = true;
      break;
    }

    case END_CODE: {
      int top = stack_top(ls);
      stack_pop_n(ls, top - base);  /*弹出局部变量*/
      firstResult = base;
      stop = true;
      gc::check_gc(ls);
      break;
    }

    default:
      break;
    }
  }
  LLAMA_CATCH(RuntimeError& e) {
    e.line = get_line(proto, ls->currPc);
    printf("============= execute exception =============\n");
    printf("[FILE] %s:%d, [ERROR] %s\n", proto->filename.data, e.line, e.msg.data);
  }
  return firstResult;
}

Number Vm::calc(Number n1, Number n2, OpCode op) {
  switch (op) {
  case OP_ADD: return n1 + n2;
  case OP_SUB: return n1 - n2;
  case OP_MUL: return n1 * n2;
  case OP_DIV: return n1 / n2;
  default:break;
  }
  return 0;
}

bool Vm::compare(Number n1, Number n2, OpCode op) {
  switch (op) {
  case OP_GT: return (n1 > n2 ? true : false);
  case OP_GE: return (n1 >= n2 ? true : false);
  case OP_LT: return (n1 < n2 ? true : false);
  case OP_LE: return (n1 <= n2 ? true : false);
  case OP_EQ: return (n1 == n2 ? true : false);
  case OP_NE: return (n1 != n2 ? true : false);
  default:break;
  }
  return false;
}

void Vm::concat(LlamaState* ls, const TObject& o1, const TObject& o2, TObject& result) {
  LongString buff;
  int len1 = o1.str_len();
  int len2 = o2.str_len();
  buff.init(len1 + len2, FUNC_NAME);

  memcpy(buff.data, o1.c_str(), len1);
  memcpy(buff.data + len1, o2.c_str(), len2);
  ls->new_string(result, buff.data, len1 + len2, FUNC_NAME, GCSTATE_NO_MARK);
}

/* 堆栈布局示例, nParams=2, nResults=2
[0]
...
[3] func1           <-- funcBase
[4] param1          <-- base
[5] param2
[6] result1         <-- firstResult
[7] result2
...
[n]
*/
void Vm::call(LlamaState* ls, int params, int results) {
  int base = stack_top(ls) - params;
  TObject func = stack_at(ls, base - 1);
  int firstResult = 0;

  if (OBJECT_CPROTO == func.ot) {
    firstResult = call_c(ls, func.value.cf, base);
  }
  else if (OBJECT_PROTO == func.ot) {
    firstResult = execute(ls, func.value.proto, base);
  }
  else {
    ls->error("function call not exist");
  }

  if (VM_MULTI_RETURN == results) {  /*若为任意返回值数，从第一个返回值到栈顶都为返回值*/
    results = stack_top(ls) - firstResult;
  }
  else {  /*调整返回值数*/
    adjust_top(ls, firstResult + results);
  }

  /*将 result1~resultN 移动到 func1~paramN的位置上 */
  int funcBase = base - 1;
  for (int i = 0; i < results; i++) {
    stack_at(ls, funcBase + i) = stack_at(ls, firstResult + i);
  }

  /*funcBase ~ firstResult 之间func1和参数列表的总数*/
  int extra = firstResult - funcBase;
  stack_pop_n(ls, extra);
}

/*若实际返回值数和需要的返回值数不一致，需要增减保持一致*/
void Vm::adjust_top(LlamaState* ls, int newtop) {
  int extra = stack_top(ls) - newtop;
  if (extra >= 0) {  /*丢弃多余的返回值*/
    stack_pop_n(ls, extra);
  }
  else {  /*填充缺少的返回值*/
    while (extra++) {
      stack_push_nil(ls);
    }
  }
}

int Vm::call_c(LlamaState* ls, CFunction func, int base) {
  CStack oldSk = ls->cstack;
  int firstResult = 0;

  ls->cstack.params = stack_top(ls) - base;
  ls->cstack.base = base;
  ls->cstack.resultBase = base + ls->cstack.params;

  (*func)(ls);

  firstResult = ls->cstack.resultBase;
  ls->cstack = oldSk;
  return firstResult;
}

int Vm::get_line(ObjectProto* proto, int pc) {
  int i = 0, j = proto->lines.size() - 1;
  while (i <= j) {
    int m = (i + j) / 2;
    LineInfo& li = proto->lines.data[m];
    if (li.pc == pc) { return li.line; }
    else if (li.pc < pc) { i = m + 1; }
    else { j = m - 1; }
  }
  return 0;
}
