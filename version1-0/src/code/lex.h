/*
** �ʷ�����
��1�����������ַ������������char�ַ�
��2�����ݵ�ǰcharֵ���Լ��¸������¸�charֵ��ȷ��token���͡���ӦLL(k)�ķ�����ǰ��k���ַ�
��3������token���ͣ�����tokenֵ����������С�����ַ���
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
