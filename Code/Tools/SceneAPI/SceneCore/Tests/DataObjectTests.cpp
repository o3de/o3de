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

#include <string>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <ScenePopulation/ScenePopulation/Containers/DataObject.h>

namespace Containers = AZ::ScenePopulation::Containers;

class ComplexObject
{
public:
    AZ_RTTI(ComplexObject, "{1969D4A3-3714-4E1C-816E-E066538A2A0B}")

    ComplexObject()
        : m_value0(42)
        , m_value1("test")
    {}

    explicit ComplexObject(int value0)
        : m_value0(value0)
        , m_value1("test")
    {}

    ComplexObject(int value0, const std::string& value1)
        : m_value0(value0)
        , m_value1(value1)
    {}

    bool operator==(const ComplexObject& rhs) const
    {
        return m_value0 == rhs.m_value0 && m_value1 == rhs.m_value1;
    }

    int m_value0;
    std::string m_value1;
};

class DestructionClass
{
public:
    AZ_RTTI(DestructionClass, "{6EFE6C0C-80E8-4D62-9705-A7315049DFBF}")

    ~DestructionClass()
    {
        Destructor();
    }
    MOCK_METHOD0(Destructor, void());
};

class BaseInterface
{
public:
    AZ_RTTI(BaseInterface, "{723F701D-B718-432B-AE67-78F999A64883}")
};

class SecondInterface
{
public:
    AZ_RTTI(SecondInterface, "{8D0420F1-C102-4615-94F3-9D39E0D3A272}")
};

class SingleInheritance
    : public BaseInterface
{
public:
    AZ_RTTI(SingleInheritance, "{2D5EA157-8137-4FD4-A08D-58941FB149B2}", BaseInterface);
};

class MultipleInheritance
    : public BaseInterface
    , public SecondInterface
{
public:
    AZ_RTTI(MultipleInheritance, "{277E1CAA-EB89-4820-B37E-B49922FD0DF9}", BaseInterface, SecondInterface);
};

class ReflectionClass
{
public:
    AZ_RTTI(ReflectionClass, "{81E5CB64-B879-440F-9659-9D45AA544144}")

    uint32_t testVar;

    MOCK_METHOD0(Reflect, void());

    static ReflectionClass* currentTestClass;
    static void Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->
                Class<ReflectionClass>()->
                Version(1)->
                Field("testVar", &ReflectionClass::testVar);
        }
        currentTestClass->Reflect();
    }
};
ReflectionClass* ReflectionClass::currentTestClass = nullptr;

class DataObjectTest
    : public ::testing::Test
{
public:
    DataObjectTest()
    {
    }
};

namespace TestNamespace
{
    class TestClassInNamespace
    {
    public:
        AZ_RTTI(TestClassInNamespace, "{F6CF64ED-6255-4E93-A34D-8BE58546C12E}")
    };
}

// Create
TEST_F(DataObjectTest, Create_CreateUninitializedPOD_ValidDataObject)
{
    std::unique_ptr<Containers::DataObject> result = Containers::DataObject::Create<int>();
    EXPECT_NE(nullptr, result);
}

TEST_F(DataObjectTest, Create_CreateInitializedPOD_ValidDataObject)
{
    std::unique_ptr<Containers::DataObject> result = Containers::DataObject::Create<int>(42);
    EXPECT_NE(nullptr, result);
}

TEST_F(DataObjectTest, Create_CreateUninitializedComplexObject_ValidDataObject)
{
    std::unique_ptr<Containers::DataObject> result = Containers::DataObject::Create<ComplexObject>();
    EXPECT_NE(nullptr, result);
}

TEST_F(DataObjectTest, Create_CreateComplexObjectWithSingleParamter_ValidDataObject)
{
    std::unique_ptr<Containers::DataObject> result = Containers::DataObject::Create<ComplexObject>(42);
    EXPECT_NE(nullptr, result);
}

TEST_F(DataObjectTest, Create_CreateComplexObjectWithMultipileParamters_ValidDataObject)
{
    std::unique_ptr<Containers::DataObject> result = Containers::DataObject::Create<ComplexObject>(42, "Test string");
    EXPECT_NE(nullptr, result);
}

// IsType
TEST_F(DataObjectTest, IsType_GetPODType_PODIsRecognizedAsType)
{
    std::unique_ptr<Containers::DataObject> result = Containers::DataObject::Create<int>();
    ASSERT_NE(nullptr, result);
    EXPECT_TRUE(result->IsType<int>());
}

TEST_F(DataObjectTest, IsType_GetComplexClassType_ComplexObjectIsRecognizedAsType)
{
    std::unique_ptr<Containers::DataObject> result = Containers::DataObject::Create<ComplexObject>();
    ASSERT_NE(nullptr, result);
    EXPECT_TRUE(result->IsType<ComplexObject>());
}

TEST_F(DataObjectTest, IsType_SingleInheritance_BaseInterfaceFoundFromDerivedClass)
{
    std::unique_ptr<Containers::DataObject> result = Containers::DataObject::Create<SingleInheritance>();
    ASSERT_NE(nullptr, result);
    EXPECT_TRUE(result->IsType<BaseInterface>());
}

TEST_F(DataObjectTest, IsType_MultipleInheritance_BothInterfacesFoundFromDerivedClass)
{
    std::unique_ptr<Containers::DataObject> result = Containers::DataObject::Create<MultipleInheritance>();
    ASSERT_NE(nullptr, result);
    EXPECT_TRUE(result->IsType<BaseInterface>());
    EXPECT_TRUE(result->IsType<SecondInterface>());
}

// GetTypeName
TEST_F(DataObjectTest, GetTypeName_GetNameOfPOD_NameOfPODIsInt)
{
    std::unique_ptr<Containers::DataObject> result = Containers::DataObject::Create<int>();
    ASSERT_NE(nullptr, result);
    EXPECT_STRCASEEQ("int", result->GetTypeName());
}

TEST_F(DataObjectTest, GetTypeName_GetNameOfComplexClass_NameIsComplexObject)
{
    std::unique_ptr<Containers::DataObject> result = Containers::DataObject::Create<ComplexObject>();
    ASSERT_NE(nullptr, result);
    EXPECT_STRCASEEQ("ComplexObject", result->GetTypeName());
}

TEST_F(DataObjectTest, GetTypeName_GetNameOfComplexClassInNamespace_TypeNameOfClassWithoutNamespace)
{
    std::unique_ptr<Containers::DataObject> result = Containers::DataObject::Create<TestNamespace::TestClassInNamespace>();
    ASSERT_NE(nullptr, result);
    EXPECT_STRCASEEQ("TestClassInNamespace", result->GetTypeName());
}

TEST_F(DataObjectTest, GetTypeName_GetNameOfTypedefedType_NameOfOriginalType)
{
    using U32 = int;
    std::unique_ptr<Containers::DataObject> result = Containers::DataObject::Create<U32>();
    ASSERT_NE(nullptr, result);
    EXPECT_STRCASEEQ("int", result->GetTypeName());
}

// DynamicCast
TEST_F(DataObjectTest, DynamicCast_CastToGivenPOD_ValidPointer)
{
    std::unique_ptr<Containers::DataObject> result = Containers::DataObject::Create<int>();
    ASSERT_NE(nullptr, result);

    EXPECT_NE(nullptr, result->DynamicCast<int*>());
}

TEST_F(DataObjectTest, DynamicCast_CastToInvalidType_Nullptr)
{
    std::unique_ptr<Containers::DataObject> result = Containers::DataObject::Create<int>();
    ASSERT_NE(nullptr, result);

    EXPECT_EQ(nullptr, result->DynamicCast<float*>());
}

TEST_F(DataObjectTest, DynamicCast_GivenPODDataIsAccesible_SameValueAsStored)
{
    std::unique_ptr<Containers::DataObject> result = Containers::DataObject::Create<int>(42);
    ASSERT_NE(nullptr, result);

    int* value = result->DynamicCast<int*>();
    EXPECT_EQ(42, *value);
}

TEST_F(DataObjectTest, DynamicCast_GivenComplextDataIsAccesible_SameValueAsStored)
{
    ComplexObject comparison = ComplexObject(42, "test");

    std::unique_ptr<Containers::DataObject> result = Containers::DataObject::Create<ComplexObject>(42, "test");
    ASSERT_NE(nullptr, result);

    ComplexObject* value = result->DynamicCast<ComplexObject*>();
    EXPECT_EQ(comparison, *value);
}

// Destruction
TEST_F(DataObjectTest, Destruction_DestructorCalledOnConstructedObject_DestructorCalled)
{
    std::unique_ptr<Containers::DataObject> result = Containers::DataObject::Create<DestructionClass>();
    ASSERT_NE(nullptr, result);

    DestructionClass* value = result->DynamicCast<DestructionClass*>();
    ASSERT_NE(nullptr, value);
    EXPECT_CALL(*value, Destructor()).Times(1);
}

// Reflect
TEST_F(DataObjectTest, Reflect_ReflectIsCalledOnStoredObject_ReflectCalled)
{
    std::unique_ptr<Containers::DataObject> result = Containers::DataObject::Create<ReflectionClass>();

    ReflectionClass* value = result->DynamicCast<ReflectionClass*>();
    ASSERT_NE(nullptr, value);
    ReflectionClass::currentTestClass = value;
    EXPECT_CALL(*value, Reflect()).Times(1);

    AZ::SerializeContext context;
    result->ReflectData(&context);
}

TEST_F(DataObjectTest, Reflect_ReflectIsCalledMultipleTimesOnSameStoredObject_ReflectCalledOnceAndNowAssertsFromSerializeContext)
{
    std::unique_ptr<Containers::DataObject> result = Containers::DataObject::Create<ReflectionClass>();

    ReflectionClass* value = result->DynamicCast<ReflectionClass*>();
    ASSERT_NE(nullptr, value);
    ReflectionClass::currentTestClass = value;
    EXPECT_CALL(*value, Reflect()).Times(1);

    AZ::SerializeContext context;
    result->ReflectData(&context);
    result->ReflectData(&context);
}