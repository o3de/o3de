/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Source/Viewport/InputController/Behavior.h>

namespace MaterialEditor
{
    //! Rotates camera around its own axis, allowing to look up/down/left/right
    class PanCameraBehavior final
        : public Behavior
    {
    public:
        PanCameraBehavior() = default;
        virtual ~PanCameraBehavior() = default;
        
        void End() override;

    protected:
        void TickInternal(float x, float y, float z) override;
        float GetSensitivityX() override;
        float GetSensitivityY() override;

    private:
        static constexpr float SensitivityX = 0.005f;
        static constexpr float SensitivityY = 0.005f;
    };
} // namespace MaterialEditor
