#pragma once

// From https://github.com/paboyle/Grid/blob/develop/Grid/threads/Accelerator.h
// Orignal author: Peter Boyle <paboyle@ph.ed.ac.uk>

#include <qutils/qacc.h>
#include <qutils/qutils-io.h>

namespace qlat
{  //

inline int qacc_num_threads_default()
{
  TIMER_VERBOSE("qacc_threads_default");
  const std::string n1 = get_env("Q_ACC_NUM_THREADS");
  if (n1 != "") {
    const int n = read_long(n1);
    displayln_info(
        fname +
        ssprintf(": qacc_num_threads() = %d via Q_ACC_NUM_THREADS.", n));
    return n;
  }
  const std::string n2 = get_env("q_acc_num_threads");
  if (n2 != "") {
    const int n = read_long(n2);
    displayln_info(
        fname +
        ssprintf(": qacc_num_threads() = %d via q_acc_num_threads.", n));
    return n;
  }
  const std::string n3 = get_env("OMP_NUM_THREADS");
  if (n3 != "") {
    const int n = read_long(n3);
    displayln_info(
        fname + ssprintf(": qacc_num_threads() = %d via OMP_NUM_THREADS.", n));
    return n;
  }
  displayln_info(fname + ssprintf(": qacc_num_threads() = %d.", 32));
  return 32;
}

inline int& qacc_num_threads()
// qlat parameter
{
  static int nt = qacc_num_threads_default();
  return nt;
}

#define DO_PRAGMA(x) _Pragma(#x)

#define qthread_for(iter1, num, ...)            \
  DO_PRAGMA(omp parallel for schedule(static)) \
  for (long iter1 = 0; iter1 < num; ++iter1) { \
    __VA_ARGS__                                \
  };

#define qthread_for2d(iter1, num1, iter2, num2, ...) \
  DO_PRAGMA(omp parallel for collapse(2))           \
  for (long iter1 = 0; iter1 < num1; ++iter1) {     \
    for (long iter2 = 0; iter2 < num2; ++iter2) {   \
      {__VA_ARGS__};                                \
    }                                               \
  }

#ifdef QLAT_USE_ACC

#define qacc_continue return

#define qacc_for2dNB(iter1, num1, iter2, num2, ...)                     \
  {                                                                     \
    typedef long Iterator;                                              \
    auto lambda = [=] __host__ __device__(Iterator iter1,               \
                                          Iterator iter2) mutable {     \
      __VA_ARGS__;                                                      \
    };                                                                  \
    const int nt = qlat::qacc_num_threads();                            \
    dim3 cu_threads(nt, 1, 1);                                          \
    dim3 cu_blocks((num1 + nt - 1) / nt, num2, 1);                      \
    cudaError err = cudaGetLastError();                                 \
    if (cudaSuccess != err) {                                           \
      qlat::displayln(                                                  \
          qlat::ssprintf("qacc_for: Cuda error %s from '%s' Line %d.",  \
                         cudaGetErrorString(err), __FILE__, __LINE__)); \
      qassert(false);                                                   \
    }                                                                   \
    qlambda_apply<<<cu_blocks, cu_threads>>>(num1, num2, lambda);       \
  }

template <typename Lambda>
__global__ void qlambda_apply(long num1, long num2, Lambda lam)
{
  long x = threadIdx.x + blockDim.x * blockIdx.x;
  long y = threadIdx.y + blockDim.y * blockIdx.y;
  if ((x < num1) && (y < num2)) {
    lam(x, y);
  }
}

#define qacc_barrier(dummy)                                                \
  {                                                                        \
    cudaDeviceSynchronize();                                               \
    cudaError err = cudaGetLastError();                                    \
    if (cudaSuccess != err) {                                              \
      qlat::displayln(                                                     \
          qlat::ssprintf("qacc_barrier: Cuda error %s from '%s' Line %d.", \
                         cudaGetErrorString(err), __FILE__, __LINE__));    \
      qassert(false);                                                      \
    }                                                                      \
  }

#define qacc_forNB(iter1, num1, ...) \
  qacc_for2dNB(iter1, num1, iter2, 1, {__VA_ARGS__});

#define qacc_for(iter, num, ...)        \
  qacc_forNB(iter, num, {__VA_ARGS__}); \
  qacc_barrier(dummy);

#define qacc_for2d(iter1, num1, iter2, num2, ...)        \
  qacc_for2dNB(iter1, num1, iter2, num2, {__VA_ARGS__}); \
  qacc_barrier(dummy);

#else

#define qacc_continue continue

#define qacc_for2dNB(iter1, num1, iter2, num2, ...) \
  qthread_for2d(iter1, num1, iter2, num2, {__VA_ARGS__});

#define qacc_forNB(iter1, num1, ...) qthread_for(iter1, num1, {__VA_ARGS__});

#define qacc_barrier(dummy)

#define qacc_for2d(iter1, num1, iter2, num2, ...)        \
  qacc_for2dNB(iter1, num1, iter2, num2, {__VA_ARGS__}); \
  qacc_barrier(dummy);

#define qacc_for(iter, num, ...)        \
  qacc_forNB(iter, num, {__VA_ARGS__}); \
  qacc_barrier(dummy);

#endif

}  // namespace qlat
