/*
** 基本类型定义
*/
#ifndef commondef_h__
#define commondef_h__

struct ObjectString;
struct ObjectProto;
struct ObjectTable;
struct LlamaState;

typedef double Number;
typedef int Instruction;
typedef unsigned int HashNum;
typedef unsigned char Byte;
typedef int(*CFunction)(LlamaState*);

#define FUNC_NAME __FUNCTION__
#define LLAMA_TRY try{
#define LLAMA_CATCH }catch
#define MAX_VAL(a,b) ((a)>(b)?(a):(b))
#define MIN_VAL(a,b) ((a)<(b)?(a):(b))
#define MOD_VAL(a,b) ((b)==0?0:(a)%(b))
#define BETWEEN(v,minv,maxv) ((v)>=(minv) && (v)<=(maxv))
#define NUMBER_FMT "%.14g"

#define NUM_VAL(obj) (obj.value.num)
#define TABLE_VAL(obj) (obj.value.table->t)
#define BOOL_VAL(obj) (IS_NIL(obj)?0:1)
#define IS_NIL(obj) (OBJECT_NIL==(obj).ot)
#define NOT_NIL(obj) (OBJECT_NIL!=(obj).ot)
#define IS_INTEGER(obj) ((Number)(int)(obj).value.num == (obj).value.num)

enum {
  LEX_EOZ = -1,

  GRAM_CODE_ARG_NUM_POS = 0,
  GRAM_LOCAL_VARS = 300,
  GRAM_OP_STACK_NUM = 60,
  GRAM_LOOP_JMP_NUM = 30,

  VM_MULTI_RETURN = -1,
  VM_CODE_START = 1,

  GC_INIT_THRESHOLD = 16,
  STD_BUFFER_LEN = 512,
  BUILT_IN_FUNC_NUM = 200,
  SHORT_STR_SIZE = 16,
};

enum {
  INIT_SIZE_CODE = 1 << 10,
  INIT_SIZE_CONSTANT = 1 << 4,
  INIT_SIZE_STACK = 1 << 8,
  INIT_SIZE_TABLE_HASH = 1 << 3,
  INIT_SIZE_TABLE_ARRAY = 1 << 2,
  INIT_SIZE_BUFFER = 1 << 9,
  INIT_SIZE_STRING_SET = 1 << 6,
  INIT_SIZE_GLOBAL_VAR = 1 << 5,
};

enum TokenType {
  TK_NONE = 0,
  TK_NUMBER,     /* 1.2 */
  TK_STRING,     /* "abc" */
  TK_NAME,       /* a1 */
  TK_EOS,

  TK_ASSIGN,     /* = */
  TK_EQ,         /* == */
  TK_NE,         /* ~= */
  TK_POINT,      /* . */
  TK_CONCAT,     /* .. */
  TK_DOTS,       /* ... */

  TK_ADD,        /* + */
  TK_SUB_MINUS,  /* - */
  TK_MUL,        /* * */
  TK_DIV,        /* / */

  TK_GT,         /* > */
  TK_GE,         /* >= */
  TK_LT,         /* < */
  TK_LE,         /* <= */

  TK_L_PAREN,    /* ( */
  TK_R_PAREN,    /* ) */
  TK_L_BRACES,   /* { */
  TK_R_BRACES,   /* } */
  TK_L_SQUARE,   /* [ */
  TK_R_SQUARE,   /* ] */
  TK_SEMICOLON,  /* ; */
  TK_COMMA,      /* , */

  TK_AND,        /* 保留字 */
  TK_DO,
  TK_ELSE,
  TK_ELSEIF,
  TK_END,
  TK_FUNCTION,
  TK_IF,
  TK_LOCAL,
  TK_NIL,
  TK_NOT,
  TK_OR,
  TK_RETURN,
  TK_THEN,
  TK_WHILE,
  TK_BREAK,
  TK_CONTINUE,
  TK_FOR,

  TK_MAX,
};

enum OpCode {
  OP_PUSH_NIL = 1,          /* [nil数量] */
  OP_POP,                   /* [弹出数量] */
  OP_SET_GLOBAL,            /* [常量索引] */
  OP_GET_GLOBAL,            /* [常量索引] */
  OP_PUSH_CONSTANT,         /* [常量索引] */
  OP_PUSH_LOCAL,            /* [局部变量索引] */
  OP_SET_LOCAL,             /* [局部变量索引] */
  OP_NOT,                   /* [] */
  OP_MINUS,                 /* [] */
  OP_EQ,                    /* [] */
  OP_NE,                    /* [] */
  OP_GT,                    /* [] */
  OP_LT,                    /* [] */
  OP_GE,                    /* [] */
  OP_LE,                    /* [] */
  OP_CONCAT,                /* [] */
  OP_ADD,                   /* [] */
  OP_SUB,                   /* [] */
  OP_MUL,                   /* [] */
  OP_DIV,                   /* [] */
  OP_ON_TRUE_JMP,           /* [跳转偏移] */
  OP_ON_FALSE_JMP,          /* [跳转偏移] */
  OP_IF_FALSE_JMP,          /* [跳转偏移] */
  OP_JMP,                   /* [跳转偏移] */
  OP_CALL,                  /* [返回值数] [参数个数] */
  OP_RETURN,                /* [函数体活跃的局部变量总数] */
  OP_TAILCALL,              /* [函数体活跃的局部变量总数] [参数个数] */
  OP_SET_ARRAY,             /* [字段数量] */
  OP_SET_HASH,              /* [字段数量] */
  OP_NEW_TABLE,             /* [数组长度] [哈希长度] */
  OP_SET_TABLE,             /* [table/index和value中间间隔的堆栈数] */
  OP_SET_TABLE_AND_POP,     /* [] */
  OP_TABLE_DOT_GET,         /* [常量索引] */
  OP_TABLE_INDEXED_GET,     /* [] */
  OP_FOR_INIT,              /* [跳转偏移] */
  OP_FOR_LOOP,              /* [跳转偏移] */
  END_CODE,

  OP_MAX,
};

enum ObjectType {
  OBJECT_UNKNOWN = 0,
  OBJECT_NIL,
  OBJECT_NUMBER,
  OBJECT_CPROTO,
  OBJECT_STRING,
  OBJECT_PROTO,
  OBJECT_TABLE,
};

struct Token {
  Token() { reset(); }
  void reset() { tt = TK_NONE; str = 0; line = 0; }

  TokenType tt;
  char* str;
  int line;
};

/*通用对象，内部分为值类型和引用类型
warn：谨慎使用 TObject& 和 TObject*，避免stack自动扩容导致引用失效*/
struct TObject {
  TObject() : shortHash(0), shortLen(0), ot(OBJECT_NIL) {
    value.num = 0;
    value.cf = 0;
    value.shortstr[0] = '\0';
    value.longstr = 0;
    value.proto = 0;
    value.table = 0;
  }

  bool equal(const TObject& t) const;

  /*长短字符串*/
  const char* c_str() const;
  int str_len() const;
  HashNum str_hash() const;
  bool is_long_str() const { return 0 == shortLen; }
  void set_short_string(const char* s, int len);
  void set_long_string(ObjectString* s) { ot = OBJECT_STRING; value.longstr = s; }
  
  void as_nil() { ot = OBJECT_NIL; }
  void as_number(Number n) { ot = OBJECT_NUMBER; value.num = n; }
  void as_cfunc(CFunction f) { ot = OBJECT_CPROTO; value.cf = f; }
  void as_proto(ObjectProto* p) { ot = OBJECT_PROTO; value.proto = p; }
  void as_table(ObjectTable* t) { ot = OBJECT_TABLE; value.table = t; }

  union {
    /*值类型*/
    Number num;
    CFunction cf;
    char shortstr[SHORT_STR_SIZE];

    /*引用类型*/
    ObjectString* longstr;
    ObjectProto* proto;
    ObjectTable* table;
  } value;

  HashNum shortHash;
  Byte shortLen;
  Byte ot;
};

struct CFuncInfo {
  const char* name;
  CFunction func;
};

struct CStack {
  CStack() : params(0), base(0), resultBase(0) {}

  int params;
  int base;
  int resultBase;
};

struct LineInfo {
  LineInfo() : line(0), pc(0) {}
  LineInfo(int argLine, int argPc) : line(argLine), pc(argPc) {}

  int line;
  int pc;
};

struct ReservedToken {
  const char* word;
  TokenType tt;
};

struct OpcodeAttr {
  const char* name;
  int args;
};

struct OpPriority {
  int priority;
  OpCode opcode;
};

#endif // commondef_h__
