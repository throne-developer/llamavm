-- ²âÊÔ´úÂë£ºtable
-- 

a = {}
b = {1,2 ; f=8,['g']=9,[1+2]=3}

a.a, a.b, a['c'] = 10, 20, 30
a1, a2, a3 = a.a, a.b, a.c
d, e, f, g = b[3], b['f'], b.g, b[1]

assert(a1, 10)
assert(a2, 20)
assert(a3, 30)

assert(d, 3)
assert(e, 8)
assert(f, 9)
assert(g, 1)