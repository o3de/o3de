/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/Memory.h>

#if defined(AZ_GLOBAL_NEW_AND_DELETE_DEFINED)
#pragma error("NewAndDelete.inl has been included multiple times in a single module")
#endif

#define AZ_GLOBAL_NEW_AND_DELETE_DEFINED
[[nodiscard]] void* operator new(std::size_t size) { return AZ::OperatorNew(size); }
[[nodiscard]] void* operator new[](std::size_t size) { return AZ::OperatorNewArray(size); }
[[nodiscard]] void* operator new(size_t size, const std::nothrow_t&) noexcept { return AZ::OperatorNew(size); }
[[nodiscard]] void* operator new[](size_t size, const std::nothrow_t&) noexcept { return AZ::OperatorNewArray(size); }
void operator delete(void* ptr) noexcept { AZ::OperatorDelete(ptr); }
void operator delete[](void* ptr) noexcept{ AZ::OperatorDeleteArray(ptr); }
void operator delete(void* ptr, std::size_t size) noexcept { AZ::OperatorDelete(ptr, size); }
void operator delete[](void* ptr, std::size_t size) noexcept { AZ::OperatorDeleteArray(ptr, size); }
void operator delete(void* ptr, const std::nothrow_t&) noexcept { AZ::OperatorDelete(ptr); }
void operator delete[](void* ptr, const std::nothrow_t&) noexcept { AZ::OperatorDeleteArray(ptr); }

#if __cpp_aligned_new
// C++17 provides allocating operator new overloads for over aligned types
//an http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0035r4.html
[[nodiscard]] void* operator new(std::size_t size, std::align_val_t align) { return AZ::OperatorNew(size, align); }
[[nodiscard]] void* operator new(std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept { return AZ::OperatorNew(size, align); }
[[nodiscard]] void* operator new[](std::size_t size, std::align_val_t align) { return AZ::OperatorNewArray(size, align); }
[[nodiscard]] void* operator new[](std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept { return AZ::OperatorNewArray(size, align); }
void operator delete(void* ptr, std::size_t size, std::align_val_t align) noexcept { AZ::OperatorDelete(ptr, size, align); }
void operator delete[](void* ptr, std::size_t size, std::align_val_t align) noexcept { AZ::OperatorDeleteArray(ptr, size, align); }
#endif
