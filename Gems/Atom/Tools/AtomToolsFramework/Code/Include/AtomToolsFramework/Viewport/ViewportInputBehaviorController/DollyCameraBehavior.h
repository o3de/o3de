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
    //! Moves(zooms) camera back and forth towards the object
    class DollyCameraBehavior final : public ViewportInputBehavior
    {
    public:
        DollyCameraBehavior(ViewportInputBehaviorControllerInterface* controller);
        virtual ~DollyCameraBehavior() = default;

    protected:
        void TickInternal(float x, float y, float z) override;
        float GetSensitivityX() override;
        float GetSensitivityY() override;

    private:
        static constexpr float SensitivityX = 0;
        static constexpr float SensitivityY = 0.005f;
    };
} // namespace AtomToolsFramework
