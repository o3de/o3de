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
    class DetachSubsliceInstanceCommand
        : public BaseSliceCommand
    {
    public:
        AZ_CLASS_ALLOCATOR(DetachSubsliceInstanceCommand, AZ::SystemAllocator, 0);
        AZ_RTTI(DetachSubsliceInstanceCommand, "{FCDE52C0-F334-4701-9CA6-43FA089007EE}");

        DetachSubsliceInstanceCommand(const AZ::SliceComponent::SliceInstanceEntityIdRemapList& subsliceRootList, const AZStd::string& friendlyName);

        ~DetachSubsliceInstanceCommand() {}

        void Undo() override;
        void Redo() override;

    protected:
        AZ::SliceComponent::SliceInstanceEntityIdRemapList m_subslices;
    };
} // namespace AzToolsFramework
