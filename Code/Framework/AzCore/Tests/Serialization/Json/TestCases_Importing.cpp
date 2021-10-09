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


        AZ::JsonSerializationResult::ResultCode ResolveImport(rapidjson::Value& importedValueOut,
            const rapidjson::Value& importDirective, const AZ::IO::FixedMaxPath& importedFilePath,
            rapidjson::Document::AllocatorType& allocator) override;
        
        AZ::JsonSerializationResult::ResultCode RestoreImport(rapidjson::Value& importDirectiveOut,
            const rapidjson::Value& currentValue, const rapidjson::Value& importedValue,
            rapidjson::Document::AllocatorType& allocator, const AZStd::string& importFilename) override;
        
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

        void GetTestDocument(const AZStd::string& docName, rapidjson::Value& out, rapidjson::Document::AllocatorType& allocator)
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
            
            if (docName.compare("object.json") == 0)
            {
                rapidjson::Document object;
                object.Parse(objectJson);
                ASSERT_FALSE(object.HasParseError());
                out.CopyFrom(object, allocator);
            }
            else if (docName.compare("array.json") == 0)
            {
                rapidjson::Document array;
                array.Parse(arrayJson);
                ASSERT_FALSE(array.HasParseError());
                out.CopyFrom(array, allocator);
            }
            else if (docName.compare("nested_import.json") == 0)
            {
                rapidjson::Document nestedImport;
                nestedImport.Parse(nestedImportJson);
                ASSERT_FALSE(nestedImport.HasParseError());
                out.CopyFrom(nestedImport, allocator);
            }
        }

    protected:
        void TestImportLoadStore(const char* input, const char* expectedImportedValue)
        {
            m_jsonDocument->Parse(input);
            ASSERT_FALSE(m_jsonDocument->HasParseError());

            AZ::StackedString pathLoad(AZ::StackedString::Format::JsonPointer);
            AZ::IO::FixedMaxPath filePath;
            AZ::JsonImportResolver::ImportPathStack importPathStack;
            importPathStack.push_back(filePath);
            JsonImporterCustom* importerObj = new JsonImporterCustom(this);

            AZ::JsonImportResolver::ResolveImports(m_jsonDocument->GetObject(), m_jsonDocument->GetAllocator(), importPathStack, importerObj, pathLoad);

            rapidjson::Document expectedOutcome;
            expectedOutcome.Parse(expectedImportedValue);
            ASSERT_FALSE(expectedOutcome.HasParseError());

            Expect_DocStrEq(m_jsonDocument->GetObject(), expectedOutcome.GetObject());

            rapidjson::Document originalInput;
            originalInput.Parse(input);
            ASSERT_FALSE(originalInput.HasParseError());
            
            AZ::JsonImportResolver::RestoreImports(m_jsonDocument->GetObject(), m_jsonDocument->GetAllocator(), filePath, importerObj);
            
            Expect_DocStrEq(m_jsonDocument->GetObject(), originalInput.GetObject());
            
            m_jsonDocument->SetObject();
            delete importerObj;
        }
    };

    AZ::JsonSerializationResult::ResultCode JsonImporterCustom::ResolveImport(rapidjson::Value& importedValueOut,
            const rapidjson::Value& importDirective, const AZ::IO::FixedMaxPath& importedFilePath,
            rapidjson::Document::AllocatorType& allocator)
    {
        AZ::JsonSerializationResult::ResultCode resultCode(AZ::JsonSerializationResult::Tasks::Import);
        
        rapidjson::Value importedDoc;
        testClass->GetTestDocument(importedFilePath.String(), importedDoc, allocator);

        rapidjson::Value patch;
        if (importDirective.IsObject())
        {
            auto patchField = importDirective.FindMember("patch");
            if (patchField != importDirective.MemberEnd())
            {
                patch.CopyFrom(patchField->value, allocator);
            }
        }

        if ((patch.IsObject() && patch.MemberCount() > 0) || (patch.IsArray() && !patch.Empty()))
        {
            AZ::JsonSerialization::ApplyPatch(importedDoc, allocator, patch, AZ::JsonMergeApproach::JsonMergePatch);
        }
        importedValueOut.CopyFrom(importedDoc, allocator);

        return resultCode;
    }

    AZ::JsonSerializationResult::ResultCode JsonImporterCustom::RestoreImport(rapidjson::Value& importDirectiveOut,
            const rapidjson::Value& currentValue, const rapidjson::Value& importedValue,
            rapidjson::Document::AllocatorType& allocator, const AZStd::string& importFilename)
    {
        AZ::JsonSerializationResult::ResultCode resultCode(AZ::JsonSerializationResult::Tasks::Import);

        rapidjson::Value patch;
        AZ::JsonSerialization::CreatePatch(patch, allocator, currentValue, importedValue, AZ::JsonMergeApproach::JsonMergePatch);
        if ((patch.IsObject() && patch.MemberCount() > 0) || (patch.IsArray() && !patch.Empty()))
        {
            importDirectiveOut.AddMember(rapidjson::StringRef("filename"), rapidjson::StringRef(importFilename.c_str()), allocator);
            importDirectiveOut.AddMember(rapidjson::StringRef("patch"), patch, allocator);
        }

        return resultCode;
    }

    // Test Cases

    TEST_F(JsonImportingTests, ImportSimpleObjectLoadStoreTest)
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

    TEST_F(JsonImportingTests, ImportSimpleArrayLoadStoreTest)
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

    TEST_F(JsonImportingTests, ImportNestedImportLoadStoreTest)
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
}
