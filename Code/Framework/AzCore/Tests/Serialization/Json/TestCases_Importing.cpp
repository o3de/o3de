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


        AZ::JsonSerializationResult::ResultCode Load(rapidjson::Value& importedValueOut, const rapidjson::Value& importDirective, rapidjson::Pointer pathToImportDirective, rapidjson::Document::AllocatorType& allocator) override;
        AZ::JsonSerializationResult::ResultCode Store(rapidjson::Value& importDirectiveOut, const rapidjson::Value& importedValue, rapidjson::Pointer pathToImportDirective, rapidjson::Document::AllocatorType& allocator, AZStd::string& importFilename) override;
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

            rapidjson::Document array;
            array.Parse(arrayJson);
            ASSERT_FALSE(array.HasParseError());
            
            if (docName.compare("object.json") == 0)
            {
                rapidjson::Document object;
                object.Parse(objectJson);
                ASSERT_FALSE(object.HasParseError());
                out.CopyFrom(object, allocator);
            }
            else
            {
                rapidjson::Document array;
                array.Parse(arrayJson);
                ASSERT_FALSE(array.HasParseError());
                out.CopyFrom(array, allocator);
            }
        }

    protected:
        void TestImportLoadStore(const char* input, const char* expectedImportedValue)
        {
            m_jsonDocument->Parse(input);
            ASSERT_FALSE(m_jsonDocument->HasParseError());

            JsonImporterCustom* importerObj = new JsonImporterCustom(this);
            AZStd::unique_ptr<AZ::JsonImportResolver> importResolver(new AZ::JsonImportResolver(importerObj));

            AZ::StackedString pathLoad(AZ::StackedString::Format::JsonPointer);
            importResolver->LoadImports(m_jsonDocument->GetObject(), pathLoad, m_jsonDocument->GetAllocator());

            rapidjson::Document expectedOutcome;
            expectedOutcome.Parse(expectedImportedValue);
            ASSERT_FALSE(expectedOutcome.HasParseError());

            Expect_DocStrEq(m_jsonDocument->GetObject(), expectedOutcome.GetObject());
            
            rapidjson::Document originalInput;
            originalInput.Parse(input);
            ASSERT_FALSE(originalInput.HasParseError());
            
            AZ::StackedString pathStore(AZ::StackedString::Format::JsonPointer);
            importResolver->StoreImports(m_jsonDocument->GetObject(), pathStore, m_jsonDocument->GetAllocator());
            
            Expect_DocStrEq(m_jsonDocument->GetObject(), originalInput.GetObject());
        }
    };

    AZ::JsonSerializationResult::ResultCode JsonImporterCustom::Load(rapidjson::Value& importedValueOut, const rapidjson::Value& importDirective, rapidjson::Pointer pathToImportDirective, rapidjson::Document::AllocatorType& allocator)
    {
        AZ::JsonSerializationResult::ResultCode resultCode(AZ::JsonSerializationResult::Tasks::Import);
        AZ::IO::FixedMaxPath filePath;
        rapidjson::Value patch;
        bool hasPatch = false;
        if (importDirective.IsObject())
        {
            auto filenameField = importDirective.FindMember("filename");
            if (filenameField != importDirective.MemberEnd())
            {
                filePath.Append(filenameField->value.GetString());
            }
            auto patchField = importDirective.FindMember("patch");
            if (patchField != importDirective.MemberEnd())
            {
                patch.CopyFrom(patchField->value, allocator);
                hasPatch = true;
            }
        }
        else
        {
            filePath.Append(importDirective.GetString());
        }
        
        rapidjson::Value importedDoc;
        testClass->GetTestDocument(filePath.String(), importedDoc, allocator);
        if (hasPatch)
        {
            AZ::JsonSerialization::ApplyPatch(importedDoc, allocator, patch, AZ::JsonMergeApproach::JsonMergePatch);
        }
        importedValueOut.CopyFrom(importedDoc, allocator);

        return resultCode;
    }

    AZ::JsonSerializationResult::ResultCode JsonImporterCustom::Store(rapidjson::Value& importDirectiveOut, const rapidjson::Value& importedValue, rapidjson::Pointer pathToImportDirective, rapidjson::Document::AllocatorType& allocator, AZStd::string& importFilename)
    {
        AZ::JsonSerializationResult::ResultCode resultCode(AZ::JsonSerializationResult::Tasks::Import);
        AZ::IO::FixedMaxPath filePath;
        filePath.Append(importFilename.c_str());

        rapidjson::Value importedDoc;
        testClass->GetTestDocument(filePath.String(), importedDoc, allocator);
        rapidjson::Value patch;
        AZ::JsonSerialization::CreatePatch(patch, allocator, importedDoc, importedValue, AZ::JsonMergeApproach::JsonMergePatch);
        if (patch.IsObject())
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
}
