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

#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>
#include <AzCore/Component/ComponentBus.h>
#include <QString>
#include <QPointer>
#include <QWidget>

namespace AzToolsFramework
{
    namespace ComponentModeFramework
    {
        //! Bus for interacting with the widget that handles Viewport UI for component mode.
        class ComponentModeViewportUiRequests
            : public AZ::EBusTraits
        {
        public:
            using BusIdType = AZ::Uuid;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

            //! Set the given clusterIds to visible in the Viewport UI.
            virtual void RegisterViewportElementGroup(
                const AZ::EntityComponentIdPair& entityComponentIdPair,
                const AZStd::vector<ViewportUi::ClusterId>& clusterIds) = 0;

            //! Set this handler to be the active (on display) handler that will display in the Viewport UI.
            virtual void SetComponentModeViewportUiActive(bool) = 0;

            //! Set this handler to display for this specific EntityComponentIdPair only.
            virtual void SetViewportUiActiveEntityComponentId(const AZ::EntityComponentIdPair& entityComponentId) = 0;
        };

        //! Type to inherit to implement ComponentModeSystemRequests.
        using ComponentModeViewportUiRequestBus = AZ::EBus<ComponentModeViewportUiRequests>;
    } // namespace ComponentModeFramework
} // namespace AzToolsFramework
