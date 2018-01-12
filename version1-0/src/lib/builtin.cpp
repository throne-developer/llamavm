#include <stdio.h>
#include <time.h>
#include "builtin.h"
#include "base/state.h"

CFuncInfo BuiltIn::funcs[BUILT_IN_FUNC_NUM] = {
  { "tostring", BuiltIn::tostring },
  { "print", BuiltIn::print },
  { "assert", BuiltIn::assert },
  { "floor", BuiltIn::floor },
  { "clock", BuiltIn::clock },

  { "getn", BuiltIn::getn },
  { "tnext", BuiltIn::tnext },
  { "tinsert", BuiltIn::tinsert },
  { "tremove", BuiltIn::tremove },
  { "tconcat", BuiltIn::tconcat },

  { NULL, NULL },
};

void BuiltIn::open_lib(LlamaState* ls) {
  for (int i = 0; i < BUILT_IN_FUNC_NUM; i++) {
    if (!funcs[i].name) {
      break;
    }

    TObject key, value;
    ls->new_string(key, funcs[i].name, 0, FUNC_NAME, GCSTATE_FIXED);
    value.as_cfunc(funcs[i].func);
    ls->set_global(key, value);
  }
}

TObject BuiltIn::get_param(LlamaState* ls, int order, ObjectType ot) {
  if (order < 1 || order > ls->cstack.params) {
    ls->error("wrong param order");
  }

  int address = ls->cstack.base + order - 1;
  TObject obj = stack_at(ls, address);
  if (OBJECT_UNKNOWN != ot && obj.ot != ot) {
    ls->error("c function param type wrong");
  }
  return obj;
}

int BuiltIn::param_num(LlamaState* ls) {
  return ls->cstack.params;
}

int BuiltIn::tostring(LlamaState* ls) {
  TObject obj = get_param(ls, 1, OBJECT_UNKNOWN);
  ls->push_object(to_string_object(ls, obj));
  return 0;
}

int BuiltIn::print(LlamaState* ls) {
  int params = param_num(ls);

  for (int i = 1; i <= params; i++) {
    TObject obj = get_param(ls, i, OBJECT_UNKNOWN);
    if (i > 1) {
      printf("\t");
    }
    printf("%s", to_string_object(ls, obj).c_str());
  }
  printf("\n");
  return 0;
}

TObject BuiltIn::to_string_object(LlamaState* ls, TObject obj) {
  ShortString buff;

  switch (obj.ot) {
  case OBJECT_NIL:
    buff.set_str("nil");
    break;
  case OBJECT_NUMBER:
    buff.format(NUMBER_FMT, NUM_VAL(obj));
    break;
  case OBJECT_CPROTO:
    buff.format("cproto %p", obj.value.cf);
    break;
  case OBJECT_STRING:
    return obj;
  case OBJECT_PROTO:
    buff.format("proto %p", obj.value.proto);
    break;
  case OBJECT_TABLE:
    buff.format("table %p", obj.value.table);
    break;
  default:
    buff.set_str("unknown");
    break;
  }

  TObject result;
  ls->new_string(result, buff.data, buff.length, FUNC_NAME, GCSTATE_NO_MARK);
  return result;
}

int BuiltIn::assert(LlamaState* ls) {
  TObject real = get_param(ls, 1, OBJECT_UNKNOWN);
  TObject target = get_param(ls, 2, OBJECT_UNKNOWN);

  if (!real.equal(target)) {
    ShortString buff;
    buff.format("assert failed! real=%s, target=%s ",
      to_string_object(ls, real).c_str(),
      to_string_object(ls, target).c_str());
    ls->error(buff.data);
  }
  return 0;
}

int BuiltIn::floor(LlamaState* ls) {
  TObject obj = get_param(ls, 1, OBJECT_NUMBER);
  ls->push_number((int)NUM_VAL(obj));
  return 0;
}

int BuiltIn::clock(LlamaState* ls) {
  Number t = (Number)::clock();
  ls->push_number(t / 1000);
  return 0;
}

int BuiltIn::getn(LlamaState* ls) {
  TObject tb = get_param(ls, 1, OBJECT_TABLE);
  ls->push_number(TABLE_VAL(tb).getn());
  return 0;
}

int BuiltIn::tnext(LlamaState* ls) {
  TObject tb = get_param(ls, 1, OBJECT_TABLE);
  TObject key = get_param(ls, 2, OBJECT_UNKNOWN);

  TObject nextkey = TABLE_VAL(tb).next_key(key);
  TObject nextval = TABLE_VAL(tb).get(nextkey);
  ls->push_object(nextkey);
  ls->push_object(nextval);
  return 0;
}

int BuiltIn::tinsert(LlamaState* ls) {
  int params = param_num(ls);
  if (2 != params && 3 != params) {
    ls->error("tinsert params wrong");
  }
  TObject tb = get_param(ls, 1, OBJECT_TABLE);
  TObject value = get_param(ls, (2 == params ? 2 : 3), OBJECT_UNKNOWN);

  Table& t = TABLE_VAL(tb);
  int maxn = t.getn();
  int npos = maxn + 1;
  if (3 == params) {
    TObject pos = get_param(ls, 2, OBJECT_NUMBER);
    if (NUM_VAL(pos) <= 0 || !IS_INTEGER(pos)) {
      ls->error("wrong insert pos");
    }
    npos = (int)NUM_VAL(pos);
  }

  for (int i = maxn + 1; i > npos; i--) {
    TObject obj = t.get_num(i - 1);
    t.set_num(i, obj);
  }
  t.set_num(npos, value);
  return 0;
}

int BuiltIn::tremove(LlamaState* ls) {
  int params = param_num(ls);
  if (1 != params && 2 != params) {
    ls->error("tremove params wrong");
  }
  TObject tb = get_param(ls, 1, OBJECT_TABLE);

  Table& t = TABLE_VAL(tb);
  int maxn = t.getn();
  int npos = maxn;
  if (2 == params) {
    TObject pos = get_param(ls, 2, OBJECT_NUMBER);
    if (!IS_INTEGER(pos)) {
      ls->error("wrong insert pos");
    }
    npos = (int)NUM_VAL(pos);
  }

  if (npos >= 1 && npos <= maxn) {
    for (int i = npos; i < maxn; i++) {
      TObject obj = t.get_num(i + 1);
      t.set_num(i, obj);
    }
    t.set_num(maxn, TObject());
  }
  return 0;
}

int BuiltIn::tconcat(LlamaState* ls) {
  int params = param_num(ls);
  if (params < 1 || params > 4) {
    ls->error("tconcat params wrong");
  }
  TObject tb = get_param(ls, 1, OBJECT_TABLE);

  Table& t = TABLE_VAL(tb);
  int maxn = t.getn();
  ShortString sep = "";
  int start = 1, end = maxn;

  if (params >= 2) {
    sep = get_param(ls, 2, OBJECT_STRING).c_str();
  }
  if (params >= 3) {
    start = (int)NUM_VAL(get_param(ls, 3, OBJECT_NUMBER));
    start = MAX_VAL(start, 1);
  }
  if (params >= 4) {
    end = (int)NUM_VAL(get_param(ls, 4, OBJECT_NUMBER));
    end = MIN_VAL(end, maxn);
  }

  LongString buff;
  buff.init(INIT_SIZE_BUFFER, FUNC_NAME);
  ShortString temp;
  for (int i = start; i <= end && i <= maxn; i++) {
    TObject obj = t.get_num(i);
    if (OBJECT_STRING == obj.ot) {
      ls->add_buff(buff, obj.c_str(), obj.str_len());
    }
    else if (OBJECT_NUMBER == obj.ot) {
      temp.format(NUMBER_FMT, NUM_VAL(obj));
      ls->add_buff(buff, temp.data, temp.length);
    }
    else {
      ls->error("tconcat failed! element must be string or number");
    }

    if (i != end) {
      ls->add_buff(buff, sep.data, sep.length);
    }
  }
  buff.add('\0');

  TObject obj;
  ls->new_string(obj, buff.data, 0, FUNC_NAME, GCSTATE_NO_MARK);
  ls->push_object(obj);
  return 0;
}