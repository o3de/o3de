/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiEditorChangeNotificationInterface
    : public AZ::EBusTraits
{
public: // member functions

    virtual ~UiEditorChangeNotificationInterface() {}

    //! Called when the transform properties in the UI editor need to be refreshed
    virtual void OnEditorTransformPropertiesNeedRefresh() = 0;

    //! Forces a refresh of the entire properties tree in the UI Editor.
    virtual void OnEditorPropertiesRefreshEntireTree() = 0;
};

typedef AZ::EBus<UiEditorChangeNotificationInterface> UiEditorChangeNotificationBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Notify components who store directories as properties when directory contents change.
class UiEditorRefreshDirectoryNotificationInterface
    : public AZ::EBusTraits
{
public: // member functions

    virtual ~UiEditorRefreshDirectoryNotificationInterface() {}

    //! Notify directory properties that they should refresh their contents
    virtual void OnRefreshDirectory() = 0;
};

typedef AZ::EBus<UiEditorRefreshDirectoryNotificationInterface> UiEditorRefreshDirectoryNotificationBus;
