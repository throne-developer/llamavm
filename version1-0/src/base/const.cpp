#include <stdlib.h>
#include <string.h>
#include "const.h"

#define NAME(n) #n

TokenType CharTokens[256];
OpPriority UnaryOps[TK_MAX];
OpPriority BinOps[TK_MAX];

ReservedToken Reserved[TK_MAX] = {
  { "and", TK_AND },
  { "do", TK_DO },
  { "else", TK_ELSE },
  { "elseif", TK_ELSEIF },
  { "end", TK_END },
  { "function", TK_FUNCTION },
  { "if", TK_IF },
  { "local", TK_LOCAL },
  { "nil", TK_NIL },
  { "not", TK_NOT },
  { "or", TK_OR },
  { "return", TK_RETURN },
  { "then", TK_THEN },
  { "while", TK_WHILE },
  { "break", TK_BREAK },
  { "continue", TK_CONTINUE },
  { "for", TK_FOR },
  { NULL, TK_NONE },
};

const char* TokenNames[TK_MAX] = {
  NAME(TK_NONE),
  NAME(TK_NUMBER),
  NAME(TK_STRING),
  NAME(TK_NAME),
  NAME(TK_EOS),
  NAME(TK_ASSIGN),
  NAME(TK_EQ),
  NAME(TK_NE),
  NAME(TK_POINT),
  NAME(TK_CONCAT),
  NAME(TK_DOTS),
  NAME(TK_ADD),
  NAME(TK_SUB_MINUS),
  NAME(TK_MUL),
  NAME(TK_DIV),
  NAME(TK_GT),
  NAME(TK_GE),
  NAME(TK_LT),
  NAME(TK_LE),
  NAME(TK_L_PAREN),
  NAME(TK_R_PAREN),
  NAME(TK_L_BRACES),
  NAME(TK_R_BRACES),
  NAME(TK_L_SQUARE),
  NAME(TK_R_SQUARE),
  NAME(TK_SEMICOLON),
  NAME(TK_COMMA),
  NAME(TK_AND),
  NAME(TK_DO),
  NAME(TK_ELSE),
  NAME(TK_ELSEIF),
  NAME(TK_END),
  NAME(TK_FUNCTION),
  NAME(TK_IF),
  NAME(TK_LOCAL),
  NAME(TK_NIL),
  NAME(TK_NOT),
  NAME(TK_OR),
  NAME(TK_RETURN),
  NAME(TK_THEN),
  NAME(TK_WHILE),
  NAME(TK_BREAK),
  NAME(TK_CONTINUE),
  NAME(TK_FOR),
};

OpcodeAttr OpAttrs[OP_MAX] = {
  { "", 0 },
  { NAME(OP_PUSH_NIL), 1 },
  { NAME(OP_POP), 1 },
  { NAME(OP_SET_GLOBAL), 1 },
  { NAME(OP_GET_GLOBAL), 1 },
  { NAME(OP_PUSH_CONSTANT), 1 },
  { NAME(OP_PUSH_LOCAL), 1 },
  { NAME(OP_SET_LOCAL), 1 },
  { NAME(OP_NOT), 0 },
  { NAME(OP_MINUS), 0 },
  { NAME(OP_EQ), 0 },
  { NAME(OP_NE), 0 },
  { NAME(OP_GT), 0 },
  { NAME(OP_LT), 0 },
  { NAME(OP_GE), 0 },
  { NAME(OP_LE), 0 },
  { NAME(OP_CONCAT), 0 },
  { NAME(OP_ADD), 0 },
  { NAME(OP_SUB), 0 },
  { NAME(OP_MUL), 0 },
  { NAME(OP_DIV), 0 },
  { NAME(OP_ON_TRUE_JMP), 1 },
  { NAME(OP_ON_FALSE_JMP), 1 },
  { NAME(OP_IF_FALSE_JMP), 1 },
  { NAME(OP_JMP), 1 },
  { NAME(OP_CALL), 2 },
  { NAME(OP_RETURN), 1 },
  { NAME(OP_TAILCALL), 2 },
  { NAME(OP_SET_ARRAY), 1 },
  { NAME(OP_SET_HASH), 1 },
  { NAME(OP_NEW_TABLE), 2 },
  { NAME(OP_SET_TABLE), 1 },
  { NAME(OP_SET_TABLE_AND_POP), 0 },
  { NAME(OP_TABLE_DOT_GET), 1 },
  { NAME(OP_TABLE_INDEXED_GET), 0 },
  { NAME(OP_FOR_INIT), 1 },
  { NAME(OP_FOR_LOOP), 1 },
  { NAME(END_CODE), 0 },
};

static void init_priority(OpPriority ops[], TokenType op, int priority, OpCode opcode) {
  ops[op].priority = priority;
  ops[op].opcode = opcode;
}

void init_constant() {
  memset(CharTokens, 0, sizeof(CharTokens));
  CharTokens['+'] = TK_ADD;
  CharTokens['-'] = TK_SUB_MINUS;
  CharTokens['*'] = TK_MUL;
  CharTokens['/'] = TK_DIV;
  CharTokens['('] = TK_L_PAREN;
  CharTokens[')'] = TK_R_PAREN;
  CharTokens['['] = TK_L_SQUARE;
  CharTokens[']'] = TK_R_SQUARE;
  CharTokens['{'] = TK_L_BRACES;
  CharTokens['}'] = TK_R_BRACES;
  CharTokens[';'] = TK_SEMICOLON;
  CharTokens[','] = TK_COMMA;
  CharTokens['='] = TK_ASSIGN;
  CharTokens['>'] = TK_GT;
  CharTokens['<'] = TK_LT;

  memset(UnaryOps, 0, sizeof(UnaryOps));
  memset(BinOps, 0, sizeof(BinOps));
  init_priority(BinOps, TK_EQ, 1, OP_EQ);  /* == ~= > < >= <= */
  init_priority(BinOps, TK_NE, 1, OP_NE);
  init_priority(BinOps, TK_GT, 1, OP_GT);
  init_priority(BinOps, TK_LT, 1, OP_LT);
  init_priority(BinOps, TK_LE, 1, OP_LE);
  init_priority(BinOps, TK_GE, 1, OP_GE);
  init_priority(BinOps, TK_CONCAT, 2, OP_CONCAT);  /* .. */
  init_priority(BinOps, TK_ADD, 3, OP_ADD);  /* + - */
  init_priority(BinOps, TK_SUB_MINUS, 3, OP_SUB);
  init_priority(BinOps, TK_MUL, 4, OP_MUL);  /* * / */
  init_priority(BinOps, TK_DIV, 4, OP_DIV);
  init_priority(UnaryOps, TK_NOT, 5, OP_NOT);  /* not - */
  init_priority(UnaryOps, TK_SUB_MINUS, 5, OP_MINUS);
}
