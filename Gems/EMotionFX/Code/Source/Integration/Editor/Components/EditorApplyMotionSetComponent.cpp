/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(CARBONATED)
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Script/ScriptProperty.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <EMotionFX/Source/Parameter/BoolParameter.h>
#include <EMotionFX/Source/Parameter/FloatParameter.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MainWindow.h>
#include <Integration/ActorComponentBus.h>
#include <Integration/Editor/Components/EditorApplyMotionSetComponent.h>
#include <EMotionFX/Source/MotionSet.h>

#include <QApplication>


namespace EMotionFX
{
    namespace Integration
    {
        //////////////////////////////////////////////////////////////////////////
        void EditorApplyMotionSetComponent::Reflect(AZ::ReflectContext* context)
        {
            auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<EditorApplyMotionSetComponent, AzToolsFramework::Components::EditorComponentBase>()
#if defined(CARBONATED)
                    ->Version(3)
                    ->Field("MotionSetAsset", &EditorApplyMotionSetComponent::m_motionSetAsset)
                    ->Field("ActiveMotionSetNames", &EditorApplyMotionSetComponent::m_motionSetAssetNames)
                    ->Field("MotionSetAssetMap", &EditorApplyMotionSetComponent::m_motionSetAssetMap)
#else
                    ->Version(2)
                    ->Field("MotionSetAsset", &EditorApplyMotionSetComponent::m_motionSetAsset)
                    ->Field("ActiveMotionSetName", &EditorApplyMotionSetComponent::m_activeMotionSetName)
#endif
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<EditorApplyMotionSetComponent>(
                        "Apply Motion Set", "Applies the configured MotionSetAsset to any compatible animation graph")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Animation")
                            ->Attribute(AZ::Edit::Attributes::Icon, ":/EMotionFX/AnimGraphComponent.svg")
                            ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, azrtti_typeid<MotionSetAsset>())
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, ":/EMotionFX/AnimGraphComponent.svg")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorApplyMotionSetComponent::m_motionSetAsset,
                            "Motion set asset", "EMotion FX motion set asset to be loaded onto calling actor at runtime.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorApplyMotionSetComponent::OnMotionSetAssetSelected)
#if defined(CARBONATED)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorApplyMotionSetComponent::m_motionSetAssetNames, "Motion set options", "Available motion sets to use for this anim graph instance")
                            ->Attribute(AZ::Edit::Attributes::StringList, &EditorApplyMotionSetComponent::GetMotionAssetOptionNames)
                            ->Attribute(AZ::Edit::Attributes::ForceAutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                            ->ElementAttribute(AZ::Edit::Attributes::ReadOnly, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorApplyMotionSetComponent::m_motionSetAssetMap)
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                            ->Attribute(AZ::Edit::Attributes::HideIcon, true)
                            ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::SliceFlags::HideAllTheTime | AZ::Edit::SliceFlags::PushWhenHidden)
#else
                        ->DataElement(AZ_CRC("MotionSetName", 0xcf534ea6), &EditorApplyMotionSetComponent::m_activeMotionSetName, "Active motion set", "Motion set to use for this anim graph instance")
                            ->Attribute(AZ_CRC("MotionSetAsset", 0xd4e88984), &EditorApplyMotionSetComponent::GetMotionAsset)
#endif
                        ;
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        EditorApplyMotionSetComponent::EditorApplyMotionSetComponent()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        EditorApplyMotionSetComponent::~EditorApplyMotionSetComponent()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorApplyMotionSetComponent::Activate()
        {
            // Refresh parameters in case anim graph asset changed since last session.
            OnMotionSetAssetSelected();
            EditorApplyMotionSetComponentRequestBus::Handler::BusConnect(GetEntityId());
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorApplyMotionSetComponent::Deactivate()
        {
            EditorApplyMotionSetComponentRequestBus::Handler::BusDisconnect();
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();
            m_motionSetAsset.Release();
        }

        AZ::u32 EditorApplyMotionSetComponent::OnMotionSetAssetSelected()
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();

            if (m_motionSetAsset.GetId().IsValid())
            {
                AZ::Data::AssetBus::MultiHandler::BusConnect(m_motionSetAsset.GetId());
                m_motionSetAsset.QueueLoad();
            }
#if defined(CARBONATED)
            else
            {
                // Case when clearing the motion set field manually, clear other serialized data 
                m_motionSetAssetNames.clear();
                m_motionSetAssetMap.clear();
            }
#endif

            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorApplyMotionSetComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            // Re-process anim graph asset.
            OnAssetReady(asset);
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorApplyMotionSetComponent::SetPrimaryAsset([[maybe_unused]] const AZ::Data::AssetId& assetId)
        {
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorApplyMotionSetComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
#if defined(CARBONATED)
            // To ignore loading m_derivedMotionSetAsset if called in-line when connecting to the bus
            if (!m_loadDerivedDeferred)
                return;

            AZ_Assert(asset == m_motionSetAsset || asset == m_derivedMotionSetAsset, "Unexpected asset");
#else
            AZ_Assert(asset == m_motionSetAsset, "Unexpected asset");
#endif

            if (asset == m_motionSetAsset)
            {
#if defined(CARBONATED)
                static const char* GENDER_SUFFIX_FEMALE = "_f";
                static const char* GENDER_SUFFIX_MALE = "_m";
#endif

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

#if defined(CARBONATED)
                        AZStd::string activeMotionBasePath;
                        EBUS_EVENT_RESULT(activeMotionBasePath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, m_motionSetAsset.GetId());

                        //Base path post processing
                        auto end = activeMotionBasePath.find_last_of("/\\");
                        if (end != AZStd::string::npos)
                            activeMotionBasePath = activeMotionBasePath.substr(0, end);

                        //Identify selected gender
                        AZStd::string genderSuffix;
                        if (m_activeMotionSetName.size() > 2)
                            genderSuffix = m_activeMotionSetName.substr(m_activeMotionSetName.size() - 2);

                        // Clear derived motion data
                        m_derivedMotionSetAsset.Reset();
                        m_derivedMotionSetAsset = AZ::Data::Asset<MotionSetAsset>();
                        m_derivedMotionSetGender = MotionSetGender::MOTION_NONE;
                        m_derivedMotionSetName.clear();

                        MotionSetGender activeMotionSetGender = MotionSetGender::MOTION_NONE;
                        AZStd::string derivedGenderSuffix = "";

                        if (strcmp(genderSuffix.c_str(), GENDER_SUFFIX_FEMALE) == 0)
                        {
                            activeMotionSetGender = MotionSetGender::MOTION_FEMALE;
                            m_derivedMotionSetGender = MotionSetGender::MOTION_MALE;
                            derivedGenderSuffix = GENDER_SUFFIX_MALE;

                        }
                        else if (strcmp(genderSuffix.c_str(), GENDER_SUFFIX_MALE) == 0)
                        {
                            activeMotionSetGender = MotionSetGender::MOTION_MALE;
                            m_derivedMotionSetGender = MotionSetGender::MOTION_FEMALE;
                            derivedGenderSuffix = GENDER_SUFFIX_FEMALE;
                        }
                        else
                        {
                            activeMotionSetGender = MotionSetGender::MOTION_NEUTRAL;
                        }     

                        // Search for derived motion
                        AZ::Data::AssetId derivedAssetId;

                        if (m_derivedMotionSetGender != MotionSetGender::MOTION_NONE)
                        {
                            m_derivedMotionSetName = AZStd::string::format("%s%s", m_activeMotionSetName.substr(0, m_activeMotionSetName.size() - 2).c_str(), derivedGenderSuffix.c_str());
                            AZStd::string path = AZStd::string::format("%s/%s.motionset", activeMotionBasePath.c_str(), m_derivedMotionSetName.c_str());
                            EBUS_EVENT_RESULT(derivedAssetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, path.c_str(), AZ::AzTypeInfo<MotionSetAsset>::Uuid(), false);

                            if (!derivedAssetId.IsValid())
                            {
                                // If we cannot find the derived motion asset, make the active one be the gender neutral option
                                activeMotionSetGender = MotionSetGender::MOTION_NEUTRAL;
                            }
                        }

                        // Prevent duplicates and pushing uninitialized motion sets 
                        if (m_motionSetAsset.GetData() && m_motionSetAssetMap[activeMotionSetGender] != m_motionSetAsset)
                        {
                            // Clear other serialized data 
                            m_motionSetAssetNames.clear();
                            m_motionSetAssetMap.clear();

                            m_motionSetAssetMap[activeMotionSetGender] = m_motionSetAsset;
                            m_motionSetAssetNames.push_back(m_activeMotionSetName);
                        }

                        // Create new derived asset
                        if (derivedAssetId.IsValid())
                        {
                            // Connecting to the bus with m_derivedMotionSetAssset's id will pre-emptively attempt to load it in-line 
                            // Our local m_derivedMotionSetAsset object is not yet created the first time around until we call Create()
                            // So we ignore the bus induced loading and force reload if needed once the m_derivedMotionSetAsset object is created
                            m_loadDerivedDeferred = false;
                            AZ::Data::AssetBus::MultiHandler::BusConnect(derivedAssetId);
                            m_loadDerivedDeferred = true;

                            // Sets m_derivedMotionSetAsset without loading it automatically
                            if (m_derivedMotionSetAsset.Create(derivedAssetId, false))
                            {
                                // Manually try to queue a load, if unsucessful, means asset was already loaded when connecting to the bus 
                                if (!m_derivedMotionSetAsset.QueueLoad())
                                {
                                    m_derivedMotionSetAsset.Reload();
                                }
                            }
                            else
                            {
                                AZ_Assert(false, "Could not load derived Asset");
                            }
                        }
#endif
                    }
                }
            }
#if defined(CARBONATED)
            else if (asset == m_derivedMotionSetAsset)
            {
                // Prevent duplicates and pushing uninitialized motion sets 
                if (m_derivedMotionSetAsset.GetData() && m_motionSetAssetMap[m_derivedMotionSetGender] != m_derivedMotionSetAsset)
                {
                    // Storing the loaded motion sets derived from the primary one
                    m_motionSetAssetMap[m_derivedMotionSetGender] = m_derivedMotionSetAsset;
                    m_motionSetAssetNames.push_back(m_derivedMotionSetName);
                }
            }
#endif

            // Force-refresh the property grid.
            using namespace AzToolsFramework;
            EBUS_EVENT(ToolsApplicationEvents::Bus, InvalidatePropertyDisplay, Refresh_EntireTree);
        }

        const AZ::Data::AssetId& EditorApplyMotionSetComponent::GetMotionSetAssetId()
        {
            return m_motionSetAsset.GetId();
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorApplyMotionSetComponent::BuildGameEntity(AZ::Entity* gameEntity)
        {
#if defined(CARBONATED)
            ApplyMotionSetComponent::Configuration cfg;
            cfg.m_motionSetAssetMap = m_motionSetAssetMap;
#else
            ApplyMotionSetComponent::Configuration cfg;
            cfg.m_motionSetAsset = m_motionSetAsset;
            cfg.m_activeMotionSetName = m_activeMotionSetName;
#endif

            gameEntity->AddComponent(aznew ApplyMotionSetComponent(&cfg));
        }
    } //namespace Integration
} // namespace EMotionFX
#endif
