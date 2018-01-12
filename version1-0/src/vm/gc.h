#ifndef gc_h__
#define gc_h__

struct LlamaState;
struct TObject;
struct ObjectString;
struct ObjectTable;

class gc {
public:
  static void check_gc(LlamaState* ls);
  static void collect_garbage(LlamaState* ls);

private:
  static int block_num(LlamaState* ls);
  static void visit_stack(LlamaState* ls);
  static void visit_global(LlamaState* ls);

  static void mark_object(LlamaState* ls, TObject* obj);
  static void mark_string(LlamaState* ls, ObjectString* str);
  static void mark_table(LlamaState* ls, ObjectTable* tb);

public:
  static int gcThreshold;
};

#endif // gc_h__