/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace JsonSerializationTests
{
    template<typename T>
    struct InstanceWithSomeDefaults
    {
        AZStd::unique_ptr<T> m_instance;
        const char* m_jsonWithStrippedDefaults;
        const char* m_jsonWithKeptDefaults;

        InstanceWithSomeDefaults(AZStd::unique_ptr<T> instance,
            const char* jsonForStrippedDefaults, const char* jsonForKeptDefaults)
            : m_instance(AZStd::move(instance))
            , m_jsonWithStrippedDefaults(jsonForStrippedDefaults)
            , m_jsonWithKeptDefaults(jsonForKeptDefaults)
        {
        }
    };

    template<typename T>
    InstanceWithSomeDefaults<T> MakeInstanceWithSomeDefaults(AZStd::unique_ptr<T> instance,
        const char* jsonWithStrippedDefaults, const char* jsonWithKeptDefaults)
    {
        return InstanceWithSomeDefaults<T>(AZStd::move(instance), jsonWithStrippedDefaults, jsonWithKeptDefaults);
    }

    template<typename T>
    struct InstanceWithoutDefaults
    {
        AZStd::unique_ptr<T> m_instance;
        const char* m_jsonWithStrippedDefaults;
        const char* m_jsonWithKeptDefaults;

        InstanceWithoutDefaults(AZStd::unique_ptr<T>&& instance, const char* jsonWithStrippedDefaults, const char* jsonWithKeptDefaults)
            : m_instance(AZStd::move(instance))
            , m_jsonWithStrippedDefaults(jsonWithStrippedDefaults)
            , m_jsonWithKeptDefaults(jsonWithKeptDefaults)
        {
        }
    };

    template<typename T>
    InstanceWithoutDefaults<T> MakeInstanceWithoutDefaults(AZStd::unique_ptr<T> instance, const char* jsonWithStrippedDefaults, const char* jsonWithKeptDefaults = "")
    {
        return InstanceWithoutDefaults<T>(AZStd::move(instance), jsonWithStrippedDefaults, !AZStd::string_view(jsonWithKeptDefaults).empty() ? jsonWithKeptDefaults : jsonWithStrippedDefaults);
    }

    struct BaseClass
    {
        template<class T>
        friend struct AZStd::IntrusivePtrCountPolicy;

        AZ_CLASS_ALLOCATOR(BaseClass, AZ::SystemAllocator);
        AZ_RTTI(BaseClass, "{E0A66EB6-1AC5-4C73-B1A7-6367A47EC026}");

        virtual ~BaseClass() = default;

        virtual void add_ref();
        virtual void release();

        virtual BaseClass* Clone() const;
        bool Equals(const BaseClass& rhs, bool) const;
        static void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context);

        mutable int m_refCount = 0;
        float m_baseVar{ -42.0f };
    };

    struct BaseClass2
    {
        template<class T>
        friend struct AZStd::IntrusivePtrCountPolicy;

        AZ_CLASS_ALLOCATOR(BaseClass2, AZ::SystemAllocator);
        AZ_RTTI(BaseClass2, "{F9D704C1-04AF-463C-B47A-02C28805AAEE}");

        virtual ~BaseClass2() = default;

        virtual void add_ref();
        virtual void release();

        virtual BaseClass2* Clone() const;
        bool Equals(const BaseClass2& rhs, bool) const;
        static void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context);

        mutable int m_refCount = 0;
        double m_base2Var1{ -133.0 };
        double m_base2Var2{ -233.0 };
        double m_base2Var3{ -333.0 };
    };
} // namespace JsonSerializationTests
