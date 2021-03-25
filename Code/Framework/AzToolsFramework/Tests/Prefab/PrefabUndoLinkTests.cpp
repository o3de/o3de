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

#include <Prefab/PrefabTestDomUtils.h>
#include <Prefab/PrefabTestUndoFixture.h>

#include <Prefab/PrefabUndo.h>

namespace UnitTest
{
    using PrefabUndoLinkTests = PrefabTestUndoFixture;

    TEST_F(PrefabUndoLinkTests, PrefabUndoLinkAdd)
    {
        AZStd::unique_ptr<Instance> firstInstance = nullptr;
        AZStd::unique_ptr<Instance> secondInstance = nullptr;
        TemplateId firstTemplateId = InvalidTemplateId;
        TemplateId secondTemplateId = InvalidTemplateId;

        SetupInstances(firstInstance, secondInstance, firstTemplateId, secondTemplateId);

        firstInstance->AddInstance(AZStd::move(secondInstance));
        AZStd::vector<InstanceAlias> aliases = firstInstance->GetNestedInstanceAliases(secondTemplateId);

        //parent prefab2 to prefab 1 by creating a link
        //capture the link addition in undo node
        PrefabUndoInstanceLink undoLink("Undo Link Add Node");
        undoLink.Capture(firstTemplateId, secondTemplateId, aliases[0]);
        undoLink.Redo();

        AZStd::unique_ptr<Instance> newInstance = m_prefabSystemComponent->InstantiatePrefab(firstTemplateId);
        AZStd::vector<InstanceAlias> instances = newInstance->GetNestedInstanceAliases(secondTemplateId);
        
        ASSERT_TRUE(instances.size() > 0);
        instances.clear();

        //undo the parenting
        undoLink.Undo();

        newInstance = m_prefabSystemComponent->InstantiatePrefab(firstTemplateId);
        instances = newInstance->GetNestedInstanceAliases(secondTemplateId);

        ASSERT_TRUE(instances.size() == 0);

        //undo the parenting
        undoLink.Redo();

        newInstance = m_prefabSystemComponent->InstantiatePrefab(firstTemplateId);
        instances = newInstance->GetNestedInstanceAliases(secondTemplateId);

        ASSERT_TRUE(instances.size() == 1);
    }
} // namespace UnitTest
