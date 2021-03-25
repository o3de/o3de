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
        firstInstance = AZStd::move(m_prefabSystemComponent->CreatePrefab({ }, {}, "test/path0"));
        ASSERT_TRUE(firstInstance);

        //get template id
        ownerId = firstInstance->GetTemplateId();

        //create prefab 2
        secondInstance = AZStd::move(m_prefabSystemComponent->CreatePrefab({ }, {}, "test/path1"));
        ASSERT_TRUE(secondInstance);

        //get template id
        referenceId = secondInstance->GetTemplateId();
    }
}
