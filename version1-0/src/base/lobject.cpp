#include "lobject.h"
#include "common.h"

bool TObject::equal(const TObject& t) const {
  if (ot != t.ot)
    return false;

  switch (ot) {
  case OBJECT_NIL:
    return true;
  case OBJECT_NUMBER:
    return value.num == t.value.num;
  case OBJECT_CPROTO:
    return value.cf == t.value.cf;
  case OBJECT_STRING: {
    if (shortLen > 0 && t.shortLen > 0) {
      return shortHash == t.shortHash &&
        shortLen == t.shortLen &&
        strncmp(value.shortstr, t.value.shortstr, SHORT_STR_SIZE) == 0;
    }
    else if (0 == shortLen && 0 == t.shortLen) {
      return value.longstr->equal(*t.value.longstr);
    }
    return false;
  }
  case OBJECT_TABLE:
    return value.table == t.value.table;
  case OBJECT_PROTO:
    return value.proto == t.value.proto;
  }
  return false;
}

void TObject::set_short_string(const char* s, int len)  {
  shortLen = MIN_VAL(len, SHORT_STR_SIZE - 1);
  for (int i = 0; i < shortLen; i++) {
    value.shortstr[i] = s[i];
  }
  value.shortstr[shortLen] = '\0';

  ot = OBJECT_STRING;
  shortHash = BaseFunc::hash_string(value.shortstr, shortLen);
}

const char* TObject::c_str() const {
  return shortLen > 0 ? value.shortstr : value.longstr->str;
}

int TObject::str_len() const {
  return shortLen > 0 ? shortLen : value.longstr->length;
}

HashNum TObject::str_hash() const {
  return shortLen > 0 ? shortHash : value.longstr->hash;
}