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
[[nodiscard]] void* operator new(std::size_t size, const AZ::Internal::AllocatorDummy*) { return AZ::OperatorNew(size); }
[[nodiscard]] void* operator new[](std::size_t size, const AZ::Internal::AllocatorDummy*) { return AZ::OperatorNewArray(size); }
[[nodiscard]] void* operator new(std::size_t size, const char* fileName, int lineNum, const char* name, const AZ::Internal::AllocatorDummy*) { return AZ::OperatorNew(size, fileName, lineNum, name); }
[[nodiscard]] void* operator new[](std::size_t size, const char* fileName, int lineNum, const char* name, const AZ::Internal::AllocatorDummy*) { return AZ::OperatorNewArray(size, fileName, lineNum, name); }
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
void operator delete(void* ptr, [[maybe_unused]] const char* fileName, [[maybe_unused]] int lineNum, [[maybe_unused]] const char* name) noexcept { AZ::OperatorDelete(ptr); }
void operator delete[](void* ptr, [[maybe_unused]] const char* fileName, [[maybe_unused]] int lineNum, [[maybe_unused]] const char* name) noexcept { AZ::OperatorDeleteArray(ptr); }

#if __cpp_aligned_new
// C++17 provides allocating operator new overloads for over aligned types
//an http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0035r4.html
[[nodiscard]] void* operator new(std::size_t size, std::align_val_t align) { return AZ::OperatorNew(size, align); }
[[nodiscard]] void* operator new(std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept { return AZ::OperatorNew(size, align); }
[[nodiscard]] void* operator new(std::size_t size, std::align_val_t align, const char* fileName, int lineNum, const char* name, const AZ::Internal::AllocatorDummy*) { return AZ::OperatorNew(size, align, fileName, lineNum, name); }
[[nodiscard]] void* operator new[](std::size_t size, std::align_val_t align) { return AZ::OperatorNewArray(size, align); }
[[nodiscard]] void* operator new[](std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept { return AZ::OperatorNewArray(size, align); }
[[nodiscard]] void* operator new[](std::size_t size, std::align_val_t align, const char* fileName, int lineNum, const char* name) { return AZ::OperatorNewArray(size, align, fileName, lineNum, name); }
void operator delete(void* ptr, std::align_val_t align) noexcept { AZ::OperatorDelete(ptr, 0, align); }
void operator delete(void* ptr, std::size_t size, std::align_val_t align) noexcept { AZ::OperatorDelete(ptr, size, align); }
void operator delete(void* ptr, std::align_val_t align, const std::nothrow_t&) noexcept { AZ::OperatorDelete(ptr, 0, align); }
void operator delete(void* ptr, std::align_val_t align, [[maybe_unused]] const char* fileName, [[maybe_unused]] int lineNum, [[maybe_unused]] const char* name) noexcept { AZ::OperatorDelete(ptr, 0, align); }
void operator delete[](void* ptr, std::align_val_t align) noexcept { AZ::OperatorDeleteArray(ptr, 0, align); }
void operator delete[](void* ptr, std::size_t size, std::align_val_t align) noexcept { AZ::OperatorDeleteArray(ptr, size, align); }
void operator delete[](void* ptr, std::align_val_t align, const std::nothrow_t&) noexcept { AZ::OperatorDeleteArray(ptr, 0, align); }
void operator delete[](void* ptr, std::align_val_t align, [[maybe_unused]] const char* fileName, [[maybe_unused]] int lineNum, [[maybe_unused]] const char* name) noexcept { AZ::OperatorDeleteArray(ptr, 0, align); }
#endif
