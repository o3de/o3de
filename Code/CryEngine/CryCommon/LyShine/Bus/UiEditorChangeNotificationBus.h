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
