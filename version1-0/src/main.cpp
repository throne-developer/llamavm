#include <stdio.h>
#include <stdlib.h>
#include "base/const.h"
#include "base/state.h"
#include "test/test.h"

int main() {
  init_constant();
  Test::self_test();

  LlamaState ls;
  ls.execute_file_with_debug("test.lua");

#ifdef WIN32 
  getchar();
#endif
  return 0;
}
