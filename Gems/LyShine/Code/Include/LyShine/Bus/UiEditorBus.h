/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiEditorInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiEditorInterface() {}

    //! Test if this entity should be visible in the Ui Canvas Editor
    virtual bool GetIsVisible() = 0;

    //! Set whether this entity should be visible in the Ui Canvas Editor
    virtual void SetIsVisible(bool isVisible) = 0;

    //! Test if this entity is selectable in the UI Canvas Editor
    virtual bool GetIsSelectable() = 0;

    //! Set whether this entity is selectable in the UI Canvas Editor
    virtual void SetIsSelectable(bool isSelectable) = 0;

    //! Test if this entity is currently selected in the UI Canvas Editor
    virtual bool GetIsSelected() = 0;

    //! Set whether this entity is currently selected in the UI Canvas Editor
    virtual void SetIsSelected(bool isSelected) = 0;

    //! Test if this entity is currently expanded in the UI Canvas Editor
    virtual bool GetIsExpanded() = 0;

    //! Set whether this entity is currently expanded in the UI Canvas Editor
    virtual void SetIsExpanded(bool isExpanded) = 0;

    //! Test if all the parents of this UI element are visible in the editor
    virtual bool AreAllAncestorsVisible() = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiEditorInterface> UiEditorBus;
