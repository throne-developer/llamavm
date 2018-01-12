-- 测试代码：变量赋值

-- （1）全局变量
a1=100
b1=200
c1='this is a string'

assert(a1, 100)
assert(b1, 200)
assert(c1, 'this is a string')

-- （2）局部变量
local a2=100
local b2=200

assert(a2, 100)
assert(b2, 200)

-- （3）多重赋值
local a3,b3=100,200
c3,d3=a3,b3
e3,f3=300,400
g3=55,66
h3,i3=11

assert(c3, 100)
assert(d3, 200)
assert(e3, 300)
assert(f3, 400)
assert(g3, 55)
assert(h3, 11)
assert(i3, nil)
