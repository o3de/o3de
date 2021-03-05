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