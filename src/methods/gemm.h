#pragma once
#include <string>
#include <vector>
#include <matrixop.h>
#include <executor.h>
#include "base_options.h"

namespace rtat {

template<typename T>
struct GEMM_Inputs {
  using Scalar = T;

  gpu::blasHandle_t handle;
  BLAS_Operation transa; BLAS_Operation transb;
  const Matrix<T> A;
  const Matrix<T> B;
        Matrix<T> C;
  const T alpha; const T beta;

  GEMM_Inputs(gpu::blasHandle_t handle, BLAS_Operation transa, BLAS_Operation transb,
              const Matrix<T> A, const Matrix<T> B, Matrix<T> C, T alpha, T beta)
        : handle(handle), transa(transa), transb(transb), A(A), B(B), C(C), 
          alpha(alpha), beta(beta) {}

  size_t m() {return C.dims().m;}
  size_t n() {return C.dims().n;}
  size_t k() {return (transa == gpu::BLAS_OP_N) ? A.dims().n : A.dims().m;}
};


struct GEMM_Key {
  BLAS_Operation transa; BLAS_Operation transb;
  int m; int n; int k;

  template<typename T>
  GEMM_Key(GEMM_Inputs<T> i) : transa(i.transa), transb(i.transb), 
                            m(i.m()), n(i.n()), k(i.k()) {}

  GEMM_Key(BLAS_Operation transa, BLAS_Operation transb,
           int m, int k, int n) : transa(transa), transb(transb), 
                                  m(m), n(n), k(k) {}

  operator std::string() const;
  bool operator<(const GEMM_Key&) const;
  friend std::ostream& operator<<(std::ostream&, const GEMM_Key&); 
};


struct GEMM_Options {
  BLAS_Op transa;
  BLAS_Op transb;
  BLAS_Op transc;

  GEMM_Options() = default;
  GEMM_Options(BLAS_Op transa,
               BLAS_Op transb,
               BLAS_Op transc) :
    transa(transa), 
    transb(transb),
    transc(transc) {}

  static GEMM_Options default_opts() {
    return GEMM_Options();
  }

  static std::vector<GEMM_Options> enumerate();

  operator std::string() const;

  bool operator<(const GEMM_Options&) const;

  friend std::ostream& operator<<(std::ostream&, const GEMM_Options);
  friend std::istream& operator>>(std::istream&, GEMM_Options&); 

  template<typename T>
  std::unique_ptr<MatrixOp<T>> form_operation(GEMM_Inputs<T>);
};

struct GEMM_Options_Pad {
  BLAS_Op transa;
  Pad_Op  pada;
  BLAS_Op transb;
  Pad_Op  padb;
  BLAS_Op transc;
  Pad_Op  padc;

  GEMM_Options_Pad() = default;
  GEMM_Options_Pad(BLAS_Op transa, Pad_Op pada,
               BLAS_Op transb, Pad_Op padb,
               BLAS_Op transc, Pad_Op padc) :
    transa(transa), pada(pada), 
    transb(transb), padb(padb),
    transc(transc), padc(padc) {}

  static GEMM_Options_Pad default_opts() {
    return GEMM_Options_Pad();
  }

  static std::vector<GEMM_Options_Pad> enumerate();

  operator std::string() const;

  bool operator<(const GEMM_Options_Pad&) const;

  friend std::ostream& operator<<(std::ostream&, const GEMM_Options_Pad);
  friend std::istream& operator>>(std::istream&, GEMM_Options_Pad&); 

  template<typename T>
  std::unique_ptr<MatrixOp<T>> form_operation(GEMM_Inputs<T>);
};


template<typename T>
class GEMM_Executor : public Executor<GEMM_Inputs<T>, GEMM_Key, GEMM_Options> {
protected:
  void warmup(GEMM_Inputs<T> params, [[maybe_unused]] GEMM_Options opts,
              [[maybe_unused]] Stream s) override {
    size_t n = 8;
    double *A, *B, *C;
    gpuAssert(gpu::Malloc(&A, n*n*sizeof(double)));
    gpuAssert(gpu::Malloc(&B, n*n*sizeof(double)));
    gpuAssert(gpu::Malloc(&C, n*n*sizeof(double)));

    std::vector<BLAS_Operation> ops = {gpu::BLAS_OP_N, gpu::BLAS_OP_T};

    for (auto &opA : ops) {
      for (auto &opB : ops) {
        double alpha = 1.0;
        double beta = 0.0;
        gpu::blasDgemm(params.handle, opA, opB, n,n,n, &alpha, A,n,B,n, &beta, C,n);
        gpu::blasDgeam(params.handle, opA, opB, n,n, &alpha, A, n, &beta, B, n, C, n);
      }
    }
    gpuAssert(gpu::DeviceSynchronize());
    gpuAssert(gpu::Free(A));
    gpuAssert(gpu::Free(B));
    gpuAssert(gpu::Free(C));
  }
};

template<typename T>
class GEMM_Executor_Pad : public Executor<GEMM_Inputs<T>, GEMM_Key, GEMM_Options_Pad> {
protected:
  void warmup(GEMM_Inputs<T> params, [[maybe_unused]] GEMM_Options_Pad opts,
              [[maybe_unused]] Stream s) override {
    size_t n = 8;
    double *A, *B, *C;
    gpuAssert(gpu::Malloc(&A, n*n*sizeof(double)));
    gpuAssert(gpu::Malloc(&B, n*n*sizeof(double)));
    gpuAssert(gpu::Malloc(&C, n*n*sizeof(double)));

    std::vector<BLAS_Operation> ops = {gpu::BLAS_OP_N, gpu::BLAS_OP_T};

    for (auto &opA : ops) {
      for (auto &opB : ops) {
        double alpha = 1.0;
        double beta = 0.0;
        gpu::blasDgemm(params.handle, opA, opB, n,n,n, &alpha, A,n,B,n, &beta, C,n);
        gpu::blasDgeam(params.handle, opA, opB, n,n, &alpha, A, n, &beta, B, n, C, n);
      }
    }
    gpuAssert(gpu::DeviceSynchronize());
    gpuAssert(gpu::Free(A));
    gpuAssert(gpu::Free(B));
    gpuAssert(gpu::Free(C));
  }
};

}
