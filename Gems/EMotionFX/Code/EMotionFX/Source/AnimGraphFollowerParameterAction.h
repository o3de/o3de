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

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraphTriggerAction.h>
#include <Integration/Assets/AnimGraphAsset.h>

namespace EMotionFX
{
    // forward declarations
    class AnimGraphInstance;

    /**
     * AnimGraphFollowerParameterAction is a specific type of trigger action that send a parameter (change) event to the follower graph.
     */
    class EMFX_API AnimGraphFollowerParameterAction
        : public AnimGraphTriggerAction
        , private AZ::Data::AssetBus::MultiHandler
    {
    public:
        AZ_RTTI(AnimGraphFollowerParameterAction, "{F24D01FF-C245-402E-877B-2ED29B952979}", AnimGraphTriggerAction)
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphFollowerParameterAction();
        AnimGraphFollowerParameterAction(AnimGraph* animGraph);
        ~AnimGraphFollowerParameterAction() override;

        bool InitAfterLoading(AnimGraph* animGraph) override;

        void GetSummary(AZStd::string* outResult) const override;
        void GetTooltip(AZStd::string* outResult) const override;
        const char* GetPaletteName() const override;

        void TriggerAction(AnimGraphInstance* animGraphInstance) const override;

        void SetParameterName(const AZStd::string& parameterName);
        const AZStd::string& GetParameterName() const;

        static void Reflect(AZ::ReflectContext* context);

        AnimGraph* GetRefAnimGraph() const;

        // AZ::Data::AssetBus::MultiHandler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

    private:

        // Callbacks from the Reflected Property Editor
        void OnAnimGraphAssetChanged();
        void LoadAnimGraphAsset();
        void OnAnimGraphAssetReady();

        AZ::Data::Asset<Integration::AnimGraphAsset>  m_refAnimGraphAsset;
        AZStd::string                                 m_parameterName;
        float                                         m_triggerValue;
    };
} // namespace EMotionFX
