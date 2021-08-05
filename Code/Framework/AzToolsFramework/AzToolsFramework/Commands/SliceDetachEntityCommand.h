/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Component/ComponentBus.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Commands/BaseSliceCommand.h>

namespace AzToolsFramework
{
    /**
    * Stores information about Entities that are being detached from their slice
    */
    class SliceDetachEntityCommand
        : public BaseSliceCommand
    {
    public:
        AZ_CLASS_ALLOCATOR(SliceDetachEntityCommand, AZ::SystemAllocator, 0);
        AZ_RTTI(SliceDetachEntityCommand, "{FD26D09E-FD95-4D8C-8575-E01DA7100240}");

        SliceDetachEntityCommand(const AzToolsFramework::EntityIdList& entityIds, const AZStd::string& friendlyName);

        ~SliceDetachEntityCommand() {}

        void Undo() override;
        void Redo() override;

    protected:
        AzToolsFramework::EntityIdList m_entityIds; // Used for redo when no information is known other than a selected entity list
    };
} // namespace AzToolsFramework
