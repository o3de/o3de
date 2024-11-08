/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/Bus/UiDropTargetBus.h>
#include <LyShine/UiComponentTypes.h>

#include <AzCore/Component/Component.h>

#include "UiInteractableState.h"
#include "UiStateActionManager.h"
#include "UiNavigationSettings.h"

namespace AZ
{
    class ReflectContext;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiDropTargetComponent
    : public AZ::Component
    , public UiDropTargetBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiDropTargetComponent, LyShine::UiDropTargetComponentUuid, AZ::Component);

    UiDropTargetComponent();
    ~UiDropTargetComponent() override;

    // UiDropTargetInterface
    const LyShine::ActionName& GetOnDropActionName() override;
    void SetOnDropActionName(const LyShine::ActionName& actionName) override;
    void HandleDropHoverStart(AZ::EntityId draggable) override;
    void HandleDropHoverEnd(AZ::EntityId draggable) override;
    void HandleDrop(AZ::EntityId draggable) override;
    DropState GetDropState() override;
    void SetDropState(DropState dropState) override;
    // ~UiDropTargetInterface

protected: // member functions

    // AZ::Component
    void Init() override;
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    void OnDropValidStateActionsChanged();
    void OnDropInvalidStateActionsChanged();

protected: // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("UiDropTargetService"));
        provided.push_back(AZ_CRC_CE("UiNavigationService"));
        provided.push_back(AZ_CRC_CE("UiStateActionsService"));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("UiDropTargetService"));
        incompatible.push_back(AZ_CRC_CE("UiNavigationService"));
        incompatible.push_back(AZ_CRC_CE("UiStateActionsService"));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("UiElementService"));
        required.push_back(AZ_CRC_CE("UiTransformService"));
    }

    static void Reflect(AZ::ReflectContext* context);

private: // member functions

    AZ_DISABLE_COPY_MOVE(UiDropTargetComponent);

private: // static member functions

    //! Get the drop targets that could be valid options for custom navigation from this drop target
    static LyShine::EntityArray GetNavigableDropTargets(AZ::EntityId sourceEntity);

private: // persistent data

    using StateActions = AZStd::vector<UiInteractableStateAction*>;

    //! Dragging state action properties - allow visual states to be defined
    StateActions m_dropValidStateActions;
    StateActions m_dropInvalidStateActions;

private: // data

    LyShine::ActionName m_onDropActionName;

    DropState m_dropState;

    UiStateActionManager m_stateActionManager;
    UiNavigationSettings m_navigationSettings;
};
