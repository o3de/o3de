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
    //! Rotates the camera around target position,
    //! this can either be model center or any position in world
    class OrbitCameraBehavior final
        : public Behavior
    {
    public:
        OrbitCameraBehavior() = default;
        virtual ~OrbitCameraBehavior() = default;

    protected:
        void TickInternal(float x, float y, float z) override;
        float GetSensitivityX() override;
        float GetSensitivityY() override;

    private:
        void Align();

        static constexpr float SensitivityX = 0.005f;
        static constexpr float SensitivityY = 0.005f;
        
        bool m_aligned = false;
    };
} // namespace MaterialEditor
