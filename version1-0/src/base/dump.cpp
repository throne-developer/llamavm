#include <stdio.h>
#include <stdarg.h>
#include "dump.h"
#include "base/const.h"
#include "base/common.h"
#include "base/memory.h"
#include "state.h"

static FileWriter Logger;

static void init_offset(int depth, ShortString& str) {
  str.clear();
  for (int i = 1; i <= depth * 4; i++) {
    str.append(' ');
  }
  str.append('\0');
}

void dump_proto(ObjectProto* proto, int depth) {
  ShortString offset;
  init_offset(depth, offset);

  log("\n%s==================== CONSTANTS ====================\n", offset.data);
  for (int i = 0; i < proto->constants.size(); i++) {
    TObject& data = proto->constants.data[i];
    switch (data.ot) {
    case OBJECT_STRING: 
      log("%s[%d] %s\n", offset.data, i, data.c_str());
      break;
    case OBJECT_NUMBER: 
      log("%s[%d] " NUMBER_FMT "\n", offset.data, i, data.value.num);
      break;
    case OBJECT_PROTO: 
      log("%s[%d] PROTO\n", offset.data, i);
      dump_object(&data, depth + 1);
      break;
    case OBJECT_TABLE: 
      log("table");
      break;
    default:
      break;
    }
  }

  log("\n%s==================== CODES ====================\n", offset.data);
  log("%s[%03d] arg_num = %d\n", offset.data, GRAM_CODE_ARG_NUM_POS, 
    proto->codes.data[GRAM_CODE_ARG_NUM_POS]);

  for (int i = VM_CODE_START; i < proto->codes.size();) {
    OpcodeAttr& attr = OpAttrs[proto->codes.data[i]];
    log("%s[%03d] %s", offset.data, i, attr.name);

    for (int k = 1; k <= attr.args; k++) {
      log("\t\t%d", proto->codes.data[i + k]);
    }
    log("\n");

    i += 1 + attr.args;
  }
}

void dump_object(TObject* obj, int depth) {
  switch (obj->ot) {
  case OBJECT_NIL:
    log("nil "); break;
  case OBJECT_NUMBER:
    log("%.2f ", obj->value.num); break;
  case OBJECT_CPROTO:
    log("cproto "); break;
  case OBJECT_STRING:
    log("%s ", obj->c_str()); break;
  case OBJECT_PROTO:
    dump_proto(obj->value.proto, depth); break;
  case OBJECT_TABLE:
    log("table "); break;
  default: break;
  }
}

void dump_global(LlamaState* ls) {
  log("\n==================== GLOBAL VARS ====================\n");
  TObject key, value;
  int order = 0;
  while (true) {
    key = ls->globals.next_key(key);
    if (IS_NIL(key))
      break;

    log("[%03d]", order++);
    value = ls->globals.get(key);
    dump_object(&key, 0);
    if (OBJECT_PROTO == value.ot) {
      log("proto ");
    }
    else {
      dump_object(&value, 0);
    }
    log("\n");
  }
}

void log(const char *fmt, ...) {
  static char buff[2048] = { 0 };

  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buff, sizeof(buff)-1, fmt, ap);
  va_end(ap);

  if (!Logger.fp) {
    Logger.open("debug.log");
  }
  Logger.write(buff, strlen(buff));
}
