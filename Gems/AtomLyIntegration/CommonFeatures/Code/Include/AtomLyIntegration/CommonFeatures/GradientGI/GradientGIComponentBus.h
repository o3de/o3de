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
#include <AtomLyIntegration/CommonFeatures/GradientGI/GradientGIComponentConstants.h>

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
            // Single-layer color setters/getters
            virtual void SetLowColor(const Color& color) = 0;
            virtual Color GetLowColor() const = 0;

            virtual void SetMidColor(const Color& color) = 0;
            virtual Color GetMidColor() const = 0;

            virtual void SetHighColor(const Color& color) = 0;
            virtual Color GetHighColor() const = 0;

            // Set all three gradient layers at once
            virtual void SetGradientColors(const Color& low, const Color& mid, const Color& high) = 0;

            virtual void SetExposure(float exposure) = 0;
            virtual float GetExposure() const = 0;

            // Script Canvas / Lua represent numbers as a signed type, so the scripted
            // face-resolution API uses int to avoid an unsigned conversion node. Values
            // are clamped to the valid [4..256] range by the controller.
            virtual void SetFaceResolution(int resolution) = 0;
            virtual int GetFaceResolution() const = 0;

            virtual void SetUpdateMode(GradientGIUpdateMode mode) = 0;
            virtual GradientGIUpdateMode GetUpdateMode() const = 0;
        };

        using GradientGIComponentRequestBus = EBus<GradientGIComponentRequests>;

    } // namespace Render
} // namespace AZ
