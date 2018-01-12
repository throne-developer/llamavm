/*
** 词法解析
（1）根据输入字符流，逐个读入char字符
（2）根据当前char值，以及下个或下下个char值，确定token类型。对应LL(k)文法，向前看k个字符
（3）根据token类型，解析token值，如整数、小数、字符串
*/
#ifndef lex_h__
#define lex_h__

#include <stdlib.h>
#include "base/common.h"

class Lex {
public:
  Lex();

  bool open_file(const char* filename);
  void close_file();
  Token nextToken();

private:
  void next();
  void genToken(TokenType tt);

  void error(ShortString msg);
  bool parse_string();
  bool parse_number();
  int check_reserved();

public:
  FileReader reader;
  LongString buff;
  int line;
  char curr;
  bool found;
  Token token;
};

#endif // lex_h__
