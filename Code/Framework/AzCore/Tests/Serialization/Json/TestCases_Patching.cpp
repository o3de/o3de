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

#include <AzCore/Math/Vector3.h>
#include <AzCore/std/string/string_view.h>
#include <Tests/Serialization/Json/JsonSerializationTests.h>

namespace JsonSerializationTests
{
    class JsonPatchingSerializationTests
        : public JsonSerializationTests
    {
    protected:
        void CheckApplyPatch_Core(const char* target, const char* patch, AZStd::string_view outcome, AZ::JsonMergeApproach approach)
        {
            m_jsonDocument->Parse(target);
            ASSERT_FALSE(m_jsonDocument->HasParseError());

            rapidjson::Document patchDocument;
            patchDocument.Parse(patch);
            ASSERT_FALSE(patchDocument.HasParseError());

            AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::ApplyPatch(*m_jsonDocument, m_jsonDocument->GetAllocator(),
                patchDocument, approach);
            EXPECT_EQ(result.GetTask(), AZ::JsonSerializationResult::Tasks::Merge);
            ASSERT_EQ(result.GetOutcome(), AZ::JsonSerializationResult::Outcomes::Success);

            Expect_DocStrEq(outcome);
        }

        void CheckApplyMergePatch(const char* target, const char* patch, AZStd::string_view outcome)
        {
            CheckApplyPatch_Core(target, patch, outcome, AZ::JsonMergeApproach::JsonMergePatch);
        }

        void CheckApplyPatch(const char* target, const char* patch, AZStd::string_view outcome)
        {
            CheckApplyPatch_Core(target, patch, outcome, AZ::JsonMergeApproach::JsonPatch);
        }

        void CheckApplyPatchOutcome(const char* target, const char* patch,
            AZ::JsonSerializationResult::Outcomes outcome, AZ::JsonSerializationResult::Processing processing)
        {
            m_jsonDocument->Parse(target);
            ASSERT_FALSE(m_jsonDocument->HasParseError());

            rapidjson::Document patchDocument;
            patchDocument.Parse(patch);
            ASSERT_FALSE(patchDocument.HasParseError());

            AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::ApplyPatch(*m_jsonDocument,
                m_jsonDocument->GetAllocator(), patchDocument, AZ::JsonMergeApproach::JsonPatch);
            EXPECT_EQ(result.GetTask(), AZ::JsonSerializationResult::Tasks::Merge);
            EXPECT_EQ(result.GetOutcome(), outcome);
            EXPECT_EQ(result.GetProcessing(), processing);
        }

        void CheckCreatePatch_Core(const char* source, AZStd::string_view patch, const char* target,
            AZ::JsonMergeApproach approach)
        {
            rapidjson::Document sourceDocument;
            sourceDocument.Parse(source);
            ASSERT_FALSE(sourceDocument.HasParseError());

            rapidjson::Document targetDocument;
            targetDocument.Parse(target);
            ASSERT_FALSE(targetDocument.HasParseError());

            AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::CreatePatch(*m_jsonDocument,
                m_jsonDocument->GetAllocator(), sourceDocument, targetDocument, approach);
            EXPECT_EQ(result.GetTask(), AZ::JsonSerializationResult::Tasks::CreatePatch);
            bool success =
                result.GetOutcome() == AZ::JsonSerializationResult::Outcomes::Success ||
                result.GetOutcome() == AZ::JsonSerializationResult::Outcomes::DefaultsUsed ||
                result.GetOutcome() == AZ::JsonSerializationResult::Outcomes::PartialDefaults;
            ASSERT_TRUE(success);

            Expect_DocStrEq(patch);

            // Do round trip. This assumes that all tests to apply have already been done and have passed.
            result = AZ::JsonSerialization::ApplyPatch(sourceDocument, sourceDocument.GetAllocator(), *m_jsonDocument, approach);
            success =
                result.GetOutcome() == AZ::JsonSerializationResult::Outcomes::Success ||
                result.GetOutcome() == AZ::JsonSerializationResult::Outcomes::DefaultsUsed ||
                result.GetOutcome() == AZ::JsonSerializationResult::Outcomes::PartialDefaults;
            ASSERT_TRUE(success);
            Expect_DocStrEq(sourceDocument, targetDocument);
        }

        void CheckCreateMergePatch(const char* source, AZStd::string_view patch, const char* target)
        {
            CheckCreatePatch_Core(source, patch, target, AZ::JsonMergeApproach::JsonMergePatch);
        }

        void CheckCreatePatch(const char* source, const char* target, AZStd::string_view patch)
        {
            CheckCreatePatch_Core(source, patch, target, AZ::JsonMergeApproach::JsonPatch);
        }

        void CheckUnsupportedCreateMergePatch(const char* source, const char* target)
        {
            rapidjson::Document sourceDocument;
            sourceDocument.Parse(source);
            ASSERT_FALSE(sourceDocument.HasParseError());

            rapidjson::Document targetDocument;
            targetDocument.Parse(target);
            ASSERT_FALSE(targetDocument.HasParseError());

            AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::CreatePatch(*m_jsonDocument, m_jsonDocument->GetAllocator(),
                sourceDocument, targetDocument, AZ::JsonMergeApproach::JsonMergePatch);
            EXPECT_EQ(result.GetTask(), AZ::JsonSerializationResult::Tasks::CreatePatch);
            ASSERT_EQ(result.GetOutcome(), AZ::JsonSerializationResult::Outcomes::Unsupported);
        }
    };

    // ApplyPatch
    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchNoArrayAtRoot_ReportError)
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome("{}", R"({})", Outcomes::TypeMismatch, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchInvalidEntryInArray_ReportError)
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome("{}", "[42]", Outcomes::TypeMismatch, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchMissingOpField_ReportError)
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome("{}", R"([{ "path": "/test", "value": "test" }])",
            Outcomes::Missing, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchIncorrectOpValueType_ReportError)
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome("{}", R"([{ "op": 42, "path": "/test", "value": "test" }])",
            Outcomes::TypeMismatch, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchMissingPathField_ReportError)
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome("{}", R"([{ "op": "add", "value": "test" }])",
            Outcomes::Missing, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchIncorrectPathValueType_ReportError)
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome("{}", R"([{ "op": "add", "path": 42, "value": "test" }])",
            Outcomes::TypeMismatch, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchIncorrectPathPointer_ReportError)
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome("{}", R"([{ "op": "add", "path": "test~", "value": "test" }])",
            Outcomes::Invalid, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchInvalidOperation_ReportError)
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome("{}", R"([{ "op": "invalid", "path": "/test", "value": "test" }])",
            Outcomes::Unknown, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchAddObjectMember_ReportsSuccess) // Official example A.1
    {
        CheckApplyPatch(
            R"( { "foo": "bar"})",
            R"( [
                    { "op": "add", "path": "/baz", "value": "qux" }
                ])",
            R"( {
                    "foo": "bar",
                    "baz": "qux"
                })");
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchAddArrayMember_ReportsSuccess) // Official example A.2
    {
        CheckApplyPatch(
            R"( { "foo": [ "bar", "baz" ] })",
            R"( [
                    { "op": "add", "path": "/foo/1", "value": "qux" }
                ])",
            R"( { "foo": [ "bar", "qux", "baz" ] })");
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchAddArrayMemberAtEdge_ReportsSuccess)
    {
        CheckApplyPatch(
            R"( { "test": [ 142, 242 ] })",
            R"( [
                    { "op": "add", "path": "/test/2", "value": 342 }
                ])",
            R"( { "test": [ 142, 242, 342 ] })");
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchAddArrayMemberBeforeFirst_ReportsSuccess)
    {
        CheckApplyPatch(
            R"( { "test": [ 142, 242 ] })",
            R"( [
                    { "op": "add", "path": "/test/0", "value": 342 }
                ])",
            R"( { "test": [ 342, 142, 242] })");
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchAddOperationPathIsRoot_ReportSuccess)
    {
        CheckApplyPatch(
            R"( { "test": {} })",
            R"( [
                    { "op": "add", "path": "", "value": 42 }
                ])",
            R"( 42 )");
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchAddOperationMissingValue_ReportError)
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome(
            R"({ "test": {} })",
            R"([{ "op": "add", "path": "/test" }])",
            Outcomes::Missing, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchAddOperationInvalidArrayPath_ReportError)
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome(
            R"({ "test": [] })",
            R"([{ "op": "add", "path": "/test/invalid", "value": "test" }])",
            Outcomes::Invalid, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchAddOperationInvalidArrayIndex_ReportError)
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome(
            R"({ "test": [42] })",
            R"([{ "op": "add", "path": "/test/2", "value": "test" }])",
            Outcomes::Invalid, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchAddOperationInvalidTargetType_ReportError)
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome(
            R"({ "test": 42 })",
            R"([{ "op": "add", "path": "/test/new_member", "value": "test" }])",
            Outcomes::TypeMismatch, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchAddUnnamedMember_ReportsSuccess)
    {
        CheckApplyPatch(
            R"( { "test": 142 })",
            R"( [
                    { "op": "add", "path": "/", "value": 242 }
                ])",
            R"( {
                    "test": 142,
                    "": 242
                })");
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchRemoveObjectMember_ReportsSuccess) // Official example A.3
    {
        CheckApplyPatch(
            R"( {
                    "baz": "qux",
                    "foo": "bar"
                })",
            R"( [
                    { "op": "remove", "path": "/baz" }
                ])",
            R"({ "foo": "bar" })");
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchRemoveArrayMember_ReportsSuccess) // Official example A.4
    {
        CheckApplyPatch(
            R"( { "foo": [ "bar", "qux", "baz" ] })",
            R"( [
                    { "op": "remove", "path": "/foo/1" }
                ])",
            R"( { "foo": [ "bar", "baz" ] })");
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchRemoveOperationInvalidParent_ReportError)
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome(
            R"({ "test": { "element": 42 }})",
            R"([{ "op": "remove", "path": "/invalid/element" }])",
            Outcomes::Invalid, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchRemoveOperationInvalidMemberPath_ReportError)
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome(
            R"({ "test": { "element": 42 }})",
            R"([{ "op": "remove", "path": "/test/invalid" }])",
            Outcomes::Invalid, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchRemoveOperationInvalidArrayIndex_ReportError)
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome(
            R"({ "test": [42] })",
            R"([{ "op": "remove", "path": "/test/2" }])",
            Outcomes::Invalid, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchRemoveOperationPathToRoot_ReportError)
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome(
            R"({ "test": { "element": 42 }})",
            R"([{ "op": "remove", "path": "" }])",
            Outcomes::Invalid, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchReplaceValue_ReportsSuccess) // Official example A.5
    {
        CheckApplyPatch(
            R"( {
                    "baz": "qux",
                    "foo": "bar"
                })",
            R"( [
                    { "op": "replace", "path": "/baz", "value": "boo" }
                ])",
            R"( {
                    "baz": "boo",
                    "foo": "bar"
                })");
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchReplaceObjectMemberWithOtherType_ReportsSuccess)
    {
        CheckApplyPatch(
            R"( {
                    "baz": "qux",
                    "foo": "bar"
                })",
            R"( [
                    { "op": "replace", "path": "/foo", "value": 626172 }
                ])",
            R"( {
                    "baz": "qux",
                    "foo": 626172
                })");
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchReplaceArrayMember_ReportsSuccess)
    {
        CheckApplyPatch(
            R"( { "foo": [ "bar", "qux", "baz" ] })",
            R"( [
                    { "op": "replace", "path": "/foo/1", "value": 42 }
                ])",
            R"( { "foo": [ "bar", 42, "baz" ] })");
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchReplaceOperationMissingValue_ReportError)
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome(
            R"({ "test": {}})",
            R"([{ "op": "replace", "path": "/test" }])",
            Outcomes::Missing, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchReplaceOperationInvalidPath_ReportError)
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome(
            R"({ "test": { "member": 42 }})",
            R"([{ "op": "replace", "path": "/test/invalid", "value": 88 }])",
            Outcomes::Invalid, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchMoveValue_ReportsSuccess) // Official example A.6
    {
        CheckApplyPatch(
            R"( {
                    "foo":
                    {
                        "bar": "baz",
                        "waldo": "fred"
                    },
                    "qux":
                    {
                        "corge": "grault"
                    }
                })",
            R"( [
                    { "op": "move", "from": "/foo/waldo", "path": "/qux/thud" }
                ])",
            R"( {
                    "foo":
                    {
                        "bar": "baz"
                    },
                    "qux":
                    {
                        "corge": "grault",
                        "thud": "fred"
                    }
                })");
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchMoveArrayElement_ReportsSuccess) // Official example A.7
    {
        CheckApplyPatch(
            R"( { "foo": [ "all", "grass", "cows", "eat" ] })",
            R"( [
                    { "op": "move", "from": "/foo/1", "path": "/foo/3" }
                ])",
            R"( { "foo": [ "all", "cows", "eat", "grass" ] })");
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchMoveOperationMissingFromPath_ReportError)
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome(
            R"({})",
            R"([{ "op": "move", "path": "/test/invalid" }])",
            Outcomes::Missing, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchMoveOperationFromPathWrongType_ReportError)
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome(
            R"({})",
            R"([{ "op": "move", "path": "/test", "from": 42 }])",
            Outcomes::TypeMismatch, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchMoveOperationInvalidFromPath_ReportError)
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome(
            R"({})",
            R"([{ "op": "move", "path": "/test", "from": "test~" }])",
            Outcomes::Invalid, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchMoveOperationMissingFromTarget_ReportError)
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome(
            R"({ "test": { "Member1": "Value1" } })",
            R"([{ "op": "move", "path": "/test", "from": "/test/invalid" }])",
            Outcomes::Invalid, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchMoveOperationMoveIntoChild_ReportError)
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome(
            R"( {
                    "test":
                    {
                        "Member1":
                        {
                            "Member2" : "ValueNested"
                        }
                    }
                })",
            R"([{ "op": "move", "path": "/test/Member1", "from": "/test" }])",
            Outcomes::Invalid, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchCopyValue_ReportsSuccess)
    {
        CheckApplyPatch(
            R"( {
                    "Member1": "Value1",
                    "Member2": "Value2"
                })",
            R"( [
                    { "op": "copy", "from": "/Member1", "path": "/Member3" }
                ])",
            R"( {
                    "Member1": "Value1",
                    "Member2": "Value2",
                    "Member3": "Value1"
                })");
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchCopyFromChild_ReportsSuccess)
    {
        CheckApplyPatch(
            R"( {
                    "test":
                    {
                        "Member1":
                        {
                            "Nested" : "ValueNested"
                        }
                    }
                })",
            R"( [
                    { "op": "copy", "from": "/test/Member1", "path": "/test/Member1/NewMember" }
                ])",
            R"( {
                    "test":
                    {
                        "Member1":
                        {
                            "Nested" : "ValueNested",
                            "NewMember" :
                            {
                                "Nested" : "ValueNested"
                            }
                        }
                    }
                })");
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchTestOperationPasses_ReportsSuccess) // Official example A.8
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome(
            R"( {
                    "baz": "qux",
                    "foo": [ "a", 2, "c" ]
                })",
            R"( [
                    { "op": "test", "path": "/baz", "value": "qux" },
                    { "op": "test", "path": "/foo/1", "value": 2 }
                ])",
            Outcomes::Success, Processing::Completed);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchTestStops_ReportsFailues) // Official example A.9
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome(
            R"({ "baz": "qux" })",
            R"( [
                    { "op": "test", "path": "/baz", "value": "bar" }
                ])",
            Outcomes::TestFailed, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchTestOperationMissingValueMember_ReportError)
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome(
            R"({})",
            R"([{ "op": "test", "path": "/test" }])",
            Outcomes::Missing, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchTestOperationInvalidPathTarget_ReportError)
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome(
            R"({ "test": { "Member1": "Value1" } })",
            R"([{ "op": "test", "path": "/test/invalid", "value": 42 }])",
            Outcomes::Invalid, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchAddNestedMemberObject_ReportsSuccess) // Official example A.10
    {
        CheckApplyPatch(
            R"( { "foo": "bar" })",
            R"( [
                    { "op": "add", "path": "/child", "value": { "grandchild": { } } }
                ])",
            R"( {
                    "foo": "bar",
                    "child":
                    {
                        "grandchild":{}
                    }
                })");
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchIgnoringUnrecognizedElements_ReportsSuccess) // Official example A.11
    {
        CheckApplyPatch(
            R"( { "foo": "bar" })",
            R"( [
                    { "op": "add", "path": "/baz", "value": "qux", "xyz": 123 }
                ])",
            R"( {
                    "foo": "bar",
                    "baz": "qux"
                })");
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchAddingToNonexistentTarget_ReportsFailure) // Official example A.12
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome(
            R"( { "foo": "bar" })",
            R"( [
                    { "op": "add", "path": "/baz/bat", "value": "qux" }
                ])",
            Outcomes::Invalid, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchDuplicatedArguments_ReportsSuccess) // Official example A.13
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome(
            R"( { "baz": {} })",
            R"( [
                    { "op": "add", "path": "/baz", "value": "qux", "op": "remove" }
                ])",
            // This succeeds while the official standard indicates it should fail. The JSON library used for patching
            // breaks with the JSON standard and allows objects to have members with the same name. Checking the validity
            // for all members would be expensive to do, especially for larger patches and also not needed because the
            // way patching is implemented it uses the FindMember function which will only ever return the first
            // occurrence of the member. In this regard, ApplyPatch treats the duplicates for any field as if they're
            // additional unrecognized members, which is allowed according to the JSON Patch standard.
            Outcomes::Success, Processing::Completed);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchEscapeOrdering_ReportsSuccess) // Official example A.14
    {
        CheckApplyPatch(
            R"( {
                    "/": 9,
                    "~1": 10
                })",
            R"( [
                    {"op": "test", "path": "/~01", "value": 10}
                ])",
            R"( {
                    "/": 9,
                    "~1": 10
                })");
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchComparingStringsAndNumbers_ReportsTestFailure) // Official example A.15
    {
        using namespace AZ::JsonSerializationResult;
        CheckApplyPatchOutcome(
            R"( {
                    "/": 9,
                    "~1": 10
                })",
            R"( [
                    {"op": "test", "path": "/~01", "value": "10"}
                ])",
            Outcomes::TestFailed, Processing::Halted);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonPatchAddArrayObject_ReportsSuccess) // Official example A.16
    {
        CheckApplyPatch(
            R"( { "foo": ["bar"] })",
            R"( [
                    { "op": "add", "path": "/foo/-", "value": ["abc", "def"] }
                ])",
            R"( { "foo": ["bar", ["abc", "def"]] })");
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_UseJsonMergePatchOfficialTestCases_AllMergesComplete)
    {
        using namespace AZ::JsonSerializationResult;

        CheckApplyMergePatch(R"({"a":"b"})", R"({"a":"c"})", R"({"a":"c"})");
        CheckApplyMergePatch(R"({"a":"b"})", R"({"b":"c"})", R"({"a":"b", "b":"c"})");
        CheckApplyMergePatch(R"({"a":"b"})", R"({"a":null})", R"({})");
        CheckApplyMergePatch(R"({"a":"b","b":"c"})", R"({"a":null})", R"({"b":"c"})");
        CheckApplyMergePatch(R"({"a":["b"]})", R"({"a":"c"})", R"({"a":"c"})");
        CheckApplyMergePatch(R"({"a":"c"})", R"({"a":["b"]})", R"({"a":["b"]})");
        CheckApplyMergePatch(R"({"a":{"b":"c"}})", R"({"a": {"b":"d","c":null}})", R"({"a":{"b":"d"}})");
        CheckApplyMergePatch(R"({"a":{"b":"c"}})", R"({"a":[1]})", R"({"a": [1]})");
        CheckApplyMergePatch(R"(["a", "b"])", R"(["c", "d"])", R"(["c", "d"])");
        CheckApplyMergePatch(R"({"a":"b"})", R"(["c"])", R"(["c"])");
        CheckApplyMergePatch(R"({"a":"foo"})", R"(null)", R"(null)");
        CheckApplyMergePatch(R"({"a":"foo"})", R"("bar")", R"("bar")");
        CheckApplyMergePatch(R"({"e":null})", R"({"a":1})", R"({"e":null,"a": 1})");
        CheckApplyMergePatch(R"([1, 2])", R"({"a":"b","c":null})", R"({"a":"b"})");
        CheckApplyMergePatch(R"({})", R"({"a":{"bb":{"ccc":null}}})", R"({"a":{"bb":{}}})");
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_CopiedSourceOverload_DefaultObject)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document sourceDocument;
        sourceDocument.Parse(R"({ "value0": 42 })");
        ASSERT_FALSE(sourceDocument.HasParseError());

        rapidjson::Document patchDocument;
        patchDocument.Parse(R"([{ "op": "add", "path": "/value1", "value": 88 }])");
        ASSERT_FALSE(patchDocument.HasParseError());

        AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::ApplyPatch(*m_jsonDocument,
            m_jsonDocument->GetAllocator(), sourceDocument, patchDocument, AZ::JsonMergeApproach::JsonPatch);
        EXPECT_EQ(result.GetProcessing(), Processing::Completed);

        static constexpr char output[] =
            R"(
                {
                    "value0": 42,
                    "value1": 88
                })";
        Expect_DocStrEq(output, true);
    }

    TEST_F(JsonPatchingSerializationTests, ApplyPatch_CopiedSourceReturnsDefaultOnError_DefaultObject)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document sourceDocument;
        sourceDocument.Parse(R"({ "value": 42 })");
        ASSERT_FALSE(sourceDocument.HasParseError());

        rapidjson::Document patchDocument;
        patchDocument.Parse("{}");
        ASSERT_FALSE(patchDocument.HasParseError());

        AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::ApplyPatch(*m_jsonDocument,
            m_jsonDocument->GetAllocator(), sourceDocument, patchDocument, AZ::JsonMergeApproach::JsonPatch);
        EXPECT_EQ(result.GetProcessing(), Processing::Halted);

        EXPECT_TRUE(m_jsonDocument->IsObject());
        EXPECT_EQ(0, m_jsonDocument->MemberCount());
    }

    // CreatePatch
    TEST_F(JsonPatchingSerializationTests, CreatePatch_UseJsonPatchUpdateValue_OperationToUpdateValue)
    {
        CheckCreatePatch(
            R"( 42 )",
            R"( 88 )",
            R"( [
                    { "op": "replace", "path": "", "value": 88 }
                ])"
        );
    }

    TEST_F(JsonPatchingSerializationTests, CreatePatch_UseJsonPatchIdenticalValues_EmptyPatch)
    {
        CheckCreatePatch("42", "42", "[]");
    }

    TEST_F(JsonPatchingSerializationTests, CreatePatch_UseJsonPatchObjectToValue_OperationToUpdateToValue)
    {
        CheckCreatePatch(
            R"( { "value": "foo" })",
            R"( 42 )",
            R"( [
                    { "op": "replace", "path": "", "value": 42 }
                ])"
        );
    }

    TEST_F(JsonPatchingSerializationTests, CreatePatch_UseJsonPatchValueToObject_OperationToUpdateToObject)
    {
        CheckCreatePatch(
            R"( 42 )",
            R"( { "value": "foo" })",
            R"( [
                    { "op": "replace", "path": "", "value": { "value": "foo" } }
                ])"
        );
    }

    TEST_F(JsonPatchingSerializationTests, CreatePatch_UseJsonPatchArrayToValue_OperationToUpdateToValue)
    {
        CheckCreatePatch(
            R"( [ 42, 88 ] )",
            R"( 42 )",
            R"( [
                    { "op": "replace", "path": "", "value": 42 }
                ])"
        );
    }

    TEST_F(JsonPatchingSerializationTests, CreatePatch_UseJsonPatchValueToArray_OperationToUpdateToArray)
    {
        CheckCreatePatch(
            R"( 42 )",
            R"( [ 42, 88 ] )",
            R"( [
                    { "op": "replace", "path": "", "value": [ 42, 88 ] }
                ])"
        );
    }

    TEST_F(JsonPatchingSerializationTests, CreatePatch_UseJsonPatchUpdateMember_OperationToUpdateMember)
    {
        CheckCreatePatch(
            R"( { "value": "foo" })",
            R"( { "value": "bar" })",
            R"( [
                    { "op": "replace", "path": "/value", "value": "bar" }
                ])"
        );
    }

    TEST_F(JsonPatchingSerializationTests, CreatePatch_UseJsonPatchIdenticalMembers_EmptyPatch)
    {
        CheckCreatePatch(
            R"( { "value": "foo" })",
            R"( { "value": "foo" })",
            R"( [])"
        );
    }

    TEST_F(JsonPatchingSerializationTests, CreatePatch_UseJsonPatchUpdateMemberType_OperationToUpdateMember)
    {
        CheckCreatePatch(
            R"( { "value": "foo" })",
            R"( { "value": 42 })",
            R"( [
                    { "op": "replace", "path": "/value", "value": 42 }
                ])"
        );
    }

    TEST_F(JsonPatchingSerializationTests, CreatePatch_UseJsonPatchAddNewMember_OperationToAddMember)
    {
        CheckCreatePatch(
            R"( {
                    "value0": "foo"
                })",
            R"( {
                    "value0": "foo",
                    "value1": "bar"
                })",
            R"( [
                    { "op": "add", "path": "/value1", "value": "bar" }
                ])"
        );
    }

    TEST_F(JsonPatchingSerializationTests, CreatePatch_UseJsonPatchRemoveMember_OperationToRemoveMember)
    {
        CheckCreatePatch(
            R"( {
                    "value0": "foo",
                    "value1": "bar"
                })",
            R"( {
                    "value0": "foo"
                })",
            R"( [
                    { "op": "remove", "path": "/value1" }
                ])"
        );
    }


    TEST_F(JsonPatchingSerializationTests, CreatePatch_UseJsonPatchEmptyObjects_EmptyPatch)
    {
        CheckCreatePatch("{}", "{}", "[]");
    }

    TEST_F(JsonPatchingSerializationTests, CreatePatch_UseJsonPatchUpdateArrayEntry_OperationToUpdateMember)
    {
        CheckCreatePatch(
            R"( [ "foo" ])",
            R"( [ "bar" ])",
            R"( [
                    { "op": "replace", "path": "/0", "value": "bar" }
                ])"
        );
    }

    TEST_F(JsonPatchingSerializationTests, CreatePatch_UseJsonPatchIdenticalArrays_EmptyPatch)
    {
        CheckCreatePatch(R"(["foo"])", R"(["foo"])", R"([])");
    }

    TEST_F(JsonPatchingSerializationTests, CreatePatch_UseJsonPatchUpdateArrayEntryType_OperationToUpdateMember)
    {
        CheckCreatePatch(
            R"( [ "foo" ])",
            R"( [ 42 ])",
            R"( [
                    { "op": "replace", "path": "/0", "value": 42 }
                ])"
        );
    }

    TEST_F(JsonPatchingSerializationTests, CreatePatch_UseJsonPatchAddNewArrayEntryAtEnd_OperationToAddMember)
    {
        CheckCreatePatch(
            R"( [ "foo" ])",
            R"( [ "foo", "bar" ])",
            R"( [
                    { "op": "add", "path": "/1", "value": "bar" }
                ])"
        );
    }

    TEST_F(JsonPatchingSerializationTests, CreatePatch_UseJsonPatchAddNewArrayEntryInMiddle_MultipleOperations)
    {
        CheckCreatePatch(
            R"( [ "foo", "bar" ])",
            R"( [ "foo", "hello", "bar" ])",
            R"( [
                    { "op": "replace", "path": "/1", "value": "hello" },
                    { "op": "add", "path": "/2", "value": "bar" }
                ])"
        );
    }

    TEST_F(JsonPatchingSerializationTests, CreatePatch_UseJsonPatchRemoveLastArrayEntry_OperationToRemoveMember)
    {
        CheckCreatePatch(
            R"( [ "foo", "bar" ])",
            R"( [ "foo" ])",
            R"( [
                    { "op": "remove", "path": "/1" }
                ])"
        );
    }

    TEST_F(JsonPatchingSerializationTests, CreatePatch_UseJsonPatchRemoveArrayEntryInMiddle_MultipleOperations)
    {
        CheckCreatePatch(
            R"( [ "foo", "hello", "bar" ])",
            R"( [ "foo", "bar"])",
            R"( [
                    { "op": "replace", "path": "/1", "value": "bar" },
                    { "op": "remove", "path": "/2" }
                ])"
        );
    }

    TEST_F(JsonPatchingSerializationTests, CreatePatch_UseJsonPatchRemoveObjectFromArrayInMiddle_OperationToUpdateMember)
    {
        CheckCreatePatch(
            R"( [
                    { "foo1": 142, "bar1": 188 },
                    { "foo2": 242, "bar2": 288 },
                    { "foo3": 342, "bar3": 388 },
                    { "foo4": 442, "bar4": 488 }
                ])",
            R"( [
                    { "foo1": 142, "bar1": 188 },
                    { "foo3": 342, "bar3": 388 },
                    { "foo4": 442, "bar4": 488 }
                ])",
            R"( [
                    { "op": "add", "path": "/1/foo3", "value": 342 },
                    { "op": "add", "path": "/1/bar3", "value": 388 },
                    { "op": "remove", "path": "/1/foo2" },
                    { "op": "remove", "path": "/1/bar2" },
                    { "op": "add", "path": "/2/foo4", "value": 442 },
                    { "op": "add", "path": "/2/bar4", "value": 488 },
                    { "op": "remove", "path": "/2/foo3" },
                    { "op": "remove", "path": "/2/bar3" },
                    { "op": "remove", "path": "/3" }
                ])"
        );
    }

    TEST_F(JsonPatchingSerializationTests, CreatePatch_UseJsonPatchEmptyArrays_EmptyPatch)
    {
        CheckCreatePatch("[]", "[]", "[]");
    }

    TEST_F(JsonPatchingSerializationTests, CreatePatch_UseJsonPatchNestedUpdate_OperationToUpdateMember)
    {
        CheckCreatePatch(
            R"( [
                    { "foo": 42 },
                    {
                        "bar":
                        [
                            42, 88, { "hello": 88 }, 1024
                        ]
                    }
                ])",
            R"( [
                    { "foo": 42 },
                    {
                        "bar":
                        [
                            42, 88, { "hello": 256 }, 1024
                        ]
                    }
                ])",
            R"( [
                    { "op": "replace", "path": "/1/bar/2/hello", "value": 256 }
                ])"
        );
    }

    TEST_F(JsonPatchingSerializationTests, CreatePatch_UseJsonMergePatchOfficialTestCases_AllMergesComplete)
    {
        using namespace AZ::JsonSerializationResult;

        CheckCreateMergePatch(R"({"a":"b"})", R"({"a":"c"})", R"({"a":"c"})");
        CheckCreateMergePatch(R"({"a":"b"})", R"({"b":"c"})", R"({"a":"b", "b":"c"})");
        CheckCreateMergePatch(R"({"a":"b"})", R"({"a":null})", R"({})");
        CheckCreateMergePatch(R"({"a":"b","b":"c"})", R"({"a":null})", R"({"b":"c"})");
        CheckCreateMergePatch(R"({"a":["b"]})", R"({"a":"c"})", R"({"a":"c"})");
        CheckCreateMergePatch(R"({"a":"c"})", R"({"a":["b"]})", R"({"a":["b"]})");
        // Removed "c" from the official test as the test was for valid application, but it wouldn't be created.
        CheckCreateMergePatch(R"({"a":{"b":"c"}})", R"({"a": {"b":"d"}})", R"({"a":{"b":"d"}})");
        CheckCreateMergePatch(R"({"a":{"b":"c"}})", R"({"a":[1]})", R"({"a": [1]})");
        CheckCreateMergePatch(R"(["a", "b"])", R"(["c", "d"])", R"(["c", "d"])");
        CheckCreateMergePatch(R"({"a":"b"})", R"(["c"])", R"(["c"])");
        // Removed official test as the test was for valid application, but it wouldn't be created.
        //CheckCreateMergePatch(R"({"a":"foo"})",       R"(null)",                          R"(null)");
        CheckCreateMergePatch(R"({"a":"foo"})", R"("bar")", R"("bar")");
        CheckCreateMergePatch(R"({"e":null})", R"({"a":1})", R"({"e":null,"a": 1})");
        // Removed "c" from the official test as the test was for valid application, but it wouldn't be created.
        CheckCreateMergePatch(R"([1, 2])", R"({"a":"b"})", R"({"a":"b"})");
        // Removed "ccc" from the official test as the test was for valid application, but it wouldn't be created.
        CheckCreateMergePatch(R"({})", R"({"a":{"bb":{}}})", R"({"a":{"bb":{}}})");
    }

    TEST_F(JsonPatchingSerializationTests, CreatePatch_UseJsonMergePatchWithUnsupportedTarget_UnsupportedReported)
    {
        CheckUnsupportedCreateMergePatch(R"({"a":42})", R"(null)");
        CheckUnsupportedCreateMergePatch(R"({"a":42})", R"({"a":null})");
        CheckUnsupportedCreateMergePatch(R"(42)", R"({"a":null})");
        CheckUnsupportedCreateMergePatch(R"(42)", R"({"a": { "b": null }})");
    }
} // namespace JsonSerializationTests
