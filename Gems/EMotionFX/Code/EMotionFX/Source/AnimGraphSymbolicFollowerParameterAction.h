/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
     * AnimGraphSymbolicFollowerParameterAction is a specific type of trigger action that send a parameter (change) event to the follower graph.
     * Compare to AnimGraphFollowerParameterAction, this action use a parameter from the main graph to sync its value to the follower graph's parameter.
     */
    class EMFX_API AnimGraphSymbolicFollowerParameterAction
        : public AnimGraphTriggerAction
        , private AZ::Data::AssetBus::MultiHandler
    {
    public:
        AZ_RTTI(AnimGraphSymbolicFollowerParameterAction, "{1A7A4EB7-759E-4278-ADAF-0CF75516B9CE}", AnimGraphTriggerAction)
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphSymbolicFollowerParameterAction();
        AnimGraphSymbolicFollowerParameterAction(AnimGraph* animGraph);
        ~AnimGraphSymbolicFollowerParameterAction() override;

        bool InitAfterLoading(AnimGraph* animGraph) override;

        void GetSummary(AZStd::string* outResult) const override;
        void GetTooltip(AZStd::string* outResult) const override;
        const char* GetPaletteName() const override;

        void TriggerAction(AnimGraphInstance* animGraphInstance) const override;

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

        static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

        AZ::Data::Asset<Integration::AnimGraphAsset>  m_refAnimGraphAsset;
        AZStd::string                                 m_followerParameterName;
        AZStd::string                                 m_leaderParameterName;

        AZStd::vector<AZStd::string>                  m_maskedParameterNames;
    };
} // namespace EMotionFX
