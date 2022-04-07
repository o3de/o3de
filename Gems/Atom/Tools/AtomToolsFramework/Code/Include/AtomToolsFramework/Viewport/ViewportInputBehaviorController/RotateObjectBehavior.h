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
    //! Rotates object in viewport
    class RotateObjectBehavior final : public ViewportInputBehavior
    {
    public:
        RotateObjectBehavior(ViewportInputBehaviorControllerInterface* controller);
        virtual ~RotateObjectBehavior() = default;

        void Start() override;

    protected:
        void TickInternal(float x, float y, float z) override;
        float GetSensitivityX() override;
        float GetSensitivityY() override;

    private:
        static constexpr float SensitivityX = 0.01f;
        static constexpr float SensitivityY = 0.01f;

        AZ::EntityId m_objectEntityId;
        AZ::Vector3 m_cameraRight = AZ::Vector3::CreateAxisX();
        AZ::Vector3 m_cameraUp = AZ::Vector3::CreateAxisZ();
    };
} // namespace AtomToolsFramework
