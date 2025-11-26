#pragma once

#include "config.h"
#include <atomic>
namespace coro::detail
{

template<typename T>
struct alignas(config::kCacheLineSize) atomic_ref_wrapper
{
    alignas(std::atomic_ref<T>::required_alignment) T val;
};

} // namespace coro::detail