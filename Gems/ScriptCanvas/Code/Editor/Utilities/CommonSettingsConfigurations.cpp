/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include "CommonSettingsConfigurations.h"

#include <AzCore/IO/FileIO.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

namespace ScriptCanvasEditor
{
    AZStd::string GetEditingGameDataFolder()
    {
        const char* resultValue = nullptr;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(resultValue, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetAbsoluteDevGameFolderPath);
        if (!resultValue)
        {
            if (AZ::IO::FileIOBase::GetInstance())
            {
                resultValue = AZ::IO::FileIOBase::GetInstance()->GetAlias("@devassets@");
            }
        }

        if (!resultValue)
        {
            resultValue = ".";
        }

        return resultValue;
    }
}
