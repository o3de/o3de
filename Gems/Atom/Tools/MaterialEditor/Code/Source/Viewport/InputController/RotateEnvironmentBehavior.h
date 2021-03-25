/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
