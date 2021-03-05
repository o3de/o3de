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
