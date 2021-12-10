/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/Backends/JSON/JsonBackend.h>
#include <AzCore/DOM/Backends/JSON/JsonSerializationUtils.h>
#include <AzCore/DOM/DomUtils.h>
#include <AzCore/DOM/DomValue.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/numeric.h>

namespace AZ::Dom::Tests
{
    class DomValueTests : public UnitTest::AllocatorsFixture
    {
    public:
        void SetUp() override
        {
            UnitTest::AllocatorsFixture::SetUp();
            NameDictionary::Create();
            AZ::AllocatorInstance<ValueAllocator>::Create();
        }

        void TearDown() override
        {
            m_value = Value();

            AZ::AllocatorInstance<ValueAllocator>::Destroy();
            NameDictionary::Destroy();
            UnitTest::AllocatorsFixture::TearDown();
        }

        void PerformValueChecks()
        {
            Value shallowCopy = m_value;
            EXPECT_EQ(m_value, shallowCopy);
            EXPECT_TRUE(m_value.DeepCompareIsEqual(shallowCopy));

            Value deepCopy = m_value.DeepCopy();
            EXPECT_TRUE(m_value.DeepCompareIsEqual(deepCopy));
        }

        Value m_value;
    };

    TEST_F(DomValueTests, EmptyArray)
    {
        m_value.SetArray();

        EXPECT_TRUE(m_value.IsArray());
        EXPECT_EQ(m_value.Size(), 0);

        PerformValueChecks();
    }

    TEST_F(DomValueTests, SimpleArray)
    {
        m_value.SetArray();

        for (int i = 0; i < 5; ++i)
        {
            m_value.PushBack(Value(i));
            EXPECT_EQ(m_value.Size(), i + 1);
            EXPECT_EQ(m_value[i].GetInt32(), i);
        }

        PerformValueChecks();
    }

    TEST_F(DomValueTests, NestedArrays)
    {
        m_value.SetArray();
        for (int j = 0; j < 5; ++j)
        {
            Value nestedArray(Type::ArrayType);
            for (int i = 0; i < 5; ++i)
            {
                nestedArray.PushBack(Value(i));
            }
            m_value.PushBack(AZStd::move(nestedArray));
        }

        EXPECT_EQ(m_value.Size(), 5);
        for (int i = 0; i < 3; ++i)
        {
            EXPECT_EQ(m_value[i].Size(), 5);
            for (int j = 0; j < 5; ++j)
            {
                EXPECT_EQ(m_value[i][j].GetInt32(), j);
            }
        }

        PerformValueChecks();
    }

    TEST_F(DomValueTests, EmptyObject)
    {
        m_value.SetObject();
        EXPECT_EQ(m_value.MemberCount(), 0);

        PerformValueChecks();
    }

    TEST_F(DomValueTests, SimpleObject)
    {
        m_value.SetObject();
        for (int i = 0; i < 5; ++i)
        {
            AZStd::string key = AZStd::string::format("Key%i", i);
            m_value.AddMember(key, Value(i));
            EXPECT_EQ(m_value.MemberCount(), i + 1);
            EXPECT_EQ(m_value[key].GetInt32(), i);
        }

        PerformValueChecks();
    }

    TEST_F(DomValueTests, NestedObjects)
    {
        m_value.SetObject();
        for (int j = 0; j < 3; ++j)
        {
            Value nestedObject(Type::ObjectType);
            for (int i = 0; i < 5; ++i)
            {
                nestedObject.AddMember(AZStd::string::format("Key%i", i), Value(i));
            }
            m_value.AddMember(AZStd::string::format("Obj%i", j), AZStd::move(nestedObject));
        }

        EXPECT_EQ(m_value.MemberCount(), 3);
        for (int j = 0; j < 3; ++j)
        {
            const Value& nestedObject = m_value[AZStd::string::format("Obj%i", j)];
            EXPECT_EQ(nestedObject.MemberCount(), 5);
            for (int i = 0; i < 5; ++i)
            {
                EXPECT_EQ(nestedObject[AZStd::string::format("Key%i", i)].GetInt32(), i);
            }
        }

        PerformValueChecks();
    }

    TEST_F(DomValueTests, EmptyNode)
    {
        m_value.SetNode("Test");
        EXPECT_EQ(m_value.GetNodeName(), AZ::Name("Test"));
        EXPECT_EQ(m_value.MemberCount(), 0);
        EXPECT_EQ(m_value.Size(), 0);

        PerformValueChecks();
    }

    TEST_F(DomValueTests, SimpleNode)
    {
        m_value.SetNode("Test");

        for (int i = 0; i < 10; ++i)
        {
            m_value.PushBack(Value(i));
            EXPECT_EQ(m_value.Size(), i + 1);
            EXPECT_EQ(m_value[i].GetInt32(), i);

            if (i < 5)
            {
                AZ::Name key = AZ::Name(AZStd::string::format("TwoTimes%i", i));
                m_value.AddMember(key, Value(i * 2));
                EXPECT_EQ(m_value.MemberCount(), i + 1);
                EXPECT_EQ(m_value[key].GetInt32(), i * 2);
            }
        }

        PerformValueChecks();
    }

    TEST_F(DomValueTests, NestedNodes)
    {
        m_value.SetNode("TopLevel");

        const AZ::Name childNodeName("ChildNode");

        for (int i = 0; i < 5; ++i)
        {
            Value childNode(Type::NodeType);
            childNode.SetNodeName(childNodeName);
            childNode.SetNodeValue(i);

            childNode.AddMember("foo", i);
            childNode.AddMember("bar", Value("test", false));

            m_value.PushBack(childNode);
        }

        EXPECT_EQ(m_value.Size(), 5);
        for (int i = 0; i < 5; ++i)
        {
            const Value& childNode = m_value[i];
            EXPECT_EQ(childNode.GetNodeName(), childNodeName);
            EXPECT_EQ(childNode.GetNodeValue().GetInt32(), i);
            EXPECT_EQ(childNode["foo"].GetInt32(), i);
            EXPECT_EQ(childNode["bar"].GetString(), "test");
        }

        PerformValueChecks();
    }

    TEST_F(DomValueTests, Int64)
    {
        m_value.SetObject();
        m_value["int64_min"] = AZStd::numeric_limits<int64_t>::min();
        m_value["int64_max"] = AZStd::numeric_limits<int64_t>::max();

        EXPECT_EQ(m_value["int64_min"].GetType(), Type::NumberType);
        EXPECT_EQ(m_value["int64_min"].GetInt64(), AZStd::numeric_limits<int64_t>::min());
        EXPECT_EQ(m_value["int64_max"].GetType(), Type::NumberType);
        EXPECT_EQ(m_value["int64_max"].GetInt64(), AZStd::numeric_limits<int64_t>::max());

        PerformValueChecks();
    }

    TEST_F(DomValueTests, Uint64)
    {
        m_value.SetObject();
        m_value["uint64_min"] = AZStd::numeric_limits<uint64_t>::min();
        m_value["uint64_max"] = AZStd::numeric_limits<uint64_t>::max();

        EXPECT_EQ(m_value["uint64_min"].GetType(), Type::NumberType);
        EXPECT_EQ(m_value["uint64_min"].GetInt64(), AZStd::numeric_limits<uint64_t>::min());
        EXPECT_EQ(m_value["uint64_max"].GetType(), Type::NumberType);
        EXPECT_EQ(m_value["uint64_max"].GetInt64(), AZStd::numeric_limits<uint64_t>::max());

        PerformValueChecks();
    }

    TEST_F(DomValueTests, Double)
    {
        m_value.SetObject();
        m_value["double_min"] = AZStd::numeric_limits<double>::min();
        m_value["double_max"] = AZStd::numeric_limits<double>::max();

        EXPECT_EQ(m_value["double_min"].GetType(), Type::NumberType);
        EXPECT_EQ(m_value["double_min"].GetDouble(), AZStd::numeric_limits<double>::min());
        EXPECT_EQ(m_value["double_max"].GetType(), Type::NumberType);
        EXPECT_EQ(m_value["double_max"].GetDouble(), AZStd::numeric_limits<double>::max());

        PerformValueChecks();
    }

    TEST_F(DomValueTests, Null)
    {
        m_value.SetObject();
        m_value["null_value"] = Value(Type::NullType);

        EXPECT_EQ(m_value["null_value"].GetType(), Type::NullType);
        EXPECT_EQ(m_value["null_type"], Value());

        PerformValueChecks();
    }

    TEST_F(DomValueTests, Bool)
    {
        m_value.SetObject();
        m_value["true_value"] = true;
        m_value["false_value"] = false;

        EXPECT_EQ(m_value["true_value"].GetType(), Type::TrueType);
        EXPECT_EQ(m_value["true_value"].GetBool(), true);
        EXPECT_EQ(m_value["false_value"].GetType(), Type::FalseType);
        EXPECT_EQ(m_value["false_value"].GetBool(), false);

        PerformValueChecks();
    }

    TEST_F(DomValueTests, String)
    {
        const char* s1 = "reference string long enough to avoid SSO";
        const char* s2 = "copy string long enough to avoid SSO";

        m_value.SetObject();
        AZStd::string stringToReference = s1;
        m_value["no_copy"] = Value(stringToReference, false);
        AZStd::string stringToCopy = s2;
        m_value["copy"] = Value(stringToCopy, true);

        EXPECT_EQ(m_value["no_copy"].GetType(), Type::StringType);
        EXPECT_EQ(m_value["no_copy"].GetString(), s1);
        stringToReference.at(0) = 'F';
        EXPECT_NE(m_value["no_copy"].GetString(), s1);

        EXPECT_EQ(m_value["copy"].GetType(), Type::StringType);
        EXPECT_EQ(m_value["copy"].GetString(), s2);
        stringToCopy.at(0) = 'F';
        EXPECT_EQ(m_value["copy"].GetString(), s2);

        PerformValueChecks();
    }

    TEST_F(DomValueTests, CopyOnWrite_Object)
    {
        Value v1(Type::ObjectType);
        v1["foo"] = 5;

        Value nestedObject(Type::ObjectType);
        v1["obj"] = nestedObject;

        Value v2 = v1;
        EXPECT_EQ(&v1.GetObject(), &v2.GetObject());
        EXPECT_EQ(&v1.FindMember("obj")->second.GetObject(), &v2.FindMember("obj")->second.GetObject());

        v2["foo"] = 0;

        EXPECT_NE(&v1.GetObject(), &v2.GetObject());
        EXPECT_EQ(&v1.FindMember("obj")->second.GetObject(), &v2.FindMember("obj")->second.GetObject());

        v2["obj"]["key"] = true;

        EXPECT_NE(&v1.GetObject(), &v2.GetObject());
        EXPECT_NE(&v1.FindMember("obj")->second.GetObject(), &v2.FindMember("obj")->second.GetObject());

        v2 = v1;

        EXPECT_EQ(&v1.GetObject(), &v2.GetObject());
        EXPECT_EQ(&v1.FindMember("obj")->second.GetObject(), &v2.FindMember("obj")->second.GetObject());
    }

    TEST_F(DomValueTests, CopyOnWrite_Array)
    {
        Value v1(Type::ArrayType);
        v1.PushBack(1);
        v1.PushBack(2);

        Value nestedArray(Type::ArrayType);
        v1.PushBack(nestedArray);
        Value v2 = v1;

        EXPECT_EQ(&v1.GetArray(), &v2.GetArray());
        EXPECT_EQ(&v1.At(2).GetArray(), &v2.At(2).GetArray());

        v2[0] = 0;

        EXPECT_NE(&v1.GetArray(), &v2.GetArray());
        EXPECT_EQ(&v1.At(2).GetArray(), &v2.At(2).GetArray());

        v2[2].PushBack(42);

        EXPECT_NE(&v1.GetArray(), &v2.GetArray());
        EXPECT_NE(&v1.At(2).GetArray(), &v2.At(2).GetArray());

        v2 = v1;

        EXPECT_EQ(&v1.GetArray(), &v2.GetArray());
        EXPECT_EQ(&v1.At(2).GetArray(), &v2.At(2).GetArray());
    }

    TEST_F(DomValueTests, CopyOnWrite_Node)
    {
        Value v1;
        v1.SetNode("TopLevel");

        v1.PushBack(1);
        v1.PushBack(2);
        v1["obj"].SetNode("Nested");
        Value v2 = v1;

        EXPECT_EQ(&v1.GetNode(), &v2.GetNode());
        EXPECT_EQ(&v1["obj"].GetNode(), &v2["obj"].GetNode());

        v2[0] = 0;

        EXPECT_NE(&v1.GetNode(), &v2.GetNode());
        EXPECT_EQ(&v1["obj"].GetNode(), &v2["obj"].GetNode());

        v2["obj"].PushBack(42);

        EXPECT_NE(&v1.GetNode(), &v2.GetNode());
        EXPECT_NE(&v1["obj"].GetNode(), &v2["obj"].GetNode());

        v2 = v1;

        EXPECT_EQ(&v1.GetNode(), &v2.GetNode());
        EXPECT_EQ(&v1["obj"].GetNode(), &v2["obj"].GetNode());
    }
} // namespace AZ::Dom::Tests
