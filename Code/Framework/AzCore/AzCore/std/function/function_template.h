/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/function/function_base.h>
#include <AzCore/std/function/invoke.h>
#include <AzCore/std/typetraits/is_integral.h>
#include <AzCore/std/typetraits/remove_cvref.h>
#include <AzCore/std/allocator.h>

namespace AZStd
{
    namespace Internal
    {
        namespace function_util
        {
            template<typename R, typename... Args>
            struct basic_vtable;

            template<typename R>
            struct invoke_void_return_wrapper
            {
                template<typename... Args>
                static R call(Args&&... args)
                {
                    return AZStd::invoke(AZStd::forward<Args>(args)...);
                }
            };
            template<>
            struct invoke_void_return_wrapper<void>
            {
                template<typename... Args>
                static void call(Args&&... args)
                {
                    AZStd::invoke(AZStd::forward<Args>(args)...);
                }
            };
            /* Given the tag returned by get_function_tag, retrieve the
            actual invoker that will be used for the given function
            object.

            This class contains two typedefs, "invoker_type" and "manager_type",
            which correspond to the invoker and manager types. */
            template<typename FunctionType, typename Tag, typename Allocator = AZStd::allocator>
            struct get_invoker { };

            /* Retrieve the invoker for a function pointer. */
            template<typename R, typename... Args, typename Allocator>
            struct get_invoker<R(Args...), function_ptr_tag, Allocator>
            {
                template<typename FunctionPtr>
                static basic_vtable<R, Args...> create_vtable()
                {
                    auto funcPtr = &call<FunctionPtr>;
                    return { { &functor_manager_a<FunctionPtr, Allocator>::manage }, funcPtr };
                }
            private:
                template<typename FunctionPtr>
                static R call(function_buffer& function_ptr, Args&&... args)
                {
                    FunctionPtr f = reinterpret_cast<FunctionPtr>(function_ptr.func_ptr);
                    return invoke_void_return_wrapper<R>::call(f, AZStd::forward<Args>(args)...);
                }
            };
            template<typename R, typename... Args>
            struct get_invoker<R(Args...), function_ptr_tag, AZStd::allocator>
            {
                template<typename FunctionPtr>
                static basic_vtable<R, Args...> create_vtable()
                {
                    auto funcPtr = &call<FunctionPtr>;
                    return { { &functor_manager<FunctionPtr>::manage }, funcPtr };
                }
            private:
                template<typename FunctionPtr>
                static R call(function_buffer& function_ptr, Args&&... args)
                {
                    FunctionPtr f = reinterpret_cast<FunctionPtr>(function_ptr.func_ptr);
                    return invoke_void_return_wrapper<R>::call(f, AZStd::forward<Args>(args)...);
                }
            };
            /* Retrieve the invoker for a member pointer. */
            template<typename R, typename... Args, typename Allocator>
            struct get_invoker<R(Args...), member_ptr_tag, Allocator>
            {
                template<typename MemberPtr>
                static basic_vtable<R, Args...> create_vtable()
                {
                    auto funcPtr = &call<MemberPtr>;
                    return { vtable_base{ &functor_manager_a<MemberPtr, Allocator>::manage }, funcPtr };
                }
            private:
                template<typename MemberPtr>
                static R call(function_buffer& function_obj_ptr, Args&&... args)
                {
                    MemberPtr* f = reinterpret_cast<MemberPtr*>(&function_obj_ptr.data);
                    return invoke_void_return_wrapper<R>::call(*f, AZStd::forward<Args>(args)...);
                }
            };
            template<typename R, typename... Args>
            struct get_invoker<R(Args...), member_ptr_tag, AZStd::allocator>
            {
                template<typename MemberPtr>
                static basic_vtable<R, Args...> create_vtable()
                {
                    auto funcPtr = &call<MemberPtr>;
                    return { vtable_base{ &functor_manager<MemberPtr>::manage }, funcPtr };
                }
            private:
                template<typename MemberPtr>
                static R call(function_buffer& function_obj_ptr, Args&&... args)
                {
                    MemberPtr* f = reinterpret_cast<MemberPtr*>(&function_obj_ptr.data);
                    return invoke_void_return_wrapper<R>::call(*f, AZStd::forward<Args>(args)...);
                }
            };

            /* Retrieve the invoker for a function object. */
            template<typename R, typename... Args, typename Allocator>
            struct get_invoker<R(Args...), function_obj_tag, Allocator>
            {
                template<typename FunctionObj>
                static basic_vtable<R, Args...> create_vtable()
                {
                    auto funcPtr = &call<FunctionObj>;
                    return { vtable_base{ &functor_manager_a<FunctionObj, Allocator>::manage }, funcPtr };
                }
            private:
                template<typename FunctionObj>
                static R call(function_buffer& function_obj_ptr, Args&&... args)
                {
                    FunctionObj* f;
                    if (function_allows_small_object_optimization<FunctionObj>::value)
                    {
                        f = reinterpret_cast<FunctionObj*>(&function_obj_ptr.data);
                    }
                    else
                    {
                        f = reinterpret_cast<FunctionObj*>(function_obj_ptr.obj_ptr);
                    }
                    return invoke_void_return_wrapper<R>::call(*f, AZStd::forward<Args>(args)...);
                }
            };
            template<typename R, typename... Args>
            struct get_invoker<R(Args...), function_obj_tag, AZStd::allocator>
            {
                template<typename FunctionObj>
                static basic_vtable<R, Args...> create_vtable()
                {
                    auto funcPtr = &call<FunctionObj>;
                    return { vtable_base{ &functor_manager<FunctionObj>::manage }, funcPtr };
                }
            private:
                template<typename FunctionObj>
                static R call(function_buffer& function_obj_ptr, Args&&... args)
                {
                    FunctionObj* f;
                    if (function_allows_small_object_optimization<FunctionObj>::value)
                    {
                        f = reinterpret_cast<FunctionObj*>(&function_obj_ptr.data);
                    }
                    else
                    {
                        f = reinterpret_cast<FunctionObj*>(function_obj_ptr.obj_ptr);
                    }
                    return invoke_void_return_wrapper<R>::call(*f, AZStd::forward<Args>(args)...);
                }
            };

            /* Retrieve the invoker for a reference to a function object. */
            template<typename R, typename... Args, typename Allocator>
            struct get_invoker<R(Args...), function_obj_ref_tag, Allocator>
            {
                template<typename RefWrapper>
                static basic_vtable<R, Args...> create_vtable()
                {
                    auto funcPtr = call<typename RefWrapper::type>;
                    return { vtable_base{ &reference_manager<typename RefWrapper::type>::manage }, funcPtr };
                }
            private:
                template<typename FunctionObj>
                static R call(function_buffer& function_obj_ptr, Args&&... args)
                {
                    FunctionObj* f = reinterpret_cast<FunctionObj*>(function_obj_ptr.obj_ptr);
                    return invoke_void_return_wrapper<R>::call(*f, AZStd::forward<Args>(args)...);
                }
            };


            /**
            * vtable for a specific boost::function instance. This
            * structure must be an aggregate so that we can use static
            * initialization in boost::function's assign_to and assign_to_a
            * members. It therefore cannot have any constructors,
            * destructors, base classes, etc.
            */
            template<typename R, typename... Args>
            struct basic_vtable
            {
                using invoker_type = R(*)(function_buffer&, Args&&...);

                template<typename F>
                bool assign_to(F&& f, function_buffer& functor)
                {
                    typedef typename get_function_tag<F>::type tag;
                    return assign_to(AZStd::forward<F>(f), functor, tag());
                }
                template<typename F, typename Allocator>
                bool assign_to_a(F&& f, function_buffer& functor, Allocator a)
                {
                    typedef typename get_function_tag<F>::type tag;
                    return assign_to_a(AZStd::forward<F>(f), functor, a, tag());
                }

                void clear(function_buffer& functor)
                {
                    if (base.manager)
                    {
                        base.manager(functor, functor, destroy_functor_tag);
                    }
                }
            private:
                // Function pointers
                template<typename FunctionPtr>
                bool
                assign_to(FunctionPtr f, function_buffer& functor, function_ptr_tag)
                {
                    this->clear(functor);
                    if (f)
                    {
                        // should be a reinterpret cast, but some compilers insist
                        // on giving cv-qualifiers to free functions
                        functor.func_ptr = (void (*)())(f);
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
                template<typename FunctionPtr, typename Allocator>
                bool
                assign_to_a(FunctionPtr f, function_buffer& functor, Allocator, function_ptr_tag)
                {
                    return assign_to(f, functor, function_ptr_tag());
                }

                // Member pointers
                template<typename MemberPtr>
                bool assign_to(MemberPtr f, function_buffer& functor, member_ptr_tag)
                {
                    // DPG TBD: Add explicit support for member function
                    // objects, so we invoke through mem_fn() but we retain the
                    // right target_type() values.
                    if (f)
                    {
                        this->assign_to(mem_fn(f), functor);
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
                template<typename MemberPtr, typename Allocator>
                bool assign_to_a(MemberPtr f, function_buffer& functor, Allocator a, member_ptr_tag)
                {
                    // DPG TBD: Add explicit support for member function
                    // objects, so we invoke through mem_fn() but we retain the
                    // right target_type() values.
                    if (f)
                    {
                        this->assign_to_a(mem_fn(f), functor, a);
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }

                // Function objects
                // Assign to a function object using the small object optimization
                template<typename FunctionObj>
                void
                assign_functor(FunctionObj&& f, function_buffer& functor, AZStd::true_type)
                {
                    using RawFunctorObjType = AZStd::decay_t<FunctionObj>;
                    new ((void*)&functor.data)RawFunctorObjType(AZStd::forward<FunctionObj>(f));
                }
                template<typename FunctionObj, typename Allocator>
                void
                assign_functor_a(FunctionObj&& f, function_buffer& functor, Allocator, AZStd::true_type)
                {
                    assign_functor(AZStd::forward<FunctionObj>(f), functor, AZStd::true_type());
                }

                // Assign to a function object allocated on the heap.
                template<typename FunctionObj>
                void assign_functor(FunctionObj&& f, function_buffer& functor, AZStd::false_type)
                {
                    using RawFunctorObjType = AZStd::decay_t<FunctionObj>;
                    AZStd::allocator a;
                    functor.obj_ptr = new (a.allocate(sizeof(RawFunctorObjType), AZStd::alignment_of<RawFunctorObjType>::value))RawFunctorObjType(AZStd::forward<FunctionObj>(f));
                }
                template<typename FunctionObj, typename Allocator>
                void
                assign_functor_a(FunctionObj&& f, function_buffer& functor, const Allocator& a, AZStd::false_type)
                {
                    using RawFunctorObjType = AZStd::decay_t<FunctionObj>;
                    typedef functor_wrapper<RawFunctorObjType, Allocator> functor_wrapper_type;
                    functor_wrapper_type* new_f = new (const_cast<Allocator&>(a).allocate(sizeof(functor_wrapper_type), AZStd::alignment_of<functor_wrapper_type>::value))functor_wrapper_type(AZStd::forward<FunctionObj>(f), a);
                    functor.obj_ptr = new_f;
                }

                template<typename FunctionObj>
                bool
                assign_to(FunctionObj&& f, function_buffer& functor, function_obj_tag)
                {
                    using RawFunctorObjType = AZStd::decay_t<FunctionObj>;
                    if (!AZStd::Internal::function_util::has_empty_target(AZStd::addressof(f)))
                    {
                        assign_functor(AZStd::forward<FunctionObj>(f), functor, AZStd::integral_constant<bool, function_allows_small_object_optimization<RawFunctorObjType>::value>());
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
                template<typename FunctionObj, typename Allocator>
                bool
                assign_to_a(FunctionObj&& f, function_buffer& functor, Allocator a, function_obj_tag)
                {
                    using RawFunctorObjType = AZStd::decay_t<FunctionObj>;
                    if (!AZStd::Internal::function_util::has_empty_target(AZStd::addressof(f)))
                    {
                        assign_functor_a(AZStd::forward<FunctionObj>(f), functor, a, AZStd::integral_constant<bool, function_allows_small_object_optimization<RawFunctorObjType>::value>());
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }

                // Reference to a function object
                template<typename FunctionObj>
                bool
                assign_to(const reference_wrapper<FunctionObj>& f,
                    function_buffer& functor, function_obj_ref_tag)
                {
                    if (!AZStd::Internal::function_util::has_empty_target(&f.get()))
                    {
                        functor.obj_ref.obj_ptr = (void*)&f.get();
                        functor.obj_ref.is_const_qualified = is_const<FunctionObj>::value;
                        functor.obj_ref.is_volatile_qualified = AZStd::is_volatile<FunctionObj>::value;
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
                template<typename FunctionObj, typename Allocator>
                bool
                assign_to_a(const reference_wrapper<FunctionObj>& f, function_buffer& functor, Allocator, function_obj_ref_tag)
                {
                    return assign_to(f, functor, function_obj_ref_tag());
                }

            public:
                vtable_base base;
                invoker_type invoker;
            };
        } // end namespace function_util
    } // end namespace Internal

    template<typename R, typename... Args>
    class function_intermediate
        : public function_base
    {
    public:
        typedef R result_type;
    private:
        typedef AZStd::Internal::function_util::basic_vtable<R, Args...>  vtable_type;
    public:
        typedef function_intermediate self_type;

        function_intermediate()
            : function_base() { }

        // MSVC chokes if the following two constructors are collapsed into
        // one with a default parameter.
        template<typename Functor>
        function_intermediate(Functor&& f, enable_if_t<!is_integral<Functor>::value && !is_same<remove_cvref_t<Functor>, function_intermediate>::value, int> = 0)
            : function_base()
        {
            this->assign_to(AZStd::forward<Functor>(f));
        }
        template<typename Functor, typename Allocator>
        function_intermediate(Functor&& f, Allocator a, enable_if_t<!is_integral<Functor>::value && !is_same<remove_cvref_t<Functor>, function_intermediate>::value, int> = 0)
            : function_base()
        {
            this->assign_to_a(AZStd::forward<Functor>(f), a);
        }

        function_intermediate(nullptr_t)
            : function_base() { }
        function_intermediate(const function_intermediate& f)
            : function_base()
        {
            this->assign_to_own(f);
        }

        function_intermediate(function_intermediate&& f)
            : function_base()
        {
            this->move_assign(f);
        }

        ~function_intermediate() { clear(); }

        R operator()(Args&&... args) const;

        template<typename Functor>
        enable_if_t<!is_integral<Functor>::value && !is_same<remove_cvref_t<Functor>, function_intermediate>::value, function_intermediate&>
        operator=(Functor&& f)
        {
            this->clear();
            this->assign_to(AZStd::forward<Functor>(f));
            return *this;
        }
        template<typename Functor, typename Allocator>
        void assign(Functor&& f, Allocator a)
        {
            this->clear();
            this->assign_to_a(AZStd::forward<Functor>(f), a);
        }

        function_intermediate& operator=(nullptr_t)
        {
            this->clear();
            return *this;
        }

        // Assignment from another function_intermediate
        function_intermediate& operator=(const function_intermediate& f)
        {
            if (&f == this)
            {
                return *this;
            }

            this->clear();
            this->assign_to_own(f);
            return *this;
        }

        // Move assignment from another AZSTD_FUNCTION_FUNCTION
        function_intermediate& operator=(function_intermediate&& f)
        {
            if (&f == this)
            {
                return *this;
            }

            this->clear();
            this->move_assign(f);
            return *this;
        }

        void swap(function_intermediate& other)
        {
            if (&other == this)
            {
                return;
            }

            function_intermediate tmp;
            tmp.move_assign(*this);
            this->move_assign(other);
            other.move_assign(tmp);
        }

        // Clear out a target, if there is one
        void clear()
        {
            if (vtable)
            {
                reinterpret_cast<vtable_type*>(vtable)->clear(this->functor);
                vtable = 0;
            }
        }

    private:
        using function_base::empty; // hide non-standard empty() function

    public:
        explicit operator bool () const
        {
            return !this->empty();
        }

        bool operator!() const
        { return this->empty(); }

    private:
        void assign_to_own(const function_intermediate& f)
        {
            if (!f.empty())
            {
                this->vtable = f.vtable;
                f.vtable->manager(f.functor, this->functor, AZStd::Internal::function_util::clone_functor_tag);
            }
        }

        template<typename Functor>
        void assign_to(Functor&& f)
        {
            using Internal::function_util::vtable_base;

            typedef typename Internal::function_util::get_function_tag<Functor>::type tag;
            typedef Internal::function_util::get_invoker<R(Args...), tag> get_invoker;

            //! A static vtable is used to avoid the need to dynamically allocate a vtable
            //! whose purpose is to contain a function ptr that can the manage the function buffer
            //! i.e performs the copy, move and destruction operations for the function buffer
            //! as well as to validate if a the stored function can be type_cast to the type supplied in
            //! std::function::target
            //! The vtable other purpose is to store a function ptr that is used to wrap the invocation of the underlying function
            static vtable_type stored_vtable = get_invoker::template create_vtable<decay_t<Functor>>();

            if (stored_vtable.assign_to(AZStd::forward<Functor>(f), functor))
            {
                vtable = &stored_vtable.base;
            }
            else
            {
                vtable = 0;
            }
        }

        template<typename Functor, typename Allocator>
        void assign_to_a(Functor&& f, const Allocator& a)
        {
            using Internal::function_util::vtable_base;
            typedef typename Internal::function_util::get_function_tag<Functor>::type tag;
            typedef Internal::function_util::get_invoker<R(Args...), tag, Allocator> get_invoker;

            //! A static vtable is used to avoid the need to dynamically allocate a vtable
            //! whose purpose is to contain a function ptr that can the manage the function buffer
            //! i.e performs the copy, move and destruction operations for the function buffer
            //! as well as to validate if a the stored function can be type_cast to the type supplied in
            //! std::function::target
            //! The vtable other purpose is to store a function ptr that is used to wrap the invocation of the underlying function
            static vtable_type stored_vtable = get_invoker::template create_vtable<decay_t<Functor>>();

            if (stored_vtable.assign_to_a(AZStd::forward<Functor>(f), functor, a))
            {
                vtable = &stored_vtable.base;
            }
            else
            {
                vtable = 0;
            }
        }


        // Moves the value from the specified argument to *this. If the argument
        // has its function object allocated on the heap, move_assign will pass
        // its buffer to *this, and set the argument's buffer pointer to NULL.
        void move_assign(function_intermediate& f)
        {
            if (&f == this)
            {
                return;
            }
            if (!f.empty())
            {
                this->vtable = f.vtable;
                f.vtable->manager(f.functor, this->functor, Internal::function_util::move_functor_tag);
                f.vtable = 0;
            }
        }
    };

    template<typename R, typename... Args>
    inline void swap(function_intermediate<R, Args...>& f1, function_intermediate<R, Args...>& f2)
    {
        f1.swap(f2);
    }

    template<typename R, typename... Args>
    R function_intermediate<R, Args...>::operator()(Args&&... args) const
    {
        AZ_Assert(!this->empty(), "Bad function call!");
        return reinterpret_cast<const vtable_type*>(vtable)->invoker(this->functor, AZStd::forward<Args>(args)...);
    }

    //! Don't allow comparisons between objects of the same type.
    //! Comparing the objects stored within two type erased functions does not have a well define way
    //! to determine if both objects are equality comparable
    //! This can be illustrated in the following `function_intermediate<WellDefinedSignature>(Functor1) == function_intermediate<WellDefinedSignature>(Functor2)`
    //! The only way an equality comparison can occur is if Functor1 can compare to Functor2 and that information cannot be known by the function class
    template<typename R, typename... Args>
    void operator==(const function_intermediate<R, Args...>&, const function_intermediate<R, Args...>&) = delete;
    template<typename R, typename... Args>
    void operator!=(const function_intermediate<R, Args...>&, const function_intermediate<R, Args...>&) = delete;


    template<typename R, typename... Args>
    class function<R(Args...)>
        : public function_intermediate<R, Args...>
    {
        typedef function_intermediate<R, Args...> base_type;
        typedef function self_type;
    public:
        function()
            : base_type() {}
        template<typename Functor>
        function(Functor&& f, enable_if_t<!is_integral<Functor>::value && !is_same<remove_cvref_t<Functor>, self_type>::value, int> = 0)
            : base_type(AZStd::forward<Functor>(f))
        {}
        template<typename Functor, typename Allocator>
        function(Functor&& f, const Allocator& a, enable_if_t<!is_integral<Functor>::value && !is_same<remove_cvref_t<Functor>, self_type>::value, int> = 0)
            : base_type(AZStd::forward<Functor>(f), a)
        {}

        function(nullptr_t)
            : base_type(nullptr) {}
        function(const self_type& f)
            : base_type(static_cast<const base_type&>(f)){}
        function(const base_type& f)
            : base_type(static_cast<const base_type&>(f)){}
        function(self_type&& f)
            : base_type(static_cast<base_type &&>(f)){}
        function(base_type&& f)
            : base_type(static_cast<base_type &&>(f)){}

        self_type& operator=(const self_type& f)
        {
            self_type(f).swap(*this);
            return *this;
        }
        self_type& operator=(self_type&& f)
        {
            self_type(static_cast<self_type &&>(f)).swap(*this);
            return *this;
        }

        template<typename Functor>
        enable_if_t<!is_integral<Functor>::value && !is_same<remove_cvref_t<Functor>, self_type>::value, self_type&>
        operator=(Functor&& f)
        {
            self_type(AZStd::forward<Functor>(f)).swap(*this);
            return *this;
        }

        self_type& operator=(nullptr_t)
        {
            this->clear();
            return *this;
        }
        self_type& operator=(const base_type& f)
        {
            self_type(f).swap(*this);
            return *this;
        }
        self_type& operator=(base_type&& f)
        {
            self_type(static_cast<base_type &&>(f)).swap(*this);
            return *this;
        }

        R operator()(Args... args) const
        {
            return base_type::operator()(AZStd::forward<Args>(args)...);
        }
    };
} // end namespace AZStd
