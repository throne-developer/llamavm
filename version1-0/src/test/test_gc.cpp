#include <stdio.h>
#include <time.h>
#include <math.h>
#include "test.h"
#include "base/dump.h"

struct GCNodeInt {
  int val;
  GCNode<GCNodeInt> gc;
};
typedef GCLinkedList<GCNodeInt> GCListInt;

struct NodeTestData {
  int val;
  GCState state;
};

static void find_in_gclist(GCListInt& ls, int val) {
  bool exist = false;

  GCNodeInt* curr = ls.head;
  while (curr) {
    if (curr->val == val) {
      exist = true;
      break;
    }
    curr = curr->gc.next;
  }

  if (!exist) {
    printf("[TEST Failed XXXXXXXXXX] %d not found\n", val);
    getchar();
  }
}

static void test_gc(GCListInt& listMarked, GCListInt& listNoMark, NodeTestData data[], int length) {
  /*ÃÓ≥‰£¨ ’ºØ*/
  for (int i = 0; i < length; i++) {
    GCNodeInt* node = new GCNodeInt();
    node->val = data[i].val;
    node->gc.state = data[i].state;
    listMarked.insert_head(node);
  }

  listMarked.collect_unmarked(listNoMark);

  /*ºÏ≤‚*/
  for (int i = 0; i < length; i++) {
    if (GCSTATE_MARKED == data[i].state) {
      find_in_gclist(listMarked, data[i].val);
    }
    else if (GCSTATE_NO_MARK == data[i].state) {
      find_in_gclist(listNoMark, data[i].val);
    }
  }
  listMarked.free();
  listNoMark.free();
}

void Test::test_gclist() {
  GCListInt list1, list2;

  NodeTestData data1[] = {  /* 1 1 0 1 1 */
    { 100, GCSTATE_MARKED },
    { 200, GCSTATE_MARKED },
    { 300, GCSTATE_NO_MARK },
    { 400, GCSTATE_MARKED },
    { 500, GCSTATE_MARKED },
  };
  test_gc(list1, list2, data1, sizeof(data1) / sizeof(NodeTestData));

  NodeTestData data2[] = {  /* 0 0 1 0 0 */
    { 100, GCSTATE_NO_MARK },
    { 200, GCSTATE_NO_MARK },
    { 300, GCSTATE_MARKED },
    { 400, GCSTATE_NO_MARK },
    { 500, GCSTATE_NO_MARK },
  };
  test_gc(list1, list2, data2, sizeof(data2) / sizeof(NodeTestData));
  log("[TEST Passed]\n");
}