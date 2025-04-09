/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

#include <ScriptCanvas/Data/Data.h>

class QMainWindow;

namespace ScriptCanvasEditor
{
    class SystemRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        // Inserts into the supplied set types that are creatable within the editor
        virtual void GetEditorCreatableTypes(AZStd::unordered_set<ScriptCanvas::Data::Type>& outCreatableTypes) = 0;

        // Creates all editor components needed to associate the script canvas engine with an entity
        virtual void CreateEditorComponentsOnEntity(AZ::Entity* entity, const AZ::Data::AssetType& assetType) = 0;

        virtual void RequestGarbageCollect() = 0;
    };

    using SystemRequestBus = AZ::EBus<SystemRequests>;

    class UIRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual QMainWindow* GetMainWindow() = 0;

        virtual void OpenValidationPanel() = 0;

        virtual void RefreshSelection() = 0;
    };

    using UIRequestBus = AZ::EBus<UIRequests>;

    class UINotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual void MainWindowCreationEvent(QMainWindow* /*mainWindow*/) {};
    };

    using UINotificationBus = AZ::EBus<UINotifications>;
}
