/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Script/ScriptProperty.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <EMotionFX/Source/Parameter/BoolParameter.h>
#include <EMotionFX/Source/Parameter/FloatParameter.h>
#include <EMotionFX/Source/Parameter/StringParameter.h>
#include <EMotionFX/Source/Parameter/IntParameter.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MainWindow.h>
#include <Integration/ActorComponentBus.h>
#include <Integration/Editor/Components/EditorAnimGraphComponent.h>
#include <EMotionFX/Source/MotionSet.h>

#include <QApplication>


namespace EMotionFX
{
    namespace Integration
    {
        //////////////////////////////////////////////////////////////////////////
        void EditorAnimGraphComponent::Reflect(AZ::ReflectContext* context)
        {
            auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<EditorAnimGraphComponent, AzToolsFramework::Components::EditorComponentBase>()
                    ->Version(2)
                    ->Field("AnimGraphAsset", &EditorAnimGraphComponent::m_animGraphAsset)
                    ->Field("MotionSetAsset", &EditorAnimGraphComponent::m_motionSetAsset)
                    ->Field("ActiveMotionSetName", &EditorAnimGraphComponent::m_activeMotionSetName)
                    ->Field("DebugVisualization", &EditorAnimGraphComponent::m_visualize)
                    ->Field("ParameterDefaults", &EditorAnimGraphComponent::m_parameterDefaults)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<AnimGraphComponent::ParameterDefaults>(
                        "Parameter Defaults", "Default values for anim graph parameters.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Button, &AnimGraphComponent::ParameterDefaults::m_parameters, "", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                        ;

                    editContext->Class<EditorAnimGraphComponent>(
                        "Anim Graph", "The Anim Graph component manages a set of assets that are built in the Animation Editor, including the animation graph, default parameter settings, and assigned motion set for the associated Actor")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Animation")
                            ->Attribute(AZ::Edit::Attributes::Icon, ":/EMotionFX/AnimGraphComponent.svg")
                            ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, azrtti_typeid<AnimGraphAsset>())
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, ":/EMotionFX/Viewport/AnimGraphComponent.svg")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/animation/animgraph/")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorAnimGraphComponent::m_motionSetAsset,
                            "Motion set asset", "EMotion FX motion set asset to be loaded for this actor.")
                            ->Attribute("EditButton", "")
                            ->Attribute("EditDescription", "Open in Animation Editor")
                            ->Attribute("EditCallback", &EditorAnimGraphComponent::LaunchAnimationEditor)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorAnimGraphComponent::OnMotionSetAssetSelected)
                        ->DataElement(AZ_CRC("MotionSetName", 0xcf534ea6), &EditorAnimGraphComponent::m_activeMotionSetName, "Active motion set", "Motion set to use for this anim graph instance")
                            ->Attribute(AZ_CRC("MotionSetAsset", 0xd4e88984), &EditorAnimGraphComponent::GetMotionAsset)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorAnimGraphComponent::m_visualize, "Debug visualization", "Enable this to allow the anim graph to render debug visualization. Enable debug rendering on anim graph nodes first.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorAnimGraphComponent::m_animGraphAsset,
                            "Anim graph", "EMotion FX anim graph to be assigned to this actor.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorAnimGraphComponent::OnAnimGraphAssetSelected)
                            ->Attribute("EditButton", "")
                            ->Attribute("EditDescription", "Open in Animation Editor")
                            ->Attribute("EditCallback", &EditorAnimGraphComponent::LaunchAnimationEditor)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorAnimGraphComponent::m_parameterDefaults,
                            "Parameters", "Anim graph default parameter values.")
                        ;
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        EditorAnimGraphComponent::EditorAnimGraphComponent()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        EditorAnimGraphComponent::~EditorAnimGraphComponent()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorAnimGraphComponent::Activate()
        {
            // Refresh parameters in case anim graph asset changed since last session.
            OnAnimGraphAssetSelected();
            OnMotionSetAssetSelected();
            EditorAnimGraphComponentRequestBus::Handler::BusConnect(GetEntityId());
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorAnimGraphComponent::Deactivate()
        {
            EditorAnimGraphComponentRequestBus::Handler::BusDisconnect();
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();
            m_animGraphAsset.Release();
            m_motionSetAsset.Release();
        }

        void EditorAnimGraphComponent::LaunchAnimationEditor(const AZ::Data::AssetId& assetId, [[maybe_unused]] const AZ::Data::AssetType& assetType)
        {
            // call to open must be done before LoadCharacter
            const char* panelName = EMStudio::MainWindow::GetEMotionFXPaneName();
            EBUS_EVENT(AzToolsFramework::EditorRequests::Bus, OpenViewPane, panelName);

            if (assetId.IsValid())
            {
                AZ::Data::AssetId actorAssetId;
                actorAssetId.SetInvalid();
                EditorActorComponentRequestBus::EventResult(actorAssetId, GetEntityId(), &EditorActorComponentRequestBus::Events::GetActorAssetId);

                EMStudio::MainWindow* mainWindow = EMStudio::GetMainWindow();
                if (mainWindow)
                {
                    mainWindow->LoadCharacter(actorAssetId, m_animGraphAsset.GetId(), m_motionSetAsset.GetId());
                    mainWindow->show();
                    mainWindow->LoadLayoutAfterShow();

                    // Force the window to be fully loaded before loading
                    // things. Remember that QMainWindow::show() doesn't
                    // actually show anything syncronously. All it does is put
                    // a QShowEvent onto the event queue. This call makes the
                    // ShowEvent process, blocking until it is done.
                    QApplication::instance()->processEvents(QEventLoop::ExcludeUserInputEvents);


                    // After loading we want to activate based on what we have in this component (anim grah and motion set)
                    // Only activate if we have a valid anim graph and a valid motion set. An empty m_motionSetName will use
                    // the root motionset from the motion set asset
                    if (m_animGraphAsset.IsReady() && m_motionSetAsset.IsReady())
                    {   
                        AnimGraphAsset* animGraphAsset = m_animGraphAsset.GetAs<AnimGraphAsset>();
                        AZ_Assert(animGraphAsset, "Expected anim graph asset");
                        EMotionFX::AnimGraph* animGraph = animGraphAsset->GetAnimGraph();

                        MotionSetAsset*  motionSetAsset = m_motionSetAsset.GetAs<MotionSetAsset>();
                        AZ_Assert(motionSetAsset, "Expected motion set asset");
                        EMotionFX::MotionSet* rootMotionSet = motionSetAsset->m_emfxMotionSet.get();
                        EMotionFX::MotionSet* motionSet = rootMotionSet;
                        if (!m_activeMotionSetName.empty())
                        {
                            motionSet = rootMotionSet->RecursiveFindMotionSetByName(m_activeMotionSetName, true);
                            if (!motionSet)
                            {
                                AZ_Warning("EMotionFX", false, "Failed to find motion set \"%s\" in motion set file %s.",
                                    m_activeMotionSetName.c_str(),
                                    rootMotionSet->GetName());

                                motionSet = rootMotionSet;
                            }
                        }
                        
                        mainWindow->Activate(actorAssetId, animGraph, motionSet);
                    }
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////       
        AZ::u32 EditorAnimGraphComponent::OnAnimGraphAssetSelected()
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();

            if (m_motionSetAsset.GetId().IsValid())
            {
                AZ::Data::AssetBus::MultiHandler::BusConnect(m_motionSetAsset.GetId());
            }
            if (m_animGraphAsset.GetId().IsValid())
            {
                AZ::Data::AssetBus::MultiHandler::BusConnect(m_animGraphAsset.GetId());
                m_animGraphAsset.QueueLoad();
            }
            else
            {
                m_parameterDefaults.m_parameters.clear();
            }

            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        AZ::u32 EditorAnimGraphComponent::OnMotionSetAssetSelected()
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();
            
            if (m_animGraphAsset.GetId().IsValid())
            {
                AZ::Data::AssetBus::MultiHandler::BusConnect(m_animGraphAsset.GetId());
            }
            if (m_motionSetAsset.GetId().IsValid())
            {
                AZ::Data::AssetBus::MultiHandler::BusConnect(m_motionSetAsset.GetId());
                m_motionSetAsset.QueueLoad();
            }

            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorAnimGraphComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            // Re-process anim graph asset.
            OnAssetReady(asset);
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorAnimGraphComponent::SetPrimaryAsset(const AZ::Data::AssetId& assetId)
        {
            AZ::Data::Asset<AnimGraphAsset> asset = AZ::Data::AssetManager::Instance().FindOrCreateAsset<AnimGraphAsset>(assetId, m_animGraphAsset.GetAutoLoadBehavior());
            if (asset)
            {
                m_animGraphAsset = asset;
            }
        }

        bool EditorAnimGraphComponent::IsSupportedScriptPropertyType(const ValueParameter* param) const
        {
            return (azrtti_istypeof<FloatParameter>(param) ||
                    azrtti_istypeof<IntParameter>(param) ||
                    azrtti_istypeof<BoolParameter>(param) ||
                    azrtti_istypeof<StringParameter>(param));
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorAnimGraphComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            AZ_Assert(asset == m_animGraphAsset || asset == m_motionSetAsset, "Unexpected asset");

            if (asset == m_animGraphAsset)
            {
                m_animGraphAsset = asset;
                AnimGraphAsset* data = m_animGraphAsset.GetAs<AnimGraphAsset>();
                if (!data)
                {
                    return;
                }

                EMotionFX::AnimGraph* animGraph = data->GetAnimGraph();

                // Remove any parameters we have values for that are no longer in the anim graph.
                for (auto iter = m_parameterDefaults.m_parameters.begin(); iter != m_parameterDefaults.m_parameters.end(); )
                {
                    const ValueParameter* valueParameter = animGraph->FindValueParameterByName((*iter)->m_name);
                    if (!valueParameter || !IsSupportedScriptPropertyType(valueParameter))
                    {
                        delete *iter;
                        iter = m_parameterDefaults.m_parameters.erase(iter);
                    }
                    else
                    {
                        ++iter;
                    }
                }

                // Populate property array based on parameters found in the anim graph.
                const EMotionFX::ValueParameterVector& valueParameters = animGraph->RecursivelyGetValueParameters();
                for (const EMotionFX::ValueParameter* param : valueParameters)
                {
                    const AZStd::string& paramName = param->GetName();

                    // If we already have a value for this property, skip it.
                    bool found = false;
                    for (AZ::ScriptProperty* prop : m_parameterDefaults.m_parameters)
                    {
                        if (paramName == prop->m_name)
                        {
                            found = true;
                            break;
                        }
                    }

                    if (found)
                    {
                        continue;
                    }

                    // Based on the anim graph param type, create an appropriate script property for serialization and editing.
                    if (azrtti_istypeof<EMotionFX::FloatParameter>(param))
                    {
                        const EMotionFX::FloatParameter* floatParam = static_cast<const EMotionFX::FloatParameter*>(param);
                        m_parameterDefaults.m_parameters.emplace_back(aznew AZ::ScriptPropertyNumber(paramName.c_str(), floatParam->GetDefaultValue()));
                    }
                    else if (azrtti_istypeof<EMotionFX::IntParameter>(param))
                    {
                        const EMotionFX::IntParameter* intParam = static_cast<const EMotionFX::IntParameter*>(param);
                        m_parameterDefaults.m_parameters.emplace_back(aznew AZ::ScriptPropertyNumber(paramName.c_str(), intParam->GetDefaultValue()));
                    }
                    else if (azrtti_istypeof<EMotionFX::BoolParameter>(param))
                    {
                        const EMotionFX::BoolParameter* boolParam = static_cast<const EMotionFX::BoolParameter*>(param);
                        m_parameterDefaults.m_parameters.emplace_back(aznew AZ::ScriptPropertyBoolean(paramName.c_str(), boolParam->GetDefaultValue()));
                    }
                    else if (azrtti_istypeof<EMotionFX::StringParameter>(param))
                    {
                        const EMotionFX::StringParameter* stringParam = static_cast<const EMotionFX::StringParameter*>(param);
                        m_parameterDefaults.m_parameters.emplace_back(aznew AZ::ScriptPropertyString(paramName.c_str(), stringParam->GetDefaultValue().c_str()));
                    }
                    else
                    {
                        AZ_Assert(!IsSupportedScriptPropertyType(param), "This value parameter of this type ('%s') should not be supported. Please update the IsSupportedScriptPropertyType() method.", param->GetTypeDisplayName());
                    }
                }
            }
            else if (asset == m_motionSetAsset)
            {
                m_motionSetAsset = asset;
                const MotionSetAsset* data = m_motionSetAsset.GetAs<MotionSetAsset>();
                if (data)
                {
                    const EMotionFX::MotionSet* rootMotionSet = data->m_emfxMotionSet.get();
                    if (rootMotionSet)
                    {
                        if (m_activeMotionSetName.empty())
                        {
                            // if motion set name is empty, grab the root
                            m_activeMotionSetName = rootMotionSet->GetName();
                        }
                        else
                        {
                            const EMotionFX::MotionSet* motionSet = rootMotionSet->RecursiveFindMotionSetByName(m_activeMotionSetName, /*isOwnedByRuntime = */true);
                            if (!motionSet)
                            {
                                m_activeMotionSetName = rootMotionSet->GetName();
                            }
                        }
                    }
                }
            }

            // Force-refresh the property grid.
            using namespace AzToolsFramework;
            EBUS_EVENT(ToolsApplicationEvents::Bus, InvalidatePropertyDisplay, Refresh_EntireTree);
        }


        const AZ::Data::AssetId& EditorAnimGraphComponent::GetAnimGraphAssetId()
        {
            return m_animGraphAsset.GetId();
        }

        const AZ::Data::AssetId& EditorAnimGraphComponent::GetMotionSetAssetId()
        {
            return m_motionSetAsset.GetId();
        }

        void EditorAnimGraphComponent::SetAnimGraphAssetId(const AZ::Data::AssetId& assetId)
        {
            m_animGraphAsset = AZ::Data::Asset<AnimGraphAsset>(assetId, azrtti_typeid<AnimGraphAsset>());
        }

        void EditorAnimGraphComponent::SetMotionSetAssetId(const AZ::Data::AssetId& assetId)
        {
            m_motionSetAsset = AZ::Data::Asset<MotionSetAsset>(assetId, azrtti_typeid<MotionSetAsset>());
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorAnimGraphComponent::BuildGameEntity(AZ::Entity* gameEntity)
        {
            AnimGraphComponent::Configuration cfg;
            cfg.m_animGraphAsset = m_animGraphAsset;
            cfg.m_motionSetAsset = m_motionSetAsset;
            cfg.m_activeMotionSetName = m_activeMotionSetName;
            cfg.m_parameterDefaults = m_parameterDefaults;
            cfg.m_visualize = m_visualize;

            gameEntity->AddComponent(aznew AnimGraphComponent(&cfg));
        }

       
    } //namespace Integration
} // namespace EMotionFX

