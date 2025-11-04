/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/base.h>
#include <AzCore/std/typetraits/integral_constant.h>
#include <AzCore/std/typetraits/void_t.h>
#include <AzCore/std/createdestroy.h>
#include <AzCore/std/limits.h>

namespace AZStd
{
    inline namespace AllocatorInternal
    {
        //! value_type
        template <typename Allocator, typename = void>
        struct get_value_type
        {
            using type = uint8_t;
        };
        template <typename Allocator>
        struct get_value_type<Allocator, void_t<typename Allocator::value_type>>
        {
            using type = typename Allocator::value_type;
        };

        //! pointer
        //! Attempts to use the C++ standard allocator_type::pointer typedef if available,
        //! otherwise attempt to fallback to the AZStd pointer typedef.
        //! if neither typedef is available fallbacks to value_type*
        template <typename Allocator, typename = void>
        static constexpr bool has_pointer = false;
        template <typename Allocator>
        static constexpr bool has_pointer<Allocator, void_t<typename Allocator::pointer>> = true;

        template <typename Allocator, typename = void>
        static constexpr bool has_pointer_type = false;
        template <typename Allocator>
        static constexpr bool has_pointer_type<Allocator, void_t<typename Allocator::pointer>> = true;

        template <typename Allocator, typename ValueType, bool has_std_pointer_alias = has_pointer<Allocator>, bool has_azstd_pointer_alias = has_pointer_type<Allocator>>
        struct get_pointer_type
        {
            using type = ValueType*;
        };
        template <typename Allocator, typename ValueType, bool has_azstd_pointer_alias>
        struct get_pointer_type<Allocator, ValueType, true, has_azstd_pointer_alias>
        {
            using type = typename Allocator::pointer;
        };
        template <typename Allocator, typename ValueType>
        struct get_pointer_type<Allocator, ValueType, false, true>
        {
            using type = typename Allocator::pointer;
        };

        //! const_pointer
        template <typename Allocator, typename PointerType, typename ValueType, typename = void>
        struct get_const_pointer_type
        {
            using type = typename std::pointer_traits<PointerType>::template rebind<const ValueType>;
        };
        template <typename Allocator, typename PointerType, typename ValueType>
        struct get_const_pointer_type<Allocator, PointerType, ValueType, void_t<typename Allocator::const_pointer>>
        {
            using type = typename Allocator::const_pointer;
        };

        //! void_pointer
        template <typename Allocator, typename PointerType, typename = void>
        struct get_void_pointer_type
        {
            using type = typename std::pointer_traits<PointerType>::template rebind<void>;
        };
        template <typename Allocator, typename PointerType>
        struct get_void_pointer_type<Allocator, PointerType, void_t<typename Allocator::void_pointer>>
        {
            using type = typename Allocator::void_pointer;
        };

        //! const_void_pointer
        template <typename Allocator, typename PointerType, typename = void>
        struct get_const_void_pointer_type
        {
            using type = typename std::pointer_traits<PointerType>::template rebind<const void>;
        };
        template <typename Allocator, typename PointerType>
        struct get_const_void_pointer_type<Allocator, PointerType, void_t<typename Allocator::const_void_pointer>>
        {
            using type = typename Allocator::const_void_pointer;
        };

        //! difference_type
        template <typename Allocator, typename PointerType, typename = void>
        struct get_difference_type
        {
            using type = typename std::pointer_traits<PointerType>::difference_type;
        };
        template <typename Allocator, typename PointerType>
        struct get_difference_type<Allocator, PointerType, void_t<typename Allocator::difference_type>>
        {
            using type = typename Allocator::difference_type;
        };

        //! align_type
        template<typename Allocator, typename AlignType, typename = void >
        struct get_align_type
        {
            using type = typename std::make_unsigned_t<AlignType>;
        };
        template<typename Allocator, typename AlignType>
        struct get_align_type<Allocator, AlignType, void_t<typename Allocator::align_type>>
        {
            using type = typename Allocator::align_type;
        };

        //! size_type
        template <typename Allocator, typename DifferenceType, typename = void>
        struct get_size_type
        {
            using type = typename std::make_unsigned<DifferenceType>::type;
        };
        template <typename Allocator, typename DifferenceType>
        struct get_size_type<Allocator, DifferenceType, void_t<typename Allocator::size_type>>
        {
            using type = typename Allocator::size_type;
        };

        //! propagate_on_container_copy_assignment
        template <typename Allocator, typename = void>
        struct get_propagate_on_container_copy_assignment_type
        {
            using type = AZStd::true_type;
        };
        template <typename Allocator>
        struct get_propagate_on_container_copy_assignment_type<Allocator, void_t<typename Allocator::propagate_on_container_copy_assignment>>
        {
            using type = typename Allocator::propagate_on_container_copy_assignment;
        };

        //! propagate_on_container_move_assignment
        template <typename Allocator, typename = void>
        struct get_propagate_on_container_move_assignment_type
        {
            using type = AZStd::true_type;
        };
        template <typename Allocator>
        struct get_propagate_on_container_move_assignment_type<Allocator, void_t<typename Allocator::propagate_on_container_move_assignment>>
        {
            using type = typename Allocator::propagate_on_container_move_assignment;
        };

        //! propagate_on_container_swap
        template <typename Allocator, typename = void>
        struct get_propagate_on_container_swap_type
        {
            using type = AZStd::true_type;
        };
        template <typename Allocator>
        struct get_propagate_on_container_swap_type<Allocator, void_t<typename Allocator::propagate_on_container_swap>>
        {
            using type = typename Allocator::propagate_on_container_swap;
        };

        //! is_always_equal
        template <typename Allocator, typename = void>
        struct get_is_always_equal_type
        {
            using type = typename std::is_empty<Allocator>::type;
        };
        template <typename Allocator>
        struct get_is_always_equal_type<Allocator, void_t<typename Allocator::is_always_equal>>
        {
            using type = typename Allocator::is_always_equal;
        };

        //! rebind
        template <typename Allocator, typename NewType, typename = void>
        struct has_rebind
            : false_type
        {};
        template <typename Allocator, typename NewType>
        struct has_rebind<Allocator, NewType, void_t<typename Allocator::template rebind<NewType>::other>>
            : true_type
        {};
        //! allocator_type is not a template nor does it have a rebind alias template therefore the allocator_type will be used directly
        template <typename Allocator, typename NewType, bool = has_rebind<Allocator, NewType>::value>
        struct get_rebind_type
        {
            using type = Allocator;
        };
        //! allocator_type has rebind alias template, so it will be used
        template <typename Allocator, typename NewType>
        struct get_rebind_type<Allocator, NewType, true>
        {
            using type = typename Allocator::template rebind<NewType>::other;
        };
        //! allocator_type is a class template and does not have rebind alias template so replace the first argument with the new type
        template <template <typename, typename...> class Allocator, typename T, typename... Args, typename NewType>
        struct get_rebind_type <Allocator<T, Args...>, NewType, false>
        {
            using type = Allocator<NewType, Args...>;
        };

        //! max_size
        template <class Allocator>
        static auto has_max_size_test(Allocator&& alloc) -> decltype(alloc.max_size(), true_type());
        template <class Allocator>
        static auto has_max_size_test(const volatile Allocator&)->false_type;
        template <typename Allocator>
        static constexpr bool has_max_size = integral_constant<bool, is_same<decltype(has_max_size_test(declval<Allocator&>())), true_type>::value>::value;

        template <class Allocator>
        static auto has_get_max_size_test(Allocator&& alloc) -> decltype(alloc.get_max_size(), true_type());
        template <class Allocator>
        static auto has_get_max_size_test(const volatile Allocator&)->false_type;
        template <typename Allocator>
        static constexpr bool has_get_max_size = integral_constant<bool, is_same<decltype(has_get_max_size_test(declval<Allocator&>())), true_type>::value>::value;
        // Prefer the C++ standard max_size function before the AZStd get_max_size function
        template <typename Allocator, typename ValueType, typename SizeType, bool max_size_callable, bool get_max_size_callable>
        struct call_max_size
        {
            static constexpr SizeType invoke(const Allocator&)
            {
                return (std::numeric_limits<SizeType>::max)() / sizeof(ValueType);
            }
        };
        template <typename Allocator, typename ValueType, typename SizeType, bool get_max_size_callable>
        struct call_max_size<Allocator, ValueType, SizeType, true, get_max_size_callable>
        {
            static SizeType invoke(const Allocator& alloc)
            {
                return alloc.max_size();
            }
        };
        template <typename Allocator, typename ValueType, typename SizeType>
        struct call_max_size<Allocator, ValueType, SizeType, false, true>
        {
            static SizeType invoke(const Allocator& alloc)
            {
                return alloc.get_max_size();
            }
        };

        //! select_on_container_copy_construction
        template <class Allocator>
        static auto has_select_on_container_copy_construction_test(Allocator&& alloc) -> decltype(alloc.select_on_container_copy_construction(), true_type());
        template <class Allocator>
        static auto has_select_on_container_copy_construction_test(const volatile Allocator&) -> false_type;
        template <typename Allocator, typename = void>
        static constexpr bool has_select_on_container_copy_construction = integral_constant<bool, is_same<decltype(has_select_on_container_copy_construction_test(declval<Allocator&>())), true_type>::value>::value;

        template <typename Allocator, bool select_on_container_copy_construction_callable>
        struct call_select_on_container_copy_construction
        {
            static Allocator invoke(const Allocator& alloc)
            {
                return alloc;
            }
        };
        template <typename Allocator>
        struct call_select_on_container_copy_construction<Allocator, true>
        {
            static Allocator invoke(const Allocator& alloc)
            {
                return alloc.select_on_container_copy_construction();
            }
        };
    }
    /*
    The allocator_traits class template provides the standardized way to access various properties of AZStd allocators
    */
    template <class Alloc>
    struct allocator_traits
    {
        using allocator_type = Alloc;
        using value_type = typename get_value_type<allocator_type>::type;
        using pointer = typename get_pointer_type<allocator_type, value_type>::type;
        using const_pointer = typename get_const_pointer_type<allocator_type, pointer, value_type>::type;
        using void_pointer = typename get_void_pointer_type<allocator_type, pointer>::type;
        using const_void_pointer = typename get_const_void_pointer_type<allocator_type, pointer>::type;
        using difference_type = typename get_difference_type<allocator_type, pointer>::type;
        using align_type = typename get_align_type<allocator_type, difference_type>::type;
        using size_type = typename get_size_type<allocator_type, difference_type>::type;
        using propagate_on_container_move_assignment = typename get_propagate_on_container_move_assignment_type<allocator_type>::type;
        using propagate_on_container_copy_assignment = typename get_propagate_on_container_copy_assignment_type<allocator_type>::type;
        using propagate_on_container_swap = typename get_propagate_on_container_swap_type<allocator_type>::type;
        using is_always_equal = typename get_is_always_equal_type<allocator_type>::type;

        template <class T> using rebind_alloc = typename get_rebind_type<allocator_type, T>::type;
        template <class T> using rebind_traits = allocator_traits<rebind_alloc<T>>;

        // static allocator_trait functions
        static pointer allocate(allocator_type& alloc, size_type size)
        {
            return allocate(alloc, size, alignof(value_type));
        }
        static pointer allocate(allocator_type& alloc, size_type size, const_void_pointer hint)
        {
            return allocate(alloc, size, alignof(value_type));
        }
        //! Extension AZStd allocators supports supplying alignment
        static pointer allocate(allocator_type& alloc, size_type size, align_type alignment)
        {
            return alloc.allocate(size, alignment);
        }

        static void deallocate(allocator_type& alloc, pointer ptr, size_type size)
        {
            deallocate(alloc, ptr, size, alignof(value_type));
        }

        //! Extension AZStd allocators supports supplying alignment
        static void deallocate(allocator_type& alloc, pointer ptr, size_type size, align_type alignment)
        {
            alloc.deallocate(ptr, size, alignment);
        }

        template <class T, class... Args>
        static void construct(allocator_type&, T* ptr, Args&&... args)
        {
            Internal::construct<T*>::single(ptr, AZStd::forward<Args>(args)...);
        }

        template <class T>
        static void destroy(allocator_type&, T* ptr)
        {
            Internal::destroy<T*>::single(ptr);
        }

        static size_type max_size(const allocator_type& alloc)
        {
            return call_max_size<allocator_type, value_type, size_type, has_max_size<allocator_type>, has_get_max_size<allocator_type>>::invoke(alloc);
        }

        static allocator_type select_on_container_copy_construction(const allocator_type& alloc)
        {
            return call_select_on_container_copy_construction<allocator_type, has_select_on_container_copy_construction<allocator_type>>::invoke(alloc);
        }
    };
}
