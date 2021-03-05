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

#include <AzCore/EBus/EBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that the UI Editor needs to implement
class UiEditorInernalRequests
    : public AZ::EBusTraits
{
public: // member functions

    //! Get the selected entityIds in the UiEditor
    virtual AzToolsFramework::EntityIdList GetSelectedEntityIds() = 0;

    //! Get the selected components in the UiEditor
    virtual AZ::Entity::ComponentArrayType GetSelectedComponents() = 0;

    //! Get the entityId of the active Canvas in the UiEditor
    virtual AZ::EntityId GetActiveCanvasEntityId() = 0;
};

typedef AZ::EBus<UiEditorInernalRequests> UiEditorInternalRequestBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that the UI Editor needs to implement
class UiEditorInternalNotifications
    : public AZ::EBusTraits
{
public:

    //! Called when a property on the selected entities has changed
    virtual void OnSelectedEntitiesPropertyChanged() = 0;

    //! Called before making an undoable change to entities
    virtual void OnBeginUndoableEntitiesChange() = 0;

    //! Called after making an undoable change to entities
    virtual void OnEndUndoableEntitiesChange(const AZStd::string& commandText) = 0;
};

typedef AZ::EBus<UiEditorInternalNotifications> UiEditorInternalNotificationBus;
