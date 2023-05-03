/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomPatch.h>
#include <AzCore/DOM/DomComparison.h>
#include <Tests/DOM/DomFixtures.h>

namespace AZ::Dom::Tests
{
    class DomPatchTests : public DomTestFixture
    {
    public:
        void SetUp() override
        {
            DomTestFixture::SetUp();

            m_dataset = Value(Type::Object);
            m_dataset["arr"].SetArray();

            m_dataset["node"].SetNode("SomeNode");
            m_dataset["node"]["int"] = 5;
            m_dataset["node"]["null"] = Value();

            for (int i = 0; i < 5; ++i)
            {
                m_dataset["arr"].ArrayPushBack(Value(i));
                m_dataset["node"].ArrayPushBack(Value(i * 2));
            }

            m_dataset["obj"].SetObject();
            m_dataset["obj"]["foo"] = true;
            m_dataset["obj"]["bar"] = false;

            m_deltaDataset = m_dataset;
        }

        void TearDown() override
        {
            m_dataset = m_deltaDataset = Value();

            DomTestFixture::TearDown();
        }

        PatchUndoRedoInfo GenerateAndVerifyDelta()
        {
            PatchUndoRedoInfo info = GenerateHierarchicalDeltaPatch(m_dataset, m_deltaDataset);

            auto result = info.m_forwardPatches.Apply(m_dataset);
            EXPECT_TRUE(result.IsSuccess());
            EXPECT_TRUE(Utils::DeepCompareIsEqual(result.GetValue(), m_deltaDataset));

            result = info.m_inversePatches.Apply(result.GetValue());
            EXPECT_TRUE(result.IsSuccess());
            EXPECT_TRUE(Utils::DeepCompareIsEqual(result.GetValue(), m_dataset));

            // Verify serialization of the patches
            auto VerifySerialization = [](const Patch& patch)
            {
                Value serializedPatch = patch.GetDomRepresentation();
                auto deserializePatchResult = Patch::CreateFromDomRepresentation(serializedPatch);
                EXPECT_TRUE(deserializePatchResult.IsSuccess());
                EXPECT_EQ(deserializePatchResult.GetValue(), patch);
            };
            VerifySerialization(info.m_forwardPatches);
            VerifySerialization(info.m_inversePatches);

            return info;
        }

        Value m_dataset;
        Value m_deltaDataset;
    };

    TEST_F(DomPatchTests, AddOperation_InsertInObject_Succeeds)
    {
        Path p("/obj/baz");
        PatchOperation op = PatchOperation::AddOperation(p, Value(42));
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_EQ(result.GetValue()[p].GetInt64(), 42);
    }

    TEST_F(DomPatchTests, AddOperation_ReplaceInObject_Succeeds)
    {
        Path p("/obj/foo");
        PatchOperation op = PatchOperation::AddOperation(p, Value(false));
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_EQ(result.GetValue()[p].GetBool(), false);
    }

    TEST_F(DomPatchTests, AddOperation_InsertObjectKeyInArray_Fails)
    {
        Path p("/arr/key");
        PatchOperation op = PatchOperation::AddOperation(p, Value(999));
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, AddOperation_AppendInArray_Succeeds)
    {
        Path p("/arr/-");
        PatchOperation op = PatchOperation::AddOperation(p, Value(42));
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_EQ(result.GetValue()["arr"][5].GetInt64(), 42);
    }

    TEST_F(DomPatchTests, AddOperation_InsertKeyInNode_Succeeds)
    {
        Path p("/node/attr");
        PatchOperation op = PatchOperation::AddOperation(p, Value(500));
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_EQ(result.GetValue()[p].GetInt64(), 500);
    }

    TEST_F(DomPatchTests, AddOperation_ReplaceIndexInNode_Succeeds)
    {
        Path p("/node/0");
        PatchOperation op = PatchOperation::AddOperation(p, Value(42));
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_EQ(result.GetValue()[p].GetInt64(), 42);
    }

    TEST_F(DomPatchTests, AddOperation_AppendInNode_Succeeds)
    {
        Path p("/node/-");
        PatchOperation op = PatchOperation::AddOperation(p, Value(42));
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_EQ(result.GetValue()["node"][5].GetInt64(), 42);
    }

    TEST_F(DomPatchTests, AddOperation_InvalidPath_Fails)
    {
        Path p("/non/existent/path");
        PatchOperation op = PatchOperation::AddOperation(p, Value(0));
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, RemoveOperation_RemoveKeyFromObject_Succeeds)
    {
        Path p("/obj/foo");
        PatchOperation op = PatchOperation::RemoveOperation(p);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_FALSE(result.GetValue()["obj"].HasMember("foo"));
    }

    TEST_F(DomPatchTests, RemoveOperation_RemoveIndexFromArray_Succeeds)
    {
        Path p("/arr/0");
        PatchOperation op = PatchOperation::RemoveOperation(p);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_EQ(result.GetValue()["arr"].ArraySize(), 4);
        EXPECT_EQ(result.GetValue()["arr"][0].GetInt64(), 1);
    }

    TEST_F(DomPatchTests, RemoveOperation_PopArray_Fails)
    {
        Path p("/arr/-");
        PatchOperation op = PatchOperation::RemoveOperation(p);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, RemoveOperation_RemoveKeyFromNode_Succeeds)
    {
        Path p("/node/int");
        PatchOperation op = PatchOperation::RemoveOperation(p);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_FALSE(result.GetValue()["node"].HasMember("int"));
    }

    TEST_F(DomPatchTests, RemoveOperation_RemoveIndexFromNode_Succeeds)
    {
        Path p("/node/1");
        PatchOperation op = PatchOperation::RemoveOperation(p);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_EQ(result.GetValue()["node"].ArraySize(), 4);
        EXPECT_EQ(result.GetValue()["node"][1].GetInt64(), 4);
    }

    TEST_F(DomPatchTests, RemoveOperation_PopIndexFromNode_Fails)
    {
        Path p("/node/-");
        PatchOperation op = PatchOperation::RemoveOperation(p);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, RemoveOperation_RemoveKeyFromArray_Fails)
    {
        Path p("/arr/foo");
        PatchOperation op = PatchOperation::RemoveOperation(p);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, RemoveOperation_InvalidPath_Fails)
    {
        Path p("/non/existent/path");
        PatchOperation op = PatchOperation::RemoveOperation(p);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, ReplaceOperation_InsertInObject_Fails)
    {
        Path p("/obj/baz");
        PatchOperation op = PatchOperation::ReplaceOperation(p, Value(42));
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, ReplaceOperation_ReplaceInObject_Succeeds)
    {
        Path p("/obj/foo");
        PatchOperation op = PatchOperation::ReplaceOperation(p, Value(false));
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_EQ(result.GetValue()[p].GetBool(), false);
    }

    TEST_F(DomPatchTests, ReplaceOperation_InsertObjectKeyInArray_Fails)
    {
        Path p("/arr/key");
        PatchOperation op = PatchOperation::ReplaceOperation(p, Value(999));
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, ReplaceOperation_AppendInArray_Fails)
    {
        Path p("/arr/-");
        PatchOperation op = PatchOperation::ReplaceOperation(p, Value(42));
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, ReplaceOperation_InsertKeyInNode_Fails)
    {
        Path p("/node/attr");
        PatchOperation op = PatchOperation::ReplaceOperation(p, Value(500));
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, ReplaceOperation_ReplaceIndexInNode_Succeeds)
    {
        Path p("/node/0");
        PatchOperation op = PatchOperation::ReplaceOperation(p, Value(42));
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_EQ(result.GetValue()[p].GetInt64(), 42);
    }

    TEST_F(DomPatchTests, ReplaceOperation_AppendInNode_Fails)
    {
        Path p("/node/-");
        PatchOperation op = PatchOperation::ReplaceOperation(p, Value(42));
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, ReplaceOperation_InvalidPath_Fails)
    {
        Path p("/non/existent/path");
        PatchOperation op = PatchOperation::ReplaceOperation(p, Value(0));
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, CopyOperation_ArrayToObject_Succeeds)
    {
        Path dest("/obj/arr");
        Path src("/arr");
        PatchOperation op = PatchOperation::CopyOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_TRUE(Utils::DeepCompareIsEqual(m_dataset[src], result.GetValue()[dest]));
        EXPECT_TRUE(Utils::DeepCompareIsEqual(result.GetValue()[src], result.GetValue()[dest]));
    }

    TEST_F(DomPatchTests, CopyOperation_ObjectToArrayInRange_Succeeds)
    {
        Path dest("/arr/0");
        Path src("/obj");
        PatchOperation op = PatchOperation::CopyOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_TRUE(Utils::DeepCompareIsEqual(m_dataset[src], result.GetValue()[dest]));
        EXPECT_TRUE(Utils::DeepCompareIsEqual(result.GetValue()[src], result.GetValue()[dest]));
    }

    TEST_F(DomPatchTests, CopyOperation_ObjectToArrayOutOfRange_Fails)
    {
        Path dest("/arr/5");
        Path src("/obj");
        PatchOperation op = PatchOperation::CopyOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, CopyOperation_ObjectToNodeChildInRange_Succeeds)
    {
        Path dest("/node/0");
        Path src("/obj");
        PatchOperation op = PatchOperation::CopyOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_TRUE(Utils::DeepCompareIsEqual(m_dataset[src], result.GetValue()[dest]));
        EXPECT_TRUE(Utils::DeepCompareIsEqual(result.GetValue()[src], result.GetValue()[dest]));
    }

    TEST_F(DomPatchTests, CopyOperation_ObjectToNodeChildOutOfRange_Fails)
    {
        Path dest("/node/5");
        Path src("/obj");
        PatchOperation op = PatchOperation::CopyOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, CopyOperation_InvalidSourcePath_Fails)
    {
        Path dest("/node/0");
        Path src("/invalid/path");
        PatchOperation op = PatchOperation::CopyOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, CopyOperation_InvalidDestinationPath_Fails)
    {
        Path dest("/invalid/path");
        Path src("/arr/0");
        PatchOperation op = PatchOperation::CopyOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, MoveOperation_ArrayToObject_Succeeds)
    {
        Path dest("/obj/arr");
        Path src("/arr");
        PatchOperation op = PatchOperation::MoveOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_TRUE(Utils::DeepCompareIsEqual(m_dataset[src], result.GetValue()[dest]));
        EXPECT_FALSE(result.GetValue().HasMember("arr"));
    }

    TEST_F(DomPatchTests, MoveOperation_ObjectToArrayInRange_Succeeds)
    {
        Path dest("/arr/0");
        Path src("/obj");
        PatchOperation op = PatchOperation::MoveOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_TRUE(Utils::DeepCompareIsEqual(m_dataset[src], result.GetValue()[dest]));
        EXPECT_FALSE(result.GetValue().HasMember("obj"));
    }

    TEST_F(DomPatchTests, MoveOperation_ObjectToArrayOutOfRange_Fails)
    {
        Path dest("/arr/5");
        Path src("/obj");
        PatchOperation op = PatchOperation::MoveOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, MoveOperation_ObjectToNodeChildInRange_Succeeds)
    {
        Path dest("/node/0");
        Path src("/obj");
        PatchOperation op = PatchOperation::MoveOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_TRUE(Utils::DeepCompareIsEqual(m_dataset[src], result.GetValue()[dest]));
        EXPECT_FALSE(result.GetValue().HasMember("obj"));
    }

    TEST_F(DomPatchTests, MoveOperation_ObjectToNodeChildOutOfRange_Fails)
    {
        Path dest("/node/5");
        Path src("/obj");
        PatchOperation op = PatchOperation::MoveOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, MoveOperation_InvalidSourcePath_Fails)
    {
        Path dest("/node/0");
        Path src("/invalid/path");
        PatchOperation op = PatchOperation::MoveOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, MoveOperation_InvalidDestinationPath_Fails)
    {
        Path dest("/invalid/path");
        Path src("/arr/0");
        PatchOperation op = PatchOperation::MoveOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, TestOperation_TestCorrectValue_Succeeds)
    {
        Path path("/arr/1");
        Value value(1);
        PatchOperation op = PatchOperation::TestOperation(path, value);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, TestOperation_TestIncorrectValue_Fails)
    {
        Path path("/arr/1");
        Value value(55);
        PatchOperation op = PatchOperation::TestOperation(path, value);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, TestOperation_TestCorrectComplexValue_Succeeds)
    {
        Path path;
        Value value = m_dataset;
        PatchOperation op = PatchOperation::TestOperation(path, value);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, TestOperation_TestIncorrectComplexValue_Fails)
    {
        Path path;
        Value value = m_dataset;
        value["arr"][4] = 9;
        PatchOperation op = PatchOperation::TestOperation(path, value);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, TestOperation_TestInvalidPath_Fails)
    {
        Path path("/invalid/path");
        Value value;
        PatchOperation op = PatchOperation::TestOperation(path, value);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, TestOperation_TestInsertArrayPath_Fails)
    {
        Path path("/arr/-");
        Value value(4);
        PatchOperation op = PatchOperation::TestOperation(path, value);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, TestPatch_ReplaceArrayValue)
    {
        m_deltaDataset["arr"][0] = 5;
        GenerateAndVerifyDelta();
    }

    TEST_F(DomPatchTests, TestPatch_AppendArrayValue)
    {
        m_deltaDataset["arr"].ArrayPushBack(Value(7));
        auto result = GenerateAndVerifyDelta();

        // Ensure the generated patch uses the array append operation
        ASSERT_EQ(result.m_forwardPatches.Size(), 1);
        EXPECT_TRUE(result.m_forwardPatches[0].GetDestinationPath()[1].IsEndOfArray());
    }

    TEST_F(DomPatchTests, TestPatch_AppendArrayValues)
    {
        m_deltaDataset["arr"].ArrayPushBack(Value(7));
        m_deltaDataset["arr"].ArrayPushBack(Value(8));
        m_deltaDataset["arr"].ArrayPushBack(Value(9));
        GenerateAndVerifyDelta();
    }

    TEST_F(DomPatchTests, TestPatch_InsertArrayValue)
    {
        auto& arr = m_deltaDataset["arr"].GetMutableArray();
        arr.insert(arr.begin(), Value(42));
        GenerateAndVerifyDelta();
    }

    TEST_F(DomPatchTests, TestPatch_InsertObjectKey)
    {
        m_deltaDataset["obj"]["newKey"].CopyFromString("test");
        GenerateAndVerifyDelta();
    }

    TEST_F(DomPatchTests, TestPatch_DeleteObjectKey)
    {
        m_deltaDataset["obj"].RemoveMember("foo");
        GenerateAndVerifyDelta();
    }

    TEST_F(DomPatchTests, TestPatch_AppendNodeValues)
    {
        m_deltaDataset["node"].ArrayPushBack(Value(7));
        m_deltaDataset["node"].ArrayPushBack(Value(8));
        m_deltaDataset["node"].ArrayPushBack(Value(9));
        GenerateAndVerifyDelta();
    }

    TEST_F(DomPatchTests, TestPatch_InsertNodeValue)
    {
        auto& node = m_deltaDataset["node"].GetMutableNode();
        node.GetChildren().insert(node.GetChildren().begin(), Value(42));
        GenerateAndVerifyDelta();
    }

    TEST_F(DomPatchTests, TestPatch_InsertNodeKey)
    {
        m_deltaDataset["node"]["newKey"].CopyFromString("test");
        GenerateAndVerifyDelta();
    }

    TEST_F(DomPatchTests, TestPatch_DeleteNodeKey)
    {
        m_deltaDataset["node"].RemoveMember("int");
        GenerateAndVerifyDelta();
    }

    TEST_F(DomPatchTests, TestPatch_RenameNode)
    {
        m_deltaDataset["node"].SetNodeName("RenamedNode");
        GenerateAndVerifyDelta();
    }

    TEST_F(DomPatchTests, TestPatch_ReplaceRoot)
    {
        m_deltaDataset = Value(Type::Array);
        m_deltaDataset.ArrayPushBack(Value(2));
        m_deltaDataset.ArrayPushBack(Value(4));
        m_deltaDataset.ArrayPushBack(Value(6));
        GenerateAndVerifyDelta();
    }

    TEST_F(DomPatchTests, TestPatch_DenormalizeOnApply)
    {
        m_dataset = Value(Type::Array);
        m_dataset.ArrayPushBack(Value(2));

        m_deltaDataset = m_dataset;
        m_deltaDataset.ArrayPushBack(Value(4));

        DeltaPatchGenerationParameters params;
        params.m_generateDenormalizedPaths = false;
        PatchUndoRedoInfo info = GenerateHierarchicalDeltaPatch(m_dataset, m_deltaDataset, params);

        EXPECT_TRUE(info.m_forwardPatches.ContainsNormalizedEntries());

        auto result = info.m_forwardPatches.ApplyAndDenormalize(m_dataset);
        EXPECT_TRUE(result.IsSuccess());
        EXPECT_TRUE(Utils::DeepCompareIsEqual(result.GetValue(), m_deltaDataset));

        EXPECT_FALSE(info.m_forwardPatches.ContainsNormalizedEntries());
    }
} // namespace AZ::Dom::Tests
