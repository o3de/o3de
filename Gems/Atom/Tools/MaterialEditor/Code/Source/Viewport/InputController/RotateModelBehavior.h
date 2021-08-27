/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Source/Viewport/InputController/Behavior.h>

namespace MaterialEditor
{
    //! Rotates target model in viewport
    class RotateModelBehavior final
        : public Behavior
    {
    public:
        RotateModelBehavior() = default;
        virtual ~RotateModelBehavior() = default;

        void Start() override;

    protected:
        void TickInternal(float x, float y, float z) override;
        float GetSensitivityX() override;
        float GetSensitivityY() override;

    private:
        static constexpr float SensitivityX = 0.01f;
        static constexpr float SensitivityY = 0.01f;

        AZ::EntityId m_targetEntityId;
        AZ::Vector3 m_cameraRight = AZ::Vector3::CreateAxisX();
    };
} // namespace MaterialEditor
