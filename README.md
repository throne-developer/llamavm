# llama虚拟机
> **用C++实现的仿lua虚拟机 **

> 关注公众号：“**Lua探索之旅**”


### （1）编译方法
##### Windows系统
1. 安装Visual Studio 2013开发工具
2. 打开llama.vcxproj 工程文件
3. F5调试运行

##### Linux系统
1. 执行命令： bash build.sh
2. 编译成功后，生成 llamavm 可执行程序
3. 执行命令： ./llamavm test.lua
4. 调试信息写入 debug.log 文件

###（2）功能列表
- 虚拟机基础功能：词法分析、语法分析、指令生成、执行指令、垃圾回收
- 表达式求值：加减乘除、比较运算、and/or逻辑运算、字符串拼接
- 控制语句：if、while、for、break、continue
- 变量：全局变量、局部变量、多重赋值
- 函数：函数定义、函数调用、多返回值
- 数据结构：table
- 内置函数：tostring、print、floor等

###（3）源码说明
- src目录存放源代码，testcase目录存放测试用例
- base目录：基础类型定义
- code目录：源码编译和指令生成
- lib目录：内置函数库
- test目录：测试用例
- vm目录：执行指令和gc

