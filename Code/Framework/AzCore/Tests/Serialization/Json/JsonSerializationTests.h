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

#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>
#include <Tests/Serialization/Json/TestCases_Base.h>

namespace JsonSerializationTests
{
    namespace A
    {
        struct Inherited
            : public BaseClass
        {
            AZ_RTTI(Inherited, "{E7829F37-C577-4F2B-A85B-6F331548354C}", BaseClass);

            ~Inherited() override = default;
        };
    }

    namespace B
    {
        struct Inherited
            : public BaseClass
        {
            AZ_RTTI(Inherited, "{0DF033C4-3EEC-4F61-8B79-59FE31545029}", BaseClass);

            ~Inherited() override = default;
        };
    }

    namespace C
    {
        struct Inherited
        {
            AZ_RTTI(Inherited, "{682089DB-9794-4590-87A4-9AF70BD1C202}");
            virtual ~Inherited() = default;
        };
    }

    struct ConflictingNameTestClass
    {
        AZ_RTTI(ConflictingNameTestClass, "{370EFD10-781B-47BF-B0A1-6FC4E9D55CBC}");

        virtual ~ConflictingNameTestClass() = default;

        AZStd::shared_ptr<BaseClass> m_pointer;
        ConflictingNameTestClass()
        {
            m_pointer = AZStd::make_shared<A::Inherited>();
        }
    };

    struct EmptyClass
    {
        AZ_RTTI(EmptyClass, "{9E69A37B-22BD-4E3F-9A80-63D902966591}");

        virtual ~EmptyClass() = default;
    };

    struct ClassWithPointerToUnregisteredClass
    {
        AZ_RTTI(ClassWithPointerToUnregisteredClass, "{7CBA9A8F-8576-456E-B4C0-DE5797D54694}");

        EmptyClass* m_ptr;

        ClassWithPointerToUnregisteredClass()
            : m_ptr(new EmptyClass())
        {
        }

        virtual ~ClassWithPointerToUnregisteredClass()
        {
            delete m_ptr;
            m_ptr = nullptr;
        }
    };

    class JsonSerializationTests
        : public BaseJsonSerializerFixture
    {
    };
} // namespace JsonSerializationTests
