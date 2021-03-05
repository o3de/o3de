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
