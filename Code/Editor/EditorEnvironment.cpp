/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "EditorEnvironment.h"

void SetEditorEnvironment(SSystemGlobalEnvironment* pEnv)
{
    assert(!gEnv || gEnv != pEnv);
    gEnv = pEnv;
}

void AttachEditorAZEnvironment(AZ::EnvironmentInstance azEnv)
{
    AZ::Environment::Attach(azEnv, true);
}

void DetachEditorAZEnvironment()
{
    AZ::Environment::Detach();
}
