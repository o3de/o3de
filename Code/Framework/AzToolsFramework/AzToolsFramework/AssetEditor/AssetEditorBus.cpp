/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetEditorBus.h"
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework::AssetEditor
{
    void AssetEditorWindowSettings::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AssetEditorWindowSettings>()
                ->Field("m_openAssets", &AssetEditorWindowSettings::m_openAssets)
                ;
        }
    }
} // namespace AzToolsFramework::AssetEditor
