/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
