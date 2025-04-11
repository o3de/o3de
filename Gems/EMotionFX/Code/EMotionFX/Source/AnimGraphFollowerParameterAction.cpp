/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphBus.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphFollowerParameterAction.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <MCore/Source/AttributeBool.h>
#include <MCore/Source/AttributeFloat.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphFollowerParameterAction, AnimGraphAllocator)

    AnimGraphFollowerParameterAction::AnimGraphFollowerParameterAction()
        : AnimGraphTriggerAction()
        , m_triggerValue(0.0f)
    {
    }


    AnimGraphFollowerParameterAction::AnimGraphFollowerParameterAction(AnimGraph* animGraph)
        : AnimGraphFollowerParameterAction()
    {
        InitAfterLoading(animGraph);
    }


    AnimGraphFollowerParameterAction::~AnimGraphFollowerParameterAction()
    {
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
    }


    bool AnimGraphFollowerParameterAction::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphTriggerAction::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        return true;
    }


    const char* AnimGraphFollowerParameterAction::GetPaletteName() const
    {
        return "Follower Parameter Action";
    }


    void AnimGraphFollowerParameterAction::TriggerAction(AnimGraphInstance* animGraphInstance) const
    {
        const AZStd::vector<AnimGraphInstance*>& followerGraphs = animGraphInstance->GetFollowerGraphs();

        for (AnimGraphInstance* followerGraph : followerGraphs)
        {
            MCore::Attribute* attribute = followerGraph->FindParameter(m_parameterName);
            if (attribute)
            {
                switch (attribute->GetType())
                {
                case MCore::AttributeBool::TYPE_ID:
                {
                    MCore::AttributeBool* atrBool = static_cast<MCore::AttributeBool*>(attribute);
                    atrBool->SetValue(m_triggerValue != 0.0f);
                    break;
                }
                case MCore::AttributeFloat::TYPE_ID:
                {
                    MCore::AttributeFloat* atrFloat = static_cast<MCore::AttributeFloat*>(attribute);
                    atrFloat->SetValue(m_triggerValue);
                    break;
                }
                default:
                {
                    AZ_Assert(false, "Type %d of attribute %s are not supported", attribute->GetType(), m_parameterName.c_str());
                    break;
                }
                }

                AZ::Outcome<size_t> index = followerGraph->FindParameterIndex(m_parameterName);
                const ValueParameter* valueParameter = followerGraph->GetAnimGraph()->FindValueParameter(index.GetValue());

                AnimGraphNotificationBus::Broadcast(&AnimGraphNotificationBus::Events::OnParameterActionTriggered, valueParameter);
            }
        }
    }


    void AnimGraphFollowerParameterAction::SetParameterName(const AZStd::string& parameterName)
    {
        m_parameterName = parameterName;
    }


    const AZStd::string& AnimGraphFollowerParameterAction::GetParameterName() const
    {
        return m_parameterName;
    }


    // Construct and output the information summary string for this object
    void AnimGraphFollowerParameterAction::GetSummary(AZStd::string* outResult) const
    {
        *outResult = AZStd::string::format("%s: Parameter Name='%s", RTTI_GetTypeName(), m_parameterName.c_str());
    }


    // Construct and output the tooltip for this object
    void AnimGraphFollowerParameterAction::GetTooltip(AZStd::string* outResult) const
    {
        AZStd::string columnName, columnValue;

        // Add the action type
        columnName = "Action Type: ";
        columnValue = RTTI_GetTypeName();
        *outResult = AZStd::string::format("<table border=\"0\"><tr><td width=\"120\"><b>%s</b></td><td><nobr>%s</nobr></td>", columnName.c_str(), columnValue.c_str());

        // Add the parameter
        columnName = "Parameter Name: ";
        *outResult += AZStd::string::format("</tr><tr><td><b><nobr>%s</nobr></b></td><td><nobr>%s</nobr></td>", columnName.c_str(), m_parameterName.c_str());
    }


    AnimGraph* AnimGraphFollowerParameterAction::GetRefAnimGraph() const
    {
        if (m_refAnimGraphAsset.GetId().IsValid() && m_refAnimGraphAsset.IsReady())
        {
            return m_refAnimGraphAsset.Get()->GetAnimGraph();
        }
        return nullptr;
    }


    void AnimGraphFollowerParameterAction::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphFollowerParameterAction, AnimGraphTriggerAction>()
            ->Version(1)
            ->Field("animGraphAsset", &AnimGraphFollowerParameterAction::m_refAnimGraphAsset)
            ->Field("parameterName", &AnimGraphFollowerParameterAction::m_parameterName)
            ->Field("triggerValue", &AnimGraphFollowerParameterAction::m_triggerValue)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphFollowerParameterAction>("Follower Parameter Action", "Follower parameter action attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphFollowerParameterAction::m_refAnimGraphAsset, "Follower anim graph", "Follower anim graph that we want to pick a parameter from")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphFollowerParameterAction::OnAnimGraphAssetChanged)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ_CRC_CE("AnimGraphParameter"), &AnimGraphFollowerParameterAction::m_parameterName, "Follower parameter", "The follower parameter that we want to sync to.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                ->Attribute(AZ_CRC_CE("AnimGraph"), &AnimGraphFollowerParameterAction::GetRefAnimGraph)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphFollowerParameterAction::m_triggerValue, "Trigger value", "The value that the parameter will be override to.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ;
    }


    void AnimGraphFollowerParameterAction::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_refAnimGraphAsset)
        {
            m_refAnimGraphAsset = asset;

            // TODO: remove once "owned by runtime" is gone
            asset.GetAs<Integration::AnimGraphAsset>()->GetAnimGraph()->SetIsOwnedByRuntime(false);

            OnAnimGraphAssetReady();
        }
    }


    void AnimGraphFollowerParameterAction::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_refAnimGraphAsset)
        {
            // TODO: remove once "owned by runtime" is gone
            asset.GetAs<Integration::AnimGraphAsset>()->GetAnimGraph()->SetIsOwnedByRuntime(false);
            m_refAnimGraphAsset = asset;

            OnAnimGraphAssetReady();
        }
    }

    void AnimGraphFollowerParameterAction::OnAnimGraphAssetChanged()
    {
        LoadAnimGraphAsset();
    }

    void AnimGraphFollowerParameterAction::LoadAnimGraphAsset()
    {
        if (m_refAnimGraphAsset.GetId().IsValid())
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();
            m_refAnimGraphAsset.QueueLoad();
            AZ::Data::AssetBus::MultiHandler::BusConnect(m_refAnimGraphAsset.GetId());
        }
    }

    void AnimGraphFollowerParameterAction::OnAnimGraphAssetReady()
    {
        // Verify if the m_parameterName is valid in the ref anim graph
        AnimGraph* refAnimGraph = GetRefAnimGraph();
        if (refAnimGraph)
        {
            if (!refAnimGraph->FindParameterByName(m_parameterName))
            {
                m_parameterName.clear();
            }
        }
    }
} // namespace EMotionFX
