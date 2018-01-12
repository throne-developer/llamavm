/*
** 虚拟机执行字节码
** 输入：proto对象
** 输出：指令运行结果
*/

#ifndef vm_h__
#define vm_h__

#include "base/lobject.h"

struct LlamaState;

class Vm {
public:
  static int run(LlamaState* ls, ObjectProto* proto);

private:
  static int execute(LlamaState* ls, ObjectProto* proto, int base);
  static int call_c(LlamaState* ls, CFunction func, int base);

  static Number calc(Number n1, Number n2, OpCode op);
  static bool compare(Number n1, Number n2, OpCode op);
  static void concat(LlamaState* ls, const TObject& o1, const TObject& o2, TObject& result);
  static void call(LlamaState* ls, int params, int results);
  static void adjust_top(LlamaState* ls, int newtop);

  static int get_line(ObjectProto* proto, int pc);
};

#endif // vm_h__
