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

#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/IO/LocalFileIO.h>

#include <Framework/JsonObjectHandler.h>
#include <Framework/JsonWriter.h>

namespace AWSCoreTestingUtils
{
    static const AZStd::string STRING_VALUE{"s"};
    static constexpr const char STRING_VALUE_STRING[] = "s";
    static constexpr const char STRING_VALUE_JSON[] = "\"s\"";

    static constexpr const char CHARPTR_VALUE[]{"s"};
    static constexpr const char CHARPTR_VALUE_STRING[] = "s";
    static constexpr const char CHARPTR_VALUE_JSON[] = "\"s\"";

    static constexpr bool BOOL_VALUE{true};
    static constexpr const char BOOL_VALUE_STRING[] = "true";

    static constexpr int INT_VALUE{-2};
    static constexpr const char INT_VALUE_STRING[] = "-2";

    static constexpr unsigned UINT_VALUE{2};
    static constexpr const char UINT_VALUE_STRING[] = "2";

    static constexpr unsigned UINT_VALUE_MAX{4294967295};
    static constexpr const char UINT_VALUE_MAX_STRING[] = "4294967295";

    static constexpr int64_t INT64_VALUE{INT64_C(-3000000000)}; // < INT_MIN
    static constexpr const char INT64_VALUE_STRING[] = "-3000000000";

    static constexpr uint64_t UINT64_VALUE{UINT64_C(5000000000)}; // > UINT_MAX and < INT64_MAX
    static constexpr const char UINT64_VALUE_STRING[] = "5000000000";

    static constexpr uint64_t UINT64_VALUE_MAX{18446744073709551615U};
    static constexpr const char UINT64_VALUE_MAX_STRING[] = "18446744073709551615";

    static constexpr double DOUBLE_VALUE{1.0};
    static constexpr const char DOUBLE_VALUE_STRING[] = "1.0";

    static constexpr const char OBJECT_VALUE_JSON[] = "{\"value\":\"s\"}";

    static constexpr const char ARRAY_VALUE_JSON[] = "[\"a\",\"b\",\"c\"]";

    static constexpr const char ARRAY_OF_ARRAY_VALUE_JSON[] = "[[\"a1\",\"b1\",\"c1\"],[\"a2\",\"b2\",\"c2\"]]";

    static constexpr const char ARRAY_OF_OBJECT_VALUE_JSON[] = "[{\"value\":\"s1\"},{\"value\":\"s2\"}]";

    static constexpr const char UNESCAPED[] = "abc !#$%&'()*+,/:;=?@[]123";
    static constexpr const char ESCAPED[] = "abc%20%21%23%24%25%26%27%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5D123";

    inline AZStd::string TestObjectJson(const char* value)
    {
        return AZStd::string::format("{\"value\":%s}", value);
    }
}; // namespace AWSCoreTestingUtils

template<class T>
struct TestObject
{
    TestObject() = default;

    TestObject(const T& initialValue)
        : value{initialValue}
    {
    }

    T value;

    bool OnJsonKey(const char* key, AWSCore::JsonReader& reader)
    {
        if (strcmp(key, "value") == 0)
        {
            return reader.Accept(value);
        }
        return reader.Ignore();
    }

    bool WriteJson(AWSCore::JsonWriter& writer) const
    {
        bool ok = true;
        ok = ok && writer.StartObject();
        ok = ok && writer.Write("value", value);
        ok = ok && writer.EndObject();
        return ok;
    }

    bool operator==(const TestObject& other) const
    {
        return value == other.value;
    }
};

class AWSCoreFixture
    : public UnitTest::ScopedAllocatorSetupFixture
{
public:
    AWSCoreFixture() {}
    virtual ~AWSCoreFixture() = default;

    void SetUp() override
    {
        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();
        AZ::AllocatorInstance<AZ::PoolAllocator>::Create();

        m_localFileIO = aznew AZ::IO::LocalFileIO();
        m_otherFileIO = AZ::IO::FileIOBase::GetInstance();
        AZ::IO::FileIOBase::SetInstance(nullptr);
        AZ::IO::FileIOBase::SetInstance(m_localFileIO);
    }

    void TearDown() override
    {
        AZ::IO::FileIOBase::SetInstance(nullptr);
        delete m_localFileIO;
        AZ::IO::FileIOBase::SetInstance(m_otherFileIO);

        AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();
        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
    }

    AZ::IO::FileIOBase* m_localFileIO = nullptr;

private:
    AZ::IO::FileIOBase* m_otherFileIO = nullptr;
};
