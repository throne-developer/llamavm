/*
** 全局常量配置
*/
#ifndef const_h__
#define const_h__

#include "basedef.h"

extern const char* TokenNames[TK_MAX];    /*token名称*/
extern TokenType CharTokens[256];         /*单字符对应的Token*/
extern ReservedToken Reserved[TK_MAX];    /*保留字*/
extern OpcodeAttr OpAttrs[OP_MAX];        /*opcode参数个数*/
extern OpPriority UnaryOps[TK_MAX];       /*一元运算符的优先级*/
extern OpPriority BinOps[TK_MAX];         /*二元运算符的优先级*/

void init_constant();

#endif // const_h__
