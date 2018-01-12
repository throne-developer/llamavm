#include "table.h"
#include "base/lobject.h"
#include "base/common.h"

int Table::nkey(const TObject& key) {
  return (OBJECT_NUMBER == key.ot && IS_INTEGER(key)) ? (int)NUM_VAL(key) : -1;
}

Table::Node* Table::search_node(const TObject& key) {
  if (OBJECT_NIL != key.ot) {
    Node* pos = main_position(key);
    while (pos) {  /*������ͻ��*/
      if (pos->key.equal(key)) {
        return pos;
      }
      pos = pos->next;
    }
  }
  return NULL;
}

void Table::set(const TObject& key, const TObject& value) {
  if (IS_NIL(key))
    return;

  /*exist key*/
  int n = nkey(key);
  if (n >= 1 && n <= arraySize) {
    arrayData[n - 1] = value;
    return;
  }
  Node* node = search_node(key);
  if (node) {
    node->value = value;
    return;
  }

  /*new key*/
  Node* mainPos = main_position(key);
  if (mainPos->used()) {
    Node* freePos = get_last_free_pos();
    if (NULL == freePos) {
      rehash(key);  /*����*/
      return set(key, value);
    }
    mainPos = adjust_pos(mainPos, freePos, main_position(mainPos->key));
  }
  else {
    mainPos->next = NULL;
  }
  mainPos->key = key;
  mainPos->value = value;
}

TObject Table::get(const TObject& key) {
  int n = nkey(key);
  if (n >= 1 && n <= arraySize) {
    return arrayData[n - 1];
  }

  Node* node = search_node(key);
  return node ? node->value : TObject();
}

void Table::set_num(int n, const TObject& value) {
  TObject key;
  key.as_number(n);
  set(key, value);
}

TObject Table::get_num(int n) {
  TObject key;
  key.as_number(n);
  return get(key);
}

Table::Node* Table::main_position(const TObject& key) {
  HashNum hash = 0;
  switch (key.ot) {
  case OBJECT_NUMBER:
    hash = BaseFunc::hash_num(NUM_VAL(key));
    break;
  case OBJECT_CPROTO:
    hash = (HashNum)(long)key.value.cf;
    break;
  case OBJECT_STRING:
    hash = key.str_hash();
    break;
  case OBJECT_TABLE:
    hash = (HashNum)(long)key.value.table;
    break;
  case OBJECT_PROTO:
    hash = (HashNum)(long)key.value.proto;
    break;
  }
  return &hashData[MOD_VAL(hash, hashSize)];
}

Table::Node* Table::get_last_free_pos() {
  while (last_free) {
    if (!hashData[last_free].used()) {
      return &hashData[last_free];
    }
    last_free--;
  }
  return NULL;
}

/* �����ͻ���
mainPos���½�����λ��
freePos�����н��λ��
actualPos��mainPosλ�������н���ʵ����λ�� */
Table::Node* Table::adjust_pos(Node* mainPos, Node* freePos, Node* actualPos) {
  /*���н������ȷλ���ϣ��½��ͷ�������ͻ��*/
  if (actualPos == mainPos) {  
    freePos->next = mainPos->next;
    mainPos->next = freePos;
    return freePos;
  }

  /*���н���ڴ���λ���ϣ���ȡ��ʵ����λ�õ�ǰ�ý��*/
  while (actualPos->next != mainPos) {
    actualPos = actualPos->next;
  }
  *freePos = *mainPos;  /*���н�����freepos*/
  actualPos->next = freePos;  /*ǰ�ý��ָ��freepos*/

  /*�½�����mainpos*/
  mainPos->next = NULL;
  return mainPos;
}

void Table::rehash(const TObject& key) {
  int sections[INT_BITS + 1] = { 0 };
  int nkeys = 0, keys = 1;

  /*��ȡ��������key����ͳ�Ƹ������������*/
  array_stat(sections, nkeys, keys);
  hash_stat(sections, nkeys, keys);
  nkeys += (count_int_key(key, sections) ? 1 : 0);

  nkeys = compute_array_size(sections, nkeys);
  resize(nkeys, keys - nkeys);
}

void Table::array_stat(int sections[], int& nkeys, int& keys) {
  int currPos = 1;

  /*( 2^(i-1), 2^i ] Ϊһ������*/
  for (int i = 0, square_i = 1;
    i <= INT_BITS && currPos <= arraySize;
    i++, square_i *= 2) 
  {
    int maxn = MIN_VAL(square_i, arraySize);
    int count = 0;
    for (; currPos <= maxn; currPos++) {
      count += IS_NIL(arrayData[currPos - 1]) ? 0 : 1;
    }
    sections[i] += count;
    nkeys += count;
  }
  keys += nkeys;
}

bool Table::count_int_key(const TObject& key, int sections[]) {
  int n = nkey(key);
  if (-1 != n) {
    sections[BaseFunc::ceil_log2(n)]++;
    return true;
  }
  return false;
}

void Table::hash_stat(int sections[], int& nkeys, int& keys) {
  for (int i = 0; i < hashSize; i++) {
    if (hashData[i].used()) {
      nkeys += (count_int_key(hashData[i].key, sections) ? 1 : 0);
      keys++;
    }
  }
}

int Table::compute_array_size(int sections[], int nkeys) {
  int count = 0, arrays = 0;
  for (int i = 0, square_i = 1; i <= INT_BITS; i++, square_i *= 2) {
    if (sections[i] > 0) {
      count += sections[i];
      if (count > square_i / 2) {  /*����ʹ���ʳ���50%*/
        arrays = count;
      }
    }
    if (count == nkeys)
      break;
  }
  return arrays;
}

void Table::resize(int arrays, int hashs) {
  TObject* oldArray = arrayData;
  int oldArraySize = arraySize;
  Node* oldHash = hashData;
  int oldHashSize = hashSize;

  arraySize = BaseFunc::square_size(arrays, INIT_SIZE_TABLE_ARRAY);
  hashSize = BaseFunc::square_size(hashs, INIT_SIZE_TABLE_HASH);
  hashData = Allocator<Node>::new_array(hashSize, FUNC_NAME, FUNC_NAME);
  last_free = hashSize - 1;

  /*�ط�������*/
  if (arraySize != oldArraySize) {
    arrayData = Allocator<TObject>::new_array(arraySize, FUNC_NAME, FUNC_NAME);
    memcpy(arrayData, oldArray, MIN_VAL(arraySize, oldArraySize)*sizeof(TObject));

    /*�������飬���������key����hash*/
    for (int i = arraySize; i < oldArraySize; i++) {
      if (NOT_NIL(oldArray[i])) {
        set_num(i, oldArray[i]);
      }
    }
    Allocator<TObject>::del_array(oldArray);
  }

  /*hash����Ǩ��*/
  for (int i = oldHashSize - 1; i >= 0; i--) {
    if (oldHash[i].used()) {
      set(oldHash[i].key, oldHash[i].value);
    }
  }
  Allocator<Node>::del_array(oldHash);
}

/*��ȡ���鲿�ֳ���
��1��ֻ����������key�������޿ն������
��2������key�п�����hash���� */
int Table::getn() {
  /*�������һ��Ԫ��Ϊnil�����ֲ�������*/
  if (arraySize > 0 && IS_NIL(arrayData[arraySize - 1])) {
    int i = 0, j = arraySize - 1;
    while (i <= j) {
      int m = (i + j) / 2;
      if (NOT_NIL(arrayData[m])) {
        if (IS_NIL(arrayData[m + 1])) { return m + 1; }
        else { i = m + 1; }
      }
      else { j = m; }
    }
    return 0;
  }
  if (0 == hashSize) { return arraySize; }

  /*�������һ��Ԫ�ز�Ϊnil����������hash���ֵ�����key��
  �� arraySize��ʼ�������������key�Ƿ���� */
  int i = arraySize - 1, j = arraySize;
  while (NOT_NIL(get_num(j))) {
    i = j;
    j *= 2;
  }

  /*���ֲ������һ������key*/
  while (i <= j) {
    int mpos = (i + j) / 2;
    if (NOT_NIL(get_num(mpos))) {
      if (IS_NIL(get_num(mpos + 1))) { return mpos; }
      else { i = mpos + 1; }
    }
    else { j = mpos; }
  }
  return 0;
}

/* ����λ��
��1������� 1 �� arraySize
��2����ϣ�� arraySize+1 �� arraySize+hashSize */
int Table::get_pos(const TObject& key) {
  if (IS_NIL(key)) {
    return 0;
  }
  int n = nkey(key);
  if (n >= 1 && n <= arraySize) {
    return n;
  }
  Node* node = search_node(key);
  if (node) {
    return (node - hashData) + arraySize + 1;
  }
  return -1;
}

TObject Table::next_key(const TObject& key) {
  TObject nextKey;
  int pos = get_pos(key);
  if (-1 == pos) {
    return nextKey;
  }

  /*��һ������key*/
  pos++;
  for (; pos <= arraySize; pos++) {
    if (NOT_NIL(arrayData[pos - 1])) {
      nextKey.as_number(pos);
      return nextKey;
    }
  }

  /*��һ����ϣkey*/
  pos -= arraySize;
  for (; pos <= hashSize; pos++) {
    if (hashData[pos - 1].used()) {
      return hashData[pos - 1].key;
    }
  }
  return nextKey;
}
