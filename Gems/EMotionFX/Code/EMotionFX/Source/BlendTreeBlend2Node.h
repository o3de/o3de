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

#include "EMotionFXConfig.h"
#include "BlendTreeBlend2NodeBase.h"


namespace EMotionFX
{
    class AnimGraphPose;
    class AnimGraphInstance;

    class EMFX_API BlendTreeBlend2Node
        : public BlendTreeBlend2NodeBase
    {
    public:      
        AZ_RTTI(BlendTreeBlend2Node, "{218AFAE7-C5A3-4E75-A69B-E4B0D67D5C7A}", BlendTreeBlend2NodeBase)
        AZ_CLASS_ALLOCATOR_DECL

        BlendTreeBlend2Node();
        ~BlendTreeBlend2Node();

        const char* GetPaletteName() const override;

        static void Reflect(AZ::ReflectContext* context);

    private:
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void Output(AnimGraphInstance* animGraphInstance) override;
        void OutputNoFeathering(AnimGraphInstance* animGraphInstance);
        void OutputFeathering(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData);
        void UpdateMotionExtraction(AnimGraphInstance* animGraphInstance, AnimGraphNode* nodeA, AnimGraphNode* nodeB, float weight, UniqueData* uniqueData);
    };
} // namespace EMotionFX