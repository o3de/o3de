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

namespace AZ::Dom::Tests
{
    class DomValueTests : public UnitTest::AllocatorsFixture
    {
    public:
        void SetUp() override
        {
            UnitTest::AllocatorsFixture::SetUp();
            NameDictionary::Create();
        }

        void TearDown() override
        {
            m_value = Value();

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
} // namespace AZ::Dom::Tests
