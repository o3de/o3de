/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Note: The utilities in this file should typically not be used directly,
// consider including UnicodeFunctions.h or UnicodeIterator.h instead.
//
// (At least) the following string types can be bound with these helper functions:
// Types                                            Input  Output Null-Terminator
// CryStringT<T>, (::string, ::wstring):            yes    yes    implied by type (also Stack and Fixed variants)
// std::basic_string<T>, std::string, std::wstring: yes    yes    implied by type
// QString:                                         yes    yes    implied by type
// std::vector<T>, std::list<T>, std::deque<T>:     yes    yes    not present
// T[] (fixed-length buffer):                       yes    yes    guaranteed to be emitted on output, accepted on input
// T * and size_t (user-specified-size buffer):     no     yes    guaranteed to be emitted on output
// const T * (null-terminated string):              yes    no     expected
// const T[] (literal):                             yes    no     implied as the last item in the array
// pair of iterators over T:                        yes    no     should not be included in the range
// uint32 (single UCS code-point):                  yes    no     not present
// If some other string type is not listed, you can still use it for input easily by passing begin/end iterators.
// Note: For all types, T can be any 8-bit, 16-bit or 32-bit integral or character type.
// Further T types may be processed by explicitly passing InputEncoding and OutputEncoding.
// We never actively tested such scenario's, so no guarantees on floating and user-defined types as code-units.


#pragma once

#ifndef assert
// Some tools use CRT's assert, most engine and game modules use CryAssert.h (via platform.h maybe).
// We don't want to force a choice upon all code that uses Unicode utilities, so we just assume assert is defined.
#error This header uses assert macro, please provide an applicable definition before including UnicodeXXX.h
#endif

#include "UnicodeEncoding.h"
#include <string.h>                 // For str(n)len and memcpy.
#include <wchar.h>                  // For wcs(n)len.
#include <stddef.h>                 // For size_t and ptrdiff_t.
#include <iterator>                 // For std::iterator_traits.
#include <string>                   // For std::basic_string.
#include <vector>                   // For std::vector.
#include <list>                     // For std::list.
#include <deque>                    // For std::deque.
#include <type_traits>              // ... standard type-traits (as of C++11).

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define UNICODEBINDING_H_SECTION_1 1
#define UNICODEBINDING_H_SECTION_2 2
#endif

// Forward declare the supported types.
// Before actually instantiating a binding however, you need to have the full definition included.
// Also, this allows us to work with QChar/QString as declared names without a dependency on Qt.
template<typename T, size_t S>
class CryStackStringT;
template<size_t S>
class CryFixedStringT;
template<size_t S>
class CryFixedWStringT;
template<typename T>
class CryStringLocalT;
template<typename T>
class CryStringT;
class QChar;
class QString;
namespace Unicode
{
    namespace Detail
    {
        // Import standard type traits.
        // This requires C++11 compiler support.
        using std::add_const;
        using std::conditional;
        using std::extent;
        using std::integral_constant;
        using std::is_arithmetic;
        using std::is_array;
        using std::is_base_of;
        using std::is_const;
        using std::is_convertible;
        using std::is_integral;
        using std::is_pointer;
        using std::is_same;
        using std::make_unsigned;
        using std::remove_cv;
        using std::remove_extent;
        using std::remove_pointer;

        // SVoid<T>:
        // Result type will be void if T is well-formed.
        // Note: This is mostly used to test the presence of member types at compile-time.
        template<typename T>
        struct SVoid
        {
            typedef void type;
        };

        // SValidChar<T, InferEncoding, Input>:
        // Determine if T is a valid character type in the given compile-time context.
        // The InferEncoding flag is set if the encoding has to be detected automatically.
        // The Input flag is set if the type is used for input (and not set if the type is used for output).
        template<typename T, bool InferEncoding, bool Input>
        struct SValidChar
        {
            typedef typename remove_cv<T>::type BaseType;
            static const bool isArithmeticType = is_arithmetic<BaseType>::value;
            static const bool isQChar = is_same<BaseType, QChar>::value;
            static const bool isUsable = isArithmeticType || isQChar;
            static const bool isValidQualified = !is_const<T>::value || Input;
            static const bool isKnownSize = sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4;
            static const bool isValidInferred = isKnownSize || !InferEncoding;
            static const bool value = isUsable && isValidQualified && isValidInferred;
        };

        // SPackedIterators<T>:
        // A pair of iterators over some range.
        // Note: Packing iterators into a single object allows us to pass them as a single argument like all other types.
        template<typename T>
        struct SPackedIterators
        {
            const T begin, end;
            SPackedIterators(const T& _begin, const T& _end)
                : begin(_begin)
                , end(_end) {}
        };

        // SPackedBuffer<T>:
        // A buffer-pointer/length tuple.
        // Note: Packing them into a single object allows us to pass them as a single argument like all other types.
        template<typename T>
        struct SPackedBuffer
        {
            T buffer;
            size_t size;
            SPackedBuffer(T _buffer, size_t _size)
                : buffer(_buffer)
                , size(_size) {}
        };

        // SDependentType<T, X>:
        // Makes the name of type T dependent on X (which is otherwise meaningless).
        // Note: This is used to force two-phase lookup so we don't need the definition of T until instantiation.
        // This way we can convince standards-compliant compilers Clang and GCC to not require definition of forward-declared types.
        // Specifically, we forward-declare Qt's QString and QChar, for which the definition will never be available outside Editor.
        template<typename T, int X>
        struct SDependentType
        {
            typedef T type;
        };

        // EBind:
        // Methods of binding a type for input and/or output.
        // Note: These are used for tag-dispatch by binding functions, and are private to the implementation.
        enum EBind
        {                          // Input Output Description
            eBind_Impossible,      // No    No     Can't bind this type.
            eBind_Iterators,       // Yes   Yes    Bind by using begin() and end() member functions.
            eBind_Data,            // Yes   Yes    Bind by using data() and size() member functions.
            eBind_Literal,         // Yes   No     Bind a fixed size buffer (const element, aka string literal).
            eBind_Buffer,          // Yes   No     Bind a fixed size buffer (non-const element) that may be null-terminated.
            eBind_PackedBuffer,    // No    Yes    Bind a user-specified size buffer (non-const element).
            eBind_NullTerminated,  // Yes   No     Bind a null-terminated buffer of unknown length (C string).
            eBind_CodePoint,       // Yes   No     Bind a single code-point value.
        };

        // SBindIterator<T, InferEncoding>:
        // Find the EBind for input from iterator pair of type T at compile-time.
        // If the type is not supported, the resulting value will be eBind_Impossible
        template<typename T, bool InferEncoding, typename HasValueType = void, typename HasIteratorCategory = void>
        struct SBindIterator
        {
            typedef const void CharType;
            static const EBind value = eBind_Impossible;
        };
        template<typename T, bool InferEncoding, typename HasValueType, typename HasIteratorCategory>
        struct SBindIterator<T*, InferEncoding, HasValueType, HasIteratorCategory>
        {
            typedef typename add_const<T>::type CharType;
            static const bool isValid = SValidChar<CharType, InferEncoding, true>::value;
            static const EBind value = isValid ? eBind_Iterators : eBind_Impossible;
        };
        template<typename T, bool InferEncoding>
        struct SBindIterator<T, InferEncoding,
            typename SVoid<typename T::value_type>::type,
            typename SVoid<typename T::iterator_category>::type
            >
        {
            typedef typename add_const<typename T::value_type>::type CharType;
            typedef typename T::iterator_category IteratorCategory;
            static const bool isInputIterator = is_base_of<std::input_iterator_tag, IteratorCategory>::value;
            static const bool isValid = SValidChar<CharType, InferEncoding, true>::value;
            static const EBind value = isValid && isInputIterator ? eBind_Iterators : eBind_Impossible;
        };

        // SBindObject<T, InferEncoding>:
        // Find the EBind for input from object of type T at compile-time.
        // If the type is not supported, the resulting value will be eBind_Impossible.
        template<typename T, bool InferEncoding>
        struct SBindObject
        {
            typedef typename add_const<
                typename conditional<
                    is_array<T>::value,
                    typename remove_extent<T>::type,
                    typename remove_pointer<T>::type
                    >::type
                >::type CharType;
            static const size_t FixedSize = extent<T>::value;
            COMPILE_TIME_ASSERT(!is_array<T>::value || FixedSize > 0);
            static const bool isConstArray = is_array<T>::value && is_const<typename remove_extent<T>::type>::value;
            static const bool isBufferArray = is_array<T>::value && !isConstArray;
            static const bool isPointer = is_pointer<T>::value;
            static const bool isCodePoint = is_integral<T>::value;
            static const bool isValidChar = SValidChar<CharType, InferEncoding, true>::value;
            static const EBind value =
                !isValidChar ? eBind_Impossible :
                isConstArray ? eBind_Literal :
                isBufferArray ? eBind_Buffer :
                isPointer ? eBind_NullTerminated :
                isCodePoint ? eBind_CodePoint :
                eBind_Impossible;
        };
        template<typename CharT, typename Traits, typename Allocator, bool InferEncoding>
        struct SBindObject<std::basic_string<CharT, Traits, Allocator>, InferEncoding>
        {
            typedef typename add_const<CharT>::type CharType;
            static const bool isValid = SValidChar<CharT, InferEncoding, true>::value;
            static const EBind value = isValid ? eBind_Data : eBind_Impossible;
        };
        template<typename T, typename Allocator, bool InferEncoding>
        struct SBindObject<std::vector<T, Allocator>, InferEncoding>
        {
            typedef typename add_const<T>::type CharType;
            static const bool isValid = SValidChar<T, InferEncoding, true>::value;
            static const EBind value = isValid ? eBind_Data : eBind_Impossible;
        };
        template<typename T, typename Allocator, bool InferEncoding>
        struct SBindObject<std::list<T, Allocator>, InferEncoding>
        {
            typedef typename add_const<T>::type CharType;
            static const bool isValid = SValidChar<T, InferEncoding, true>::value;
            static const EBind value = isValid ? eBind_Iterators : eBind_Impossible;
        };
        template<typename T, typename Allocator, bool InferEncoding>
        struct SBindObject<std::deque<T, Allocator>, InferEncoding>
        {
            typedef typename add_const<T>::type CharType;
            static const bool isValid = SValidChar<T, InferEncoding, true>::value;
            static const EBind value = isValid ? eBind_Iterators : eBind_Impossible;
        };
        template<typename T, bool InferEncoding>
        struct SBindObject<CryStringT<T>, InferEncoding>
        {
            typedef typename add_const<T>::type CharType;
            static const bool isValid = SValidChar<T, InferEncoding, true>::value;
            static const EBind value = isValid ? eBind_Data : eBind_Impossible;
        };
        template<typename T, bool InferEncoding>
        struct SBindObject<CryStringLocalT<T>, InferEncoding>
        {
            typedef typename add_const<T>::type CharType;
            static const bool isValid = SValidChar<T, InferEncoding, true>::value;
            static const EBind value = isValid ? eBind_Data : eBind_Impossible;
        };
        template<typename T, size_t S, bool InferEncoding>
        struct SBindObject<CryStackStringT<T, S>, InferEncoding>
        {
            typedef typename add_const<T>::type CharType;
            static const bool isValid = SValidChar<T, InferEncoding, true>::value;
            static const EBind value = isValid ? eBind_Data : eBind_Impossible;
        };
        template<size_t S, bool InferEncoding>
        struct SBindObject<CryFixedStringT<S>, InferEncoding>
        {
            typedef char CharType;
            static const bool isValid = SValidChar<CharType, InferEncoding, true>::value;
            static const EBind value = isValid ? eBind_Data : eBind_Impossible;
        };
        template<size_t S, bool InferEncoding>
        struct SBindObject<CryFixedWStringT<S>, InferEncoding>
        {
            typedef wchar_t CharType;
            static const bool isValid = SValidChar<CharType, InferEncoding, true>::value;
            static const EBind value = isValid ? eBind_Data : eBind_Impossible;
        };
        template<bool InferEncoding>
        struct SBindObject<QString, InferEncoding>
        {
            typedef const QChar CharType;
            static const EBind value = eBind_Data;
        };
        template<typename T, bool InferEncoding>
        struct SBindObject<SPackedIterators<T>, InferEncoding>
        {
            typedef typename SBindIterator<T, InferEncoding>::CharType CharType;
            static const EBind value = eBind_Iterators;
        };

        // SBindOutput<T, InferEncoding>:
        // Find the EBind for output to object of type T at compile-time.
        // If the type is not supported, the resulting value will be eBind_Impossible.
        template<typename T, bool InferEncoding>
        struct SBindOutput
        {
            typedef typename remove_extent<T>::type CharType;
            static const size_t FixedSize = extent<T>::value;
            static const bool isArray = is_array<T>::value;
            static const bool isValid = SValidChar<typename remove_extent<T>::type, InferEncoding, false>::value;
            static const EBind value = isArray && isValid ? eBind_Buffer : eBind_Impossible;
        };
        template<typename OutputCharType, bool InferEncoding>
        struct SBindOutput<SPackedBuffer<OutputCharType*>, InferEncoding>
        {
            typedef OutputCharType CharType;
            static const bool isValid = SValidChar<CharType, InferEncoding, false>::value;
            static const EBind value = isValid ? eBind_PackedBuffer : eBind_Impossible;
        };
        template<typename CharT, typename Traits, typename Allocator, bool InferEncoding>
        struct SBindOutput<std::basic_string<CharT, Traits, Allocator>, InferEncoding>
        {
            typedef CharT CharType;
            static const bool isValid = SValidChar<CharT, InferEncoding, false>::value;
            static const EBind value = isValid ? eBind_Data : eBind_Impossible;
        };
        template<typename T, typename Allocator, bool InferEncoding>
        struct SBindOutput<std::vector<T, Allocator>, InferEncoding>
        {
            typedef T CharType;
            static const bool isValid = SValidChar<T, InferEncoding, false>::value;
            static const EBind value = isValid ? eBind_Data : eBind_Impossible;
        };
        template<typename T, typename Allocator, bool InferEncoding>
        struct SBindOutput<std::list<T, Allocator>, InferEncoding>
        {
            typedef T CharType;
            static const bool isValid = SValidChar<T, InferEncoding, false>::value;
            static const EBind value = isValid ? eBind_Iterators : eBind_Impossible;
        };
        template<typename T, typename Allocator, bool InferEncoding>
        struct SBindOutput<std::deque<T, Allocator>, InferEncoding>
        {
            typedef T CharType;
            static const bool isValid = SValidChar<T, InferEncoding, false>::value;
            static const EBind value = isValid ? eBind_Iterators : eBind_Impossible;
        };
        template<typename T, bool InferEncoding>
        struct SBindOutput<CryStringT<T>, InferEncoding>
        {
            typedef T CharType;
            static const bool isValid = SValidChar<T, InferEncoding, false>::value;
            static const EBind value = isValid ? eBind_Data : eBind_Impossible;
        };
        template<typename T, bool InferEncoding>
        struct SBindOutput<CryStringLocalT<T>, InferEncoding>
        {
            typedef T CharType;
            static const bool isValid = SValidChar<T, InferEncoding, false>::value;
            static const EBind value = isValid ? eBind_Data : eBind_Impossible;
        };
        template<typename T, size_t S, bool InferEncoding>
        struct SBindOutput<CryStackStringT<T, S>, InferEncoding>
        {
            typedef T CharType;
            static const bool isValid = SValidChar<T, InferEncoding, false>::value;
            static const EBind value = isValid ? eBind_Data : eBind_Impossible;
        };
        template<bool InferEncoding>
        struct SBindOutput<QString, InferEncoding>
        {
            typedef QChar CharType;
            static const EBind value = eBind_Data;
        };

        // SInferEncoding<T>:
        // Infers the encoding of the given character type.
        // Note: This will always pick an UTF encoding type based on the size of the element type.
        template<typename T, bool Input>
        struct SInferEncoding
        {
            typedef SBindObject<T, true> ObjectType;
            typedef SBindIterator<T, true> IteratorType;
            typedef typename conditional<
                IteratorType::value != eBind_Impossible,
                typename IteratorType::CharType,
                typename ObjectType::CharType
                >::type CharType;
            static const EEncoding value =
                sizeof(CharType) == 1 ? eEncoding_UTF8 :
                sizeof(CharType) == 2 ? eEncoding_UTF16 :
                eEncoding_UTF32;
            COMPILE_TIME_ASSERT(value != eEncoding_UTF32 || sizeof(CharType) == 4);
        };

        // SBindCharacter<T, Input>:
        // Pick the base character type to use during input or output with this element type.
        template<typename T, bool Input, bool Integral = is_integral<T>::value, bool IsQChar = is_same<QChar, typename remove_cv<T>::type>::value>
        struct SBindCharacter
        {
            typedef typename make_unsigned<T>::type BaseType; // The standard doesn't define if a character type is signed or unsigned.
            typedef typename remove_cv<BaseType>::type UnqualifiedType;
            typedef typename conditional<Input, const UnqualifiedType, UnqualifiedType>::type type;
        };
        template<typename T, bool Input>
        struct SBindCharacter<T, Input, false, false>
        {
            COMPILE_TIME_ASSERT(is_arithmetic<T>::value);
            typedef typename remove_cv<T>::type UnqualifiedType;
            typedef typename conditional<Input, const UnqualifiedType, UnqualifiedType>::type type;
        };
        template<typename T, bool Input>
        struct SBindCharacter<T, Input, false, true>
        {
            typedef typename conditional<Input, const uint16, uint16>::type type;
            typedef typename SDependentType<QChar, Input>::type ActuallyQChar; // Force two-phase name lookup on QChar.
            COMPILE_TIME_ASSERT(sizeof(ActuallyQChar) == sizeof(type)); // In case Qt ever changes QChar.
        };

        // SBindPointer<T, Input>:
        // Pick the pointer type to use during input or output with buffers (potentially inside string types).
        template<typename T, bool Input>
        struct SBindPointer
        {
            COMPILE_TIME_ASSERT(is_pointer<T>::value || is_array<T>::value);
            typedef typename conditional<
                is_pointer<T>::value,
                typename remove_pointer<T>::type,
                typename remove_extent<T>::type
                >::type UnboundCharType;
            typedef typename SBindCharacter<UnboundCharType, Input>::type BoundCharType;
            typedef BoundCharType* type;
        };

        // SAutomaticallyDeduced:
        // Placeholder type that is never defined, used by SRequire for SFINAE overloading.
        struct SAutomaticallyDeduced;

        // SRequire<Expr, T>:
        // Helper for SFINAE overloading.
        // Similar to C++11's std::enable_if, which is not in boost (with that exact name anyway).
        template<bool SFINAE, typename T = SAutomaticallyDeduced>
        struct SRequire
        {
            typedef T type;
        };
        template<typename T>
        struct SRequire<false, T> {};

        // SafeCast<T, SourceChar>:
        // Cast a pointer to type T, but only allowing safe casts.
        // This guards against bad code in other functions since it prevents unintended casts.
        template<typename T, typename SourceChar>
        inline T SafeCast(SourceChar* ptr, typename SRequire<is_integral<SourceChar>::value>::type* = 0)
        {
            // Allow casts from pointer-to-integral to unrelated pointer-to-integral, provided they are of the same size.
            typedef typename remove_pointer<T>::type TargetChar;
            COMPILE_TIME_ASSERT(is_integral<SourceChar>::value && is_integral<TargetChar>::value);
            COMPILE_TIME_ASSERT(sizeof(SourceChar) == sizeof(TargetChar));
            return reinterpret_cast<T>(ptr);
        }
        template<typename T, typename SourceChar>
        inline T SafeCast(SourceChar* ptr, typename SRequire<is_same<typename remove_cv<SourceChar>::type, QChar>::value>::type* = 0)
        {
            // Allow casts from pointer-to-QChar to unrelated pointer-to-integral, provided they are of the same size.
            typedef typename remove_pointer<T>::type TargetChar;
            COMPILE_TIME_ASSERT(is_integral<TargetChar>::value);
            COMPILE_TIME_ASSERT(sizeof(SourceChar) == sizeof(TargetChar));
            return reinterpret_cast<T>(ptr);
        }
        template<typename T, typename SourceChar>
        inline T SafeCast(SourceChar* ptr, typename SRequire<!is_integral<SourceChar>::value&& !is_same<typename remove_cv<SourceChar>::type, QChar>::value>::type* = 0)
        {
            // Any other casts that are allowed by C++.
            return static_cast<T>(ptr);
        }

        // SCharacterTrait<T>:
        // Exposes some basic traits for a given character.
        // Note: Map to (hopefully optimized) CRT functions where possible.
        template<typename T, size_t Size = sizeof(T)* is_integral<T>::value>
        struct SCharacterTrait
        {
            static size_t StrLen(const T* nts) // Fall-back strlen.
            {
                size_t result = 0;
                while (*nts != 0)
                {
                    ++nts;
                    ++result;
                }
                return result;
            }
            static size_t StrNLen(const T* ptr, size_t len) // Fall-back strnlen.
            {
                size_t result = 0;
                while (*ptr != 0 && result != len)
                {
                    ++ptr;
                    ++result;
                }
                return result;
            }
        };
        template<typename T>
        struct SCharacterTrait<T, sizeof(char)>
        {
            static size_t StrLen(const T* nts) // Narrow CRT strlen.
            {
                return ::strlen(SafeCast<const char*>(nts));
            }
            static size_t StrNLen(const T* ptr, size_t len) // Narrow CRT strnlen.
            {
                return ::strnlen(SafeCast<const char*>(ptr), len);
            }
        };
        template<typename T>
        struct SCharacterTrait<T, sizeof(wchar_t)>
        {
            static size_t StrLen(const T* nts) // Wide CRT strlen.
            {
                return ::wcslen(SafeCast<const wchar_t*>(nts));
            }
            static size_t StrNLen(const T* ptr, size_t len) // Wide CRT strnlen.
            {
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION UNICODEBINDING_H_SECTION_1
    #include AZ_RESTRICTED_FILE(UnicodeBinding_h) 
#endif
                return ::wcsnlen(SafeCast<const wchar_t*>(ptr), len);
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION UNICODEBINDING_H_SECTION_2
    #include AZ_RESTRICTED_FILE(UnicodeBinding_h)
#endif
            }
        };

        // void Feed(const SPackedIterators<InputIteratorType> &its, Sink &out, tag):
        // Feeds the provided sink from provided packed iterator-range.
        template<typename InputIteratorType, typename Sink>
        inline void Feed(const SPackedIterators<InputIteratorType>& its, Sink& out, integral_constant<EBind, eBind_Iterators>)
        {
            typedef typename std::iterator_traits<InputIteratorType>::value_type UnboundCharType;
            typedef typename SBindCharacter<UnboundCharType, true>::type BoundCharType;
            for (InputIteratorType it = its.begin; it != its.end; ++it)
            {
                const UnboundCharType unbound = *it;
                const BoundCharType bound = static_cast<BoundCharType>(unbound);
                const uint32 item = static_cast<uint32>(bound);
                out(item);
            }
        }

        // void Feed(const SPackedIterators<const InputCharType *> &its, Sink &out, tag):
        // Feeds the provided sink from provided packed pointer-range.
        // This is slightly better code-generation than using generic iterators.
        template<typename InputCharType, typename Sink>
        inline void Feed(const SPackedIterators<const InputCharType*>& its, Sink& out, integral_constant<EBind, eBind_Iterators>)
        {
            typedef typename SBindPointer<const InputCharType*, true>::type PointerType;
            assert(reinterpret_cast<size_t>(its.begin) <= reinterpret_cast<size_t>(its.end) && "Invalid range specified");
            const size_t length = its.end - its.begin;
            PointerType ptr = SafeCast<PointerType>(its.begin);
            assert((ptr || !length) && "Passed a non-empty range containing a null-pointer");
            for (size_t i = 0; i < length; ++i, ++ptr)
            {
                const uint32 item = static_cast<uint32>(*ptr);
                out(item);
            }
        }

        // void Feed(const InputStringType &in, Sink &out, tag):
        // Feeds the provided sink from a container, using it's iterators.
        // Note: Dispatches to one of the packed-range overloads.
        template<typename InputStringType, typename Sink>
        inline void Feed(const InputStringType& in, Sink& out, integral_constant<EBind, eBind_Iterators> tag)
        {
            typedef typename InputStringType::const_iterator IteratorType;
            Detail::SPackedIterators<IteratorType> its(in.begin(), in.end());
            Feed(its, out, tag);
        }

        // void Feed(const InputStringType &in, Sink &out, tag):
        // Feeds the provided sink from a string-object's buffer.
        template<typename InputStringType, typename Sink>
        inline void Feed(const InputStringType& in, Sink& out, integral_constant<EBind, eBind_Data>)
        {
            typedef typename InputStringType::size_type SizeType;
            typedef typename InputStringType::value_type ValueType;
            typedef typename SBindPointer<const ValueType*, true>::type PointerType;
            const SizeType length = in.size();
            if (length)
            {
                PointerType ptr = SafeCast<PointerType>(in.data());
                for (SizeType i = 0; i < length; ++i, ++ptr)
                {
                    const uint32 item = static_cast<uint32>(*ptr);
                    out(item);
                }
            }
        }

        // void Feed(const InputStringType &in, Sink &out, tag):
        // Feeds the provided sink from a string-literal.
        // Note: The literal is assumed to be null-terminated.
        // It's possible that a const-element fixed-size-buffer is mistaken as a literal.
        // However, we expect no-one uses such buffers that are not null-terminated already.
        // If somehow this use-case is desired, either terminate the buffer, or remove const from the buffer, or pass iterators.
        template<typename InputStringType, typename Sink>
        inline void Feed(const InputStringType& in, Sink& out, integral_constant<EBind, eBind_Literal>)
        {
            COMPILE_TIME_ASSERT(is_array<InputStringType>::value && extent<InputStringType>::value > 0);
            typedef typename SBindPointer<InputStringType, true>::type PointerType;
            const size_t length = extent<InputStringType>::value - 1;
            PointerType ptr = SafeCast<PointerType>(in);
            assert(ptr[length] == 0 && "Literal is not null-terminated");
            for (size_t i = 0; i < length; ++i, ++ptr)
            {
                const uint32 item = static_cast<uint32>(*ptr);
                out(item);
            }
        }

        // void Feed(const InputStringType &in, Sink &out, tag):
        // Feeds the provided sink from a non-const-element fixed-size buffer.
        // Note: The buffer is allowed to be null-terminated, but it's not required.
        template<typename InputStringType, typename Sink>
        inline void Feed(const InputStringType& in, Sink& out, integral_constant<EBind, eBind_Buffer>)
        {
            COMPILE_TIME_ASSERT(is_array<InputStringType>::value && extent<InputStringType>::value > 0);
            typedef typename SBindPointer<InputStringType, true>::type PointerType;
            typedef typename SBindPointer<InputStringType, true>::BoundCharType CharType;
            const size_t length = extent<InputStringType>::value;
            PointerType ptr = SafeCast<PointerType>(in);
            for (size_t i = 0; i < length; ++i, ++ptr)
            {
                const CharType unbound = *ptr;
                if (unbound == 0)
                {
                    break;
                }
                const uint32 item = static_cast<uint32>(unbound);
                out(item);
            }
        }

        // void Feed(const InputStringType &in, Sink &out, tag):
        // Feeds the provided sink from a null-terminated C-style string.
        template<typename InputStringType, typename Sink>
        inline void Feed(const InputStringType& in, Sink& out, integral_constant<EBind, eBind_NullTerminated>)
        {
            COMPILE_TIME_ASSERT(is_pointer<InputStringType>::value);
            typedef typename SBindPointer<InputStringType, true>::type PointerType;
            typedef typename SBindPointer<InputStringType, true>::BoundCharType CharType;
            PointerType ptr = SafeCast<PointerType>(in);
            if (ptr)
            {
                while (true)
                {
                    const CharType unbound = *ptr;
                    ++ptr;
                    if (unbound == 0)
                    {
                        break;
                    }
                    const uint32 item = static_cast<uint32>(unbound);
                    out(item);
                }
            }
        }

        // void Feed(const InputCharType &in, Sink &out, tag):
        // Feeds the provided sink from a single value (interpreted as an UCS code-point).
        template<typename InputCharType, typename Sink>
        inline void Feed(const InputCharType& in, Sink& out, integral_constant<EBind, eBind_CodePoint>)
        {
            COMPILE_TIME_ASSERT(is_arithmetic<InputCharType>::value);
            const uint32 item = static_cast<uint32>(in);
            out(item);
        }

        // size_t EncodedLength(const SPackedIterators<InputIteratorType> &its, tag):
        // Determines the length of the input sequence in a range of iterators.
        template<typename InputIteratorType>
        inline size_t EncodedLength(const SPackedIterators<InputIteratorType>& its, integral_constant<EBind, eBind_Iterators>)
        {
            return std::distance(its.begin, its.end); // std::distance will pick optimal implementation depending on iterator category.
        }

        // size_t EncodedLength(const InputStringType &in, tag):
        // Determines the length of an input container, which would otherwise be enumerated with iterators.
        template<typename InputStringType>
        inline size_t EncodedLength(const InputStringType& in, integral_constant<EBind, eBind_Iterators>)
        {
            return in.size(); // Can there be a container without size()? At the very least, not in the supported types.
        }

        // size_t EncodedLength(const InputStringType &in, tag):
        // Determines the length of the input container. The container uses contiguous element layout.
        template<typename InputStringType>
        inline size_t EncodedLength(const InputStringType& in, integral_constant<EBind, eBind_Data>)
        {
            return in.size();
        }

        // size_t EncodedLength(const InputStringType &in, tag):
        // Determines the length of the input string-literal. This is a compile-time constant.
        template<typename InputStringType>
        inline size_t EncodedLength(const InputStringType& in, integral_constant<EBind, eBind_Literal>)
        {
            COMPILE_TIME_ASSERT(is_array<InputStringType>::value && extent<InputStringType>::value > 0);
            return extent<InputStringType>::value - 1;
        }

        // size_t EncodedLength(const InputStringType &in, tag):
        // Determines the length of the input fixed-size-buffer. We look for an (optional) null-terminator in the buffer.
        template<typename InputStringType>
        inline size_t EncodedLength(const InputStringType& in, integral_constant<EBind, eBind_Buffer>)
        {
            COMPILE_TIME_ASSERT(is_array<InputStringType>::value && extent<InputStringType>::value > 0);
            typedef typename remove_extent<InputStringType>::type CharType;
            return SCharacterTrait<CharType>::StrNLen(in, extent<InputStringType>::value);
        }

        // size_t EncodedLength(const InputStringType &in, tag):
        // Determines the length of the input used-specified buffer. We look for an (optional) null-terminator in the buffer.
        template<typename InputCharType>
        inline size_t EncodedLength(const SPackedBuffer<InputCharType*>& in, integral_constant<EBind, eBind_PackedBuffer>)
        {
            return in.buffer ? SCharacterTrait<InputCharType>::StrNLen(in.buffer, in.size) : 0;
        }

        // size_t EncodedLength(const InputStringType &in, tag):
        // Determines the length of the input null-terminated c-style string. We just use strlen() if available.
        template<typename InputStringType>
        inline size_t EncodedLength(const InputStringType& in, integral_constant<EBind, eBind_NullTerminated>)
        {
            COMPILE_TIME_ASSERT(is_pointer<InputStringType>::value);
            typedef typename remove_pointer<InputStringType>::type CharType;
            return in ? SCharacterTrait<CharType>::StrLen(in) : 0;
        }

        // size_t EncodedLength(const InputCharType &in, tag):
        // Determines the length of a single UCS code-point. This is always 1.
        template<typename InputCharType>
        inline size_t EncodedLength([[maybe_unused]] const InputCharType& in, integral_constant<EBind, eBind_CodePoint>)
        {
            COMPILE_TIME_ASSERT(is_arithmetic<InputCharType>::value);
            return 1;
        }

        // const void *EncodedPointer(const SPackedIterators<const InputCharType *> &its, tag):
        // Get a pointer to contiguous storage for an iterator range.
        // Note: This can only work if the iterators are pointers, or the storage won't be guaranteed contiguous.
        template<typename InputCharType>
        inline const void* EncodedPointer(const SPackedIterators<const InputCharType*>& its, integral_constant<EBind, eBind_Iterators>)
        {
            return its.begin;
        }

        // const void *EncodedPointer(const InputStringType &in, tag):
        // Get a pointer to contiguous storage for string/vector object.
        // Note: This can only work for containers that actually use contiguous storage, which is determined by the SBindXXX helpers.
        template<typename InputStringType>
        inline const void* EncodedPointer(const InputStringType& in, integral_constant<EBind, eBind_Data>)
        {
            return in.data();
        }

        // const void *EncodedPointer(const InputStringType &in, tag):
        // Get a pointer to contiguous storage for a string-literal.
        template<typename InputStringType>
        inline const void* EncodedPointer(const InputStringType& in, integral_constant<EBind, eBind_Literal>)
        {
            COMPILE_TIME_ASSERT(is_array<InputStringType>::value && extent<InputStringType>::value > 0);
            return in; // We can just let the array type decay to a pointer.
        }

        // const void *EncodedPointer(const InputStringType &in, tag):
        // Get a pointer to contiguous storage for a fixed-size-buffer.
        template<typename InputStringType>
        inline const void* EncodedPointer(const InputStringType& in, integral_constant<EBind, eBind_Buffer>)
        {
            COMPILE_TIME_ASSERT(is_array<InputStringType>::value && extent<InputStringType>::value > 0);
            return in; // We can just let the array type decay to a pointer.
        }

        // const void *EncodedPointer(const InputStringType &in, tag):
        // Get a pointer to contiguous storage for a null-terminated c-style-string.
        template<typename InputStringType>
        inline const void* EncodedPointer(const InputStringType& in, integral_constant<EBind, eBind_NullTerminated>)
        {
            COMPILE_TIME_ASSERT(is_pointer<InputStringType>::value);
            return in; // Implied
        }

        // const void *EncodedPointer(const InputCharType &in, tag):
        // Get a pointer to contiguous storage for a single UCS code-point.
        template<typename InputCharType>
        inline const void* EncodedPointer(const InputCharType& in, integral_constant<EBind, eBind_CodePoint>)
        {
            COMPILE_TIME_ASSERT(is_arithmetic<InputCharType>::value);
            return &in; // Take the address of the parameter (which is kept on the stack of the caller).
        }

        // SWriteSink<T, Append, BindMethod>:
        // A helper that performs writing to the type T and can be passed as Sink type to a trans-coder helper.
        template<typename T, bool Append, EBind>
        struct SWriteSink;
        template<typename T, bool Append>
        struct SWriteSink<T, Append, eBind_Iterators>
        {
            typedef typename T::value_type OutputCharType;
            T& out;
            SWriteSink(T& _out, size_t)
                : out(_out)
            {
                if (!Append)
                {
                    // If not appending, clear the object beforehand.
                    out.clear();
                }
            }
            void operator()(uint32 item)
            {
                const OutputCharType bound = static_cast<OutputCharType>(item);
                out.push_back(bound); // We assume this can't fail and STL container takes care of memory.
            }
            void operator()(const void*, size_t);  // Not implemented.
            void HintSequence(uint32 length) {} // Don't care about sequences.
            bool CanWrite() const { return true; } // Always writable
        };
        template<typename T, bool Append>
        struct SWriteSink<T, Append, eBind_Data>
        {
            typedef SBindPointer<typename T::value_type*, false> BindHelper;
            typedef typename BindHelper::UnboundCharType CharType;
            CharType* ptr;
            SWriteSink(T& out, size_t length)
            {
                const size_t offset = Append ? out.size() : 0;
                length += offset;
                out.resize(length); // resize() can't fail without exceptions, so assert instead.
                assert((out.size() == length) && "Buffer resize failed (out-of-memory?)");
                const CharType* base = length ? out.data() : 0;
                ptr = const_cast<CharType*>(base + offset);
            }
            void operator()(uint32 item)
            {
                *SafeCast<typename BindHelper::type>(ptr) = static_cast<typename BindHelper::BoundCharType>(item);
                ++ptr;
            }
            void operator()(const void* src, size_t length)
            {
                ::memcpy(ptr, src, length * sizeof(CharType));
                ptr += length;
            }
            void HintSequence([[maybe_unused]] uint32 length) {} // Don't care about sequences.
            bool CanWrite() const { return true; } // Always writable
        };
        template<typename P, bool Append>
        struct SWriteSink<SPackedBuffer<P>, Append, eBind_PackedBuffer>
        {
            typedef typename remove_pointer<P>::type ElementType;
            typedef SBindPointer<ElementType*, false> BindHelper;
            typedef typename BindHelper::UnboundCharType CharType;
            CharType* ptr;
            CharType* const terminator;
            SWriteSink(CharType* _terminator)
                : terminator(_terminator) {}
            SWriteSink(SPackedBuffer<P>& out, size_t)
                : terminator(out.size && out.buffer ? out.buffer + out.size - 1 : 0)
            {
                const size_t offset = Append
                    ? EncodedLength(out, integral_constant<EBind, eBind_PackedBuffer>())
                    : 0;
                const size_t fixedOffset = Append && offset >= out.size
                    ? out.size - 1 // In case the buffer is already full and not terminated.
                    : offset;
                CharType* base = static_cast<CharType*>(out.buffer);
                ptr = terminator ? base + fixedOffset : 0;
            }
            ~SWriteSink()
            {
                if (ptr)
                {
                    *ptr = 0; // Guarantees that the output is null-terminated.
                }
            }
            void operator()(uint32 item)
            {
                if (ptr != terminator) // Guarantees we don't overflow the buffer.
                {
                    *SafeCast<typename BindHelper::type>(ptr) = static_cast<typename BindHelper::BoundCharType>(item);
                    ++ptr;
                }
            }
            void operator()(const void* src, size_t length)
            {
                const size_t maxLength = terminator - ptr;
                if (length > maxLength)
                {
                    length = maxLength;
                }
                ::memcpy(ptr, src, length * sizeof(CharType));
                ptr += length;
            }
            void HintSequence(uint32 length)
            {
                if (terminator && (ptr + length >= terminator))
                {
                    // This sequence will overflow the buffer.
                    // In this case, we prefer to not generate any part of the sequence.
                    // Terminate at the current position and flag as full.
                    *ptr = 0;
                    ptr = terminator;
                }
            }
            bool CanWrite() const
            {
                return terminator != ptr;
            }
        };
        template<typename T, bool Append>
        struct SWriteSink<T, Append, eBind_Buffer> // Uses above implementation with specialized constructor
            : SWriteSink<SPackedBuffer<typename remove_extent<T>::type*>, Append, eBind_PackedBuffer>
        {
            typedef typename remove_extent<T>::type ElementType;
            typedef SWriteSink<SPackedBuffer<ElementType*>, Append, eBind_PackedBuffer> Super;
            typedef SBindPointer<ElementType*, false> BindHelper;
            typedef typename BindHelper::UnboundCharType CharType;
            SWriteSink(T& out, size_t)
                : Super(out + extent<T>::value - 1)
            {
                const size_t offset = Append
                    ? EncodedLength(out, integral_constant<EBind, eBind_Buffer>())
                    : 0;
                const size_t fixedOffset = Append && offset >= extent<T>::value
                    ? extent<T>::value - 1 // In case the buffer is already full and not terminated.
                    : offset;
                Super::ptr = out + fixedOffset; // Qualification for Super required for two-phase lookup.
            }
        };

        // SIsBlockCopyable<InputType, OutputType>:
        // Check if block-copy optimization is possible for these types.
        // InputType should be an instantiation of SBindObject or SBindIterator.
        // OutputType should be an instantiation of SBindOutput.
        // Note: This doesn't take into account safe/unsafe conversions, just if the underlying storage types are compatible.
        template<typename InputType, typename OutputType>
        struct SIsBlockCopyable
        {
            template<EBind M>
            struct SIsContiguous
            {
                static const bool value =
                    M == eBind_Data ||
                    M == eBind_Literal ||
                    M == eBind_Buffer ||
                    M == eBind_PackedBuffer ||
                    M == eBind_NullTerminated ||
                    M == eBind_CodePoint;
            };
            template<typename T>
            struct SIsPointers
            {
                static const bool value = false;
            };
            template<typename T>
            struct SIsPointers<SPackedIterators<T*> >
            {
                static const bool value = true;
            };
            typedef typename SBindCharacter<typename InputType::CharType, true>::type InputCharType;
            typedef typename SBindCharacter<typename OutputType::CharType, false>::type OutputCharType;
            static const bool isIntegral = is_integral<InputCharType>::value && is_integral<OutputCharType>::value;
            static const bool isSameSize = sizeof(InputCharType) == sizeof(OutputCharType);
            static const bool isInputContiguous = (SIsContiguous<InputType::value>::value || SIsPointers<InputType>::value);
            static const bool isOutputContiguous = (SIsContiguous<OutputType::value>::value || SIsPointers<OutputType>::value);
            static const bool value = isIntegral && isSameSize && isInputContiguous && isOutputContiguous;
        };
    }
}
