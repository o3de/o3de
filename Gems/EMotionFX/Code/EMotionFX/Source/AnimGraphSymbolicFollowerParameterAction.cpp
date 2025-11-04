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
#include <EMotionFX/Source/AnimGraphSymbolicFollowerParameterAction.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <MCore/Source/AttributeBool.h>
#include <MCore/Source/AttributeFloat.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphSymbolicFollowerParameterAction, AnimGraphAllocator)

    AnimGraphSymbolicFollowerParameterAction::AnimGraphSymbolicFollowerParameterAction()
        : AnimGraphTriggerAction()
    {
    }


    AnimGraphSymbolicFollowerParameterAction::AnimGraphSymbolicFollowerParameterAction(AnimGraph* animGraph)
        : AnimGraphSymbolicFollowerParameterAction()
    {
        InitAfterLoading(animGraph);
    }


    AnimGraphSymbolicFollowerParameterAction::~AnimGraphSymbolicFollowerParameterAction()
    {
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
    }


    bool AnimGraphSymbolicFollowerParameterAction::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphTriggerAction::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        return true;
    }


    const char* AnimGraphSymbolicFollowerParameterAction::GetPaletteName() const
    {
        return "Symbolic Follower Parameter Action";
    }


    void AnimGraphSymbolicFollowerParameterAction::TriggerAction(AnimGraphInstance* animGraphInstance) const
    {
        if (m_leaderParameterName.empty())
        {
            return;
        }

        MCore::Attribute* leaderAttribute = animGraphInstance->FindParameter(m_leaderParameterName);
        if (!leaderAttribute)
        {
            AZ_Assert(false, "Can't find a parameter named %s in the leader graph.", m_leaderParameterName.c_str());
            return;
        }

        const AZStd::vector<AnimGraphInstance*>& followerGraphs = animGraphInstance->GetFollowerGraphs();

        for (AnimGraphInstance* followerGraph : followerGraphs)
        {
            MCore::Attribute* followerAttribute = followerGraph->FindParameter(m_followerParameterName);
            if (followerAttribute)
            {
                if (followerAttribute->InitFrom(leaderAttribute))
                {
                    // If the name matches and the type matches, we sync the attribute from leader to follower.
                    AZ::Outcome<size_t> index = followerGraph->FindParameterIndex(m_followerParameterName);
                    const ValueParameter* valueParameter = followerGraph->GetAnimGraph()->FindValueParameter(index.GetValue());
                    AnimGraphNotificationBus::Broadcast(&AnimGraphNotificationBus::Events::OnParameterActionTriggered, valueParameter);
                }
                else
                {
                    // If the name matches and but type doesn't, warns the user.
                    AZ_Warning("EMotionFX", false, "Follower parameter %s does not match leader parameter %s", m_followerParameterName.c_str(), m_leaderParameterName.c_str());
                }

            }
        }
    }


    // Construct and output the information summary string for this object
    void AnimGraphSymbolicFollowerParameterAction::GetSummary(AZStd::string* outResult) const
    {
        *outResult = AZStd::string::format("%s: Follower Parameter Name='%s, Leader Parameter Name='%s.", RTTI_GetTypeName(), m_followerParameterName.c_str(), m_leaderParameterName.c_str());
    }


    // Construct and output the tooltip for this object
    void AnimGraphSymbolicFollowerParameterAction::GetTooltip(AZStd::string* outResult) const
    {
        AZStd::string columnName, columnValue;

        // Add the action type
        columnName = "Action Type: ";
        columnValue = RTTI_GetTypeName();
        *outResult = AZStd::string::format("<table border=\"0\"><tr><td width=\"120\"><b>%s</b></td><td><nobr>%s</nobr></td>", columnName.c_str(), columnValue.c_str());

        // Add follower parameter
        columnName = "Follower Parameter Name: ";
        *outResult += AZStd::string::format("</tr><tr><td><b><nobr>%s</nobr></b></td><td><nobr>%s</nobr></td>", columnName.c_str(), m_followerParameterName.c_str());

        // Add leader parameter
        columnName = "Leader Parameter Name: ";
        *outResult += AZStd::string::format("</tr><tr><td><b><nobr>%s</nobr></b></td><td><nobr>%s</nobr></td>", columnName.c_str(), m_leaderParameterName.c_str());
    }


    AnimGraph* AnimGraphSymbolicFollowerParameterAction::GetRefAnimGraph() const
    {
        if (m_refAnimGraphAsset.GetId().IsValid() && m_refAnimGraphAsset.IsReady())
        {
            return m_refAnimGraphAsset.Get()->GetAnimGraph();
        }
        return nullptr;
    }


    bool AnimGraphSymbolicFollowerParameterAction::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        const unsigned int version = classElement.GetVersion();
        if (version < 2)
        {
            // Developer code and APIs with exclusionary terms will be deprecated as we introduce replacements across this project's related
            // codebases and APIs. Please note, some instances have been retained in the current version to provide backward compatibility
            // for assets/materials created prior to the change. These will be deprecated in the future.
            int index = classElement.FindElement(AZ_CRC_CE("servantParameterName"));
            if (index > 0)
            {
                AZStd::string oldValue;
                AZ::SerializeContext::DataElementNode& dataElementNode = classElement.GetSubElement(index);
                const bool result = dataElementNode.GetData<AZStd::string>(oldValue);
                if (!result)
                {
                    return false;
                }
                classElement.RemoveElement(index);
                classElement.AddElementWithData(context, "followerParameterName", oldValue);
            }

            // Developer code and APIs with exclusionary terms will be deprecated as we introduce replacements across this project's related
            // codebases and APIs. Please note, some instances have been retained in the current version to provide backward compatibility for
            // assets/materials created prior to the change. These will be deprecated in the future.
            index = classElement.FindElement(AZ_CRC_CE("masterParameterName"));
            if (index > 0)
            {
                AZStd::string oldValue;
                AZ::SerializeContext::DataElementNode& dataElementNode = classElement.GetSubElement(index);
                const bool result = dataElementNode.GetData<AZStd::string>(oldValue);
                if (!result)
                {
                    return false;
                }
                classElement.RemoveElement(index);
                classElement.AddElementWithData(context, "leaderParameterName", oldValue);
            }
        }
        return true;
    }

    void AnimGraphSymbolicFollowerParameterAction::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphSymbolicFollowerParameterAction, AnimGraphTriggerAction>()
            ->Version(2, VersionConverter)
            ->Field("animGraphAsset", &AnimGraphSymbolicFollowerParameterAction::m_refAnimGraphAsset)
            ->Field("followerParameterName", &AnimGraphSymbolicFollowerParameterAction::m_followerParameterName)
            ->Field("leaderParameterName", &AnimGraphSymbolicFollowerParameterAction::m_leaderParameterName)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphSymbolicFollowerParameterAction>("Follower Parameter Action", "Follower parameter action attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphSymbolicFollowerParameterAction::m_refAnimGraphAsset, "Follower anim graph", "Follower anim graph that we want to pick a parameter from")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphSymbolicFollowerParameterAction::OnAnimGraphAssetChanged)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ_CRC_CE("AnimGraphParameter"), &AnimGraphSymbolicFollowerParameterAction::m_followerParameterName, "Follower parameter", "The follower parameter that we want to sync to.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                ->Attribute(AZ_CRC_CE("AnimGraph"), &AnimGraphSymbolicFollowerParameterAction::GetRefAnimGraph)
            ->DataElement(AZ_CRC_CE("AnimGraphParameter"), &AnimGraphSymbolicFollowerParameterAction::m_leaderParameterName, "Leader parameter", "The leader parameter that we want to sync from.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                ->Attribute(AZ_CRC_CE("AnimGraph"), &AnimGraphSymbolicFollowerParameterAction::GetAnimGraph)
            ;
    }


    void AnimGraphSymbolicFollowerParameterAction::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_refAnimGraphAsset)
        {
            m_refAnimGraphAsset = asset;

            // TODO: remove once "owned by runtime" is gone
            asset.GetAs<Integration::AnimGraphAsset>()->GetAnimGraph()->SetIsOwnedByRuntime(false);

            OnAnimGraphAssetReady();
        }
    }


    void AnimGraphSymbolicFollowerParameterAction::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_refAnimGraphAsset)
        {
            // TODO: remove once "owned by runtime" is gone
            asset.GetAs<Integration::AnimGraphAsset>()->GetAnimGraph()->SetIsOwnedByRuntime(false);
            m_refAnimGraphAsset = asset;

            OnAnimGraphAssetReady();
        }
    }

    void AnimGraphSymbolicFollowerParameterAction::OnAnimGraphAssetChanged()
    {
        LoadAnimGraphAsset();
    }

    void AnimGraphSymbolicFollowerParameterAction::LoadAnimGraphAsset()
    {
        if (m_refAnimGraphAsset.GetId().IsValid())
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();
            m_refAnimGraphAsset.QueueLoad();
            AZ::Data::AssetBus::MultiHandler::BusConnect(m_refAnimGraphAsset.GetId());
        }
    }

    void AnimGraphSymbolicFollowerParameterAction::OnAnimGraphAssetReady()
    {
        // Verify if the follower parameter is valid in the ref anim graph
        AnimGraph* refAnimGraph = GetRefAnimGraph();
        if (refAnimGraph)
        {
            if (!refAnimGraph->FindParameterByName(m_followerParameterName))
            {
                m_followerParameterName.clear();
            }
        }

        // Verify if the leader parameter is valid in the leader anim graph
        AnimGraph* leaderAnimGraph = GetAnimGraph();
        if (leaderAnimGraph)
        {
            if (!refAnimGraph->FindParameterByName(m_leaderParameterName))
            {
                m_leaderParameterName.clear();
            }
        }
    }
} // namespace EMotionFX
