/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Console/ConsoleTypeHelpers.h>

namespace AZ
{
    inline const char* ConsoleFunctorBase::GetName() const
    {
        return m_name;
    }

    inline const char* ConsoleFunctorBase::GetDesc() const
    {
        return m_desc;
    }

    inline ConsoleFunctorFlags ConsoleFunctorBase::GetFlags() const
    {
        return m_flags;
    }

    inline const TypeId& ConsoleFunctorBase::GetTypeId() const
    {
        return m_typeId;
    }

    template <typename RETURN_TYPE>
    inline GetValueResult ConsoleFunctorBase::GetValue(RETURN_TYPE& outResult) const
    {
        CVarFixedString buffer;
        const GetValueResult resultCode = GetValueAsString(buffer);

        if (resultCode != GetValueResult::Success)
        {
            return resultCode;
        }

        return ConsoleTypeHelpers::ToValue(outResult, buffer)
            ? GetValueResult::Success
            : GetValueResult::TypeNotConvertible;
    }

    // This is forcibly inlined because we must guarantee this is compiled into and invoked from the calling module rather than the .exe that links AzCore
    AZ_FORCE_INLINE ConsoleFunctorBase*& ConsoleFunctorBase::GetDeferredHead()
    {
        s_deferredHeadInvoked = true;
        return s_deferredHead;
    }

    template <typename _TYPE, bool _REPLICATES_VALUE>
    inline ConsoleFunctor<_TYPE, _REPLICATES_VALUE>::ConsoleFunctor
    (
        const char* name,
        const char* desc, 
        ConsoleFunctorFlags flags,
        const TypeId& typeId,
        _TYPE& object,
        FunctorUnion function
    )
        : ConsoleFunctorBase(name, desc, flags, typeId)
        , m_object(&object)
        , m_function(AZStd::move(function))
    {
        ;
    }

    template <typename _TYPE, bool _REPLICATES_VALUE>
    inline ConsoleFunctor<_TYPE, _REPLICATES_VALUE>::ConsoleFunctor
    (
        AZ::IConsole& console,
        const char* name,
        const char* desc,
        ConsoleFunctorFlags flags,
        const TypeId& typeId,
        _TYPE& object,
        FunctorUnion function
    )
        : ConsoleFunctorBase(console, name, desc, flags, typeId)
        , m_object(&object)
        , m_function(AZStd::move(function))
    {
        ;
    }

    template <typename _TYPE, bool _REPLICATES_VALUE>
    inline void ConsoleFunctor<_TYPE, _REPLICATES_VALUE>::operator()(const ConsoleCommandContainer& arguments)
    {
        auto functorVisitor = [this, arguments](auto&& functor)
        {
            AZStd::invoke(functor, *m_object, arguments);
        };
        AZStd::visit(functorVisitor, m_function);
    }

    template <typename _TYPE, bool _REPLICATES_VALUE>
    struct ConsoleReplicateHelper;

    template <typename _TYPE>
    struct ConsoleReplicateHelper<_TYPE, false>
    {
        static bool GetReplicationString(_TYPE&, const char* name, CVarFixedString& outString)
        {
            outString = name;
            return false;
        }

        static bool StringToValue(_TYPE&, const ConsoleCommandContainer&)
        {
            return false;
        }

        static bool ValueToString(_TYPE&, CVarFixedString&)
        {
            return false;
        }
    };

    template <typename _TYPE>
    struct ConsoleReplicateHelper<_TYPE, true>
    {
        static bool GetReplicationString(_TYPE& instance, const char* name, CVarFixedString& outString)
        {
            CVarFixedString valueString;
            ValueToString(instance, valueString);
            outString = CVarFixedString(name) + " " + valueString;
            return true;
        }

        static bool StringToValue(_TYPE& instance, const ConsoleCommandContainer& arguments)
        {
            return instance.StringToValue(arguments);
        }

        static void ValueToString(_TYPE& instance, CVarFixedString& outString)
        {
            instance.ValueToString(outString);
        }
    };

    template<>
    struct ConsoleReplicateHelper<int, true>
    {
        static bool GetReplicationString(int& instance, const char* name, CVarFixedString& outString)
        {
            CVarFixedString valueString;
            ValueToString(instance, valueString);
            outString = CVarFixedString(name) + " " + valueString;
            return true;
        }

        static bool StringToValue(int& instance, const ConsoleCommandContainer& arguments)
        {
            return ConsoleTypeHelpers::ToValue(instance, arguments);
        }

        static void ValueToString(int& instance, CVarFixedString& outString)
        {
            outString = ConsoleTypeHelpers::ToString(instance);
        }
    };

    template<>
    struct ConsoleReplicateHelper<float, true>
    {
        static bool GetReplicationString(float& instance, const char* name, CVarFixedString& outString)
        {
            CVarFixedString valueString;
            ValueToString(instance, valueString);
            outString = CVarFixedString(name) + " " + valueString;
            return true;
        }

        static bool StringToValue(float& instance, const ConsoleCommandContainer& arguments)
        {
            return ConsoleTypeHelpers::ToValue(instance, arguments);
        }

        static void ValueToString(float& instance, CVarFixedString& outString)
        {
            outString = ConsoleTypeHelpers::ToString(instance);
        }
    };

    template<>
    struct ConsoleReplicateHelper<AZStd::string, true>
    {
        static bool GetReplicationString(AZStd::string&, const char*, CVarFixedString&)
        {
            return true;
        }

        static bool StringToValue(AZStd::string& instance, const ConsoleCommandContainer& arguments)
        {
            return ConsoleTypeHelpers::ToValue(instance, arguments);
        }

        static void ValueToString(AZStd::string& instance, CVarFixedString& outString)
        {
            outString = instance;
        }
    };

    template <typename _TYPE, bool _REPLICATES_VALUE>
    inline bool ConsoleFunctor<_TYPE, _REPLICATES_VALUE>::GetReplicationString(CVarFixedString& outString) const
    {
        return ConsoleReplicateHelper<_TYPE, _REPLICATES_VALUE>::GetReplicationString(*m_object, GetName(), outString);
    }

    template <typename _TYPE, bool _REPLICATES_VALUE>
    inline _TYPE& ConsoleFunctor<_TYPE, _REPLICATES_VALUE>::GetValue()
    {
        return *m_object;
    }

    template <typename _TYPE, bool _REPLICATES_VALUE>
    inline GetValueResult ConsoleFunctor<_TYPE, _REPLICATES_VALUE>::GetValueAsString(CVarFixedString& outString) const
    {
        ConsoleReplicateHelper<_TYPE, _REPLICATES_VALUE>::ValueToString(*m_object, outString);
        return GetValueResult::Success;
    }

    // ConsoleFunctor Specialization for running non-member console functions with no instance
    template <bool _REPLICATES_VALUE>
    inline ConsoleFunctor<void, _REPLICATES_VALUE>::ConsoleFunctor
    (
        const char* name,
        const char* desc,
        ConsoleFunctorFlags flags,
        const TypeId& typeId,
        FunctorSignature function
    )
        : ConsoleFunctorBase(name, desc, flags, typeId)
        , m_function(function)
    {
        ;
    }

    template <bool _REPLICATES_VALUE>
    inline ConsoleFunctor<void, _REPLICATES_VALUE>::ConsoleFunctor
    (
        AZ::IConsole& console,
        const char* name,
        const char* desc,
        ConsoleFunctorFlags flags,
        const TypeId& typeId,
        FunctorSignature function
    )
        : ConsoleFunctorBase(console, name, desc, flags, typeId)
        , m_function(function)
    {
        ;
    }

    template <bool _REPLICATES_VALUE>
    inline void ConsoleFunctor<void, _REPLICATES_VALUE>::operator()(const ConsoleCommandContainer& arguments)
    {
        (*m_function)(arguments);
    }

    template <bool _REPLICATES_VALUE>
    inline bool ConsoleFunctor<void, _REPLICATES_VALUE>::GetReplicationString(CVarFixedString& outString) const
    {
        outString = GetName();
        return false;
    }

    template <bool _REPLICATES_VALUE>
    inline GetValueResult ConsoleFunctor<void, _REPLICATES_VALUE>::GetValueAsString([[maybe_unused]] CVarFixedString& outString) const
    {
        return GetValueResult::NotImplemented;
    }
}
