/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
