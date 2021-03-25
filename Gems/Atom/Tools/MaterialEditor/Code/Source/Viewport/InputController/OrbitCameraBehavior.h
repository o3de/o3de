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
