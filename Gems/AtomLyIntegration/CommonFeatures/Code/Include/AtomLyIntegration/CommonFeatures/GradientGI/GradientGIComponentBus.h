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
        // =====================================================================
        // Request Bus
        // =====================================================================

        class GradientGIComponentRequests
            : public ComponentBus
        {
        public:
            virtual void SetLowColor(const Color& color) = 0;
            virtual Color GetLowColor() const = 0;

            virtual void SetMidColor(const Color& color) = 0;
            virtual Color GetMidColor() const = 0;

            virtual void SetHighColor(const Color& color) = 0;
            virtual Color GetHighColor() const = 0;

            virtual void SetExposure(float exposure) = 0;
            virtual float GetExposure() const = 0;

            virtual void SetFaceResolution(uint32_t resolution) = 0;
            virtual uint32_t GetFaceResolution() const = 0;
        };
        using GradientGIComponentRequestBus = EBus<GradientGIComponentRequests>;

    } // namespace Render
} // namespace AZ
