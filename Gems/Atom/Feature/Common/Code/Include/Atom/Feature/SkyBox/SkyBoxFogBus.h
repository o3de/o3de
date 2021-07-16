/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Color.h>

namespace AZ
{
    namespace Render
    {
        // EBus to get and set fog settings rendered with the sky
        class SkyBoxFogRequests
            : public ComponentBus
        {
        public:
            AZ_RTTI(AZ::Render::SkyBoxFogRequests, "{4D477566-54B1-49EC-B8FE-4264EA228482}");

            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
            virtual ~SkyBoxFogRequests() {}

            virtual void SetEnabled(bool enable) = 0;
            virtual bool IsEnabled() const = 0;
            virtual void SetColor(const AZ::Color& color) = 0;
            virtual const AZ::Color& GetColor() const = 0;
            // Set and Get the height upwards from the horizon
            virtual void SetTopHeight(float topHeight) = 0;
            virtual float GetTopHeight() const = 0;
            // Set and Get the height downwards from the horizon
            virtual void SetBottomHeight(float bottomHeight) = 0;
            virtual float GetBottomHeight() const = 0;
        };

        typedef AZ::EBus<SkyBoxFogRequests> SkyBoxFogRequestBus;
    }
}
