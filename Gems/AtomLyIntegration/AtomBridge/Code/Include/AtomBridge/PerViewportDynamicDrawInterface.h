/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawInterface.h>
#include <AzFramework/Viewport/ViewportId.h>

namespace AZ::AtomBridge
{
    //! A simple interface for allocating a DynamicDrawContext on-demand for every viewport, based on
    //! a registered initialization function.
    class PerViewportDynamicDrawInterface
    {
    public:
        AZ_RTTI(PerViewportDynamicDrawInterface, "{1FF054F5-55FF-4ADB-A86D-640B15FA0395}");

        using DrawContextFactory = AZStd::function<void(RHI::Ptr<RPI::DynamicDrawContext>)>;
        //! Register a named dynamic draw context that can be retrieved on a per-viewport basis.
        //! GetNamedDynamicDraw context can be called on a registered context name to retrieve a
        //! valid DynamicDrawContext for a given viewport.
        virtual void RegisterDynamicDrawContext(AZ::Name name, DrawContextFactory contextInitializer) = 0;

        //! Unregister a previously registered named per-viewport dynamic draw context.
        //! This will dispose of all dynamic draw contexts currently associated with this name.
        virtual void UnregisterDynamicDrawContext(AZ::Name name) = 0;

        //! Get a dynamic draw context associated with the specified viewport based on a factory registered with
        //! RegisterNamedDynamicDrawContext. This dynamic draw context will be created if it does not already exist.
        virtual RHI::Ptr<RPI::DynamicDrawContext> GetDynamicDrawContextForViewport(AZ::Name name, AzFramework::ViewportId viewportId) = 0;
    };

    using PerViewportDynamicDraw = AZ::Interface<PerViewportDynamicDrawInterface>;
} // namespace AZ::AtomBridge
