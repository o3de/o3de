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
    using Outcome                       = AZ::Outcome<PrefabDependencyTree, const char*>;

    TEST_F(PrefabDependencyViewerFixture, INVALID_TEMPLATE_ID)
    {
        Outcome outcome = PrefabDependencyTree::GenerateTreeAndSetRoot(InvalidTemplateId,
                                                                m_prefabSystemComponent);

        EXPECT_EQ(false, outcome.IsSuccess());
    }

    TEST_F(PrefabDependencyViewerFixture, TEST)
    {

        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(10))
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["emptyJSON"]));

        Outcome outcome = PrefabDependencyTree::GenerateTreeAndSetRoot(10, m_prefabSystemComponent);
        EXPECT_EQ(true, outcome.IsSuccess());
    }
}
