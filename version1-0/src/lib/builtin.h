#ifndef builtin_h__
#define builtin_h__

#include "base/basedef.h"

#define BUILT_IN_FUNC(_func) static int _func(LlamaState* ls)

class BuiltIn {
public:
  static void open_lib(LlamaState* ls);

private:
  BUILT_IN_FUNC(tostring);
  BUILT_IN_FUNC(print);
  BUILT_IN_FUNC(assert);
  BUILT_IN_FUNC(floor);
  BUILT_IN_FUNC(clock);
  BUILT_IN_FUNC(getn);
  BUILT_IN_FUNC(tnext);
  BUILT_IN_FUNC(tinsert);
  BUILT_IN_FUNC(tremove);
  BUILT_IN_FUNC(tconcat);

  static TObject get_param(LlamaState* ls, int order, ObjectType ot);
  static int param_num(LlamaState* ls);
  static TObject to_string_object(LlamaState* ls, TObject obj);

private:
  static CFuncInfo funcs[BUILT_IN_FUNC_NUM];
};

#endif // builtin_h__
