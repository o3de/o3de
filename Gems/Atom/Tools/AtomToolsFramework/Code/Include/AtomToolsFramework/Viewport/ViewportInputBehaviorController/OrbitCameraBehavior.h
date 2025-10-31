/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/ViewportInputBehavior.h>

namespace AtomToolsFramework
{
    //! Rotates the camera around object position,
    //! this can either be object center or any position in world
    class OrbitCameraBehavior final : public ViewportInputBehavior
    {
    public:
        OrbitCameraBehavior(ViewportInputBehaviorControllerInterface* controller);
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
} // namespace AtomToolsFramework
