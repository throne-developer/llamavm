/*
** ȫ�ֳ�������
*/
#ifndef const_h__
#define const_h__

#include "basedef.h"

extern const char* TokenNames[TK_MAX];    /*token����*/
extern TokenType CharTokens[256];         /*���ַ���Ӧ��Token*/
extern ReservedToken Reserved[TK_MAX];    /*������*/
extern OpcodeAttr OpAttrs[OP_MAX];        /*opcode��������*/
extern OpPriority UnaryOps[TK_MAX];       /*һԪ����������ȼ�*/
extern OpPriority BinOps[TK_MAX];         /*��Ԫ����������ȼ�*/

void init_constant();

#endif // const_h__
