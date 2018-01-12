#include <stdio.h>
#include <time.h>
#include <math.h>
#include "test.h"
#include "code/gram.h"
#include "base/dump.h"
#include "base/common.h"
#include "base/state.h"
#include "vm/vm.h"

UseTime::UseTime(const char* name) {
  this->name = name;
  st = (int)time(NULL);
}

UseTime::~UseTime() {
  log("[UseTime] %s %ds\n", name, (int)time(NULL) - st);
}

void Test::self_test() {
  log("=======TEST BEGIN=======\n");

  const char* files[] = {
    "testcase/var.lua",
    "testcase/expr.lua",
    "testcase/ifwhile.lua",
    "testcase/table.lua",
    "testcase/function.lua",
    "testcase/sort_example.lua",
    NULL,
  };

  for (int i = 0; i < sizeof(files) / sizeof(char*); i++) {
    if (!files[i]) {
      break;
    }

    execute(files[i]);
  }

  test_gclist();
  test_log2();

  log("=======TEST END=======\n\n");
}

void Test::execute(const char* filename) {
  LlamaState ls;
  ls.execute_file(filename);
}


void Test::test_log2() {
  if (false) {
    printf("log\n");
    for (int i = 1; i <= 1200; i++) {
      printf("log2(%d)=%d\t", i, BaseFunc::ceil_log2(i));
    }
  }
}