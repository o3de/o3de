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

#pragma once

#include <AzCore/Math/Uuid.h>
#include <AzCore/EBus/EBus.h>

namespace ScriptCanvasEditor
{
    class AssetGraphScene : public AZ::EBusTraits
    {
    public:
        virtual AZ::EntityId FindEditorNodeIdByAssetNodeId(const AZ::Data::AssetId& assetId, AZ::EntityId assetNodeId) const = 0;
        virtual AZ::EntityId FindAssetNodeIdByEditorNodeId(const AZ::Data::AssetId& assetId, AZ::EntityId editorNodeId) const = 0;
    };

    using AssetGraphSceneBus = AZ::EBus<AssetGraphScene>;

}