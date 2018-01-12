/*
** �ڴ����
��1��ObjectString�洢���ַ���
��2��ObjectProto�洢�������󣬰���ָ���б�ͳ����б�
��3��ObjectTable�洢table����
��4����̬�����Ķ�����gc�Զ��ͷţ�ͬһ���͵Ķ���ͨ��GCNode��������
*/
#ifndef lobject_h__
#define lobject_h__

#include "table.h"

struct ObjectString {
  ObjectString() : str(0), length(0), hash(0) {}
  ~ObjectString() { Allocator<char>::del_array(str); }

  bool equal(const ObjectString& other) const {
    return hash == other.hash && length == other.length &&
      strncmp(str, other.str, length) == 0;
  }

  char* str;
  int length;
  HashNum hash;
  GCNode<ObjectString> gc;
};

struct ObjectProto {
  ObjectProto() {
    constants.init(INIT_SIZE_CONSTANT, FUNC_NAME);
    codes.init(INIT_SIZE_CODE, FUNC_NAME);
    lines.init(INIT_SIZE_CODE, FUNC_NAME);
  }

  void add_op(OpCode op, int line) {
    lines.add(LineInfo(line, codes.size()));
    codes.add(op);
  }

  ShortString filename;
  Array<TObject> constants;
  Array<Instruction> codes;
  Array<LineInfo> lines;
  GCNode<ObjectProto> gc;
};

struct ObjectTable {
  Table t;
  GCNode<ObjectTable> gc;
};

#endif // lobject_h__