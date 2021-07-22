/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <Prefab/PrefabTestFixture.h>

namespace UnitTest
{
    class PrefabTestUndoFixture
        : public PrefabTestFixture
    {
    protected:
        void SetupInstances(
            AZStd::unique_ptr<Instance>& firstInstance,
            AZStd::unique_ptr<Instance>& secondInstance,
            TemplateId& ownerId,
            TemplateId& referenceId);
    };
}
