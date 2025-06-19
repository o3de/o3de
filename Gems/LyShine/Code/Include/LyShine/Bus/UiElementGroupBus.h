/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <LyShine/UiBase.h>

    class UiElementGroupInterface
        : public AZ::ComponentBus
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //Interactive
        virtual bool SetInteractivity(bool enabled) = 0;
        virtual bool SetParentInteractivity(bool parentEnabled) = 0;
        virtual bool GetInteractiveState() = 0;

        //Rendering
        virtual bool SetRendering(bool enabled) = 0;
        virtual bool GetRenderingState() = 0;
    };   

    typedef AZ::EBus<UiElementGroupInterface> UiElementGroupBus;