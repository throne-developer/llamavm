-- 测试代码：表达式

-- （1）数学计算
a1 = -1*2 + 5*(3 - 4*5/(9 - 5*2));
b1 = 3*a1/(a1 - 1*2) + a1*a1/(a1 + 4*a1 - 3);

assert(a1, 113)
assert(floor(b1), 25)

-- （2）and、or
a2 = 1 and 2 and 3
b2 = 1 or 2 or 3
c2 = 1 and 2 or 3

assert(a2, 3)
assert(b2, 1)
assert(c2, 2)
