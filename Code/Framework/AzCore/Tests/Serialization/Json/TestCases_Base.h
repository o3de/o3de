/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        const char* m_json;

        InstanceWithoutDefaults(AZStd::unique_ptr<T>&& instance, const char* json)
            : m_instance(AZStd::move(instance))
            , m_json(json)
        {
        }
    };

    template<typename T>
    InstanceWithoutDefaults<T> MakeInstanceWithoutDefaults(AZStd::unique_ptr<T> instance, const char* json)
    {
        return InstanceWithoutDefaults<T>(AZStd::move(instance), json);
    }

    struct BaseClass
    {
        template<class T>
        friend struct AZStd::IntrusivePtrCountPolicy;

        AZ_CLASS_ALLOCATOR(BaseClass, AZ::SystemAllocator, 0);
        AZ_RTTI(BaseClass, "{E0A66EB6-1AC5-4C73-B1A7-6367A47EC026}");

        virtual ~BaseClass() = default;

        virtual void add_ref();
        virtual void release();

        bool Equals(const BaseClass& rhs, bool) const;
        static void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context);

        mutable int m_refCount = 0;
        float m_baseVar{ -42.0f };
    };

    struct BaseClass2
    {
        template<class T>
        friend struct AZStd::IntrusivePtrCountPolicy;

        AZ_CLASS_ALLOCATOR(BaseClass2, AZ::SystemAllocator, 0);
        AZ_RTTI(BaseClass2, "{F9D704C1-04AF-463C-B47A-02C28805AAEE}");

        virtual ~BaseClass2() = default;

        virtual void add_ref();
        virtual void release();

        bool Equals(const BaseClass2& rhs, bool) const;
        static void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context);

        mutable int m_refCount = 0;
        double m_base2Var1{ -133.0 };
        double m_base2Var2{ -233.0 };
        double m_base2Var3{ -333.0 };
    };
} // namespace JsonSerializationTests
