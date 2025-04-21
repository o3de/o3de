/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <stdint.h>
#include <AzCore/base.h>
#include <AzCore/Console/IConsoleTypes.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/variant.h>

namespace AZ
{
    class IConsole;
    class Console;

    enum class GetValueResult
    {
        Success,
        NotImplemented,
        TypeNotConvertible,
        ConsoleVarNotFound,
    };
    const char* GetEnumString(GetValueResult value);

    //! @class ConsoleFunctorBase
    //! Base class for console functors.
    class ConsoleFunctorBase
    {
    public:

        //! Constructor.
        //! @param name   the string name of the functor, used to identify and invoke the functor through the console interface
        //! @param desc   a string help description of the functor
        //! @param flags  a set of flags that mutate the behaviour of the functor
        //! @param typeId the TypeId associated with this console functor
        ConsoleFunctorBase(const char* name, const char* desc, ConsoleFunctorFlags flags, const TypeId& typeId);

        //! Console registration constructor.
        //! @param console reference to valid console interface that the console functor will register with
        //! @param name    the string name of the functor, used to identify and invoke the functor through the console interface
        //! @param desc    a string help description of the functor
        //! @param flags   a set of flags that mutate the behaviour of the functor
        //! @param typeId  the TypeId associated with this console functor
        ConsoleFunctorBase(AZ::IConsole& console, const char* name, const char* desc, ConsoleFunctorFlags flags, const TypeId& typeId);

        //! Move Constructor
        //! Unlinks the old console functor base address of the console interface and links the new
        //! console base instance to it
        ConsoleFunctorBase(ConsoleFunctorBase&& other);
        ConsoleFunctorBase& operator=(ConsoleFunctorBase&& other);

        //! Destructor.
        virtual ~ConsoleFunctorBase();

        //! Returns the string name of this functor.
        //! @return the string name of this functor
        const char* GetName() const;

        //! Returns the help descriptor of this functor.
        //! @return the help descriptor of this functor
        const char* GetDesc() const;

        //! Returns the flags set on this functor instance.
        //! @return the flags set on this functor instance
        ConsoleFunctorFlags GetFlags() const;

        //! Returns the TypeId of the bound type if one exists.
        //! @return the TypeId of the bound type if one exists
        const TypeId& GetTypeId() const;

        //! Execute operator, calling this executes the functor.
        //! @param arguments set of string inputs to the functor
        virtual void operator()(const ConsoleCommandContainer& arguments) = 0;

        //! For functors that can be replicated (cvars).
        //! This will generate a replication string suitable for remote execution
        //! @param outString the output string which can be remotely executed
        //! @return boolean true if a proper replicated value was returned, false otherwise
        virtual bool GetReplicationString(CVarFixedString& outString) const = 0;

        //! Attempts to retrieve the functor object instance as the provided type.
        //! @param outResult the value to store the result in
        //! @return GetConsoleValueResult::Success or an appropriate error code
        template <typename RETURN_TYPE>
        GetValueResult GetValue(RETURN_TYPE& outResult) const;

        //! Used internally to link cvars and functors from various modules to the console as they are loaded.
        static ConsoleFunctorBase*& GetDeferredHead();

    protected:

        virtual GetValueResult GetValueAsString(CVarFixedString& outString) const;

        void Link(ConsoleFunctorBase*& head);
        void Unlink(ConsoleFunctorBase*& head);

        const char*  m_name = "";
        const char*  m_desc = "";
        ConsoleFunctorFlags m_flags = ConsoleFunctorFlags::Null;
        TypeId m_typeId;

        IConsole* m_console = nullptr;

        ConsoleFunctorBase* m_prev = nullptr;
        ConsoleFunctorBase* m_next = nullptr;

        bool m_isDeferred = true;

        static ConsoleFunctorBase* s_deferredHead;
        static bool s_deferredHeadInvoked;

        friend class Console;
    };

    template<class T, class = void>
    struct ConsoleCommandMemberFunctorSignature
    {
        using type = void (*)(T&, const ConsoleCommandContainer&);
    };

    template<class T>
    struct ConsoleCommandMemberFunctorSignature<T, AZStd::enable_if_t<AZStd::is_class_v<T> && !AZStd::is_const_v<T>>>
    {
        using type = void (T::*)(const ConsoleCommandContainer&);
    };

    template<class T>
    struct ConsoleCommandMemberFunctorSignature <T, AZStd::enable_if_t<AZStd::is_class_v<T> && AZStd::is_const_v<T>>>
    {
        using type = void (T::*)(const ConsoleCommandContainer&) const;
    };

    //! @class ConsoleFunctor
    //! @brief Console functor which wraps a function call into an object instance.
    template <typename _TYPE, bool _REPLICATES_VALUE>
    class ConsoleFunctor final
        : public ConsoleFunctorBase
    {
    public:

        using MemberFunctorSignature = typename ConsoleCommandMemberFunctorSignature<_TYPE>::type;
        using RawFunctorSignature = void(*)(_TYPE&, const ConsoleCommandContainer&);

        // This alias avoids the issue with two of the exact same types being added as alternatives to a variant,
        // which would cause ambiguity errors when initializing the variants without using the explicit index.
        // The solution is to collapse the variant to a single option as there isn't a need to have two alternatives in this case.
        using FunctorUnion = AZStd::conditional_t<
            AZStd::is_same_v<RawFunctorSignature, MemberFunctorSignature>,
            AZStd::variant<RawFunctorSignature>,
            AZStd::variant<RawFunctorSignature, MemberFunctorSignature>>;

        //! Constructors.
        //! @param name   the string name of the functor, used to identify and invoke the functor through the console interface
        //! @param desc   a string help description of the functor
        //! @param flags  a set of flags that mutate the behaviour of the functor
        //! @param typeId the TypeId associated with this console functor
        //! @param object reference to the data storage object
        ConsoleFunctor(const char* name, const char* desc, ConsoleFunctorFlags flags, const TypeId& typeId, _TYPE& object, FunctorUnion function);
        ConsoleFunctor(IConsole& console, const char* name, const char* desc, ConsoleFunctorFlags flags, const TypeId& typeId, _TYPE& object, FunctorUnion function);

        // Console Functors are movable
        ConsoleFunctor(ConsoleFunctor&&) = default;
        ConsoleFunctor& operator=(ConsoleFunctor&&) = default;

        ~ConsoleFunctor() override = default;

        //! ConsoleFunctorBase overrides
        //! @{
        void operator()(const ConsoleCommandContainer& arguments) override;
        bool GetReplicationString(CVarFixedString& outString) const override;
        //! @}

        //! Returns reference typed stored type wrapped stored by ConsoleFunctor.
        //! @return reference to underlying type
        _TYPE& GetValue();

    private:

        GetValueResult GetValueAsString(CVarFixedString& outString) const override;

        // Console Functors are not copyable
        ConsoleFunctor& operator=(const ConsoleFunctor&) = delete;
        ConsoleFunctor(const ConsoleFunctor&) = delete;

        _TYPE* m_object{};
        FunctorUnion m_function;
    };

    //! @class ConsoleFunctor
    //! @brief Console functor specialization for non-member console functions (no instance to invoke on).
    template <bool _REPLICATES_VALUE>
    class ConsoleFunctor<void, _REPLICATES_VALUE> final
        : public ConsoleFunctorBase
    {
    public:

        using FunctorSignature = void(*)(const ConsoleCommandContainer&);

        ConsoleFunctor(const char* name, const char* desc, ConsoleFunctorFlags flags, const TypeId& typeId, FunctorSignature function);
        ConsoleFunctor(AZ::IConsole& console, const char* name, const char* desc, ConsoleFunctorFlags flags, const TypeId& typeId, FunctorSignature function);

        // Console Functors are movable
        ConsoleFunctor(ConsoleFunctor&&) = default;
        ConsoleFunctor& operator=(ConsoleFunctor&&) = default;

        ~ConsoleFunctor() override = default;

        //! ConsoleFunctorBase overrides
        //! @{
        void operator()(const ConsoleCommandContainer& arguments) override;
        bool GetReplicationString(CVarFixedString& outString) const override;
        //! @}

    private:

        GetValueResult GetValueAsString([[maybe_unused]] CVarFixedString& outString) const override;

        FunctorSignature m_function;
    };
}

#include <AzCore/Console/ConsoleFunctor.inl>
