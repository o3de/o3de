/* Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <PrefabDependencyViewerEditorSystemComponent.h>
#include <PrefabDependencyTreeGenerator.h>

#include <Utils.h>
#include <PrefabDependencyViewerFixture.h>

namespace PrefabDependencyViewer
{
    using DirectedGraph                 = Utils::DirectedGraph;
    using TestComponent                 = PrefabDependencyViewerEditorSystemComponent;

    TEST_F(PrefabDependencyViewerFixture, INVALID_TEMPLATE_ID)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        DirectedGraph graph;
        PrefabDependencyTreeGenerator::GenerateTreeAndSetRoot(InvalidTemplateId, graph, m_prefabSystemComponent);
        int numExpectedAssertions = 1;
        AZ_TEST_STOP_TRACE_SUPPRESSION(numExpectedAssertions);
    }

    TEST_F(PrefabDependencyViewerFixture, TEST)
    {

        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(10))
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["emptyJSON"]));

        DirectedGraph graph;
        PrefabDependencyTreeGenerator::GenerateTreeAndSetRoot(10, graph, m_prefabSystemComponent);
    }
}
