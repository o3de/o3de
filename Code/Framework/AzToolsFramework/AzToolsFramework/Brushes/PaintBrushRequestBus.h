/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBus.h>

#include <AzToolsFramework/Viewport/ViewportTypes.h>

namespace AzToolsFramework
{
    class PaintBrushRequests : public AZ::EBusTraits
    {
    public:
        // EBusTraits
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityComponentIdPair;
        using MutexType = AZStd::recursive_mutex;

        // PaintBrushRequests interface...
        virtual float GetRadius() const = 0;
        virtual float GetIntensity() const = 0;
        virtual float GetOpacity() const = 0;

        virtual void SetRadius(float radius) = 0;
        virtual void SetIntensity(float intensity) = 0;
        virtual void SetOpacity(float opacity) = 0;

        virtual void GetValue(const AZ::Vector3& point, float& intensity, float& opacity, bool& isValid) = 0;
        virtual void GetValues(
            AZStd::span<const AZ::Vector3> points,
            AZStd::span<float> intensities,
            AZStd::span<float> opacities,
            AZStd::span<bool> validFlags) = 0;
        virtual bool HandleMouseInteraction(const ViewportInteraction::MouseInteractionEvent& mouseInteraction) = 0;

    protected:
        ~PaintBrushRequests() = default;
    };

    using PaintBrushRequestBus = AZ::EBus<PaintBrushRequests>;
} // namespace AzToolsFramework