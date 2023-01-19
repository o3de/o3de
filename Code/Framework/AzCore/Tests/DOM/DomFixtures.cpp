/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomValue.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <Tests/DOM/DomFixtures.h>

namespace AZ::Dom::Tests
{
    void DomTestHarness::SetUpHarness()
    {
        NameDictionary::Create();
    }

    void DomTestHarness::TearDownHarness()
    {
        NameDictionary::Destroy();
    }

    void DomBenchmarkFixture::SetUp(const ::benchmark::State& st)
    {
        UnitTest::AllocatorsBenchmarkFixture::SetUp(st);
        SetUpHarness();
    }

    void DomBenchmarkFixture::SetUp(::benchmark::State& st)
    {
        UnitTest::AllocatorsBenchmarkFixture::SetUp(st);
        SetUpHarness();
    }

    void DomBenchmarkFixture::TearDown(::benchmark::State& st)
    {
        TearDownHarness();
        UnitTest::AllocatorsBenchmarkFixture::TearDown(st);
    }

    void DomBenchmarkFixture::TearDown(const ::benchmark::State& st)
    {
        TearDownHarness();
        UnitTest::AllocatorsBenchmarkFixture::TearDown(st);
    }

    rapidjson::Document DomBenchmarkFixture::GenerateDomJsonBenchmarkDocument(int64_t entryCount, int64_t stringTemplateLength)
    {
        rapidjson::Document document;
        document.SetObject();

        AZStd::string entryTemplate;
        while (entryTemplate.size() < aznumeric_cast<size_t>(stringTemplateLength))
        {
            entryTemplate += "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor ";
        }
        entryTemplate.resize(stringTemplateLength);
        AZStd::string buffer;

        auto createString = [&](int n) -> rapidjson::Value
        {
            buffer = AZStd::string::format("#%i %s", n, entryTemplate.c_str());
            return rapidjson::Value(buffer.data(), aznumeric_cast<rapidjson::SizeType>(buffer.size()), document.GetAllocator());
        };

        auto createEntry = [&](int n) -> rapidjson::Value
        {
            rapidjson::Value entry(rapidjson::kObjectType);
            entry.AddMember("string", createString(n), document.GetAllocator());
            entry.AddMember("int", rapidjson::Value(n), document.GetAllocator());
            entry.AddMember("double", rapidjson::Value(aznumeric_cast<double>(n) * 0.5), document.GetAllocator());
            entry.AddMember("bool", rapidjson::Value(n % 2 == 0), document.GetAllocator());
            entry.AddMember("null", rapidjson::Value(rapidjson::kNullType), document.GetAllocator());
            return entry;
        };

        auto createArray = [&]() -> rapidjson::Value
        {
            rapidjson::Value array;
            array.SetArray();
            for (int i = 0; i < entryCount; ++i)
            {
                array.PushBack(createEntry(i), document.GetAllocator());
            }
            return array;
        };

        auto createObject = [&]() -> rapidjson::Value
        {
            rapidjson::Value object;
            object.SetObject();
            for (int i = 0; i < entryCount; ++i)
            {
                buffer = AZStd::string::format("Key%i", i);
                rapidjson::Value key;
                key.SetString(buffer.data(), aznumeric_cast<rapidjson::SizeType>(buffer.length()), document.GetAllocator());
                object.AddMember(key.Move(), createArray(), document.GetAllocator());
            }
            return object;
        };

        document.SetObject();
        document.AddMember("entries", createObject(), document.GetAllocator());

        return document;
    }

    AZStd::string DomBenchmarkFixture::GenerateDomJsonBenchmarkPayload(int64_t entryCount, int64_t stringTemplateLength)
    {
        rapidjson::Document document = GenerateDomJsonBenchmarkDocument(entryCount, stringTemplateLength);

        AZStd::string serializedJson;
        auto result = AZ::JsonSerializationUtils::WriteJsonString(document, serializedJson);
        AZ_Assert(result.IsSuccess(), "Failed to serialize generated JSON");
        return serializedJson;
    }

    Value DomBenchmarkFixture::GenerateDomBenchmarkPayload(int64_t entryCount, int64_t stringTemplateLength)
    {
        Value root(Type::Object);

        AZStd::string entryTemplate;
        while (entryTemplate.size() < aznumeric_cast<size_t>(stringTemplateLength))
        {
            entryTemplate += "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor ";
        }
        entryTemplate.resize(stringTemplateLength);
        AZStd::string buffer;

        auto createString = [&](int n) -> Value
        {
            return Value(AZStd::string::format("#%i %s", n, entryTemplate.c_str()), true);
        };

        auto createEntry = [&](int n) -> Value
        {
            Value entry(Type::Object);
            entry.AddMember("string", createString(n));
            entry.AddMember("int", Value(n));
            entry.AddMember("double", Value(aznumeric_cast<double>(n) * 0.5));
            entry.AddMember("bool", Value(n % 2 == 0));
            entry.AddMember("null", Value(Type::Null));
            return entry;
        };

        auto createArray = [&]() -> Value
        {
            Value array(Type::Array);
            for (int i = 0; i < entryCount; ++i)
            {
                array.ArrayPushBack(createEntry(i));
            }
            return array;
        };

        auto createObject = [&]() -> Value
        {
            Value object;
            object.SetObject();
            for (int i = 0; i < entryCount; ++i)
            {
                buffer = AZStd::string::format("Key%i", i);
                object.AddMember(AZ::Name(buffer), createArray());
            }
            return object;
        };

        root["entries"] = createObject();

        return root;
    }

    void DomTestFixture::SetUp()
    {
        UnitTest::LeakDetectionFixture::SetUp();
        SetUpHarness();
    }

    void DomTestFixture::TearDown()
    {
        TearDownHarness();
        UnitTest::LeakDetectionFixture::TearDown();
    }
} // namespace AZ::Dom::Tests
