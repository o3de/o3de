/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/PrefabTestUndoFixture.h>

namespace UnitTest
{
    void PrefabTestUndoFixture::SetupInstances(
        AZStd::unique_ptr<Instance>& firstInstance,
        AZStd::unique_ptr<Instance>& secondInstance,
        TemplateId& ownerId,
        TemplateId& referenceId)
    {
        //create two prefabs for test
        //create prefab 1
        firstInstance = AZStd::move(m_prefabSystemComponent->CreatePrefab({}, {}, "test/path0"));
        ASSERT_TRUE(firstInstance);

        //get template id
        ownerId = firstInstance->GetTemplateId();

        //create prefab 2
        secondInstance = AZStd::move(m_prefabSystemComponent->CreatePrefab({}, {}, "test/path1"));
        ASSERT_TRUE(secondInstance);

        //get template id
        referenceId = secondInstance->GetTemplateId();
    }
}
