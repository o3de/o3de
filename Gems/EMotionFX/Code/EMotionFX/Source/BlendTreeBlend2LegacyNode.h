/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "EMotionFXConfig.h"
#include "AnimGraphNode.h"
#include "BlendTreeBlend2NodeBase.h"


namespace EMotionFX
{
    class AnimGraphPose;
    class AnimGraphInstance;


    class EMFX_API BlendTreeBlend2LegacyNode
        : public BlendTreeBlend2NodeBase
    {
    public:
        AZ_RTTI(BlendTreeBlend2LegacyNode, "{2079733F-10C1-4ECB-91F6-03DEDAD2B3FE}", BlendTreeBlend2NodeBase)
        AZ_CLASS_ALLOCATOR_DECL

        BlendTreeBlend2LegacyNode();
        ~BlendTreeBlend2LegacyNode();

        const char* GetPaletteName() const override;
        void SetAdditiveBlending(bool additiveBlending);
        bool GetAdditiveBlending() const;

        static void Reflect(AZ::ReflectContext* context);

    private:
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void Output(AnimGraphInstance* animGraphInstance) override;
        void OutputNoFeathering(AnimGraphInstance* animGraphInstance);
        void OutputFeathering(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData);
        void UpdateMotionExtraction(AnimGraphInstance* animGraphInstance, AnimGraphNode* nodeA, AnimGraphNode* nodeB, float weight, UniqueData* uniqueData);

        bool m_additiveBlending;
    };
} // namespace EMotionFX
