/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Slice/SliceComponent.h>

#include <AzToolsFramework/Commands/BaseSliceCommand.h>
namespace AzToolsFramework
{
    class PushToSliceCommand
        : public BaseSliceCommand
    {
    public:
        AZ_CLASS_ALLOCATOR(PushToSliceCommand, AZ::SystemAllocator, 0);
        AZ_RTTI(PushToSliceCommand, "{3FDCD751-EB10-40C0-857C-F5038E592E1E}");

        PushToSliceCommand(const AZStd::string& friendlyName);

        ~PushToSliceCommand() {}

        void Capture(const AZ::Data::Asset<AZ::SliceAsset>& sliceAsset, const AZStd::unordered_map<AZ::EntityId, AZ::EntityId>& addedEntityIdRemaps);

        void Undo() override;
        void Redo() override;
    };
}
