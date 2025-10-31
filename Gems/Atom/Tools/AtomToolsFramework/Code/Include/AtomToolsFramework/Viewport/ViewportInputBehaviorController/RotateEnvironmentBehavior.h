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
    //! Rotates lighting and skybox around vertical axis
    class RotateEnvironmentBehavior final : public ViewportInputBehavior
    {
    public:
        RotateEnvironmentBehavior(ViewportInputBehaviorControllerInterface* controller);
        virtual ~RotateEnvironmentBehavior() = default;

    protected:
        void TickInternal(float x, float y, float z) override;
        float GetSensitivityX() override;
        float GetSensitivityY() override;

    private:
        static constexpr float SensitivityX = 0.01f;
        static constexpr float SensitivityY = 0.0f;
        float m_rotation = {};
    };
} // namespace AtomToolsFramework
