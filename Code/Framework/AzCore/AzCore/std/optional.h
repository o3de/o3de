/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <initializer_list>
#include <AzCore/std/typetraits/is_scalar.h>
#include <AzCore/std/utils.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZStd
{
    // 23.6.4, no-value state indicator
    struct nullopt_t{
        struct UniqueTag {};
        // nullopt_t does not have a default constructor, and is not an
        // aggregate
        constexpr explicit nullopt_t(UniqueTag) {}
    };
    static constexpr nullopt_t nullopt {nullopt_t::UniqueTag()};

    namespace Internal
    {
        template<class T, bool = std::is_trivially_destructible<T>::value>
        class OptionalDestructBase
        {
        public:
            union
            {
                char m_empty;
                T m_object;
            };
            bool m_engaged;

            constexpr OptionalDestructBase()
                : m_empty()
                , m_engaged(false)
            {
            }

            template<class... Args>
            constexpr explicit OptionalDestructBase(in_place_t, Args&&... args)
                : m_object(forward<Args>(args)...)
                , m_engaged(true)
            {
            }

            void reset()
            {
                m_engaged = false;
            }
        };

        template<class T>
        class OptionalDestructBase<T, false>
        {
        public:
            union
            {
                char m_empty;
                T m_object;
            };
            bool m_engaged;

            constexpr OptionalDestructBase()
                : m_empty()
                , m_engaged(false)
            {
            }

            template<class... Args>
            constexpr explicit OptionalDestructBase(in_place_t, Args&&... args)
                : m_object(forward<Args>(args)...)
                , m_engaged(true)
            {
            }

            ~OptionalDestructBase()
            {
                reset();
            }

            void reset()
            {
                if (m_engaged)
                {
                    m_object.T::~T();
                    m_engaged = false;
                }
            }
        };

        template<class T>
        class OptionalImplBase
            : public OptionalDestructBase<T>
        {
            using base = OptionalDestructBase<T>;
        public:
            using base::base;

            constexpr bool has_value() const
            {
                return this->m_engaged;
            }

            constexpr const T& value() const&
            {
                AZ_Assert(this->m_engaged, "Optional does not have a value");
                return this->m_object;
            }
            /*constexpr*/ T& value()&
            {
                AZ_Assert(this->m_engaged, "Optional does not have a value");
                return this->m_object;
            }

            constexpr const T&& value() const&&
            {
                AZ_Assert(this->m_engaged, "Optional does not have a value");
                return AZStd::move(this->m_object);
            }
            /*constexpr*/ T&& value()&&
            {
                AZ_Assert(this->m_engaged, "Optional does not have a value");
                return AZStd::move(this->m_object);
            }

            template<class... Args>
            void construct(Args&&... args)
            {
                ::new((void*)AZStd::addressof(this->m_object)) T(forward<Args>(args)...);
                this->m_engaged = true;
            }

            template<class U>
            void construct_from(U&& that)
            {
                if (that.has_value())
                {
                    this->construct(forward<U>(that).value());
                }
            }

            template<class U>
            void assign_from(U&& that)
            {
                if (this->has_value() == that.has_value())
                {
                    if (this->has_value())
                    {
                        this->m_object = forward<U>(that).value();
                    }
                }
                else
                {
                    if (this->has_value())
                    {
                        this->reset();
                    }
                    else
                    {
                        this->construct(forward<U>(that).value());
                    }
                }
            }
        };

        template<class T, bool = std::is_trivially_copy_constructible<T>::value>
        class OptionalCopyBase : public OptionalImplBase<T>
        {
            using base = OptionalImplBase<T>;
        public:
            using base::base;
        };

        template<class T>
        class OptionalCopyBase<T, false> : public OptionalImplBase<T>
        {
            using base = OptionalImplBase<T>;
        public:
            using base::base;

            OptionalCopyBase() = default;
            OptionalCopyBase(const OptionalCopyBase& rhs)
            {
                this->construct_from(rhs);
            }
            OptionalCopyBase(OptionalCopyBase&&) = default;
            OptionalCopyBase& operator=(const OptionalCopyBase&) = default;
            OptionalCopyBase& operator=(OptionalCopyBase&&) = default;
        };

        template<class T, bool = std::is_trivially_move_constructible<T>::value>
        class OptionalMoveBase : public OptionalCopyBase<T>
        {
            using base = OptionalCopyBase<T>;
        public:
            using base::base;
        };

        template<class T>
        class OptionalMoveBase<T, false> : public OptionalCopyBase<T>
        {
            using base = OptionalCopyBase<T>;
        public:
            using base::base;

            OptionalMoveBase() = default;
            OptionalMoveBase(const OptionalMoveBase&) = default;
            OptionalMoveBase(OptionalMoveBase&& rhs)
            {
                this->construct_from(AZStd::move(rhs));
            }
            OptionalMoveBase& operator=(const OptionalMoveBase&) = default;
            OptionalMoveBase& operator=(OptionalMoveBase&&) = default;
        };

        template<class T, bool =
               std::is_trivially_destructible<T>::value
            && std::is_trivially_copy_constructible<T>::value
            && std::is_trivially_copy_assignable<T>::value
        >
        class OptionalCopyAssignBase : public OptionalMoveBase<T>
        {
            using base = OptionalMoveBase<T>;
        public:
            using base::base;
        };

        template<class T>
        class OptionalCopyAssignBase<T, false> : public OptionalMoveBase<T>
        {
            using base = OptionalMoveBase<T>;
        public:
            using base::base;

            OptionalCopyAssignBase() = default;
            OptionalCopyAssignBase(const OptionalCopyAssignBase&) = default;
            OptionalCopyAssignBase(OptionalCopyAssignBase&&) = default;
            OptionalCopyAssignBase& operator=(const OptionalCopyAssignBase& rhs)
            {
                this->assign_from(rhs);
                return *this;
            }
            OptionalCopyAssignBase& operator=(OptionalCopyAssignBase&&) = default;
        };

        template<class T, bool =
               std::is_trivially_destructible<T>::value
            && std::is_trivially_move_constructible<T>::value
            && std::is_trivially_move_assignable<T>::value
        >
        class OptionalMoveAssignBase : public OptionalCopyAssignBase<T>
        {
            using base = OptionalCopyAssignBase<T>;
        public:
            using base::base;
        };

        template<class T>
        class OptionalMoveAssignBase<T, false> : public OptionalCopyAssignBase<T>
        {
            using base = OptionalCopyAssignBase<T>;
        public:
            using base::base;

            OptionalMoveAssignBase() = default;
            OptionalMoveAssignBase(const OptionalMoveAssignBase&) = default;
            OptionalMoveAssignBase(OptionalMoveAssignBase&&) = default;
            OptionalMoveAssignBase& operator=(const OptionalMoveAssignBase&) = default;
            OptionalMoveAssignBase& operator=(OptionalMoveAssignBase&& rhs)
            {
                this->assign_from(AZStd::move(rhs));
                return *this;
            }
        };

        // The following templates define types with certain special
        // constructors deleted. The deleted constructors are the same ones
        // that are deleted in the source type T used in optional<T>. They
        // trigger SFINAE so that type traits like
        // AZStd::is_assignable<optional<T>&, optional<T>&> yield the same
        // value as AZStd::is_assignable<T&, T&>
        template <bool CanCopy, bool CanMove>
        struct SFINAECtorBase {};
        template <>
        struct SFINAECtorBase<false, false> {
          SFINAECtorBase() = default;
          SFINAECtorBase(SFINAECtorBase const&) = delete;
          SFINAECtorBase(SFINAECtorBase &&) = delete;
          SFINAECtorBase& operator=(SFINAECtorBase const&) = default;
          SFINAECtorBase& operator=(SFINAECtorBase&&) = default;
        };
        template <>
        struct SFINAECtorBase<true, false> {
          SFINAECtorBase() = default;
          SFINAECtorBase(SFINAECtorBase const&) = default;
          SFINAECtorBase(SFINAECtorBase &&) = delete;
          SFINAECtorBase& operator=(SFINAECtorBase const&) = default;
          SFINAECtorBase& operator=(SFINAECtorBase&&) = default;
        };
        template <>
        struct SFINAECtorBase<false, true> {
          SFINAECtorBase() = default;
          SFINAECtorBase(SFINAECtorBase const&) = delete;
          SFINAECtorBase(SFINAECtorBase &&) = default;
          SFINAECtorBase& operator=(SFINAECtorBase const&) = default;
          SFINAECtorBase& operator=(SFINAECtorBase&&) = default;
        };

        template <bool CanCopy, bool CanMove>
        struct SFINAEAssignBase {};
        template <>
        struct SFINAEAssignBase<false, false> {
          SFINAEAssignBase() = default;
          SFINAEAssignBase(SFINAEAssignBase const&) = default;
          SFINAEAssignBase(SFINAEAssignBase &&) = default;
          SFINAEAssignBase& operator=(SFINAEAssignBase const&) = delete;
          SFINAEAssignBase& operator=(SFINAEAssignBase&&) = delete;
        };
        template <>
        struct SFINAEAssignBase<true, false> {
          SFINAEAssignBase() = default;
          SFINAEAssignBase(SFINAEAssignBase const&) = default;
          SFINAEAssignBase(SFINAEAssignBase &&) = default;
          SFINAEAssignBase& operator=(SFINAEAssignBase const&) = default;
          SFINAEAssignBase& operator=(SFINAEAssignBase&&) = delete;
        };
        template <>
        struct SFINAEAssignBase<false, true> {
          SFINAEAssignBase() = default;
          SFINAEAssignBase(SFINAEAssignBase const&) = default;
          SFINAEAssignBase(SFINAEAssignBase &&) = default;
          SFINAEAssignBase& operator=(SFINAEAssignBase const&) = delete;
          SFINAEAssignBase& operator=(SFINAEAssignBase&&) = default;
        };
        template <class _Tp>
        using OptionalSFINAECtorBase_t = SFINAECtorBase<
            is_copy_constructible<_Tp>::value,
            is_move_constructible<_Tp>::value
        >;

        template <class _Tp>
        using OptionalSFINAEAssignBase_t = SFINAEAssignBase<
            (is_copy_constructible<_Tp>::value && is_copy_assignable<_Tp>::value),
            (is_move_constructible<_Tp>::value && is_move_assignable<_Tp>::value)
        >;
    }// end namespace Internal

    template<class T>
    class optional
        : private Internal::OptionalMoveAssignBase<T>
        , private Internal::OptionalSFINAECtorBase_t<T>
        , private Internal::OptionalSFINAEAssignBase_t<T>
    {
        using base = Internal::OptionalMoveAssignBase<T>;
    public:
        AZ_CLASS_ALLOCATOR(optional, AZ::SystemAllocator, 0);

        using value_type = T;

        // 23.6.3.1, constructors
        constexpr optional() = default;
        constexpr optional(nullopt_t) {}
        constexpr optional(const optional& rhs) = default;
        constexpr optional(optional&& rhs) = default;

        template<class... Args, class = typename enable_if<std::is_constructible<T, Args...>::value>::type>
        constexpr explicit optional(in_place_t, Args&&... args)
            : base(in_place, forward<Args>(args)...)
        {
        }

        template<class U, class... Args, class = typename enable_if<std::is_constructible<T, std::initializer_list<U>&, Args...>::value>::type>
        constexpr explicit optional(in_place_t, std::initializer_list<U> il, Args&&... args)
            : base(in_place, il, forward<Args>(args)...)
        {
        }

        template<class U = T, typename enable_if<
               std::is_constructible<T, U&&>::value
            && !is_same<typename remove_cv<typename remove_reference<U>::type>::type, in_place_t>::value
            && !is_same<typename remove_cv<typename remove_reference<U>::type>::type, optional>::value
            && std::is_convertible<U&&, T>::value
        >::type * = nullptr>
        constexpr optional(U&& v)
            : base(in_place, forward<U>(v))
        {
        }

        template<class U = T, typename enable_if<
               std::is_constructible<T, U&&>::value
            && !is_same<typename remove_cv<typename remove_reference<U>::type>::type, in_place_t>::value
            && !is_same<typename remove_cv<typename remove_reference<U>::type>::type, optional>::value
            && !std::is_convertible<U&&, T>::value
        >::type * = nullptr>
        constexpr explicit optional(U&& v)
            : base(in_place, forward<U>(v))
        {
        }

        template<class U, typename enable_if<
               std::is_constructible<T, const U&>::value
            && !std::is_constructible<T, optional<U>&>::value
            && !std::is_constructible<T, optional<U>&&>::value
            && !std::is_constructible<T, const optional<U>&>::value
            && !std::is_constructible<T, const optional<U>&&>::value
            && !std::is_convertible<optional<U>&, T>::value
            && !std::is_convertible<optional<U>&&, T>::value
            && !std::is_convertible<const optional<U>&, T>::value
            && !std::is_convertible<const optional<U>&&, T>::value
            && std::is_convertible<const U&, T>::value
        >::type * = nullptr>
        optional(const optional<U>& rhs)
        {
            this->construct_from(rhs);
        }

        template<class U, typename enable_if<
               std::is_constructible<T, const U&>::value
            && !std::is_constructible<T, optional<U>&>::value
            && !std::is_constructible<T, optional<U>&&>::value
            && !std::is_constructible<T, const optional<U>&>::value
            && !std::is_constructible<T, const optional<U>&&>::value
            && !std::is_convertible<optional<U>&, T>::value
            && !std::is_convertible<optional<U>&&, T>::value
            && !std::is_convertible<const optional<U>&, T>::value
            && !std::is_convertible<const optional<U>&&, T>::value
            && !std::is_convertible<const U&, T>::value
        >::type * = nullptr>
        explicit optional(const optional<U>& rhs)
        {
            this->construct_from(rhs);
        }

        template<class U, typename enable_if<
               std::is_constructible<T, U&&>::value
            && !std::is_constructible<T, optional<U>&>::value
            && !std::is_constructible<T, optional<U>&&>::value
            && !std::is_constructible<T, const optional<U>&>::value
            && !std::is_constructible<T, const optional<U>&&>::value
            && !std::is_convertible<optional<U>&, T>::value
            && !std::is_convertible<optional<U>&&, T>::value
            && !std::is_convertible<const optional<U>&, T>::value
            && !std::is_convertible<const optional<U>&&, T>::value
            && std::is_convertible<const U&, T>::value
        >::type * = nullptr>
        optional(optional<U>&& rhs)
        {
            this->construct_from(AZStd::move(rhs));
        }

        template<class U, typename enable_if<
               std::is_constructible<T, U&&>::value
            && !std::is_constructible<T, optional<U>&>::value
            && !std::is_constructible<T, optional<U>&&>::value
            && !std::is_constructible<T, const optional<U>&>::value
            && !std::is_constructible<T, const optional<U>&&>::value
            && !std::is_convertible<optional<U>&, T>::value
            && !std::is_convertible<optional<U>&&, T>::value
            && !std::is_convertible<const optional<U>&, T>::value
            && !std::is_convertible<const optional<U>&&, T>::value
            && !std::is_convertible<const U&, T>::value
        >::type * = nullptr>
        explicit optional(optional<U>&& rhs)
        {
            this->construct_from(AZStd::move(rhs));
        }

        // 23.6.3.2 Destructor
        // The destructor is conditionally trivial, and is handled by
        // OptionalDestructBase

        // 23.6.3.3 Assignment
        optional<T>& operator=(nullopt_t)
        {
            this->reset();
            return *this;
        }

        optional<T>& operator=(const optional& rhs) = default;
        optional<T>& operator=(optional&& rhs) = default;

        template<class U = T, class = typename enable_if<
               !is_same<typename remove_cv<typename remove_reference<U>::type>::type, optional>::value
               // !conjunction_v<is_scalar<T>, is_same<T, decay_t<U>>>
            && !(is_scalar<value_type>::value && is_same<T, typename decay<U>::type>::value)
            && is_constructible<T, U>::value
            && is_assignable<T&, U>::value
        >::type>
        optional<T>& operator=(U&& v)
        {
            if (this->has_value())
            {
                this->value() = forward<U>(v);
            }
            else
            {
                this->construct(forward<U>(v));
            }
            return *this;
        }

        template<class U, class = typename enable_if<
               std::is_constructible<T, const U&>::value
            && std::is_assignable<T, U&>::value
            && !std::is_constructible<T, optional<U>&>::value
            && !std::is_constructible<T, optional<U>&&>::value
            && !std::is_constructible<T, const optional<U>&>::value
            && !std::is_constructible<T, const optional<U>&&>::value
            && !std::is_convertible<optional<U>&, T>::value
            && !std::is_convertible<optional<U>&&, T>::value
            && !std::is_convertible<const optional<U>&, T>::value
            && !std::is_convertible<const optional<U>&&, T>::value
            && !std::is_assignable<T&, optional<U>&>::value
            && !std::is_assignable<T&, optional<U>&&>::value
            && !std::is_assignable<T&, const optional<U>&>::value
            && !std::is_assignable<T&, const optional<U>&&>::value
        >::type>
        optional<T>& operator=(const optional<U>& rhs)
        {
            this->assign_from(rhs);
            return *this;
        }

        template<class U, class = typename enable_if<
               std::is_constructible<T, U>::value
            && std::is_assignable<T&, U>::value
            && !std::is_constructible<T, optional<U>&>::value
            && !std::is_constructible<T, optional<U>&&>::value
            && !std::is_constructible<T, const optional<U>&>::value
            && !std::is_constructible<T, const optional<U>&&>::value
            && !std::is_convertible<optional<U>&, T>::value
            && !std::is_convertible<optional<U>&&, T>::value
            && !std::is_convertible<const optional<U>&, T>::value
            && !std::is_convertible<const optional<U>&&, T>::value
            && !std::is_assignable<T&, optional<U>&>::value
            && !std::is_assignable<T&, optional<U>&&>::value
            && !std::is_assignable<T&, const optional<U>&>::value
            && !std::is_assignable<T&, const optional<U>&&>::value
        >::type>
        optional<T>& operator=(optional<U>&& rhs)
        {
            this->assign_from(AZStd::move(rhs));
            return *this;
        }

        template<class... Args, class = typename enable_if<
            std::is_constructible<T, Args&&...>::value
        >::type>
        T& emplace(Args&&... args)
        {
            this->reset();
            this->construct(forward<Args>(args)...);
            return this->value();
        }

        template<class U, class... Args, class = typename enable_if<
            std::is_constructible<T, Args&&...>::value
        >::type>
        T& emplace(std::initializer_list<U> il, Args&&... args)
        {
            this->reset();
            this->construct(il, forward<Args>(args)...);
            return this->value();
        }

        // 23.6.3.4 Swap
        void swap(optional& rhs)
        {
            using AZStd::swap;
            if (this->has_value())
            {
                if (rhs.has_value())
                {
                    swap(this->value(), rhs.value());
                }
                else
                {
                    rhs.construct(AZStd::move(this->value()));
                    this->reset();
                }
            }
            else
            {
                if (rhs.has_value())
                {
                    this->construct(AZStd::move(rhs.value()));
                    rhs.reset();
                }
            }
        }

        // 23.6.3.5 observers
        constexpr const T* operator->() const
        {
            return AZStd::addressof(this->value());
        }
        /*constexpr*/ T* operator->() // not constexpr because constexpr implies const in c++11, yielding methods differing only by return type
        {
            return AZStd::addressof(this->value());
        }

        constexpr const T& operator*() const&
        {
            return this->value();
        }
        /*constexpr*/ T& operator*() &
        {
            return this->value();
        }

        constexpr const T&& operator*() const&&
        {
            return AZStd::move(this->value());
        }
        /*constexpr*/ T&& operator*() &&
        {
            return AZStd::move(this->value());
        }

        constexpr explicit operator bool() const
        {
            return has_value();
        }

        using base::has_value;
        using base::value;

        template<class U, class = typename enable_if<std::is_copy_constructible<T>::value && std::is_convertible<U, T>::value>::type>
        constexpr T value_or(U&& v) const&
        {
            return this->m_engaged ? this->value() : static_cast<T>(forward<U>(v));
        }

        template<class U, class = typename enable_if<std::is_move_constructible<T>::value && std::is_convertible<U, T>::value>::type>
        /*constexpr*/ T value_or(U&& v) &&
        {
            return this->m_engaged ? AZStd::move(this->value()) : static_cast<T>(forward<U>(v));
        }

        // 23.6.3.6 Modifiers
        using base::reset;
    };

    // 23.6.6 Relational operators
    template <class T, class U>
    constexpr
    typename enable_if<
        // Template function is only enabled if the result of T() == U() is
        // convertible to bool
        is_convertible<
            decltype(declval<T>() == declval<U>()), bool
        >::value,
        bool // bool is the return type of operator==
    >::type
    operator==(const optional<T>& lhs, const optional<U>& rhs)
    {
        // MSVC2015 says that constexpr functions can only have 1 return
        // statement
        return (static_cast<bool>(lhs) != static_cast<bool>(rhs))
            ? false
            : (!static_cast<bool>(lhs))
                ? true
                : *lhs == *rhs;
    }

    template <class T, class U>
    constexpr
    typename enable_if<
        is_convertible<
            decltype(declval<T>() != declval<U>()), bool
        >::value,
        bool
    >::type
    operator!=(const optional<T>& lhs, const optional<U>& rhs)
    {
        return (static_cast<bool>(lhs) != static_cast<bool>(rhs))
            ? true
            : (!static_cast<bool>(lhs))
                ? false
                : *lhs != *rhs;
    }

    template <class T, class U>
    constexpr
    typename enable_if<
        is_convertible<
            decltype(declval<T>() < declval<U>()), bool
        >::value,
        bool
    >::type
    operator<(const optional<T>& lhs, const optional<U>& rhs)
    {
        // MSVC2015 constexpr functions can only have one return statement
        return (!static_cast<bool>(rhs))
            ? false
            : (!static_cast<bool>(lhs))
                ? true
                : *lhs < *rhs;
    }

    template <class T, class U>
    constexpr
    typename enable_if<
        is_convertible<
            decltype(declval<T>() > declval<U>()), bool
        >::value,
        bool
    >::type
    operator>(const optional<T>& lhs, const optional<U>& rhs)
    {
        // MSVC2015 constexpr functions can only have one return statement
        return (!static_cast<bool>(lhs))
            ? false
            : (!static_cast<bool>(rhs))
                ? true
                : *lhs > *rhs;
    }

    template <class T, class U>
    constexpr
    typename enable_if<
        is_convertible<
            decltype(declval<T>() <= declval<U>()), bool
        >::value,
        bool
    >::type
    operator<=(const optional<T>& lhs, const optional<U>& rhs)
    {
        // MSVC2015 constexpr functions can only have one return statement
        return (!static_cast<bool>(lhs))
            ? true
            : (!static_cast<bool>(rhs))
                ? false
                : *lhs <= *rhs;
    }

    template <class T, class U>
    constexpr
    typename enable_if<
        is_convertible<
            decltype(declval<T>() >= declval<U>()), bool
        >::value,
        bool
    >::type
    operator>=(const optional<T>& lhs, const optional<U>& rhs)
    {
        // MSVC2015 constexpr functions can only have one return statement
        return (!static_cast<bool>(rhs))
            ? true
            : (!static_cast<bool>(lhs))
                ? false
                : *lhs >= *rhs;
    }

    // 23.6.7 Comparison with nullopt
    template <class T>
    constexpr bool operator==(const optional<T>& lhs, nullopt_t)
    {
        return !static_cast<bool>(lhs);
    }
    template <class T>
    constexpr bool operator==(nullopt_t, const optional<T>& rhs)
    {
        return !static_cast<bool>(rhs);
    }

    template <class T>
    constexpr bool operator!=(const optional<T>& lhs, nullopt_t)
    {
        return static_cast<bool>(lhs);
    }
    template <class T>
    constexpr bool operator!=(nullopt_t, const optional<T>& rhs)
    {
        return static_cast<bool>(rhs);
    }

    template <class T>
    constexpr bool operator<(const optional<T>&, nullopt_t)
    {
        return false;
    }
    template <class T>
    constexpr bool operator<(nullopt_t, const optional<T>& rhs)
    {
        return static_cast<bool>(rhs);
    }

    template <class T>
    constexpr bool operator<=(const optional<T>& lhs, nullopt_t)
    {
        return !static_cast<bool>(lhs);
    }
    template <class T>
    constexpr bool operator<=(nullopt_t, const optional<T>&)
    {
        return true;
    }

    template <class T>
    constexpr bool operator>(const optional<T>& lhs, nullopt_t)
    {
        return static_cast<bool>(lhs);
    }
    template <class T>
    constexpr bool operator>(nullopt_t, const optional<T>&)
    {
        return false;
    }

    template <class T>
    constexpr bool operator>=(const optional<T>&, nullopt_t)
    {
        return true;
    }
    template <class T>
    constexpr bool operator>=(nullopt_t, const optional<T>& rhs)
    {
        return !static_cast<bool>(rhs);
    }

    // 23.6.8 Comparison with T
    template <class T, class U>
    constexpr
    typename enable_if<
        is_convertible<
            decltype(declval<T>() == declval<U>()), bool
        >::value,
        bool
    >::type
    operator==(const optional<T>& lhs, const U& rhs)
    {
        return static_cast<bool>(lhs) ? (*lhs == rhs) : false;
    }

    template <class T, class U>
    constexpr
    typename enable_if<
        is_convertible<
            decltype(declval<T>() == declval<U>()), bool
        >::value,
        bool
    >::type
    operator==(const T& lhs, const optional<U>& rhs)
    {
        return static_cast<bool>(rhs) ? (lhs == *rhs) : false;
    }

    template <class T, class U>
    constexpr
    typename enable_if<
        is_convertible<
            decltype(declval<T>() != declval<U>()), bool
        >::value,
        bool
    >::type
    operator!=(const optional<T>& lhs, const U& rhs)
    {
        return static_cast<bool>(lhs) ? (*lhs != rhs) : true;
    }

    template <class T, class U>
    constexpr
    typename enable_if<
        is_convertible<
            decltype(declval<T>() != declval<U>()), bool
        >::value,
        bool
    >::type
    operator!=(const T& lhs, const optional<U>& rhs)
    {
        return static_cast<bool>(rhs) ? (lhs != *rhs) : true;
    }

    template <class T, class U>
    constexpr
    typename enable_if<
        is_convertible<
            decltype(declval<T>() < declval<U>()), bool
        >::value,
        bool
    >::type
    operator<(const optional<T>& lhs, const U& rhs)
    {
        return static_cast<bool>(lhs) ? (*lhs < rhs) : true;
    }

    template <class T, class U>
    constexpr
    typename enable_if<
        is_convertible<
            decltype(declval<T>() < declval<U>()), bool
        >::value,
        bool
    >::type
    operator<(const T& lhs, const optional<U>& rhs)
    {
        return static_cast<bool>(rhs) ? (lhs < *rhs) : false;
    }

    template <class T, class U>
    constexpr
    typename enable_if<
        is_convertible<
            decltype(declval<T>() <= declval<U>()), bool
        >::value,
        bool
    >::type
    operator<=(const optional<T>& lhs, const U& rhs)
    {
        return static_cast<bool>(lhs) ? (*lhs <= rhs) : true;
    }

    template <class T, class U>
    constexpr
    typename enable_if<
        is_convertible<
            decltype(declval<T>() <= declval<U>()), bool
        >::value,
        bool
    >::type
    operator<=(const T& lhs, const optional<U>& rhs)
    {
        return static_cast<bool>(rhs) ? (lhs <= *rhs) : false;
    }

    template <class T, class U>
    constexpr
    typename enable_if<
        is_convertible<
            decltype(declval<T>() > declval<U>()), bool
        >::value,
        bool
    >::type
    operator>(const optional<T>& lhs, const U& rhs)
    {
        return static_cast<bool>(lhs) ? (*lhs > rhs) : false;
    }

    template <class T, class U>
    constexpr
    typename enable_if<
        is_convertible<
            decltype(declval<T>() > declval<U>()), bool
        >::value,
        bool
    >::type
    operator>(const T& lhs, const optional<U>& rhs)
    {
        return static_cast<bool>(rhs) ? (lhs > *rhs) : true;
    }

    template <class T, class U>
    constexpr
    typename enable_if<
        is_convertible<
            decltype(declval<T>() >= declval<U>()), bool
        >::value,
        bool
    >::type
    operator>=(const optional<T>& lhs, const U& rhs)
    {
        return static_cast<bool>(lhs) ? (*lhs >= rhs) : false;
    }

    template <class T, class U>
    constexpr
    typename enable_if<
        is_convertible<
            decltype(declval<T>() >= declval<U>()), bool
        >::value,
        bool
    >::type
    operator>=(const T& lhs, const optional<U>& rhs)
    {
        return static_cast<bool>(rhs) ? (lhs >= *rhs) : true;
    }

    // 23.6.9 Specialized algorithms
    template <class T>
    typename enable_if<
        is_move_constructible<T>::value,// && std::is_swappable_v<T>,
        void
    >::type
    swap(optional<T>& lhs, optional<T>& rhs)
    {
        lhs.swap(rhs);
    }

    template <class T>
    constexpr optional<typename decay<T>::type> make_optional(T&& v)
    {
        return optional<typename decay<T>::type>(forward<T>(v));
    }

    template <class T, class... Args>
    constexpr optional<T> make_optional(Args&&... args)
    {
        return optional<T>(in_place, forward<Args>(args)...);
    }

    template <class T, class U, class... Args>
    constexpr optional<T> make_optional(std::initializer_list<U> il, Args&&... args)
    {
        return optional<T>(in_place, il, forward<Args>(args)...);
    }

    // 23.6.10 Hash support
    template<class T>
    struct hash<optional<T>>
    {
        constexpr size_t operator()(const optional<T>& opt) const
        {
            return static_cast<bool>(opt) ? hash<T>{}(*opt) : 0;
        }
    };
}
