#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "base/const.h"
#include "lex.h"

Lex::Lex() {
  buff.init(STD_BUFFER_LEN, FUNC_NAME);
}

bool Lex::open_file(const char* filename) {
  line = 1;
  bool result = reader.open(filename);
  next();
  return result;
}

void Lex::close_file() {
  reader.close();
}

Token Lex::nextToken() {
  found = false;
  buff.clear();
  token.reset();

  while (!found) {
    switch (curr) {
    case ' ': case '\t': case '\r':  /*无效字符*/
      next();
      break;

    case '\n':  /*换行符*/
      line++;
      next();
      break;

    case '+': case '*': case '/':
    case '(': case ')': case '[': case ']': case '{': case '}':
    case ',': case ';': {  /*单字符token*/
      genToken(CharTokens[curr]);
      next();
      break;
    }

    case '-': {
      next();
      if ('-' != curr) {
        genToken(TK_SUB_MINUS);
      }
      else {  /*-- 单行注释，忽略*/
        do {
          next();
        } while ('\n' != curr && LEX_EOZ != curr);
      }
      break;
    }

    case '.': {  /*点号.，或字符串连接符..，或可变参数...*/
      TokenType tt = TK_POINT;
      next();
      if ('.' == curr) {
        next();
        if ('.' == curr) {
          tt = TK_DOTS;
          next();
        }
        else {
          tt = TK_CONCAT;
        }
      }
      genToken(tt);
      break;
    }

    case '=': case '>': case '<': {  /*赋值=，或相等==，或大于>，或大于等于>=，或小于<，或小于等于<=*/
      TokenType first = CharTokens[curr];
      next();

      if (curr == '=') {
        TokenType tt = TK_EQ;
        if (TK_GT == first) tt = TK_GE;
        else if (TK_LT == first) tt = TK_LE;

        genToken(tt);
        next();
      }
      else {
        genToken(first);
      }
      break;
    }

    case '~': {  /*不等于~=*/
      next();
      if ('=' == curr) {
        genToken(TK_NE);
        next();
      }
      break;
    }

    case '"': case '\'':  /*字符串，双引号或单引号*/
      parse_string();
      genToken(TK_STRING);
      break;

    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':  /*整数或小数*/
      parse_number();
      genToken(TK_NUMBER);
      break;

    case LEX_EOZ: case NULL:  /*结束符*/
      genToken(TK_EOS);
      break;

    default:  /*标识符，下划线或字母开头*/
      if (curr == '_' || isalpha(curr)) {
        do {
          buff.add(curr);
          next();
        } while (isalnum(curr) || curr == '_');
        buff.add('\0');

        int index = check_reserved();
        if (-1 == index) {  /*标识符*/
          genToken(TK_NAME);
        }
        else {  /*保留字*/
          genToken(Reserved[index].tt);
        }
        break;
      }
      else {
        error("unknown character");
      }
    }
  }
  return token;
}

void Lex::next() {
  curr = reader.next();
}

void Lex::genToken(TokenType tt) {
  token.tt = tt;
  token.str = buff.data;
  token.line = line;
  found = true;
}

void Lex::error(ShortString msg) {
  throw RuntimeError(msg, line);
}

bool Lex::parse_string() {
  char delimiter = curr;
  next();
  while (curr != delimiter) {
    switch (curr) {
    case LEX_EOZ:
    case '\n':  /*无效字符串*/
      error("unfinished string");
      return false;

    case '\\':
      next();
      switch (curr) {  /*转义字符*/
      case 'a': buff.add('\a'); next(); break;
      case 'b': buff.add('\b'); next(); break;
      case 'f': buff.add('\f'); next(); break;
      case 'n': buff.add('\n'); next(); break;
      case 'r': buff.add('\r'); next(); break;
      case 't': buff.add('\t'); next(); break;
      case 'v': buff.add('\v'); next(); break;
      default:
        if (isdigit(curr)) {  /*十六进制数字*/
          /* \xxx */
        }
        else {
          /* \\ \" \' */
          buff.add(curr);
          next();
        }
        break;
      }

    default:
      buff.add(curr);
      next();
    }
  }

  buff.add('\0');
  next();
  return true;
}

bool Lex::parse_number() {
  /*整数部分*/
  do {
    buff.add(curr);
    next();
  } while (isdigit(curr));

  /*小数部分*/
  if (curr == '.') {
    buff.add(curr);
    next();

    while (isdigit(curr)) {
      buff.add(curr);
      next();
    }
  }

  buff.add('\0');
  return true;
}

int Lex::check_reserved() {
  for (int i = 0; i < sizeof(Reserved) / sizeof(ReservedToken); i++) {
    if (NULL == Reserved[i].word)
      break;
    if (0 == strcmp(Reserved[i].word, buff.data)){
      return i;
    }
  }
  return -1;
}
