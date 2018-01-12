-- 测试代码：函数

-- （1）函数调用
function add(a, b) return a+b end

function sub(a, b) return a-b end

function swap(a, b) return b,a end

function sum(a, b, c)
  return add(c, add(a, b))
end

a1 = add(100, 2)
b1 = sub(100, 2)
c1,d1 = swap(300, 400)
e1 = sum(100, 200, 300)

assert(a1, 102)
assert(b1, 98)
assert(c1, 400)
assert(d1, 300)
assert(e1, 600)

-- （2）返回值
function fun(a, b, c) 
  return a+100, b+100, c+100 
end

a2,b2 = fun(1, 2, 3)
c2,d2 = fun(1,2,3), 4
e2,f2 = 5, fun(1,2,3)

assert(a2, 101)
assert(b2, 102)
assert(c2, 101)
assert(d2, 4)
assert(e2, 5)
assert(f2, 101)


