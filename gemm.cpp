/*
请注意，你的代码不能出现任何 int/short/char/float/double/auto 等局部变量/函数传参，我们仅允许使用 reg 定义的寄存器变量。
其中 reg 等价于一个 int。

你不能自己申请额外的内存，即不能使用 new/malloc，作为补偿我们传入了一段 buffer，大小为 BUFFER_SIZE = 64，你可以视情况使用。

我们的数组按照 A, B, C, buffer 的顺序在内存上连续紧密排列，且 &A = 0x30000000（这是模拟的设定，不是 A 的真实地址）

如果你需要以更自由的方式访问内存，你可以以相对 A 的方式访问，比如 A[100]，用 *(0x30000000) 是无法访问到的。

如果你有定义常量的需求（更严谨的说法是，你想定义的是汇编层面的立即数，不应该占用寄存器），请参考下面这种方式使用宏定义来完成。
*/

#include "cachelab.h"

#define m case0_m
#define n case0_n
#define p case0_p

// 我们用这个 2*2*2 的矩阵乘法来演示寄存器是怎么被分配的
void gemm_case0(ptr_reg A, ptr_reg B, ptr_reg C, [[maybe_unused]] ptr_reg buffer)
{ // allocate 0 1 2 3
  for (reg i = 0; i < m; ++i)
  { // allocate 4
    for (reg j = 0; j < p; ++j)
    {               // allocate 5
      reg tmpc = 0; // allocate 6
      for (reg k = 0; k < n; ++k)
      {                          // allocate 7
        reg tmpa = A[i * n + k]; // allocate 8
        reg tmpb = B[k * p + j]; // allocate 9
        tmpc += tmpa * tmpb;
      } // free 9 8
      // free 7
      C[i * p + j] = tmpc;
    } // free 6
    // free 5
  }
  // free 4
} // free 3 2 1 0

#undef m
#undef n
#undef p

#define CASE1_TILE_ROWS 6
#define CASE1_TILE_COLS 4
#define CASE1_ROW_TILES ((case1_m + CASE1_TILE_ROWS - 1) / CASE1_TILE_ROWS) //floor((a+b-1)/b) is equiv to ceil(a/b)
#define CASE1_COL_TILES (case1_p / CASE1_TILE_COLS)
#define CASE1_ROW_START (tile / CASE1_COL_TILES * CASE1_TILE_ROWS) //verbatim substitution
#define CASE1_COL_START (tile % CASE1_COL_TILES * CASE1_TILE_COLS)

void gemm_case1(ptr_reg A, ptr_reg B, ptr_reg C, [[maybe_unused]] ptr_reg buffer)
{
  for (reg tile = 0; tile < CASE1_ROW_TILES * CASE1_COL_TILES; ++tile)
  {
    reg sums[CASE1_TILE_ROWS][CASE1_TILE_COLS];
    for (reg depth = 0; depth < case1_n; ++depth)
    {
      reg right[CASE1_TILE_COLS];
      for (reg col = 0; col < CASE1_TILE_COLS; ++col)
        right[col] = B[depth * case1_p + CASE1_COL_START + col];

      for (reg row = 0; row < CASE1_TILE_ROWS && CASE1_ROW_START + row < case1_m; ++row)
      {
        reg left = A[(CASE1_ROW_START + row) * case1_n + depth];
        sums[row][0] += left * right[0];
        sums[row][1] += left * right[1];
        sums[row][2] += left * right[2];
        sums[row][3] += left * right[3];
      }
    }

    for (reg row = 0; row < CASE1_TILE_ROWS && CASE1_ROW_START + row < case1_m; ++row)
      for (reg col = 0; col < CASE1_TILE_COLS; ++col)
        C[(CASE1_ROW_START + row) * case1_p + CASE1_COL_START + col] = sums[row][col];
  }
}

#undef CASE1_TILE_ROWS
#undef CASE1_TILE_COLS
#undef CASE1_ROW_TILES
#undef CASE1_COL_TILES
#undef CASE1_ROW_START
#undef CASE1_COL_START

#define CASE2_TILE_SIZE 4

void gemm_case2(ptr_reg A, ptr_reg B, ptr_reg C, [[maybe_unused]] ptr_reg buffer)
{
  for (reg row = 0; row < case2_m; row += CASE2_TILE_SIZE)
  {
    for (reg col = 0; col < case2_p; col += CASE2_TILE_SIZE)
    {
      reg sums[CASE2_TILE_SIZE][CASE2_TILE_SIZE];
      for (reg depth = 0; depth < case2_n; ++depth)
      {
        reg left[CASE2_TILE_SIZE];
        for (reg local_row = 0; local_row < CASE2_TILE_SIZE; ++local_row)
          left[local_row] = A[(row + local_row) * case2_n + depth];

        for (reg local_col = 0; local_col < CASE2_TILE_SIZE; ++local_col)
        {
          reg right = B[depth * case2_p + col + local_col];
          for (reg local_row = 0; local_row < CASE2_TILE_SIZE; ++local_row)
            sums[local_row][local_col] += left[local_row] * right;
        }
      }

      for (reg local_row = 0; local_row < CASE2_TILE_SIZE; ++local_row)
        for (reg local_col = 0; local_col < CASE2_TILE_SIZE; ++local_col)
          C[(row + local_row) * case2_p + col + local_col] = sums[local_row][local_col];
    }
  }
}

#undef CASE2_TILE_SIZE

#define CASE3_TILE_ROWS 4
#define CASE3_TILE_COLS 6
#define CASE3_ROW_TILES ((case3_m + CASE3_TILE_ROWS - 1) / CASE3_TILE_ROWS)
#define CASE3_COL_TILES ((case3_p + CASE3_TILE_COLS - 1) / CASE3_TILE_COLS)
#define CASE3_ROW_START (tile / CASE3_COL_TILES * CASE3_TILE_ROWS)
#define CASE3_COL_START (tile % CASE3_COL_TILES * CASE3_TILE_COLS)

void gemm_case3(ptr_reg A, ptr_reg B, ptr_reg C, [[maybe_unused]] ptr_reg buffer)
{
  for (reg tile = 0; tile < CASE3_ROW_TILES * CASE3_COL_TILES; ++tile)
  {
    reg sums[CASE3_TILE_ROWS][CASE3_TILE_COLS];
    for (reg depth = 0; depth < case3_n; ++depth)
    {
      reg left[CASE3_TILE_ROWS];
      for (reg row = 0; row < CASE3_TILE_ROWS && CASE3_ROW_START + row < case3_m; ++row)
        left[row] = A[(CASE3_ROW_START + row) * case3_n + depth];

      for (reg col = 0; col < CASE3_TILE_COLS && CASE3_COL_START + col < case3_p; ++col)
      {
        reg right = B[depth * case3_p + CASE3_COL_START + col];
        sums[0][col] += left[0] * right;
        sums[1][col] += left[1] * right;
        sums[2][col] += left[2] * right;
        if (CASE3_ROW_START + 3 < case3_m)
          sums[3][col] += left[3] * right;
      }
    }

    for (reg row = 0; row < CASE3_TILE_ROWS && CASE3_ROW_START + row < case3_m; ++row)
      for (reg col = 0; col < CASE3_TILE_COLS && CASE3_COL_START + col < case3_p; ++col)
        C[(CASE3_ROW_START + row) * case3_p + CASE3_COL_START + col] = sums[row][col];
  }
}

#undef CASE3_TILE_ROWS
#undef CASE3_TILE_COLS
#undef CASE3_ROW_TILES
#undef CASE3_COL_TILES
#undef CASE3_ROW_START
#undef CASE3_COL_START
