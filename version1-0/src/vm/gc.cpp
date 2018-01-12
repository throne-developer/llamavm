#include "gc.h"
#include "base/state.h"

int gc::gcThreshold = GC_INIT_THRESHOLD;

void gc::check_gc(LlamaState* ls) {
  if (block_num(ls) >= gcThreshold) {
    collect_garbage(ls);
    gcThreshold = MAX_VAL(GC_INIT_THRESHOLD, 2 * block_num(ls));
  }
}

int gc::block_num(LlamaState* ls) {
  return ls->strings.used_num() + ls->tables.blocks;
}

void gc::collect_garbage(LlamaState* ls) {
  visit_stack(ls);
  visit_global(ls);

  GCLinkedList<ObjectString> freeStrings;
  GCLinkedList<ObjectTable> freeTables;

  ls->strings.collect_unmarked(freeStrings);
  ls->tables.collect_unmarked(freeTables);

  freeStrings.free();
  freeTables.free();
  
  ls->strings.shrink();
}

void gc::visit_stack(LlamaState* ls) {
  int sz = ls->sk.size();
  for (int i = 0; i < sz; i++) {
    mark_object(ls, &ls->sk.data[i]);
  }
}

void gc::visit_global(LlamaState* ls) {
  TObject key, value;
  while (true) {
    key = ls->globals.next_key(key);
    if (IS_NIL(key))
      break;

    value = ls->globals.get(key);
    mark_object(ls, &value);
  }
}

void gc::mark_object(LlamaState* ls, TObject* obj) {
  switch (obj->ot) {
  case OBJECT_STRING:
    if (obj->is_long_str()) {
      mark_string(ls, obj->value.longstr);
    }
    break;
  case OBJECT_TABLE:
    mark_table(ls, obj->value.table);
    break;
  default:
    break;
  }
}

void gc::mark_string(LlamaState* ls, ObjectString* str) {
  if (GCSTATE_NO_MARK == str->gc.state) {
    str->gc.state = GCSTATE_MARKED;
  }
}

void gc::mark_table(LlamaState* ls, ObjectTable* tb) {
  if (GCSTATE_NO_MARK == tb->gc.state) {
    tb->gc.state = GCSTATE_MARKED;

    Table& t = tb->t;
    for (int i = 0; i < t.arraySize; i++) {
      if (NOT_NIL(t.arrayData[i])) {
        mark_object(ls, &t.arrayData[i]);
      }
    }
    for (int i = 0; i < t.hashSize; i++) {
      if (t.hashData[i].used()) {
        mark_object(ls, &t.hashData[i].key);
        mark_object(ls, &t.hashData[i].value);
      }
    }
  }
}