/*
** 打印内存对象值
*/
#ifndef dump_h__
#define dump_h__

#include "base/basedef.h"

struct LlamaState;
struct ObjectProto;
struct TObject;

void dump_object(TObject* obj, int depth);
void dump_proto(ObjectProto* proto, int depth);
void dump_global(LlamaState* ls);

void log(const char *fmt, ...);

#endif // dump_h__
