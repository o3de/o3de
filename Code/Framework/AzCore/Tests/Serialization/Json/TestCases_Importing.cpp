/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/JsonImporter.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <Tests/Serialization/Json/JsonSerializationTests.h>

namespace JsonSerializationTests
{
    class JsonImportingTests;

    class JsonImporterCustom
        : public AZ::BaseJsonImporter
    {
    public:
        AZ_RTTI(JsonImporterCustom, "{003F5896-71E0-4A50-A14F-08C319B06AD0}");


        AZ::JsonSerializationResult::ResultCode ResolveImport(rapidjson::Value* importPtr,
            rapidjson::Value& patch, const rapidjson::Value& importDirective,
            const AZ::IO::FixedMaxPath& importedFilePath, rapidjson::Document::AllocatorType& allocator) override;
        
        JsonImporterCustom(JsonImportingTests* tests)
        {
            testClass = tests;
        }

    private:
        JsonImportingTests* testClass;
    };

    class JsonImportingTests
        : public BaseJsonSerializerFixture
    {
    public:
        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();
        }

        void TearDown() override
        {
            BaseJsonSerializerFixture::TearDown();
        }

        void GetTestDocument(const AZStd::string& docName, rapidjson::Document& out)
        {
            const char *objectJson = R"({
                "field_1" : "value_1",
                "field_2" : "value_2",
                "field_3" : "value_3"
            })";

            const char *arrayJson = R"([
                { "element_1" : "value_1" },
                { "element_2" : "value_2" },
                { "element_3" : "value_3" }
            ])";

            const char *nestedImportJson = R"({
                "desc" : "Nested Import",
                "obj" : {"$import" : "object.json"}
            })";
            
            const char *nestedImportCycle1Json = R"({
                "desc" : "Nested Import Cycle 1",
                "obj" : {"$import" : "nested_import_c2.json"}
            })";

            const char *nestedImportCycle2Json = R"({
                "desc" : "Nested Import Cycle 2",
                "obj" : {"$import" : "nested_import_c1.json"}
            })";

            if (docName.compare("object.json") == 0)
            {
                out.Parse(objectJson);
                ASSERT_FALSE(out.HasParseError());
            }
            else if (docName.compare("array.json") == 0)
            {
                out.Parse(arrayJson);
                ASSERT_FALSE(out.HasParseError());
            }
            else if (docName.compare("nested_import.json") == 0)
            {
                out.Parse(nestedImportJson);
                ASSERT_FALSE(out.HasParseError());
            }
            else if (docName.compare("nested_import_c1.json") == 0)
            {
                out.Parse(nestedImportCycle1Json);
                ASSERT_FALSE(out.HasParseError());
            }
            else if (docName.compare("nested_import_c2.json") == 0)
            {
                out.Parse(nestedImportCycle2Json);
                ASSERT_FALSE(out.HasParseError());
            }
        }

    protected:
        void TestImportLoadStore(const char* input, const char* expectedImportedValue)
        {
            m_jsonDocument->Parse(input);
            ASSERT_FALSE(m_jsonDocument->HasParseError());

            JsonImporterCustom* importerObj = new JsonImporterCustom(this);

            rapidjson::Document expectedOutcome;
            expectedOutcome.Parse(expectedImportedValue);
            ASSERT_FALSE(expectedOutcome.HasParseError());

            TestResolveImports(importerObj);

            Expect_DocStrEq(m_jsonDocument->GetObject(), expectedOutcome.GetObject());

            rapidjson::Document originalInput;
            originalInput.Parse(input);
            ASSERT_FALSE(originalInput.HasParseError());

            TestRestoreImports(importerObj);

            Expect_DocStrEq(m_jsonDocument->GetObject(), originalInput.GetObject());
            
            m_jsonDocument->SetObject();
            delete importerObj;
        }

        void TestImportCycle(const char* input)
        {
            m_jsonDocument->Parse(input);
            ASSERT_FALSE(m_jsonDocument->HasParseError());

            JsonImporterCustom* importerObj = new JsonImporterCustom(this);

            AZ::JsonSerializationResult::ResultCode result = TestResolveImports(importerObj);

            EXPECT_EQ(result.GetOutcome(), AZ::JsonSerializationResult::Outcomes::Catastrophic);

            m_jsonDocument->SetObject();
            delete importerObj;
        }

        void TestInsertNewImport(const char* input, const char* expectedRestoredValue)
        {
            m_jsonDocument->Parse(input);
            ASSERT_FALSE(m_jsonDocument->HasParseError());

            JsonImporterCustom* importerObj = new JsonImporterCustom(this);

            TestResolveImports(importerObj);

            importerObj->AddImportDirective(rapidjson::Pointer("/object_2"), "object.json");

            rapidjson::Document expectedOutput;
            expectedOutput.Parse(expectedRestoredValue);
            ASSERT_FALSE(expectedOutput.HasParseError());

            TestRestoreImports(importerObj);

            Expect_DocStrEq(m_jsonDocument->GetObject(), expectedOutput.GetObject());
            
            m_jsonDocument->SetObject();
            delete importerObj;
        }

        AZ::JsonSerializationResult::ResultCode TestResolveImports(JsonImporterCustom* importerObj)
        {
            AZ::JsonImportSettings settings;
            settings.m_importer = importerObj;

            return AZ::JsonSerialization::ResolveImports(m_jsonDocument->GetObject(), m_jsonDocument->GetAllocator(), settings);
        }

        AZ::JsonSerializationResult::ResultCode TestRestoreImports(JsonImporterCustom* importerObj)
        {
            AZ::JsonImportSettings settings;
            settings.m_importer = importerObj;

            return AZ::JsonSerialization::RestoreImports(m_jsonDocument->GetObject(), m_jsonDocument->GetAllocator(), settings);
        }
    };

    AZ::JsonSerializationResult::ResultCode JsonImporterCustom::ResolveImport(rapidjson::Value* importPtr,
        rapidjson::Value& patch, const rapidjson::Value& importDirective, const AZ::IO::FixedMaxPath& importedFilePath,
        rapidjson::Document::AllocatorType& allocator)
    {
        AZ::JsonSerializationResult::ResultCode resultCode(AZ::JsonSerializationResult::Tasks::Import);
        
        rapidjson::Document importedDoc;
        testClass->GetTestDocument(importedFilePath.String(), importedDoc);

        if (importDirective.IsObject())
        {
            auto patchField = importDirective.FindMember("patch");
            if (patchField != importDirective.MemberEnd())
            {
                patch.CopyFrom(patchField->value, allocator);
            }
        }

        importPtr->CopyFrom(importedDoc, allocator);

        return resultCode;
    }

    // Test Cases

    TEST_F(JsonImportingTests, ImportSimpleObjectTest)
    {
        const char* inputFile = R"(
            {
                "name" : "simple_object_import",
                "object": {"$import" : "object.json"}
            }
        )";

        const char* expectedOutput = R"(
            {
                "name" : "simple_object_import",
                "object": {
                    "field_1" : "value_1",
                    "field_2" : "value_2",
                    "field_3" : "value_3"
                }
            }
        )";

        TestImportLoadStore(inputFile, expectedOutput);
    }

    TEST_F(JsonImportingTests, ImportSimpleObjectPatchTest)
    {
        const char* inputFile = R"(
            {
                "name" : "simple_object_import",
                "object": {
                    "$import" : { 
                        "filename" : "object.json",
                        "patch" : { "field_2" : "patched_value" }
                    }
                }
            }
        )";

        const char* expectedOutput = R"(
            {
                "name" : "simple_object_import",
                "object": {
                    "field_1" : "value_1",
                    "field_2" : "patched_value",
                    "field_3" : "value_3"
                }
            }
        )";

        TestImportLoadStore(inputFile, expectedOutput);
    }

    TEST_F(JsonImportingTests, ImportSimpleArrayTest)
    {
        const char* inputFile = R"(
            {
                "name" : "simple_array_import",
                "object": {"$import" : "array.json"}
            }
        )";

        const char* expectedOutput = R"(
            {
                "name" : "simple_array_import",
                "object": [
                    { "element_1" : "value_1" },
                    { "element_2" : "value_2" },
                    { "element_3" : "value_3" }
                ]
            }
        )";

        TestImportLoadStore(inputFile, expectedOutput);
    }

    TEST_F(JsonImportingTests, ImportSimpleArrayPatchTest)
    {
        const char* inputFile = R"(
            {
                "name" : "simple_array_import",
                "object": {
                    "$import" : {
                        "filename" : "array.json",
                        "patch" : [ { "element_1" : "patched_value" } ]
                    }
                }
            }
        )";

        const char* expectedOutput = R"(
            {
                "name" : "simple_array_import",
                "object": [
                    { "element_1" : "patched_value" }
                ]
            }
        )";

        TestImportLoadStore(inputFile, expectedOutput);
    }

    TEST_F(JsonImportingTests, NestedImportTest)
    {
        const char* inputFile = R"(
            {
                "name" : "nested_import",
                "object": {"$import" : "nested_import.json"}
            }
        )";

        const char* expectedOutput = R"(
            {
                "name" : "nested_import",
                "object": {
                    "desc" : "Nested Import",
                    "obj" : {
                        "field_1" : "value_1",
                        "field_2" : "value_2",
                        "field_3" : "value_3"
                    }
                }
            }
        )";

        TestImportLoadStore(inputFile, expectedOutput);
    }

    TEST_F(JsonImportingTests, NestedImportPatchTest)
    {
        const char* inputFile = R"(
            {
                "name" : "nested_import",
                "object": {
                    "$import" : { 
                        "filename" : "nested_import.json",
                        "patch" : { "obj" : { "field_3" : "patched_value" } }
                    }
                }
            }
        )";

        const char* expectedOutput = R"(
            {
                "name" : "nested_import",
                "object": {
                    "desc" : "Nested Import",
                    "obj" : {
                        "field_1" : "value_1",
                        "field_2" : "value_2",
                        "field_3" : "patched_value"
                    }
                }
            }
        )";

        TestImportLoadStore(inputFile, expectedOutput);
    }

    TEST_F(JsonImportingTests, NestedImportCycleTest)
    {
        const char* inputFile = R"(
            {
                "name" : "nested_import_cycle",
                "object": {"$import" : "nested_import_c1.json"}
            }
        )";

        TestImportCycle(inputFile);
    }

    TEST_F(JsonImportingTests, InsertNewImportTest)
    {
        const char* inputFile = R"(
            {
                "name" : "simple_object_import",
                "object_1": {"$import" : "object.json"},
                "object_2": {
                    "field_1" : "other_value",
                    "field_2" : "value_2",
                    "field_3" : "value_3"
                }
            }
        )";

        const char* expectedOutput = R"(
            {
                "name" : "simple_object_import",
                "object_1": {"$import" : "object.json"},
                "object_2": {
                    "$import" : { 
                        "filename" : "object.json",
                        "patch" : { "field_1" : "other_value" }
                    }
                }
            }
        )";

        TestInsertNewImport(inputFile, expectedOutput);
    }
}
