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
