/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Source/Viewport/InputController/Behavior.h>

namespace AZ
{
    namespace Render
    {
        class SkyBoxFeatureProcessorInterface;
    }
}

namespace MaterialEditor
{
    //! Rotates lighting and skybox around vertical axis
    class RotateEnvironmentBehavior final
        : public Behavior
    {
    public:
        RotateEnvironmentBehavior() = default;
        virtual ~RotateEnvironmentBehavior() = default;

        void Start() override;

    protected:
        void TickInternal(float x, float y, float z) override;
        float GetSensitivityX() override;
        float GetSensitivityY() override;

    private:
        static constexpr float SensitivityX = 0.01f;
        static constexpr float SensitivityY = 0;

        AZ::EntityId m_iblEntityId;
        AZ::Render::SkyBoxFeatureProcessorInterface* m_skyBoxFeatureProcessorInterface = nullptr;
        float m_rotation = 0;
    };
} // namespace MaterialEditor
