#include <stdio.h>
#include <stdlib.h>
#include "common.h"

bool FileReader::open(const char* filename) {
  fp = fopen(filename, "rb");
  return (NULL != fp);
}

void FileReader::close() {
  if (fp) {
    fclose(fp);
  }
}

char FileReader::next() {
  if (NULL == fp) {
    return LEX_EOZ;
  }
  if (curr >= length) {
    curr = 0;
    length = fread(content, 1, sizeof(content), fp);
    if (length <= 0) {
      return LEX_EOZ;
    }
  }
  return content[curr++];
}

//////////////////////////////////////////////////////////////////////////
bool FileWriter::open(const char* filename) {
  fp = fopen(filename, "w+");
  return (NULL != fp);
}

void FileWriter::close() {
  if (fp) {
    fclose(fp);
  }
}

void FileWriter::write(const char* str, int length) {
  if (fp) {
    fwrite(str, length, 1, fp);
  }
}

//////////////////////////////////////////////////////////////////////////
static const unsigned char Log2[256] = { 
  0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};

/*指数，向上取整*/
int BaseFunc::ceil_log2(unsigned int x) {
  x -= 1;

  int n = 0;
  while (x >= 256) { 
    n += 8; 
    x >>= 8; 
  }
  return n + Log2[x];
}

int BaseFunc::square_size(int size, int base) {
  while (base < size) {
    base *= 2;
  }
  return base;
}

HashNum BaseFunc::hash_num(double n) {
  return (HashNum)((n - (int)n) * 1000000 + n);
}

HashNum BaseFunc::hash_string(const char* s, int length) {
  HashNum h = length;
  int step = (length >> 5) + 1;
  for (int i = 0; i < length; i += step) {
    h = h ^ ((h << 5) + (h >> 2) + (unsigned char)s[i]);
  }
  return h;
}

