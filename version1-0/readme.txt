﻿===========================================
V1.5.1版本

（1）实现内置函数tnext、tinsert、tremove、tconcat
（2）实现break、continue、for语句
（3）用hashset管理字符串，优化16字节短字符串，用table管理全局变量
（4）lex词法解析修改为按需解析

===========================================
V1.5版本

（1）实现table数据结构，包括构造table、读写成员、计算数组长度、自动扩容
（2）实现内置函数tostring、print、floor、assert、getn
（3）添加测试文件，实现启动自检

===========================================
V1.4.1版本

（1）重组代码结构，添加gc算法的测试用例
（2）实现单行注释、运行异常捕获、局部变量作用域
（3）编写makefile，支持linux编译

===========================================
V1.4版本

（1）实现自适应的多重赋值
（2）优化虚拟机执行效率，梳理代码逻辑，添加注释
（3）实现字符串拼接，以及gc自动回收
（4）支持以下语法：
    function factor(n)
      if n==1 then return 1 end
      return n*factor(n-1)
    end

    v = factor(4)

===========================================
V1.3版本

（1）实现全局变量的多重赋值
（2）实现局部变量，以及局部变量的多重赋值
（3）实现函数定义、调用、返回值，以及返回值数量的自适应
（4）编写测试用例
（5）支持以下语法：
    function add(a,b) return a+b end
    function sum(a,b,c) return add(c,add(a,b)) end

    d,e=sum(100,add(100,200),add(200,300))
    
    function fun(a,b,c) return a+100,b+100,c+100 end
    a,b=fun(1,2,3)
    c,d=fun(1,2,3),4
    e,f=5,fun(1,2,3)
    
===========================================
V1.2版本

（1）实现比较运算符，包括 ==、~=、>、<、>=、<=。
（2）实现一元运算符，包括 -、not。
（3）实现and、or运算符
（4）实现if条件语句
（5）实现while循环语句
（6）支持如下语法：
    a = 100
    result = 0
    while a > 0 do
      if a*a > 1600 then
        result = result + (a - 1)*2
      end
      a = a - 1
    end
    
===========================================
V1.1版本

（1）实现数学计算表达式，包括加减乘除、()、变量
（2）支持如下语法：
    a1  =100
    a2=  "this is a string"
    a1=(100+a1)*2- a1 *2
    
===========================================
V1.0版本

（1）实现基础版本的虚拟机功能，包括词法分析、语法分析、指令生成、执行指令
（2）支持如下语法：
    a=100
    str = "this is a string"
    a=200
    
===========================================
