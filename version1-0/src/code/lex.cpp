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
    case ' ': case '\t': case '\r':  /*��Ч�ַ�*/
      next();
      break;

    case '\n':  /*���з�*/
      line++;
      next();
      break;

    case '+': case '*': case '/':
    case '(': case ')': case '[': case ']': case '{': case '}':
    case ',': case ';': {  /*���ַ�token*/
      genToken(CharTokens[curr]);
      next();
      break;
    }

    case '-': {
      next();
      if ('-' != curr) {
        genToken(TK_SUB_MINUS);
      }
      else {  /*-- ����ע�ͣ�����*/
        do {
          next();
        } while ('\n' != curr && LEX_EOZ != curr);
      }
      break;
    }

    case '.': {  /*���.�����ַ������ӷ�..����ɱ����...*/
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

    case '=': case '>': case '<': {  /*��ֵ=�������==�������>������ڵ���>=����С��<����С�ڵ���<=*/
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

    case '~': {  /*������~=*/
      next();
      if ('=' == curr) {
        genToken(TK_NE);
        next();
      }
      break;
    }

    case '"': case '\'':  /*�ַ�����˫���Ż�����*/
      parse_string();
      genToken(TK_STRING);
      break;

    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':  /*������С��*/
      parse_number();
      genToken(TK_NUMBER);
      break;

    case LEX_EOZ: case NULL:  /*������*/
      genToken(TK_EOS);
      break;

    default:  /*��ʶ�����»��߻���ĸ��ͷ*/
      if (curr == '_' || isalpha(curr)) {
        do {
          buff.add(curr);
          next();
        } while (isalnum(curr) || curr == '_');
        buff.add('\0');

        int index = check_reserved();
        if (-1 == index) {  /*��ʶ��*/
          genToken(TK_NAME);
        }
        else {  /*������*/
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
    case '\n':  /*��Ч�ַ���*/
      error("unfinished string");
      return false;

    case '\\':
      next();
      switch (curr) {  /*ת���ַ�*/
      case 'a': buff.add('\a'); next(); break;
      case 'b': buff.add('\b'); next(); break;
      case 'f': buff.add('\f'); next(); break;
      case 'n': buff.add('\n'); next(); break;
      case 'r': buff.add('\r'); next(); break;
      case 't': buff.add('\t'); next(); break;
      case 'v': buff.add('\v'); next(); break;
      default:
        if (isdigit(curr)) {  /*ʮ����������*/
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
  /*��������*/
  do {
    buff.add(curr);
    next();
  } while (isdigit(curr));

  /*С������*/
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
