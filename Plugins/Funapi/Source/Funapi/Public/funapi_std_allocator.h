// Copyright (C) 2019 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_STD_ALLOCATOR_H_
#define SRC_FUNAPI_STD_ALLOCATOR_H_

#include <deque>
#include <queue>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>
#include <unordered_set>

#include <Core/Public/HAL/UnrealMemory.h>

namespace fun {

#ifdef FUNAPI_UE4_PLATFORM_IOS

// NOTE(sungjin): Create an STL compatible allocator to manage the memory explicitly.
// You may have to use these containers in several platforms like iPhone XS,
// which has been struggling with a heap corruption problem.

// define a new allocator
template <typename T>
class funapi_allocator : public std::allocator<T>
{
 public:
  typedef size_t size_type;
  typedef T* pointer;
  typedef const T* const_pointer;
  template<typename _Tp1>

  struct rebind
  {
    typedef funapi_allocator<_Tp1> other;
  };

  pointer allocate(size_type n, const void *hint = 0)
  {
    return reinterpret_cast<pointer>(FMemory::Malloc(n * sizeof(T)));
  }

  void deallocate(pointer p, size_type /* size_type */)
  {
    return FMemory::Free(p);
  }

  funapi_allocator() throw() : std::allocator<T>() { }
  funapi_allocator(const funapi_allocator &a) throw() : std::allocator<T>(a) { }

  template <class U>
  funapi_allocator(const funapi_allocator<U> &a) throw() : std::allocator<T>(a) {}
  ~funapi_allocator() throw() {}
};

typedef std::basic_stringstream<char, std::char_traits<char>, funapi_allocator<char>> stringstream;
typedef std::basic_string<char, std::char_traits<char>, funapi_allocator<char>> string;
typedef std::basic_ostringstream<char, std::char_traits<char>, funapi_allocator<char>> ostringstream;
typedef std::basic_istringstream<char, std::char_traits<char>, funapi_allocator<char>> istringstream;

template<typename _Ty>
using vector = std::vector<_Ty, funapi_allocator<_Ty>>;

template<typename _Ty>
using deque = std::deque<_Ty, funapi_allocator<_Ty>>;

template<typename _Ty>
using queue = std::queue<_Ty, deque<_Ty>>;

template<typename _Ty>
using set = std::set<_Ty, std::less<_Ty>, funapi_allocator<_Ty>>;


template<typename _Ty>
using unordered_set = std::unordered_set<_Ty, std::hash<_Ty>, std::equal_to<_Ty>, funapi_allocator<_Ty>>;

template<typename _Kty, typename _Ty>
using map = std::map<_Kty, _Ty, std::less<_Kty>, funapi_allocator<std::pair<const _Kty, _Ty>>>;

template<typename _Kty, typename _Ty>
using unordered_map =
  std::unordered_map<_Kty, _Ty, std::hash<_Kty>, std::equal_to<_Kty>, funapi_allocator<std::pair<const _Kty, _Ty>>>;

#else // Non-IOS platform

typedef std::basic_stringstream<char, std::char_traits<char>, std::allocator<char>> stringstream;
typedef std::basic_string<char, std::char_traits<char>, std::allocator<char>> string;
typedef std::basic_ostringstream<char, std::char_traits<char>, std::allocator<char>> ostringstream;
typedef std::basic_istringstream<char, std::char_traits<char>, std::allocator<char>> istringstream;

template<typename _Ty>
using vector = std::vector<_Ty>;

template<typename _Ty>
using deque = std::deque<_Ty>;

template<typename _Ty>
using queue = std::queue<_Ty>;

template<typename _Ty>
using set = std::set<_Ty, std::less<_Ty>>;

template<typename _Ty>
using unordered_set = std::unordered_set<_Ty>;

template<typename _Kty, typename _Ty>
using map = std::map<_Kty, _Ty>;

template<typename _Kty, typename _Ty>
using unordered_map = std::unordered_map<_Kty, _Ty>;

#endif

} // fun namespace

#endif // SRC_FUNAPI_STD_ALLOCATOR_H_
