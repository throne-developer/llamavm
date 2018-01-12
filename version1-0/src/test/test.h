#ifndef test_h__
#define test_h__

#include "base/lobject.h"

class UseTime {
public:
  UseTime(const char* name);
  ~UseTime();

private:
  const char* name;
  int st;
};

class Test {
public:
  static void self_test();

  static void test_gclist();
  static void test_log2();

private:
  static void execute(const char* filename);
};

#endif // test_h__
