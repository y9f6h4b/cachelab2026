## 项目编译与环境指南

本实验对系统的要求如下：

| System | Support |
| ------ | ------- |
| Ubuntu 24.04 LTS，x86-64（含 WSL2） | ✔️ |

仓库附带的 `csim-ref` 是 Linux x86-64 可执行文件，两个部分的正式本地测试都使用它。请保留这个文件及其可执行权限；不需要、也没有提供 `csim-ref.c` 源码。

安装编译和测试工具（仅首次安装依赖需要联网）：

```bash
sudo apt update
sudo apt install build-essential python3
```


> 请你最好使用Ubuntu 24.04，因为地址随机化的bug导致22.04会出现问题，但我们在Makefile中关闭了地址检查，所以理论上22.04也可以跑。

> **注意：这并不意味着你可以忽视越界访问，野指针等和地址相关的编程错误**

典型场景是用 WSL2 进行开发，如果你不具有这个环境，比如你是 Mac 用户，我们将给大家获取服务器登陆信息，在服务器上完成本实验（无法访问的时候请先考虑是否在校外且未开启校园网 VPN）。

> 助教只维护上表中的环境。使用其他操作系统开发的同学，请在受支持的环境中运行本地测试，再填写报告中的数据。

### 如何编译

```bash
make -j
```

其中，所有 C 文件以 `gcc` 用 `c23` 标准编译，C++ 项目用 `g++` 以 `c++23` 标准编译，报告内不再赘述，请勿修改 `Makefile`，以免测试环境编译失败。

### 本地评测入口

在项目根目录（`README.md` 所在目录）运行：

```bash
python3 test/csim_test.py       # Part A：正确性与百分制分数
python3 test/gemm_test.py       # Part B：正确性、性能与性能分
python3 test/overall_test.py    # 两部分一起评测，并保存 .overall_results
```

脚本会自动编译相应部分。Part A 和 Part B 可以分别测试，互不依赖对方的实现。综合测试依次输出这两份报告：Part A 的正确性分数和 Part B 的性能分数均为百分制。报告、代码风格及 Part B 的报告补分由助教评阅。

输出按 Part A、Part B 分区，使用带边框的结果表和独立的得分框。终端中 `OK` 为绿色、`ERROR` 为红色；表格和得分框在重定向到文件时自动关闭颜色，也可以用 `NO_COLOR=1 python3 test/overall_test.py` 手动关闭颜色。建议终端至少保留 80 列宽度。

## 问题简述

本实验共有两个部分。在第一部分 Part A 中，你需要在 `csim.c` 中实现一个 cache simulator，用来统计内存缓存的命中次数等（不涉及寄存器开销）。第二部分，你需要在 `gemm.cpp` 中实现矩阵乘法算子，并最优化其缓存和寄存器性能（对于寄存器的开销会由我们的判分程序替你统计）。

## Part A：缓存模拟器

我们提供了生成内存访问记录的框架，

比如运行朴素实现的 $2 \times 2 \times 2$ 矩阵乘法：

```
./printTrace case0
```

它会输出 trace 文件，形如：

```
 L 0x30000000,4 8
 L 0x30000010,4 9
...
 L 0x3000001c,4 9
 S 0x3000002c,4 6
```

其中：
- L 和 S 分别代表 load 和 store。
- 后面的数字代表地址（16进制），如 0x3000000b，但是，请兼容 3000000b 这样的输入。注意对于 64 位平台的 `scanf` 来说，使用 `%lx` 读入即可同时正常读入这两种数据。注意地址空间是 64 位的。
- 逗号后的数字代表访问的内存大小（byte记），如 1，2，4，8，我们保证大小小于等于 8。
- 最后一个数字代表的是与这行操作相关的寄存器，这个数字为 -1 时代表立即数，比如 `C[0] = 0`。这个信息其实不影响你 Part A 的实现。

**这一部分你需要做的是补全 `csim.c` 文件**，使其能够正确地模拟一个具有 LRU 替换策略，Write-Back + Write-allocate 的 cache 的行为，计算 cache 的命中次数（hits）、缺失次数（misses）、替换次数（evictions）。

其中 evictions 是指当前 cache miss，且当前的 cache 已满，我们需要做一次 LRU 策略的替换，如果此时 cache 未满，只要存入 cache 即可，就不会导致 evictions 增加。

更具体地来说，程序结束后你需要调用我们提供的 `printSummary` 函数输出这三个值。作为参考，我们提供了 `csim-ref` 程序，在你正确完成本任务后，你的结果应该和它完全一致。cache 的硬件配置通过命令行参数给出，关于怎么在 C 语言中处理命令行参数，你可以询问 AI。

命令行参数规定如下：

```txt
Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>
Options:
  -h         Print this help message.
  -v         Optional verbose flag.
  -s <num>   Number of set index bits.
  -E <num>   Number of lines per set.
  -b <num>   Number of block offset bits.
  -t <file>  Trace file.
```

你可以通过类似的 terminal 语句来给对应的测试点进行测试

```txt
Examples:
  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace
  linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace
```

完成后，我们会将你的 csim 和提供的 csim-ref 的运行结果进行比对，测试文件在 `traces` 文件夹下，你需要保证输出一致。

![](./img/cache.jpg)

### 实现要求

- 用 `C` 或 `C++` 实现，默认是 `gcc`，所以请用 `C` 语法。如果你想用 `C++`，请将文件名改为 `csim.cpp`，并强制编译一次，比如 `make -B csim` 以使得改动立即生效，之后可以正常 `make`
- 你需要以支持任意合理的 s，E，b 参数为目标设计你的代码，并至少支持 traces 文件夹下的各种样例
- 你需要保证你的代码没有任何编译警告

### 测试与分数

我们假设你的系统中安装有 python3（通常如此），那么你只需要

```bash
python3 test/csim_test.py
# 提示：请不要 cd 到 test 目录下执行
```

得到这样的输出代表你的实现是正确的：

```text
━━ Part A - Cache Simulator ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

h/m/e = hits / misses / evictions

┏━━━━━━━━━━━━━┳━━━━━━━┳━━━━━━━━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━━━━━━━━┳━━━━━━━━┓
┃ Trace       ┃ s,E,b ┃   Reference (h/m/e) ┃       Yours (h/m/e) ┃ Status ┃
┡━━━━━━━━━━━━━╇━━━━━━━╇━━━━━━━━━━━━━━━━━━━━━╇━━━━━━━━━━━━━━━━━━━━━╇━━━━━━━━┩
│ yi2.trace   │ 5,1,5 │              15/1/0 │              15/1/0 │   OK   │
│ yi.trace    │ 5,1,5 │               3/4/0 │               3/4/0 │   OK   │
│ dave.trace  │ 5,1,5 │               2/3/0 │               2/3/0 │   OK   │
│ trans.trace │ 5,1,5 │             211/7/0 │             211/7/0 │   OK   │
│ long.trace  │ 5,1,5 │  246213/21775/21743 │  246213/21775/21743 │   OK   │
│ yi2.trace   │ 2,4,3 │              14/2/0 │              14/2/0 │   OK   │
│ yi.trace    │ 2,4,3 │               2/5/0 │               2/5/0 │   OK   │
│ dave.trace  │ 2,4,3 │               0/5/0 │               0/5/0 │   OK   │
│ trans.trace │ 2,4,3 │           192/26/10 │           192/26/10 │   OK   │
│ long.trace  │ 2,4,3 │  243398/24590/24574 │  243398/24590/24574 │   OK   │
│ yi2.trace   │ 4,2,4 │              15/1/0 │              15/1/0 │   OK   │
│ yi.trace    │ 4,2,4 │               2/5/2 │               2/5/2 │   OK   │
│ dave.trace  │ 4,2,4 │               2/3/0 │               2/3/0 │   OK   │
│ trans.trace │ 4,2,4 │            206/12/0 │            206/12/0 │   OK   │
│ long.trace  │ 4,2,4 │  247163/20825/20793 │  247163/20825/20793 │   OK   │
│ yi2.trace   │ 1,1,1 │               8/8/6 │               8/8/6 │   OK   │
│ yi.trace    │ 1,1,1 │               0/7/5 │               0/7/5 │   OK   │
│ dave.trace  │ 1,1,1 │               0/5/4 │               0/5/4 │   OK   │
│ trans.trace │ 1,1,1 │          25/193/192 │          25/193/192 │   OK   │
│ long.trace  │ 1,1,1 │ 35393/232595/232594 │ 35393/232595/232594 │   OK   │
└─────────────┴───────┴─────────────────────┴─────────────────────┴────────┘

╭──────────────────────────────────────────────────────────────────────────────╮
│ Score: 100.00 / 100    Passed cases: 20 / 20                                 │
╰──────────────────────────────────────────────────────────────────────────────╯
```

我们的分数为 $100 \times 正确比例$

## Part B：矩阵乘法优化

### 故事背景

本次题目基于 RISC-V 架构的 CPU，以下是一些背景知识。

当今处理器有三大主流架构：X86，ARM 和 RISC-V，由于使用 X86 或者 ARM 架构时都需要获取授权，并缴纳最高上百万美元的授权费，且投产后每枚芯片都需要单独再收取费用，所以 RISC-V 架构逐渐成为了学术界和开源界的首选，中国是 RISC-V 架构的重要贡献者之一。

我们比较熟悉的 X86 架构是 CISC（Complex Instruction Set Computer）架构，而 ARM 和 RISC-V 是 RISC（Reduced Instruction Set Computer）架构。这两种架构的区别在于指令集的复杂度，CISC 架构的指令集更加复杂，一个指令可以完成多个操作，而 RISC 架构的指令集更加简单，一个指令只完成一个操作。这样的区别导致了 RISC 架构的 CPU 更加容易设计，同时功耗也更低，过去一般认为 CISC 架构的 CPU 性能更好，但随着技术的发展，这个结论也逐渐不绝对，比如 ARM 架构的 CPU 往往可以在相同功耗下堆叠更多的核心，从而可能获得更好的性能。

CISC 和 RISC 架构一个显著的具体区别是在 CISC（如 X86）中，操作数可以直接从内存送进算术逻辑单元（ALU）进行计算，而在 RISC（如 ARM 和 RISC-V）中，操作数必须先送入寄存器，然后再送入 ALU 进行计算。

![](./img/risc-cisc.png)

实质上，在 CISC 架构中访问内存相比于直接访问寄存器也不是零成本的，这个开销被隐藏在 CPU 内部从而不方便我们观察与优化，但是在 RISC 架构中，这个开销是可以在汇编指令层面被观察到。

以这段代码为例：

```c
void test(int mem[]) {
    mem[2] += mem[0] * mem[1];
}
```

x86-64（64 位）的[汇编代码](https://godbolt.org/z/MTEfM89jM)为：

```asm
_Z4testPi:
        movl    (%rdi), %eax
        imull   4(%rdi), %eax
        addl    %eax, 8(%rdi)
        ret
```

RISC-V (32-bits) 的[汇编代码](https://godbolt.org/z/s1boK5bE8)为：

```asm
_Z4testPi:
        lw      a4,0(a0)
        lw      a3,4(a0)
        lw      a5,8(a0)
        mul     a4,a4,a3
        add     a5,a5,a4
        sw      a5,8(a0)
        ret
```

可以发现 RISC-V 的汇编显式地将内存读取到寄存器，然后再进行计算。同时 32 位 RISC-V 是用寄存器传参的，类似 64 位 X86 的行为。

我们的题目就是基于类似 RISC-V 架构的 CPU。

具体来说，我们假设的 CPU 在工作时不能直接用内存中的数据计算，内存中的值会先加载到 cache 里，再从 cache 加载到寄存器 register 里，CPU 用寄存器完成计算后把计算结果从寄存器写回 cache，再写回内存。

为了提升这个过程的性能，我们有至少两种优化思路：

- 减少内存读写：即通过访问模式设计，减少 cache miss，尽量复用 cache
- 减少 cache 读写：即通过访问模式设计，减少去读写 cache，尽量复用寄存器

我们的题目只考虑以上内存访问过程中的性能，不考虑计算的性能，比如加法和乘法指令我们认为是零成本的。

### cache 性能优化

其中“减少内存读写”就是课上提到或将提到的内容，我们会在这里简要回顾一下，假如你在以列优先的方式访问一个二维数组 `B`：

```c
int B[256][256];
for (int j = 0; j < 256; j++) {
    for (int i = 0; i < 256; i++) {
        do_something(B[i][j]);
    }
}
```

直接实现，会导致 B 的内存访问不连续。假设 cache 的行大小是 32 字节，即 8 个 int，我们可以对访问进行分块，即每次处理一个 $8 \times 8$ 的子矩阵，这样每个子矩阵在访问的过程中都充分利用了已经读取的 cache 行，减少了 cache miss。

```c
for (int j = 0; j < 256; j += 8) {
    for (int i = 0; i < 256; i += 8) {
        for (int jj = j; jj < j + 8; jj++) {
            for (int ii = i; ii < i + 8; ii++) {
                do_something(B[ii][jj]);
            }
        }
    }
}
```

### 寄存器性能优化

本实验显式地加入了对寄存器性能的考量。我们以下一个迷你矩阵乘法为例，介绍寄存器性能的优化的含义。

考虑两个 $2 \times 2$ 矩阵相乘的场景，假设此时所有数据已经 cache 里，我们只关注寄存器层面的问题。

**Solution 1**

```c
for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
        for (int k = 0; k < 2; k++) {
            C[i][j] += A[i][k] * B[k][j];
        }
    }
}
```

这是一个典型的矩阵乘法代码，如果完全不加优化地直接执行，CPU 的执行过程类似这样：

``` c
load A[0][0] to reg1
load B[0][0] to reg2
    calculate reg3 = reg1 * reg2
load C[0][0] to reg4
    calculate reg4 = reg4 + reg3
store reg4 to C[0][0]
...
```

一共需要执行 8 次这样的操作，意味着 24 次 load 和 8 次 store。

**Solution 2**

为了优化，我们显式地声明寄存器来规定代码的行为。为了方便，我们假设 reg 就是一个寄存器类型。

```c
for (reg i = 0; i < 2; ++i) {
    for (reg j = 0; j < 2; ++j) {
        reg tmpc = 0;
        for (reg k = 0; k < 2; ++k) {
            reg tmpa = A[i][k];
            reg tmpb = B[k][j];
            tmpc += tmpa * tmpb;
        }
        C[i][j] = tmpc;
    }
}
```

此时经过优化后的执行过程类似这样：

```c
    calculate tmpc = 0
load A[0][0] to tmpa
load B[0][0] to tmpb
    calculate tmpa = tmpa * tmpb
    calculate tmpc = tmpc + tmpa
store tmpc to C[0][0]
...
```

每个输出元素完成 2 次乘加后才写回；4 个输出元素合计需要 16 次 load 和 4 次 store。累加值保存在 `tmpc` 中，初值直接设为 0，因此不需要读取原来的 `C[i][j]`，也不需要在每次乘加后立刻写回。

**Solution 3**

我们还可以优化，注意到数据实际上只有 8 个 int 要读入，我们却使用了 16 次 load，一个暴力的优化方法是，全部加载到寄存器，计算后再导出：

```c
reg tmpa[2][2];
reg tmpb[2][2];
reg tmpc[2][2];
for (reg i = 0; i < 2; ++i) {
    for (reg j = 0; j < 2; ++j) {
        tmpc[i][j] = 0;
        tmpa[i][j] = A[i][j];
        tmpb[i][j] = B[i][j];
    }
}
for (reg i = 0; i < 2; ++i) {
    for (reg j = 0; j < 2; ++j) {
        for (reg k = 0; k < 2; ++k) {
            tmpc[i][j] += tmpa[i][k] * tmpb[k][j];
        }
    }
}
for (reg i = 0; i < 2; ++i) {
    for (reg j = 0; j < 2; ++j) {
        C[i][j] = tmpc[i][j];
    }
}
```

这样我们达到了理论上的最优性能，只需要 8 次 load 和 4 次 store。代价是我们需要大量的寄存器，而 CPU 上的寄存器是有限的，我们会在问题描述中详细规定硬件配置。

### 框架说明

> [`demo.cpp`](./demo.cpp) 是本框架的详细文档，请在读完本文档后详细阅读并了解更多用法。

以上我们简要介绍了寄存器性能的含义以及 RISC-V 架构下你不能直接操作内存这个限制。

但是要控制寄存器的行为，通常需要在汇编代码层面操作，而我们并不想你们手搓汇编，所以，我们我们提供了一个框架，你只需要模仿在写汇编，我们会模拟统计寄存器和内存的访问/使用情况。

在这个框架下，你可以当作你在写一种特别的 `C/C++` 代码，但有两个特殊之处：

第一，只有两种类型，`ptr_reg` 和 `reg` 分别和 `int*` 和 `int` 对应，比如原来长这样的代码：

```c
void example(int* mem){
    int a = mem[0];
    mem[0] = a * 2;
}
```

现在对应的代码是这样的：

```c
void example_now(ptr_reg mem){
    reg a = mem[0];
    mem[0] = a * 2;
}
```

第二，内存不能直接操作，必须经过寄存器，比如原来长这样的代码：

```c
void example(int* mem){
    mem[0] = mem[0] * 2;
}
```

直接翻译过来会报错，你需要先存到寄存器里：

```c
void example_now(ptr_reg mem){
    // mem[0] = mem[0] * 2; // 这会报错
    reg a = mem[0];
    mem[0] = a * 2;
}
```

以上代码如果翻译成 RISC-V 汇编，大概是这样的，其中他会把乘2优化成左移一位，不过我们只考虑内存访问，即 `lw` 和 `sw` 相关的性能，所以这并不重要：

``` nasm
    lw      a5,0(a0)
    slli    a5,a5,1
    sw      a5,0(a0)
    ret
```

或者换句话来说，你正在写面向对象的代码，对象就是内存和寄存器。

在本实验中，`demo.cpp`文件仅用于为你介绍寄存器的使用方式，我们没有在Makefile中提供编译接口。

再次强调，**具体哪些操作是允许的请见 [`demo.cpp`](./demo.cpp)，本框架规定了可使用操作的上限**，但可能有未发现的应该禁止的行为，请各位在使用时遵从“内存和内存不能直接操作，必须经过寄存器的原则自行判断你是否绕过了框架限制。

### 题目描述

以上你应该已经理解了寄存器性能和 Cache 性能，以及我们提供的框架的含义，现在你需要基于我们提供的框架，**将这些优化实现进 [`gemm.cpp`](./gemm.cpp) 里**，目前 [`gemm.cpp`](./gemm.cpp) 只提供了最朴素的算法用作演示。

我们假设的硬件环境是：

- cache：LRU 替换策略，s = 5，E = 1，b = 4
- CPU：一共 36 个寄存器，我们的框架会在你使用超过 36 个寄存器时抛出运行时错误
- 读写一次内存需要 15 个单位时间
- 读写一次 cache 需要 1 个单位时间

你们需要优化的有 case1，case2，case3 三种情况，分别对应

- case1: $16 \times 16 \times 16$ 矩阵乘法
- case2: $32 \times 32 \times 32$ 矩阵乘法
- case3: $27 \times 41 \times 27$ 矩阵乘法

本题 $m \times n \times p$ 表示 $A, B, C$ 矩阵的形状分别为 $m \times n, n \times p, m \times p$，他们满足 $C = AB$

你可以运行 `./printTrace case1` 来查看你的算法执行 case1 的矩阵乘法时对应的 trace 文件。

运行 `make case1` 会帮你把这份 trace 文件保存到 `gemm_traces/case1.trace` 下，并调用 `csim-ref` 来计算对应的 cache 行为。

不难看出，其中 misses 就代表了你的`gemm.cpp`算法读写内存的次数，记作 $miss_{cache}$。同时，trace 文件的行数就代表了你读写 cache 的次数，记作 $miss_{reg}$。

我们的实验要求你最优化内存和 cache 的访问时间，即最优化 $latency = 15 \times miss_{cache} + miss_{reg}$。

### 实现注意事项

请严格按照以下要求进行实验（**请专注于对 cache 本身的优化**）：

- 编译器为 `g++`，允许用 `g++` 编译器支持的 `C/C++` 语法。注意，因框架原因只能用 `g++` 编译
- 你需要保证你的代码没有任何编译警告
- 你只能修改 `gemm.cpp` 文件
- 不能使用除了 `ptr_reg` 和 `reg` 外的任何数据类型
- 不能申请任何内存，如果你需要内存来暂存东西，而不是寄存器，你可以使用 `buffer` 字段
- 禁止利用任何二进制技巧在一个寄存器/内存上，存储多个数值
- 禁止使用复杂度低于 $O(n^3)$ 的矩阵乘法算法，比如 Strassen 算法
- 严格按照框架的假设来设计代码，不允许绕过框架或者改变框架，即不允许利用漏洞，注意，这意味着请再次仔细阅读 [`demo.cpp`](./demo.cpp)，彻底理解这个框架
- 你的代码需要在规定环境中通过仓库提供的本地测试，助教会使用未修改的评测框架复测提交的代码
- 尽量使用合适格式化规则来格式化你的代码

### 测试与分数

运行以下命令即可完成本地评测并获取详细性能报告，无需任何密钥：

``` bash
python3 test/gemm_test.py
# 提示：请不要 cd 到 test 目录下执行
# 本部分使用随仓库提供的 csim-ref，不要求先完成 Part A

# 可选：只查看 baseline 的数据
python3 test/gemm_test.py --baseline
```

脚本会检查代码的基本合法性、矩阵乘法结果、输入矩阵是否保持不变和寄存器使用限制，再使用 `csim-ref` 统计缓存行为。自动合法性检查不能覆盖所有实现要求，助教仍会检查源代码。报告中的数据应来自最终提交的代码，助教将以提交文件的复测结果为准。

每个 case 会显示 `OK` 或 `ERROR`。运行出错（如结果错误、寄存器超限或单个 case 超过 30 秒未结束）时，该 case 的加速比记为 0，其余 case 继续评测。编译或基本合法性检查失败时，整个文件无法生成可执行程序；参考程序缺失或 baseline 运行失败时，无法得到可信的评分基准。这些情况会报错并以非零退出码结束，请先修复后重测。

注意：脚本退出码为 0 表示评测流程完成，**不代表所有测试通过或已获满分**，请检查表格中的状态和分数。模板中的 `gemm.cpp` 与 baseline 相同，初次运行时各 case 加速比为 1、性能分为 0，这是正常结果。

性能报告会指出你实现的 gemm 算法的 $miss_{cache}$、 $miss_{reg}$ 和 $latency$，以及这个 $latency$ 和最初 baseline 实现的版本相比的加速比 $speedup$，最终这三份 $speedup$ 会以加权的形式得到一个**综合加速比**，其中不同 case 的加速比对应的加权权重为

| case  | 权重 |
| ----- | ---- |
| case0 | 0%   |
| case1 | 30%  |
| case2 | 30%  |
| case3 | 40%  |

> 本地报告会显示 case0 的结果，但它不计入性能得分。

**Part B 的性能分按综合加速比计算**，不再参考排行榜名次。规则参考 CMU / PKU 的 CacheLab：设一条满分线，成绩随综合加速比在零分线和满分线之间线性变化。设你的综合加速比为 $W = 0.3 x_1 + 0.3 x_2 + 0.4 x_3$（$x_i$ 是 case$i$ 的加速比），则性能得分率为

$$s = \min\left(1,\ \max\left(0,\ \frac{W - 1}{W_{\mathrm{full}} - 1}\right)\right),\quad W_{\mathrm{full}} = 8$$

也就是说：综合加速比为 1（与 baseline 持平或更差）得 0 分，达到 **8** 得满分，两者之间按综合加速比线性给分，超过 8 不再加分。例如综合加速比为 4.5，得分率为 50%。Part B 的性能分 $= 40 \times s$。运行 `python3 test/gemm_test.py` 时，性能报告会给出综合加速比和百分制性能得分（`Part B performance score`，满分 100）。

因为是按综合加速比计算，各个 case 之间可以互相弥补：某个 case 优化得很好、另一个 case 不够理想，仍然可以拿到满分。如果某个 case 的结果不正确，或者违反了上面的实现要求，该 case 的加速比按 0 计。

### 实验提示

我们给出一些实验提示。~~虽然并不需要完全用到，你也不一定完全理解~~

- 优化难度视你采用的方法，不一定是从 case1 到 case3 递增的。
- 分块是一个缓解 cache 冲突的好办法：[waside-blocking.pdf](https://csapp.cs.cmu.edu/public/waside/waside-blocking.pdf)，不过，我们有两级 cache，**仔细思考该如何分块**，只需要分一次吗。
- 可以考虑迭代次序的影响，即 `i、j、k` 或者别的次序，注意因为 cache 的规格不同，结论和课上（实际CPU）的情况可能不同。
- CMU cachelab 中优化的算法是矩阵转置，我们的是矩阵乘法，但是除了分块，可能还有其他相同的优化策略可以用。
- [寄存器性能优化](#寄存器性能优化) 章节中我们已经提示了寄存器的优化方法。
- 寄存器可能不够用，这时候你可以暂存到 buffer 字段里，你可以评估一下这是否值得。
- 注意到 A，B，C，buffer 的排列是紧密的，所以 buffer 的另外一个用途是调整矩阵的 offset，当我们的访问量是 $O(n^3)$ 时，用 $O(n^2)$ 来搬迁数据可能是值得的。
- 虽然不同测试点的权重不同，但他们优化后能取得的加速比也不同，全力优化权重高的测试点不一定能在最后加权时取得最好的成绩。
- 你可以看看本文档的[后记](#真实的故事背景与优化提示)，它展示了在具有多级 cache 的硬件下如何减少内存访问次数（主要看 “从global memory到shared memory” 和 “从shared memory到register” 两节）。
- 注意优化 miss_cache 和 miss_reg 的平衡，例如为了减少 miss_reg 而增加一些 miss_cache 可能是值得的。
- 在实际的工业场合，对于矩阵乘法优化问题，相较于理论分析，我们常常是穷举超参数，比如分块的大小来确定最优的参数，所以当你发现问题过于复杂时不妨尝试穷举。换言之，你可以思考不同的“大致方案”，然后跑一下看看结果。而不必强求分析出某个具体方案再实施。
- 你可以使用 [`parabuild-rust`](https://github.com/panjd123/parabuild-rust) 项目来方便地批量修改超参数并编译执行，如果你使用了这个项目，请给这个项目一个 star 并在报告中提及（你用了这个项目），使用说明见该项目的文档。

## 如何调试

### PartA

- 使用std::cout和std::cerr都是可以的。
- 使用./csim -s <num> -E <num> -b <num> -t <file> 就能看到你想要的输出。

### PartB
- 请使用std::cerr，不要使用std::cout！
- 对于每一个case，请直接使用make case{i}编译。例如你想测试case0（2*2*2），make case0就能看到输出了。

> 注意：test中的python脚本只是用来最后打分，方便大家看整体正确性和评分的，**请不要用他们进行debug。