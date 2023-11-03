/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);
void transpose_32X32(int M, int N, int A[N][M], int B[M][N]);
void transpose_64X64(int M, int N, int A[N][M], int B[M][N]);
void transpose_67X61(int M, int N, int A[N][M], int B[M][N]);
/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if (M == 32) transpose_32X32(M, N, A, B);
    else if (M == 64) transpose_64X64(M, N, A, B);
    else transpose_67X61(M, N, A, B);
} 

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
    Brute force:
        32X32 misses: 1184
        64X64 misses: 4724
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < M; ++j)
            B[j][i] = A[i][j];
}

/*
* func1 - 32X32 block size = 8  with diagonal optimazation OK!!!
*/
char transpose_32X32_desc[] = "32X32 bsize = 8";
void transpose_32X32(int M, int N, int A[N][M], int B[M][N])
{
    int ii, jj, i;
    int t0, t1, t2, t3, t4, t5, t6, t7;
    for (ii = 0; ii < N; ii += 8)
        for (jj = 0; jj < M; jj += 8)
            for (i = ii; i < ii + 8; ++i)
            {
                t0 = A[i][jj];
                t1 = A[i][jj + 1];
                t2 = A[i][jj + 2];
                t3 = A[i][jj + 3];
                t4 = A[i][jj + 4];
                t5 = A[i][jj + 5];
                t6 = A[i][jj + 6];
                t7 = A[i][jj + 7]; 

                B[jj][i] = t0;
                B[jj + 1][i] = t1;
                B[jj + 2][i] = t2;
                B[jj + 3][i] = t3;
                B[jj + 4][i] = t4;
                B[jj + 5][i] = t5;
                B[jj + 6][i] = t6;
                B[jj + 7][i] = t7; 
            }
}

/*
* transpose_64X64 - 64X64 block size = 4 with diagonal optimazation
*/
char transpose_64X64_desc[] = "64X64 bsize = 8";
void transpose_64X64(int M, int N, int A[N][M], int B[M][N])
{
    int t0, t1, t2, t3, t4, t5, t6, t7;
    int ii, jj, k;
    for (ii = 0; ii < N; ii += 8)
        for (jj = 0; jj < M; jj += 8)
        {
            for (k = ii; k < ii + 4; ++k)
            {
                t0 = A[k][jj + 0];
                t1 = A[k][jj + 1];
                t2 = A[k][jj + 2];
                t3 = A[k][jj + 3];
                t4 = A[k][jj + 4];
                t5 = A[k][jj + 5];
                t6 = A[k][jj + 6];
                t7 = A[k][jj + 7];

                B[jj + 0][k] = t0;
                B[jj + 1][k] = t1;
                B[jj + 2][k] = t2;
                B[jj + 3][k] = t3;
                
                B[jj + 0][k + 4] = t4;
                B[jj + 1][k + 4] = t5;
                B[jj + 2][k + 4] = t6;
                B[jj + 3][k + 4] = t7;
                
            }

            for (k = jj; k < jj + 4; ++k)
            {
                t0 = B[k][ii + 4];
                t1 = B[k][ii + 5];
                t2 = B[k][ii + 6];
                t3 = B[k][ii + 7];

                t4 = A[ii + 4][k];
                t5 = A[ii + 5][k];
                t6 = A[ii + 6][k];
                t7 = A[ii + 7][k];

                B[k][ii + 4] = t4;
                B[k][ii + 5] = t5;
                B[k][ii + 6] = t6;
                B[k][ii + 7] = t7;

                B[k + 4][ii + 0] = t0;
                B[k + 4][ii + 1] = t1;
                B[k + 4][ii + 2] = t2;
                B[k + 4][ii + 3] = t3;
            }

            for (k = ii; k < ii + 4; ++k)
            {
                t0 = A[k + 4][jj + 4];
                t1 = A[k + 4][jj + 5];
                t2 = A[k + 4][jj + 6];
                t3 = A[k + 4][jj + 7];

                B[jj + 4][k + 4] = t0;
                B[jj + 5][k + 4] = t1;
                B[jj + 6][k + 4] = t2;
                B[jj + 7][k + 4] = t3;
            }
        }
}

/*
* transpose_67X61 - block size = 12
*/
char transpose_67X61_desc[] = "67X61 block size = 17";
void transpose_67X61(int M, int N, int A[N][M], int B[M][N])
{
    int ii, jj, i, j;
    for (ii = 0; ii < N; ii += 17)
        for (jj = 0; jj < M; jj += 17)
            for (i = ii; i < ii + 17 && i < N; ++i)
                for (j = jj; j < jj + 17 && j < M; ++j)
                    B[j][i] = A[i][j];
}
/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    //registerTransFunction(trans, trans_desc); 
    //registerTransFunction(transpose_32X32, transpose_32X32_desc);
    //registerTransFunction(transpose_64X64, transpose_64X64_desc);

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

