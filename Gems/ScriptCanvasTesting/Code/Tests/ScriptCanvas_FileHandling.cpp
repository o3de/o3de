/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Framework/ScriptCanvasTestFixture.h>
#include <Source/Framework/ScriptCanvasTestNodes.h>
#include <Source/Framework/ScriptCanvasTestUtilities.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Core/GraphSerialization.h>

namespace ScriptCanvasTests
{
    struct EntityIdComparer
    {
        bool operator()(AZ::EntityId lhs, AZ::EntityId rhs)
        {
            return lhs < rhs;
        }
    };

    void PopulateEntityIdsFromFile(AZStd::set<AZ::EntityId, EntityIdComparer>& sortedEntityIds, const char* fileName, bool makeEntityIdsUnique)
    {
        using namespace ScriptCanvas;
        AZ::IO::FixedMaxPath filePath = ScriptCanvasTests::GetUnitTestDirPathRelative();
        filePath /= fileName;
        auto result = ScriptCanvasEditor::LoadFromFile
        (filePath.c_str()
            , makeEntityIdsUnique
            ? MakeInternalGraphEntitiesUnique::Yes
            : MakeInternalGraphEntitiesUnique::No);

        EXPECT_TRUE(result.m_isSuccess);
        if (result.m_isSuccess)
        {
            const ScriptCanvas::GraphData* graphData = result.m_handle.Get()->GetGraphDataConst();
            for (auto& entityNode : graphData->m_nodes)
            {
                sortedEntityIds.insert(entityNode->GetId());
            }
        }
    }

    TEST_F(ScriptCanvasTestFixture, LoadFromString_MultipleTimes_NotMakeEntityIdsUnique_EntityIdsMatchSourceString)
    {
        // This uses a different file extension, so it won't get processed by asset processor. This file is only intended to be used for these tests.
        AZStd::set<AZ::EntityId, EntityIdComparer> sortedEntityIds;
        PopulateEntityIdsFromFile(sortedEntityIds, "SC_UnitTest_MultipleCanvasEntities.sctestfile", /*makeEntityIdsUnique*/ false);

        EXPECT_THAT(
            sortedEntityIds,
            ::testing::ElementsAre(
                AZ::EntityId(599577287851), AZ::EntityId(1501520420011), AZ::EntityId(2231664860331), AZ::EntityId(2747060935851)));
    }

    TEST_F(ScriptCanvasTestFixture, LoadFromString_MultipleTimes_makeEntityIdsUnique_EntityIdsAreUnique)
    {
        AZStd::set<AZ::EntityId, EntityIdComparer> sortedEntityIdsFirst;
        PopulateEntityIdsFromFile(sortedEntityIdsFirst, "SC_UnitTest_MultipleCanvasEntities.sctestfile", /*makeEntityIdsUnique*/ true);

        AZStd::set<AZ::EntityId, EntityIdComparer> sortedEntityIdsSecond;
        PopulateEntityIdsFromFile(sortedEntityIdsSecond, "SC_UnitTest_MultipleCanvasEntities.sctestfile", /*makeEntityIdsUnique*/ true);

        EXPECT_THAT(sortedEntityIdsFirst, ::testing::Not(::testing::ElementsAreArray(sortedEntityIdsSecond)));
    }

} // namespace UnitTest
