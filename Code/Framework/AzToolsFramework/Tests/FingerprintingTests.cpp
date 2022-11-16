/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/Fingerprinting/TypeFingerprinter.h>

using namespace AzToolsFramework;
using namespace AzToolsFramework::Fingerprinting;

namespace UnitTest
{
    class ReflectedTestClass
    {
    public:
        AZ_TYPE_INFO(ReflectedTestClass, "{AE55A3D4-845B-457F-94BA-A708BBDD6307}");
        AZ_CLASS_ALLOCATOR(ReflectedTestClass, AZ::SystemAllocator, 0);

        ~ReflectedTestClass()
        {
            delete m_property1AsPointer;
        }

        int m_property1;

        bool m_property1AsBool;
        int *m_property1AsPointer = nullptr;

        int m_property2;

        static void ReflectDefault(AZ::SerializeContext& context)
        {
            context.Class<ReflectedTestClass>()
                ->Field("Property1", &ReflectedTestClass::m_property1);
        }

        static void ReflectHigherVersion(AZ::SerializeContext& context)
        {
            context.Class<ReflectedTestClass>()
                ->Version(2)
                ->Field("Property1", &ReflectedTestClass::m_property1);
        }

        static void ReflectRenamedProperty(AZ::SerializeContext& context)
        {
            context.Class<ReflectedTestClass>()
                ->Field("Property1Renamed", &ReflectedTestClass::m_property1);
        }

        static void ReflectPropertyWithDifferentType(AZ::SerializeContext& context)
        {
            context.Class<ReflectedTestClass>()
                ->Field("Property", &ReflectedTestClass::m_property1AsBool);
        }

        static void ReflectPropertyAsPointer(AZ::SerializeContext& context)
        {
            context.Class<ReflectedTestClass>()
                ->Field("Property1", &ReflectedTestClass::m_property1AsPointer);
        }

        static void ReflectTwoProperties(AZ::SerializeContext& context)
        {
            context.Class<ReflectedTestClass>()
                ->Field("Property1", &ReflectedTestClass::m_property1)
                ->Field("Property2", &ReflectedTestClass::m_property2);
        }
    };

    class ReflectedBaseClass
    {
    public:
        AZ_TYPE_INFO(ReflectedBaseClass, "{B53DC61E-6E8A-4F0A-82E4-864FA50326E5}");
        virtual ~ReflectedBaseClass() = default;

        static void ReflectDefault(AZ::SerializeContext& context)
        {
            context.Class<ReflectedBaseClass>();
        }
    };

    class ReflectedSubClass : public ReflectedBaseClass
    {
    public:
        AZ_TYPE_INFO(ReflectedSubClass, "{B95E143C-D97E-44F3-8F38-BAB6F317A03C}");

        static void ReflectWithInheritance(AZ::SerializeContext& context)
        {
            context.Class<ReflectedSubClass, ReflectedBaseClass>();
        }

        static void ReflectWithoutInheritance(AZ::SerializeContext& context)
        {
            context.Class<ReflectedSubClass>();
        }
    };

    class ReflectedClassWithPointer
    {
    public:
        AZ_TYPE_INFO(ReflectedClassWithPointer, "{03DE24B9-288B-41B5-952D-4749F8F400D2}");

        ~ReflectedClassWithPointer()
        {
            delete m_pointer;
        }

        ReflectedTestClass* m_pointer = nullptr;

        static void Reflect(AZ::SerializeContext& context)
        {
            context.Class<ReflectedClassWithPointer>()
                ->Field("Pointer", &ReflectedClassWithPointer::m_pointer);
        }
    };

    using FingerprintTests = UnitTest::AllocatorsTestFixture;

    TEST_F(FingerprintTests, IntFingerprint_IsValid)
    {
        AZ::SerializeContext serializeContext;
        TypeFingerprinter fingerprinter{ serializeContext };
        EXPECT_NE(InvalidTypeFingerprint, fingerprinter.GetFingerprint<int>());
    }

    TEST_F(FingerprintTests, ClassFingerprint_IsValid)
    {
        AZ::SerializeContext serializeContext;
        ReflectedTestClass::ReflectDefault(serializeContext);

        TypeFingerprinter fingerprinter{ serializeContext };
        EXPECT_NE(InvalidTypeFingerprint, fingerprinter.GetFingerprint<ReflectedTestClass>());
    }

    TEST_F(FingerprintTests, ClassWithNewVersionNumber_ChangesFingerprint)
    {
        AZ::SerializeContext serializeContext1;
        ReflectedTestClass::ReflectDefault(serializeContext1);
        TypeFingerprinter fingerprinter1{ serializeContext1 };

        AZ::SerializeContext serializeContext2;
        ReflectedTestClass::ReflectHigherVersion(serializeContext2);
        TypeFingerprinter fingerprinter2{ serializeContext2 };

        EXPECT_NE(fingerprinter1.GetFingerprint<ReflectedTestClass>(), fingerprinter2.GetFingerprint<ReflectedTestClass>());
    }

    TEST_F(FingerprintTests, ClassWithRenamedProperty_ChangesFingerprint)
    {
        AZ::SerializeContext serializeContext1;
        ReflectedTestClass::ReflectDefault(serializeContext1);
        TypeFingerprinter fingerprinter1{ serializeContext1 };

        AZ::SerializeContext serializeContext2;
        ReflectedTestClass::ReflectRenamedProperty(serializeContext2);
        TypeFingerprinter fingerprinter2{ serializeContext2 };

        EXPECT_NE(fingerprinter1.GetFingerprint<ReflectedTestClass>(), fingerprinter2.GetFingerprint<ReflectedTestClass>());
    }

    TEST_F(FingerprintTests, ClassWithPropertyThatChangesType_ChangesFingerprint)
    {
        AZ::SerializeContext serializeContext1;
        ReflectedTestClass::ReflectDefault(serializeContext1);
        TypeFingerprinter fingerprinter1{ serializeContext1 };

        AZ::SerializeContext serializeContext2;
        ReflectedTestClass::ReflectPropertyWithDifferentType(serializeContext2);
        TypeFingerprinter fingerprinter2{ serializeContext2 };

        EXPECT_NE(fingerprinter1.GetFingerprint<ReflectedTestClass>(), fingerprinter2.GetFingerprint<ReflectedTestClass>());
    }

    TEST_F(FingerprintTests, ClassWithPropertyThatChangesToPointer_ChangesFingerprint)
    {
        AZ::SerializeContext serializeContext1;
        ReflectedTestClass::ReflectDefault(serializeContext1);
        TypeFingerprinter fingerprinter1{ serializeContext1 };

        AZ::SerializeContext serializeContext2;
        ReflectedTestClass::ReflectPropertyAsPointer(serializeContext2);
        TypeFingerprinter fingerprinter2{ serializeContext2 };

        EXPECT_NE(fingerprinter1.GetFingerprint<ReflectedTestClass>(), fingerprinter2.GetFingerprint<ReflectedTestClass>());
    }

    TEST_F(FingerprintTests, ClassWithNewProperty_ChangesFingerprint)
    {
        AZ::SerializeContext serializeContext1;
        ReflectedTestClass::ReflectDefault(serializeContext1);
        TypeFingerprinter fingerprinter1{ serializeContext1 };

        AZ::SerializeContext serializeContext2;
        ReflectedTestClass::ReflectTwoProperties(serializeContext2);
        TypeFingerprinter fingerprinter2{ serializeContext2 };

        EXPECT_NE(fingerprinter1.GetFingerprint<ReflectedTestClass>(), fingerprinter2.GetFingerprint<ReflectedTestClass>());
    }

    TEST_F(FingerprintTests, ClassGainingBaseClass_ChangesFingerprint)
    {
        AZ::SerializeContext serializeContext1;
        ReflectedBaseClass::ReflectDefault(serializeContext1);
        ReflectedSubClass::ReflectWithoutInheritance(serializeContext1);
        TypeFingerprinter fingerprinter1{ serializeContext1 };

        AZ::SerializeContext serializeContext2;
        ReflectedBaseClass::ReflectDefault(serializeContext2);
        ReflectedSubClass::ReflectWithInheritance(serializeContext2);
        TypeFingerprinter fingerprinter2{ serializeContext2 };

        EXPECT_NE(fingerprinter1.GetFingerprint<ReflectedSubClass>(), fingerprinter2.GetFingerprint<ReflectedSubClass>());
    }

    TEST_F(FingerprintTests, GatherAllTypesInObject_FindsCorrectTypes)
    {
        AZ::SerializeContext serializeContext;
        ReflectedTestClass::ReflectDefault(serializeContext);
        TypeFingerprinter fingerprinter{ serializeContext };

        ReflectedTestClass object;
        TypeCollection typesInObject;
        fingerprinter.GatherAllTypesInObject(&object, typesInObject);

        EXPECT_EQ(2, typesInObject.size());
        EXPECT_EQ(1, typesInObject.count(AZ::SerializeTypeInfo<int>::GetUuid()));
        EXPECT_EQ(1, typesInObject.count(AZ::SerializeTypeInfo<ReflectedTestClass>::GetUuid()));
    }

    TEST_F(FingerprintTests, GatherAllTypesInObjectWithBaseClass_FindsCorrectTypes)
    {
        AZ::SerializeContext serializeContext;
        ReflectedBaseClass::ReflectDefault(serializeContext);
        ReflectedSubClass::ReflectWithInheritance(serializeContext);
        TypeFingerprinter fingerprinter{ serializeContext };

        ReflectedSubClass object;
        TypeCollection typesInObject;
        fingerprinter.GatherAllTypesInObject(&object, typesInObject);

        EXPECT_EQ(2, typesInObject.size());
        EXPECT_EQ(1, typesInObject.count(AZ::SerializeTypeInfo<ReflectedSubClass>::GetUuid()));
        EXPECT_EQ(1, typesInObject.count(AZ::SerializeTypeInfo<ReflectedBaseClass>::GetUuid()));
    }

    TEST_F(FingerprintTests, GatherTypesInObjectWithNullPointer_FindsCorrectTypes)
    {
        AZ::SerializeContext serializeContext;
        ReflectedClassWithPointer::Reflect(serializeContext);
        ReflectedTestClass::ReflectDefault(serializeContext);
        TypeFingerprinter fingerprinter{ serializeContext };

        ReflectedClassWithPointer classWithPointer;
        classWithPointer.m_pointer = nullptr;

        TypeCollection typesInObject;
        fingerprinter.GatherAllTypesInObject(&classWithPointer, typesInObject);

        // shouldn't gather types from ReflectedTestClass, since m_pointer is null
        EXPECT_EQ(1, typesInObject.size());
        EXPECT_EQ(1, typesInObject.count(AZ::SerializeTypeInfo<ReflectedClassWithPointer>::GetUuid()));
    }

    TEST_F(FingerprintTests, GatherTypesInObjectWithValidPointer_FindsCorrectTypes)
    {
        AZ::SerializeContext serializeContext;
        ReflectedClassWithPointer::Reflect(serializeContext);
        ReflectedTestClass::ReflectDefault(serializeContext);
        TypeFingerprinter fingerprinter{ serializeContext };

        ReflectedClassWithPointer classWithPointer;
        classWithPointer.m_pointer = aznew ReflectedTestClass();

        TypeCollection typesInObject;
        fingerprinter.GatherAllTypesInObject(&classWithPointer, typesInObject);

        // should have followed m_pointer and gathered types from ReflectedTestClass
        EXPECT_EQ(3, typesInObject.size());
        EXPECT_EQ(1, typesInObject.count(AZ::SerializeTypeInfo<ReflectedClassWithPointer>::GetUuid()));
        EXPECT_EQ(1, typesInObject.count(AZ::SerializeTypeInfo<ReflectedTestClass>::GetUuid()));
        EXPECT_EQ(1, typesInObject.count(AZ::SerializeTypeInfo<int>::GetUuid()));
    }

    TEST_F(FingerprintTests, GenerateFingerprintForAllTypesInObject_Works)
    {
        AZ::SerializeContext serializeContext;
        ReflectedTestClass::ReflectDefault(serializeContext);
        TypeFingerprinter fingerprinter{ serializeContext };

        ReflectedTestClass object;

        EXPECT_NE(InvalidTypeFingerprint, fingerprinter.GenerateFingerprintForAllTypesInObject(&object));
    }
} // namespace UnitTest
