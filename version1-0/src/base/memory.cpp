#include <stdio.h>
#include <map>
#include "memory.h"
#include "dump.h"

struct BlockDesc {
  const char* func;
  const char* prevFunc;
  int size;
};
static std::map<void*, BlockDesc> Blocks;

void BlockMonitor::reset() {
  Blocks.clear();
}

void BlockMonitor::add(void* ptr, int size, const char* func, const char* prevFunc) {
  BlockDesc desc = { func, prevFunc, size };
  Blocks[ptr] = desc;
}

void BlockMonitor::del(void* ptr) {
  Blocks.erase(ptr);
}

void BlockMonitor::dump() {
  log("\n============= block stat =============\n");
  int i = 1;
  std::map<void*, BlockDesc>::iterator iter = Blocks.begin();
  for (; iter != Blocks.end(); iter++) {
    BlockDesc& desc = iter->second;
    log("[%002d] %s -> %s %d\n", i++, desc.prevFunc, desc.func, desc.size);
  }
}
