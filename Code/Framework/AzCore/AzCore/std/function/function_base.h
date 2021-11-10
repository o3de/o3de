/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// Based on boost 1.39.0

#ifndef AZSTD_FUNCTION_BASE_HEADER
#define AZSTD_FUNCTION_BASE_HEADER

#include <AzCore/std/allocator.h>
#include <AzCore/std/base.h>
#include <AzCore/std/utils.h>
#include <AzCore/std/function/function_fwd.h>
#include <AzCore/std/typetraits/type_id.h>
#include <AzCore/std/typetraits/alignment_of.h>
#include <AzCore/std/typetraits/is_member_pointer.h>
#include <AzCore/std/typetraits/is_const.h>
#include <AzCore/std/typetraits/remove_cvref.h>
#include <AzCore/std/typetraits/is_volatile.h>
#include <AzCore/std/createdestroy.h>

#define AZSTD_FUNCTION_TARGET_FIX(x)
#define AZSTD_FUNCTION_ENABLE_IF_NOT_INTEGRAL(Functor, Type)  AZStd::enable_if_t<!std::is_integral_v<Functor> && !std::is_null_pointer_v<Functor>, Type>



namespace AZStd
{
    namespace Internal
    {
        namespace function_util
        {
            class X;

            /**
             * A buffer used to store small function objects in
             * AZStd::function. It is a union containing function pointers,
             * object pointers, and a structure that resembles a bound
             * member function pointer.
             */
            union function_buffer
            {
                // For pointers to function objects
                mutable void* obj_ptr;

                // For pointers to AZStd::type_id objects
                struct type_t
                {
                    // (get_functor_type_tag, check_functor_type_tag).
                    type_id type;

                    // Whether the type is const-qualified.
                    bool const_qualified;
                    // Whether the type is volatile-qualified.
                    bool volatile_qualified;
                } type;

                // For function pointers of all kinds
                mutable void (* func_ptr)();

                // For bound member pointers
                struct bound_memfunc_ptr_t
                {
                    void (X::* memfunc_ptr)(int);
                    void* obj_ptr;
                } bound_memfunc_ptr{};

                // For references to function objects. We explicitly keep
                // track of the cv-qualifiers on the object referenced.
                struct obj_ref_t
                {
                    mutable void* obj_ptr;
                    bool is_const_qualified;
                    bool is_volatile_qualified;
                } obj_ref;

                // To relax aliasing constraints
                mutable char data;
            };

            /**
             * The unusable class is a placeholder for unused function arguments
             * It is also completely unusable except that it constructable from
             * anything. This helps compilers without partial specialization to
             * handle Boost.Function objects returning void.
             */
            struct unusable
            {
                unusable() {}
                template<typename T>
                unusable(const T&) {}
            };

            /* Determine the return type. This supports compilers that do not support
             * void returns or partial specialization by silently changing the return
             * type to "unusable".
             */
            template<typename T>
            struct function_return_type
            {
                typedef T type;
            };

            template<>
            struct function_return_type<void>
            {
                typedef unusable type;
            };

            // The operation type to perform on the given functor/function pointer
            enum functor_manager_operation_type
            {
                clone_functor_tag,
                move_functor_tag,
                destroy_functor_tag,
                check_functor_type_tag,
                get_functor_type_tag
            };

            // Tags used to decide between different types of functions
            struct function_ptr_tag {};
            struct function_obj_tag {};
            struct member_ptr_tag {};
            struct function_obj_ref_tag {};

            template<typename F>
            class get_function_tag
            {
                typedef typename Utils::if_c<(is_pointer<F>::value),
                    function_ptr_tag,
                    function_obj_tag>::type ptr_or_obj_tag;

                typedef typename Utils::if_c<(is_member_pointer<F>::value),
                    member_ptr_tag,
                    ptr_or_obj_tag>::type ptr_or_obj_or_mem_tag;

                typedef typename Utils::if_c<(is_reference_wrapper<F>::value),
                    function_obj_ref_tag,
                    ptr_or_obj_or_mem_tag>::type or_ref_tag;

            public:
                typedef or_ref_tag type;
            };

            // The trivial manager does nothing but return the same pointer (if we
            // are cloning) or return the null pointer (if we are deleting).
            template<typename F>
            struct reference_manager
            {
                static inline void
                manage(const function_buffer& in_buffer, function_buffer& out_buffer,
                    functor_manager_operation_type op)
                {
                    switch (op)
                    {
                    case clone_functor_tag:
                        out_buffer.obj_ref.obj_ptr = in_buffer.obj_ref.obj_ptr;
                        return;

                    case move_functor_tag:
                        out_buffer.obj_ref.obj_ptr = in_buffer.obj_ref.obj_ptr;
                        in_buffer.obj_ref.obj_ptr = 0;
                        return;

                    case destroy_functor_tag:
                        out_buffer.obj_ref.obj_ptr = 0;
                        return;

                    case check_functor_type_tag:
                    {
                        const type_id& check_type = out_buffer.type.type;

                        // Check whether we have the same type. We can add
                        // cv-qualifiers, but we can't take them away.
                        if (aztypeid_cmp(check_type, aztypeid(F))
                            && (!in_buffer.obj_ref.is_const_qualified
                                || out_buffer.type.const_qualified)
                            && (!in_buffer.obj_ref.is_volatile_qualified
                                || out_buffer.type.volatile_qualified))
                        {
                            out_buffer.obj_ptr = in_buffer.obj_ref.obj_ptr;
                        }
                        else
                        {
                            out_buffer.obj_ptr = 0;
                        }
                    }
                        return;

                    case get_functor_type_tag:
                        out_buffer.type.type = aztypeid(F);
                        out_buffer.type.const_qualified = in_buffer.obj_ref.is_const_qualified;
                        out_buffer.type.volatile_qualified = in_buffer.obj_ref.is_volatile_qualified;
                        return;
                    }
                }
            };

            /**
             * Determine if AZStd::function can use the small-object
             * optimization with the function object type F.
             */
            template<typename F>
            struct function_allows_small_object_optimization
            {
                static constexpr bool value = sizeof(F) <= sizeof(function_buffer) && (alignof(function_buffer) % alignof(F) == 0);
            };

            template <typename F, typename A>
            struct functor_wrapper
                : public F
                , public A
            {
                template <typename Functor>
                functor_wrapper(Functor&& f, A a)
                    : F(AZStd::forward<Functor>(f))
                    , A(a)
                {}
            };

            /**
             * The functor_manager class contains a static function "manage" which
             * can clone or destroy the given function/function object pointer.
             */
            template<typename Functor>
            struct functor_manager_common
            {
                typedef AZStd::decay_t<Functor> functor_type;

                // Function pointers
                static inline void
                manage_ptr(const function_buffer& in_buffer, function_buffer& out_buffer,
                    functor_manager_operation_type op)
                {
                    if (op == clone_functor_tag)
                    {
                        out_buffer.func_ptr = in_buffer.func_ptr;
                    }
                    else if (op == move_functor_tag)
                    {
                        out_buffer.func_ptr = in_buffer.func_ptr;
                        in_buffer.func_ptr = 0;
                    }
                    else if (op == destroy_functor_tag)
                    {
                        out_buffer.func_ptr = 0;
                    }
                    else if (op == check_functor_type_tag)
                    {
                        const type_id& check_type = out_buffer.type.type;
                        if (aztypeid_cmp(check_type, aztypeid(functor_type)))
                        {
                            out_buffer.obj_ptr = &in_buffer.func_ptr;
                        }
                        else
                        {
                            out_buffer.obj_ptr = 0;
                        }
                    }
                    else /* op == get_functor_type_tag */
                    {
                        out_buffer.type.type = aztypeid(functor_type);
                        out_buffer.type.const_qualified = false;
                        out_buffer.type.volatile_qualified = false;
                    }
                }

                // Function objects that fit in the small-object buffer.
                static inline void
                manage_small(const function_buffer& in_buffer, function_buffer& out_buffer, functor_manager_operation_type op)
                {
                    if (op == clone_functor_tag || op == move_functor_tag)
                    {

                        functor_type* in_functor =
                            reinterpret_cast<functor_type*>(&in_buffer.data);
                        if (op == clone_functor_tag)
                        {
                            new ((void*)&out_buffer.data)functor_type(*in_functor);
                        }
                        else if (op == move_functor_tag)
                        {
                            new ((void*)&out_buffer.data)functor_type(AZStd::move(*in_functor));
                            // Casting via union to get around compiler warnings (strict type on GCC, unused variable on MSVC)
                            union
                            {
                                char* cast_data;
                                functor_type* cast_functor;
                            };
                            cast_data = &in_buffer.data;
                            cast_functor->~functor_type();
                        }
                    }
                    else if (op == destroy_functor_tag)
                    {
                        // Some compilers (Borland, vc6, ...) are unhappy with ~functor_type.
                        // Casting via union to get around compiler warnings (strict type on GCC, unused variable on MSVC)
                        union
                        {
                            char* cast_data;
                            functor_type* cast_functor;
                        };
                        cast_data = &out_buffer.data;
                        cast_functor->~functor_type();
                    }
                    else if (op == check_functor_type_tag)
                    {
                        const type_id& check_type = out_buffer.type.type;
                        if (aztypeid_cmp(check_type, aztypeid(functor_type)))
                        {
                            out_buffer.obj_ptr = &in_buffer.data;
                        }
                        else
                        {
                            out_buffer.obj_ptr = 0;
                        }
                    }
                    else /* op == get_functor_type_tag */
                    {
                        out_buffer.type.type = aztypeid(functor_type);
                        out_buffer.type.const_qualified = false;
                        out_buffer.type.volatile_qualified = false;
                    }
                }
            };

            template<typename Functor>
            struct functor_manager
            {
            private:
                typedef AZStd::decay_t<Functor> functor_type;

                // Function pointers
                static inline void
                manager(const function_buffer& in_buffer, function_buffer& out_buffer, functor_manager_operation_type op, function_ptr_tag)
                {
                    functor_manager_common<functor_type>::manage_ptr(in_buffer, out_buffer, op);
                }

                // Function objects that fit in the small-object buffer.
                static inline void
                manager(const function_buffer& in_buffer, function_buffer& out_buffer, functor_manager_operation_type op, AZStd::true_type)
                {
                    functor_manager_common<functor_type>::manage_small(in_buffer, out_buffer, op);
                }

                // Function objects that require heap allocation
                static inline void
                manager(const function_buffer& in_buffer, function_buffer& out_buffer, functor_manager_operation_type op, AZStd::false_type)
                {
                    if (op == clone_functor_tag)
                    {
                        // Clone the functor
                        // GCC 2.95.3 gets the CV qualifiers wrong here, so we
                        // can't do the static_cast that we should do.
                        const functor_type* f = (const functor_type*)(in_buffer.obj_ptr);
                        // \todo provide a more generic way to supply utility allocators
                        AZStd::allocator a;
                        functor_type* new_f = new (a.allocate(sizeof(functor_type), AZStd::alignment_of<functor_type>::value))functor_type(*f);
                        out_buffer.obj_ptr = new_f;
                    }
                    else if (op == move_functor_tag)
                    {
                        out_buffer.obj_ptr = in_buffer.obj_ptr;
                        in_buffer.obj_ptr = 0;
                    }
                    else if (op == destroy_functor_tag)
                    {
                        /* Cast from the void pointer to the functor pointer type */
                        functor_type* f = static_cast<functor_type*>(out_buffer.obj_ptr);
                        Internal::destroy<functor_type*>::single(f);
                        AZStd::allocator a;
                        a.deallocate(f, 0, 0);
                        out_buffer.obj_ptr = 0;
                    }
                    else if (op == check_functor_type_tag)
                    {
                        const type_id& check_type = out_buffer.type.type;
                        if (aztypeid_cmp(check_type, aztypeid(functor_type)))
                        {
                            out_buffer.obj_ptr = in_buffer.obj_ptr;
                        }
                        else
                        {
                            out_buffer.obj_ptr = 0;
                        }
                    }
                    else /* op == get_functor_type_tag */
                    {
                        out_buffer.type.type = aztypeid(functor_type);
                        out_buffer.type.const_qualified = false;
                        out_buffer.type.volatile_qualified = false;
                    }
                }

                // For function objects, we determine whether the function
                // object can use the small-object optimization buffer or
                // whether we need to allocate it on the heap.
                static inline void
                manager(const function_buffer& in_buffer, function_buffer& out_buffer, functor_manager_operation_type op, function_obj_tag)
                {
                    manager(in_buffer, out_buffer, op, AZStd::integral_constant<bool, function_allows_small_object_optimization<functor_type>::value >());
                }

                // For member pointers, we use the small-object optimization buffer.
                static inline void
                manager(const function_buffer& in_buffer, function_buffer& out_buffer, functor_manager_operation_type op, member_ptr_tag)
                {
                    manager(in_buffer, out_buffer, op, AZStd::true_type());
                }

            public:
                /* Dispatch to an appropriate manager based on whether we have a
                   function pointer or a function object pointer. */
                static inline void
                manage(const function_buffer& in_buffer, function_buffer& out_buffer,
                    functor_manager_operation_type op)
                {
                    typedef typename get_function_tag<functor_type>::type tag_type;
                    switch (op)
                    {
                    case get_functor_type_tag:
                        out_buffer.type.type = aztypeid(functor_type);
                        out_buffer.type.const_qualified = false;
                        out_buffer.type.volatile_qualified = false;
                        return;

                    default:
                        manager(in_buffer, out_buffer, op, tag_type());
                        return;
                    }
                }
            };

            template<typename Functor, typename Allocator>
            struct functor_manager_a
            {
            private:
                typedef AZStd::decay_t<Functor> functor_type;

                // Function pointers
                static inline void
                manager(const function_buffer& in_buffer, function_buffer& out_buffer, functor_manager_operation_type op, function_ptr_tag)
                {
                    functor_manager_common<functor_type>::manage_ptr(in_buffer, out_buffer, op);
                }

                // Function objects that fit in the small-object buffer.
                static inline void
                manager(const function_buffer& in_buffer, function_buffer& out_buffer, functor_manager_operation_type op, AZStd::true_type)
                {
                    functor_manager_common<functor_type>::manage_small(in_buffer, out_buffer, op);
                }

                // Function objects that require heap allocation
                static inline void
                manager(const function_buffer& in_buffer, function_buffer& out_buffer, functor_manager_operation_type op, AZStd::false_type)
                {
                    typedef functor_wrapper<functor_type, Allocator> functor_wrapper_type;

                    if (op == clone_functor_tag)
                    {
                        // Clone the functor
                        // GCC 2.95.3 gets the CV qualifiers wrong here, so we
                        // can't do the static_cast that we should do.
                        functor_wrapper_type* f = static_cast<functor_wrapper_type*>(in_buffer.obj_ptr);
                        functor_wrapper_type* new_f = new (f->allocate(sizeof(functor_wrapper_type), AZStd::alignment_of<functor_wrapper_type>::value))functor_wrapper_type(*f);
                        out_buffer.obj_ptr = new_f;
                    }
                    else if (op == move_functor_tag)
                    {
                        out_buffer.obj_ptr = in_buffer.obj_ptr;
                        in_buffer.obj_ptr = 0;
                    }
                    else if (op == destroy_functor_tag)
                    {
                        /* Cast from the void pointer to the functor_wrapper_type */
                        functor_wrapper_type* victim = static_cast<functor_wrapper_type*>(in_buffer.obj_ptr);
                        Allocator wrapper_allocator(static_cast<Allocator const&>(*victim)); // copy the allocator
                        Internal::destroy<functor_wrapper_type*>::single(victim);
                        wrapper_allocator.deallocate(victim, sizeof(functor_wrapper_type), AZStd::alignment_of<functor_wrapper_type>::value);
                        out_buffer.obj_ptr = 0;
                    }
                    else if (op == check_functor_type_tag)
                    {
                        const type_id& check_type = out_buffer.type.type;
                        if (aztypeid_cmp(check_type, aztypeid(functor_type)))
                        {
                            out_buffer.obj_ptr = in_buffer.obj_ptr;
                        }
                        else
                        {
                            out_buffer.obj_ptr = 0;
                        }
                    }
                    else /* op == get_functor_type_tag */
                    {
                        out_buffer.type.type = aztypeid(functor_type);
                        out_buffer.type.const_qualified = false;
                        out_buffer.type.volatile_qualified = false;
                    }
                }

                // For function objects, we determine whether the function
                // object can use the small-object optimization buffer or
                // whether we need to allocate it on the heap.
                static inline void
                manager(const function_buffer& in_buffer, function_buffer& out_buffer, functor_manager_operation_type op, function_obj_tag)
                {
                    manager(in_buffer, out_buffer, op, AZStd::integral_constant<bool, function_allows_small_object_optimization<functor_type>::value>());
                }

            public:
                /* Dispatch to an appropriate manager based on whether we have a
                   function pointer or a function object pointer. */
                static inline void
                manage(const function_buffer& in_buffer, function_buffer& out_buffer,
                    functor_manager_operation_type op)
                {
                    typedef typename get_function_tag<functor_type>::type tag_type;
                    switch (op)
                    {
                    case get_functor_type_tag:
                        out_buffer.type.type = aztypeid(functor_type);
                        out_buffer.type.const_qualified = false;
                        out_buffer.type.volatile_qualified = false;
                        return;

                    default:
                        manager(in_buffer, out_buffer, op, tag_type());
                        return;
                    }
                }
            };

            // A type that is only used for comparisons against zero
            struct useless_clear_type {};

            /**
             * Stores the "manager" portion of the vtable for a
             * AZStd::function object.
             */
            struct vtable_base
            {
                typedef void type (const function_buffer&, function_buffer&, functor_manager_operation_type);
                void (* manager)(const function_buffer& in_buffer,
                    function_buffer& out_buffer,
                    functor_manager_operation_type op);
            };
        } // end namespace function_util
    } // end namespace Internal

    /**
     * The function_base class contains the basic elements needed for the
     * function1, function2, function3, etc. classes. It is common to all
     * functions (and as such can be used to tell if we have one of the
     * functionN objects).
     */
    class function_base
    {
    public:
        function_base()
            : vtable(0) { }

        /** Determine if the function is empty (i.e., has no target). */
        bool empty() const { return !vtable; }

        /** Retrieve the type of the stored function object, or aztypeid(void)
            if this is empty. */
        type_id target_type() const
        {
            if (!vtable)
            {
                return aztypeid(void);
            }

            Internal::function_util::function_buffer type;
            vtable->manager(functor, type, Internal::function_util::get_functor_type_tag);
            return type.type.type;
        }

        template<typename Functor>
        Functor* target()
        {
            if (!vtable)
            {
                return 0;
            }

            Internal::function_util::function_buffer type_result;
            type_result.type.type = aztypeid(Functor);
            type_result.type.const_qualified = AZStd::is_const<Functor>::value;
            type_result.type.volatile_qualified = AZStd::is_volatile<Functor>::value;
            vtable->manager(functor, type_result, Internal::function_util::check_functor_type_tag);
            return static_cast<Functor*>(type_result.obj_ptr);
        }

        template<typename Functor>
        const Functor* target() const
        {
            if (!vtable)
            {
                return 0;
            }

            Internal::function_util::function_buffer type_result;
            type_result.type.type = aztypeid(Functor);
            type_result.type.const_qualified = true;
            type_result.type.volatile_qualified = AZStd::is_volatile<Functor>::value;
            vtable->manager(functor, type_result, Internal::function_util::check_functor_type_tag);
            // GCC 2.95.3 gets the CV qualifiers wrong here, so we
            // can't do the static_cast that we should do.
            return (const Functor*)(type_result.obj_ptr);
        }

        template<typename F>
        bool contains(const F& f) const
        {
            if (const F* fp = this->template target<F>())
            {
                return function_equal(*fp, f);
            }
            else
            {
                return false;
            }
        }

    protected:
        Internal::function_util::vtable_base* vtable;
        mutable Internal::function_util::function_buffer functor;
    };

    /**
     * The bad_function_call exception class is thrown when a AZStd::function
     * object is invoked
     */
    //class bad_function_call : public std::runtime_error
    //{
    //public:
    //  bad_function_call() : std::runtime_error("call to empty AZStd::function") {}
    //};

    inline bool operator==(const function_base& f, Internal::function_util::useless_clear_type*)
    {
        return f.empty();
    }

    inline bool operator!=(const function_base& f, Internal::function_util::useless_clear_type*)
    {
        return !f.empty();
    }

    inline bool operator==(Internal::function_util::useless_clear_type*,
        const function_base& f)
    {
        return f.empty();
    }

    inline bool operator!=(Internal::function_util::useless_clear_type*,
        const function_base& f)
    {
        return !f.empty();
    }

    template<typename Functor>
    AZSTD_FUNCTION_ENABLE_IF_NOT_INTEGRAL(Functor, bool)
    operator==(const function_base&f, Functor g)
    {
        if (const Functor* fp = f.template target<Functor>())
        {
            return function_equal(*fp, g);
        }
        else
        {
            return false;
        }
    }

    template<typename Functor>
    AZSTD_FUNCTION_ENABLE_IF_NOT_INTEGRAL(Functor, bool)
    operator==(Functor g, const function_base&f)
    {
        if (const Functor* fp = f.template target<Functor>())
        {
            return function_equal(g, *fp);
        }
        else
        {
            return false;
        }
    }

    template<typename Functor>
    AZSTD_FUNCTION_ENABLE_IF_NOT_INTEGRAL(Functor, bool)
    operator!=(const function_base&f, Functor g)
    {
        if (const Functor* fp = f.template target<Functor>())
        {
            return !function_equal(*fp, g);
        }
        else
        {
            return true;
        }
    }

    template<typename Functor>
    AZSTD_FUNCTION_ENABLE_IF_NOT_INTEGRAL(Functor, bool)
    operator!=(Functor g, const function_base&f)
    {
        if (const Functor* fp = f.template target<Functor>())
        {
            return !function_equal(g, *fp);
        }
        else
        {
            return true;
        }
    }

    template<typename Functor>
    AZSTD_FUNCTION_ENABLE_IF_NOT_INTEGRAL(Functor, bool)
    operator==(const function_base&f, reference_wrapper<Functor> g)
    {
        if (const Functor* fp = f.template target<Functor>())
        {
            return fp == &g.get();
        }
        else
        {
            return false;
        }
    }

    template<typename Functor>
    AZSTD_FUNCTION_ENABLE_IF_NOT_INTEGRAL(Functor, bool)
    operator==(reference_wrapper<Functor> g, const function_base&f)
    {
        if (const Functor* fp = f.template target<Functor>())
        {
            return &g.get() == fp;
        }
        else
        {
            return false;
        }
    }

    template<typename Functor>
    AZSTD_FUNCTION_ENABLE_IF_NOT_INTEGRAL(Functor, bool)
    operator!=(const function_base&f, reference_wrapper<Functor> g)
    {
        if (const Functor* fp = f.template target<Functor>())
        {
            return fp != &g.get();
        }
        else
        {
            return true;
        }
    }

    template<typename Functor>
    AZSTD_FUNCTION_ENABLE_IF_NOT_INTEGRAL(Functor, bool)
    operator!=(reference_wrapper<Functor> g, const function_base&f)
    {
        if (const Functor* fp = f.template target<Functor>())
        {
            return &g.get() != fp;
        }
        else
        {
            return true;
        }
    }

    namespace Internal {
        namespace function_util {
            inline bool has_empty_target(const function_base* f)  { return f->empty(); }

            inline bool has_empty_target(...)       { return false; }
        } // end namespace function_util
    } // end namespace Internal
} // end namespace AZStd

#undef AZSTD_FUNCTION_ENABLE_IF_NOT_INTEGRAL
//#undef aztypeid
//#undef aztypeid_cmp

#endif // AZSTD_FUNCTION_BASE_HEADER
#pragma once
