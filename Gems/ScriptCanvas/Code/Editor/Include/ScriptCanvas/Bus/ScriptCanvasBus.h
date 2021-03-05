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
#include <AzCore/std/string/string.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <ScriptCanvas/Data/Data.h>

class QWidget;

namespace ScriptCanvasEditor
{
    class SystemRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual void AddAsyncJob(AZStd::function<void()>&& jobFunc) = 0;

        // Inserts into the supplied set types that are creatable within the editor
        virtual void GetEditorCreatableTypes(AZStd::unordered_set<ScriptCanvas::Data::Type>& outCreatableTypes) = 0;

        // Creates all editor components needed to associate the script canvas engine with an entity
        virtual void CreateEditorComponentsOnEntity(AZ::Entity* entity, const AZ::Data::AssetType& assetType) = 0;
    };

    using SystemRequestBus = AZ::EBus<SystemRequests>;

    class UIRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual QWidget* GetMainWindow() = 0;

        virtual void OpenValidationPanel() = 0;
    };

    using UIRequestBus = AZ::EBus<UIRequests>;

    class UINotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual void MainWindowCreationEvent(QWidget* /*mainWindow*/) {};
    };

    using UINotificationBus = AZ::EBus<UINotifications>;
}
