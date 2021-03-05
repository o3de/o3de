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
