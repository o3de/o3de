/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/chrono/chrono.h>

#include <GraphCanvas/Editor/EditorTypes.h>

namespace GraphCanvas
{
    class EditorDockWidget;

    // Busses made specifically for the Editor Dock Widgets

    // This bus is keyed off of the individual dock widgets Id's
    // and is used for more specific interactions with a given entity.
    //
    // For more general interactions, there is a second bus which will be
    // managed by the individual dock widgets in order to provide the most features
    // possible.
    class EditorDockWidgetRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = DockWidgetId;

        virtual AZ::EntityId GetViewId() const = 0;
        virtual GraphId GetGraphId() const = 0;
        virtual EditorDockWidget* AsEditorDockWidget() = 0;
        virtual void SetTitle(const AZStd::string& title) = 0;
    };

    using EditorDockWidgetRequestBus = AZ::EBus<EditorDockWidgetRequests>;

    // Simple way for determining which DockWidgetId has focus for a given editor.
    class ActiveEditorDockWidgetRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using BusIdType = EditorId;

        virtual void ReleaseBus() = 0;
        virtual DockWidgetId GetDockWidgetId() const = 0;
    };

    using ActiveEditorDockWidgetRequestBus = AZ::EBus<ActiveEditorDockWidgetRequests>;
}
