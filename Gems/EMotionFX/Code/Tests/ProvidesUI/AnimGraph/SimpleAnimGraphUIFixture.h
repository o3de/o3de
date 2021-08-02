/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Tests/UI/UIFixture.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>

namespace EMotionFX
{
    class AnimGraph;

    class SimpleAnimGraphUIFixture
        : public UIFixture
    {
    public:
        // The animgraph contains 3 parameters, and a motion node connecting to a blend tree with a state transition.
        void SetUp() override;
        void TearDown() override;

    public:
        const AZ::u32 m_animGraphId = 64;
        AnimGraph* m_animGraph = nullptr;
        EMStudio::AnimGraphPlugin* m_animGraphPlugin = nullptr;
    };
} // namespace EMotionFX
