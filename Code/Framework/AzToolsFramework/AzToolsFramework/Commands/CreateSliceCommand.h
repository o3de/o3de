/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/Slice/SliceComponent.h>

#include <AzToolsFramework/Commands/BaseSliceCommand.h>
namespace AzToolsFramework
{
    class CreateSliceCommand
        : public BaseSliceCommand
    {
    public:
        AZ_CLASS_ALLOCATOR(CreateSliceCommand, AZ::SystemAllocator, 0);
        AZ_RTTI(CreateSliceCommand, "{295F0D67-18F4-4C4E-937B-66398258A472}");

        CreateSliceCommand(const AZStd::string& friendlyName);

        ~CreateSliceCommand() {}

        void Capture(const AZ::Data::Asset<AZ::SliceAsset>& tempSliceAsset,
            const AZStd::string& fullSliceAssetPath,
            const AZ::SliceComponent::EntityIdToEntityIdMap& liveToAssetMap);

        void Undo() override;
        void Redo() override;

    protected:
        AZ::Data::Asset<AZ::SliceAsset> PreloadSliceAsset();

        AZStd::vector<AZ::u8> m_sliceAssetBuffer;
        AZStd::string m_fullSliceAssetPath;
        AZ::SliceComponent::EntityIdToEntityIdMap m_liveToAssetMap;
        AZ::Data::AssetId m_sliceAssetId;
    };
}
