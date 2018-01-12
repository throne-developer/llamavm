/*
** 通用方法
*/
#ifndef common_h__
#define common_h__

#include "basedef.h"
#include "memory.h"

struct FileReader {
  FileReader() : fp(0), length(0), curr(0) {}

  bool open(const char* filename);
  void close();
  char next();

public:
  FILE* fp;
  char content[STD_BUFFER_LEN];
  int length;
  int curr;
};

struct FileWriter {
  FileWriter() : fp(0) {}

  bool open(const char* filename);
  void close();
  void write(const char* str, int length);

public:
  FILE* fp;
};

class BaseFunc {
public:
  static int ceil_log2(unsigned int x);
  static int square_size(int size, int base);
  static HashNum hash_num(Number n);
  static HashNum hash_string(const char* s, int length);
};

class RuntimeError {
public:
  RuntimeError(const ShortString& message, int lineNum) : msg(message), line(lineNum) {}

  ShortString msg;
  int line;
};

#endif // common_h__