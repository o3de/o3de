/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiPrefabInstance.h"

#include <AzCore/Serialization/EditContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiPrefabInstance::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
    if (serializeContext)
    {
        serializeContext->Class<UiPrefabInstance>()
            ->Version(1)
            ->Field("SourcePath", &UiPrefabInstance::m_sourcePath)
            ->Field("InstanceId", &UiPrefabInstance::m_instanceId)
            ->Field("EntityIdMap", &UiPrefabInstance::m_entityIdMap)
            ->Field("PatchesJson", &UiPrefabInstance::m_patchesJson);
    }
}
