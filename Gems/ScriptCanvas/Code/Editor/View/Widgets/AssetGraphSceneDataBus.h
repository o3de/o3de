/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
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
