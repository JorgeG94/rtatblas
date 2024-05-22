#pragma once
#ifdef CUDA
#include <cuda_runtime.h>
#include <cublas_v2.h>
#include <curand.h>
#else
#include <hip/hip_runtime.h>
#include <hipblas/hipblas.h>
#include <hiprand/hiprand.h>
#define cudaMemGetInfo hipMemGetInfo
#define cudaEventCreate hipEventCreate
#define cudaEventDestroy hipEventDestroy
#define cudaEventElapsedTime hipEventElapsedTime
#define cudaEventQuery hipEventQuery
#define cudaEventRecord hipEventRecord
#define cudaEventSynchronize hipEventSynchronize
#define cudaEvent_t hipEvent_t
#define cudaStreamCreate hipStreamCreate
#define cudaStreamDestroy hipStreamDestroy
#define cudaStreamSynchronize hipStreamSynchronize
#define cudaStream_t hipStream_t
#define cudaStreamWaitEvent hipStreamWaitEvent
#define cudaSuccess hipSuccess
#define cudaSetDevice hipSetDevice
#define cudaGetDevice hipGetDevice
#define cudaGetDeviceCount hipGetDeviceCount
#define cudaDeviceSynchronize hipDeviceSynchronize
#define cudaErrorInvalidResourceHandle hipErrorInvalidResourceHandle
#define cudaMalloc hipMalloc
#define cudaFree hipFree
#define cudaMemcpy hipMemcpy
#define cudaMemset hipMemset
#define cudaMemcpyAsync hipMemcpyAsync
#define cudaMemcpyDeviceToDevice hipMemcpyDeviceToDevice
#define cudaMemcpyDeviceToHost hipMemcpyDeviceToHost
#define cudaMemcpyHostToDevice hipMemcpyHostToDevice
#define cublasCreate hipblasCreate
#define cublasDestroy hipblasDestroy
#define cublasDgeam hipblasDgeam
#define cublasDgemm hipblasDgemm
#define cublasDtrsm hipblasDtrsm
#define cublasDsyrk hipblasDsyrk
#define cublasDgemmBatched hipblasDgemmBatched
#define cublasSgeam hipblasSgeam
#define cublasSgemm hipblasSgemm
#define cublasStrsm hipblasStrsm
#define cublasSsyrk hipblasSsyrk
#define cublasSgemmBatched hipblasSgemmBatched
#define cublasGetStream hipblasGetStream
#define cublasSetStream hipblasSetStream
#define cublasHandle_t hipblasHandle_t
#define cublasOperation_t hipblasOperation_t
#define cublasStatus_t hipblasStatus_t
#define CUBLAS_OP_N HIPBLAS_OP_N
#define CUBLAS_OP_T HIPBLAS_OP_T
#define cudaSuccess hipSuccess
#define cudaGetErrorString hipGetErrorString 
#define cudaError_t hipError_t 
#define curandGenerator_t hiprandGenerator_t
#define curandCreateGenerator hiprandCreateGenerator
#define curandDestroyGenerator hiprandDestroyGenerator
#define curandGenerateUniformDouble hiprandGenerateUniformDouble
#define curandGenerateUniform hiprandGenerateUniform
#define CURAND_RNG_PSEUDO_DEFAULT HIPRAND_RNG_PSEUDO_DEFAULT
#define cublasSideMode_t hipblasSideMode_t
#define CUBLAS_SIDE_LEFT HIPBLAS_SIDE_LEFT
#define CUBLAS_SIDE_RIGHT HIPBLAS_SIDE_RIGHT
#define cublasFillMode_t hipblasFillMode_t
#define CUBLAS_DIAG_NON_UNIT HIPBLAS_DIAG_NON_UNIT
#define CUBLAS_DIAG_UNIT HIPBLAS_DIAG_UNIT
#define cublasDiagType_t hipblasDiagType_t
#define CUBLAS_FILL_MODE_LOWER HIPBLAS_FILL_MODE_LOWER
#define CUBLAS_FILL_MODE_UPPER HIPBLAS_FILL_MODE_UPPER
#endif
#include <memory>
#include <iostream>
#include <map>

namespace rtat {

#define gpuAssert(ans)                          \
  {                                             \
    gpu_error_check((ans), __FILE__, __LINE__); \
  }

inline void gpu_error_check(cudaError_t code, const char* file, int line)
{
  if (code != cudaSuccess)
    std::cerr << "GPU Error: " << cudaGetErrorString(code) 
              << " " << file << " " << line << std::endl;
}

class Raw_Device_RNG {
public:
  friend class Device_RNG;
  virtual ~Raw_Device_RNG() = default;
protected:
  Raw_Device_RNG() = default;
  curandGenerator_t rng;
};

class Device_RNG {
public:
  Device_RNG();

  Device_RNG(const Device_RNG& other);
  Device_RNG& operator=(const Device_RNG& other);

  operator curandGenerator_t();

  template<typename T>
  void uniform(T*, size_t);

  template<>
  void uniform(double *A, size_t len) {
    curandGenerateUniformDouble(raw_rng->rng, A, len);
  }

  template<>
  void uniform(float *A, size_t len) {
    curandGenerateUniform(raw_rng->rng, A, len);
  }

private:
  std::shared_ptr<Raw_Device_RNG> raw_rng;
};


// Stream and Event wrappers, intended to mimic the semantics of 
// the native API types but with automatic resource management.
class Stream;
class Event;

class Raw_Stream {
public:
  friend class Stream;
  virtual ~Raw_Stream() = default;
  operator cudaStream_t();
protected:
  Raw_Stream() {}
  cudaStream_t stream;
};


class Stream {
public:
  Stream();
  Stream(cudaStream_t stream);

  Stream(const Stream& other);
  Stream& operator=(const Stream& other);

  operator cudaStream_t();

  void wait_event(Event e);
  void synchronize();
private:
  std::shared_ptr<Raw_Stream> raw_stream;
};


class Raw_Event {
public:
  friend class Event;
  virtual ~Raw_Event() = default;
  operator cudaEvent_t();
protected:
  Raw_Event() {}
  cudaEvent_t event;
};

class Event {
public:
  Event();
  Event(cudaEvent_t event);

  Event(const Event& other);
  Event& operator=(const Event& other);

  operator cudaEvent_t(); 

  void record(Stream s);
  void synchronize();
  bool query();

  static float elapsed_time(Event start, Event end);
private:
  std::shared_ptr<Raw_Event> raw_event;
};


template<typename T, std::map<T, std::string> str_map>
class String_Rep {
  T val;
public:
  String_Rep(T val) : val(val) {}
  operator T() {return val;}

  String_Rep(std::string str) {
    for (auto &[k,v] : str_map) {
      if (v == str) {
        val = k;
        return;
      }
    }
    throw std::runtime_error("Invalid string passed to string rep");
  }

  operator std::string() {
    if (auto search = str_map.find(val); search != str_map.end()) {
      return search->second;
    }
    throw std::runtime_error("Invalid string rep value");
  }
};

class BLAS_Operation {
  cublasOperation_t val;
  static inline const std::map<cublasOperation_t, std::string> 
    str_map = {{CUBLAS_OP_N, "N"}, 
               {CUBLAS_OP_T, "T"}};
public:
  BLAS_Operation(cublasOperation_t val) : val(val) {}
  operator cublasOperation_t() {return val;}

  BLAS_Operation(std::string str) {
    for (auto &[k,v] : str_map) {
      if (v == str) {
        val = k;
        return;
      }
    }
    throw std::runtime_error("Invalid string passed to string rep");
  }

  operator std::string() {
    if (auto search = str_map.find(val); search != str_map.end()) {
      return search->second;
    }
    throw std::runtime_error("Invalid string rep value");
  }
};

class BLAS_Fill_Mode {
  cublasFillMode_t val;
  static inline const std::map<cublasFillMode_t, std::string> 
    str_map = {{CUBLAS_FILL_MODE_LOWER, "Lower"}, 
               {CUBLAS_FILL_MODE_UPPER, "Upper"}};
public:
  BLAS_Fill_Mode(cublasFillMode_t val) : val(val) {}
  operator cublasFillMode_t() {return val;}

  BLAS_Fill_Mode(std::string str) {
    for (auto &[k,v] : str_map) {
      if (v == str) {
        val = k;
        return;
      }
    }
    throw std::runtime_error("Invalid string passed to string rep");
  }

  operator std::string() {
    if (auto search = str_map.find(val); search != str_map.end()) {
      return search->second;
    }
    throw std::runtime_error("Invalid string rep value");
  }
};

class BLAS_Side {
  cublasSideMode_t val;
  static inline const std::map<cublasSideMode_t, std::string> 
    str_map = {{CUBLAS_SIDE_LEFT,  "Left"},
               {CUBLAS_SIDE_RIGHT, "Right"}};
public:
  BLAS_Side(cublasSideMode_t val) : val(val) {}
  operator cublasSideMode_t() {return val;}

  BLAS_Side(std::string str) {
    for (auto &[k,v] : str_map) {
      if (v == str) {
        val = k;
        return;
      }
    }
    throw std::runtime_error("Invalid string passed to string rep");
  }

  operator std::string() {
    if (auto search = str_map.find(val); search != str_map.end()) {
      return search->second;
    }
    throw std::runtime_error("Invalid string rep value");
  }
};

class BLAS_Diag {
  cublasDiagType_t val;
  static inline const std::map<cublasDiagType_t, std::string> 
    str_map = {{CUBLAS_DIAG_UNIT,  "Unit"},
               {CUBLAS_DIAG_NON_UNIT, "Non-Unit"}};
public:
  BLAS_Diag(cublasDiagType_t val) : val(val) {}
  operator cublasDiagType_t() {return val;}

  BLAS_Diag(std::string str) {
    for (auto &[k,v] : str_map) {
      if (v == str) {
        val = k;
        return;
      }
    }
    throw std::runtime_error("Invalid string passed to string rep");
  }

  operator std::string() {
    if (auto search = str_map.find(val); search != str_map.end()) {
      return search->second;
    }
    throw std::runtime_error("Invalid string rep value");
  }
};
}
