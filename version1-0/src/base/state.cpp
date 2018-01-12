#include "state.h"
#include "common.h"
#include "lib/builtin.h"
#include "code/gram.h"
#include "test/test.h"
#include "base/dump.h"
#include "vm/vm.h"

LlamaState::LlamaState() {
  sk.init(INIT_SIZE_STACK, FUNC_NAME);
  globals.init(0, INIT_SIZE_GLOBAL_VAR);
  strings.init(INIT_SIZE_STRING_SET);
  open_state();
  currPc = 0;
}

LlamaState::~LlamaState() {
  close_state();
}

void LlamaState::open_state() {
  BuiltIn::open_lib(this);
}

void LlamaState::close_state() {
}

void LlamaState::execute_file(const char* filename) {
  Grammar gram;
  ObjectProto* proto = gram.parse(this, filename);
  Vm::run(this, proto);
}

void LlamaState::execute_file_with_debug(const char* filename) {
  BlockMonitor::reset();
  UseTime ut("execute");

  Grammar gram;
  ObjectProto* proto = gram.parse(this, filename);
  Vm::run(this, proto);

  dump_proto(proto, 0);
  dump_global(this);
  BlockMonitor::dump();
}

void LlamaState::new_string(TObject& obj, const char* str, int length, const char* prevFunc, GCState state) {
  /*¶Ì×Ö·û´®*/
  length = (0 == length ? strlen(str) : length);
  if (length < SHORT_STR_SIZE) {
    obj.set_short_string(str, length);
    return;
  }

  /*×Ö·û´®ÊÇ·ñ´æÔÚ*/
  ObjectString temp;
  temp.str = (char*)str;
  temp.length = length;
  temp.hash = BaseFunc::hash_string(str, length);

  ObjectString* objstr = strings.find(temp);
  temp.str = NULL;  /* ±ÜÃâÎö¹¹ÊÍ·Å */
  if (objstr) {
    obj.set_long_string(objstr);
    return;
  }

  /*ÐÂ×Ö·û´®*/
  objstr = Allocator<ObjectString>::new_one(FUNC_NAME, prevFunc);
  objstr->str = Allocator<char>::new_array(length + 1, FUNC_NAME, prevFunc);
  memcpy(objstr->str, str, length);
  objstr->str[length] = '\0';

  objstr->length = length;
  objstr->hash = temp.hash;
  objstr->gc.state = state;

  strings.add(objstr);
  obj.set_long_string(objstr);
}

ObjectProto* LlamaState::new_proto(const char* prevFunc) {
  return Allocator<ObjectProto>::new_one(FUNC_NAME, prevFunc);
}

ObjectTable* LlamaState::new_table(int arrays, int hashs, const char* prevFunc, GCState state) {
  ObjectTable* obj = Allocator<ObjectTable>::new_one(FUNC_NAME, prevFunc);
  obj->t.init(arrays, hashs);
  obj->gc.state = state;

  tables.insert_head(obj);
  return obj;
}

void LlamaState::error(ShortString msg, int line) {
  throw RuntimeError(msg, line);
}

void LlamaState::add_buff(LongString& buff, const char* s, int length) {
  for (int i = 0; i < length; i++) {
    buff.add(s[i]);
  }
}

void LlamaState::set_global(const TObject& key, const TObject& value) {
  globals.set(key, value);
}

void LlamaState::push_string(const char* s) {
  TObject obj;
  new_string(obj, (NULL == s ? "nil" : s), 0, FUNC_NAME, GCSTATE_NO_MARK);
  push_object(obj);
}

void LlamaState::push_number(Number n) {
  stack_push_number(this, n);
}

void LlamaState::push_object(TObject obj) {
  stack_push_object(this, obj);
}

void LlamaState::push_nil() {
  stack_push_nil(this);
}
