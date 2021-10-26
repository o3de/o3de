/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Asset/AssetManager.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

#include <Integration/Editor/Components/EditorActorComponent.h>
#include <Integration/AnimGraphComponentBus.h>
#include <Integration/Rendering/RenderBackendManager.h>

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MainWindow.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/NodeSelectionWindow.h>
#include <EMotionFX/CommandSystem/Source/SelectionList.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/AttachmentNode.h>
#include <MCore/Source/AzCoreConversions.h>

namespace EMotionFX
{
    namespace Integration
    {
        void EditorActorComponent::Reflect(AZ::ReflectContext* context)
        {
            auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<EditorActorComponent, AzToolsFramework::Components::EditorComponentBase>()
                    ->Version(4)
                    ->Field("ActorAsset", &EditorActorComponent::m_actorAsset)
                    ->Field("MaterialPerLOD", &EditorActorComponent::m_materialPerLOD)
                    ->Field("MaterialPerActor", &EditorActorComponent::m_materialPerActor)
                    ->Field("AttachmentType", &EditorActorComponent::m_attachmentType)
                    ->Field("AttachmentTarget", &EditorActorComponent::m_attachmentTarget)
                    ->Field("RenderSkeleton", &EditorActorComponent::m_renderSkeleton)
                    ->Field("RenderCharacter", &EditorActorComponent::m_renderCharacter)
                    ->Field("RenderBounds", &EditorActorComponent::m_renderBounds)
                    ->Field("SkinningMethod", &EditorActorComponent::m_skinningMethod)
                    ->Field("UpdateJointTransformsWhenOutOfView", &EditorActorComponent::m_forceUpdateJointsOOV)
                    ->Field("LodLevel", &EditorActorComponent::m_lodLevel)
                    ->Field("BBoxConfig", &EditorActorComponent::m_bboxConfig)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<ActorComponent::BoundingBoxConfiguration>("Actor Bounding Box Config", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ActorComponent::BoundingBoxConfiguration::m_boundsType,
                                      "Bounds type",
                                      "The method used to compute the Actor bounding box. NOTE: ordered by least expensive to compute to most expensive to compute.")
                            ->EnumAttribute(ActorInstance::BOUNDS_STATIC_BASED, "Static (Recommended)")
                            ->EnumAttribute(ActorInstance::BOUNDS_NODE_BASED, "Bone position-based")
                            ->EnumAttribute(ActorInstance::BOUNDS_MESH_BASED, "Mesh vertex-based (VERY EXPENSIVE)")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ActorComponent::BoundingBoxConfiguration::m_expandBy,
                            "Expand by",
                            "Percentage that the calculated bounding box should be automatically expanded with. "
                            "This can be used to add a tolerance area to the calculated bounding box to avoid clipping the character too early. "
                            "A static bounding box together with the expansion is the recommended way for maximum performance. (Default = 25%)")
                            ->Attribute(AZ::Edit::Attributes::Suffix, " %")
                            ->Attribute(AZ::Edit::Attributes::Min, -100.0f + AZ::Constants::Tolerance)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ActorComponent::BoundingBoxConfiguration::m_autoUpdateBounds,
                                      "Automatically update bounds?",
                                      "If true, bounds are automatically updated based on some frequency. Otherwise bounds are computed only at creation or when triggered manually")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ActorComponent::BoundingBoxConfiguration::GetVisibilityAutoUpdate)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ActorComponent::BoundingBoxConfiguration::m_updateTimeFrequency,
                                      "Update frequency",
                                      "How often to update bounds automatically")
                            ->Attribute(AZ::Edit::Attributes::Suffix, " Hz")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, FLT_MAX)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &ActorComponent::BoundingBoxConfiguration::GetVisibilityAutoUpdateSettings)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ActorComponent::BoundingBoxConfiguration::m_updateItemFrequency,
                                      "Update item skip factor",
                                      "How many items (bones or vertices) to skip when automatically updating bounds."
                                      " <br> i.e. =1 uses every single item, =2 uses every 2nd item, =3 uses every 3rd item...")
                            ->Attribute(AZ::Edit::Attributes::Suffix, " items")
                            ->Attribute(AZ::Edit::Attributes::Min, (AZ::u32)1)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &ActorComponent::BoundingBoxConfiguration::GetVisibilityAutoUpdateSettings)
                        ;

                    editContext->Class<EditorActorComponent>("Actor", "The Actor component manages an instance of an Actor")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Animation")
                        ->Attribute(AZ::Edit::Attributes::Icon, ":/EMotionFX/ActorComponent.svg")
                        ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, azrtti_typeid<ActorAsset>())
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, ":/EMotionFX/Viewport/ActorComponent.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/animation/actor/")
                        ->DataElement(0, &EditorActorComponent::m_actorAsset,
                            "Actor asset", "Assigned actor asset")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorActorComponent::OnAssetSelected)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute("EditButton", "")
                        ->Attribute("EditDescription", "Open in Animation Editor")
                        ->Attribute("EditCallback", &EditorActorComponent::LaunchAnimationEditor)
                        ->DataElement(0, &EditorActorComponent::m_materialPerActor,
                            "Material", "Material assignment for this actor")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &EditorActorComponent::IsAtomDisabled)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorActorComponent::OnMaterialPerActorChanged)
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Render options")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(0, &EditorActorComponent::m_renderCharacter,
                            "Draw character", "Toggles rendering of character mesh.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorActorComponent::OnDebugDrawFlagChanged)
                        ->DataElement(0, &EditorActorComponent::m_renderSkeleton,
                            "Draw skeleton", "Toggles rendering of skeleton.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorActorComponent::OnDebugDrawFlagChanged)
                        ->DataElement(0, &EditorActorComponent::m_renderBounds, "Draw bounds", "Toggles rendering of world space bounding boxes.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorActorComponent::OnDebugDrawFlagChanged)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &EditorActorComponent::m_skinningMethod,
                            "Skinning method", "Choose the skinning method this actor is using")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorActorComponent::OnSkinningMethodChanged)
                        ->EnumAttribute(SkinningMethod::DualQuat, "Dual quat skinning")
                        ->EnumAttribute(SkinningMethod::Linear, "Linear skinning")
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Attach To")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &EditorActorComponent::m_attachmentType,
                            "Attachment type", "Type of attachment to use when attaching to the target entity.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorActorComponent::OnAttachmentTypeChanged)
                        ->EnumAttribute(AttachmentType::None, "None")
                        ->EnumAttribute(AttachmentType::SkinAttachment, "Skin attachment")
                        ->DataElement(0, &EditorActorComponent::m_attachmentTarget,
                            "Target entity", "Entity Id whose actor instance we should attach to.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->Attribute(AZ::Edit::Attributes::RequiredService, AZ_CRC("EMotionFXActorService", 0xd6e8f48d))
                        ->Attribute(AZ::Edit::Attributes::Visibility, &EditorActorComponent::AttachmentTargetVisibility)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorActorComponent::OnAttachmentTargetChanged)
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Out of view")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(0, &EditorActorComponent::m_forceUpdateJointsOOV,
                            "Force update joints", "Force update the joint transforms of actor, even when the character is out of the camera view.")

                        ->DataElement(0, &EditorActorComponent::m_bboxConfig,
                                      "Bounding box configuration", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorActorComponent::OnBBoxConfigChanged)
                        ;
                }
            }

            AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->Class<EditorActorComponent>()->RequestBus("ActorComponentRequestBus");
            }
        }

        //////////////////////////////////////////////////////////////////////////
        EditorActorComponent::EditorActorComponent()
            : m_renderCharacter(true)
            , m_renderSkeleton(false)
            , m_renderBounds(false)
            , m_entityVisible(true)
            , m_skinningMethod(SkinningMethod::DualQuat)
            , m_attachmentType(AttachmentType::None)
            , m_attachmentJointIndex(0)
            , m_lodLevel(0)
            , m_actorAsset(AZ::Data::AssetLoadBehavior::NoLoad)
        {
            m_debugRenderFlags[RENDER_SOLID] = true;
        }

        //////////////////////////////////////////////////////////////////////////
        EditorActorComponent::~EditorActorComponent()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::Init()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::Activate()
        {
            LoadActorAsset();

            const AZ::EntityId entityId = GetEntityId();
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
                m_entityVisible, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsVisible);

            ActorComponentRequestBus::Handler::BusConnect(entityId);
            EditorActorComponentRequestBus::Handler::BusConnect(entityId);
            LmbrCentral::AttachmentComponentNotificationBus::Handler::BusConnect(entityId);
            AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(entityId);
            AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusConnect(entityId);
            AzFramework::BoundsRequestBus::Handler::BusConnect(entityId);
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::Deactivate()
        {
            AzFramework::BoundsRequestBus::Handler::BusDisconnect();
            AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusDisconnect();
            AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
            LmbrCentral::AttachmentComponentNotificationBus::Handler::BusDisconnect();
            EditorActorComponentRequestBus::Handler::BusDisconnect();
            ActorComponentRequestBus::Handler::BusDisconnect();

            AZ::TransformNotificationBus::Handler::BusDisconnect();
            AZ::TickBus::Handler::BusDisconnect();
            AZ::Data::AssetBus::Handler::BusDisconnect();

            DestroyActorInstance();
            m_actorAsset.Release();
        }

        //////////////////////////////////////////////////////////////////////////
        bool EditorActorComponent::GetRenderCharacter() const
        {
            return m_renderCharacter;
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::SetRenderCharacter(bool enable)
        {
            if (m_renderCharacter != enable)
            {
                m_renderCharacter = enable;
                OnEntityVisibilityChanged(m_renderCharacter);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        bool EditorActorComponent::GetRenderActorVisible() const
        {
            if (m_renderActorInstance)
            {
                return m_renderActorInstance->IsVisible();
            }
            return false;
        }
        size_t EditorActorComponent::GetNumJoints() const
        {
            const Actor* actor = m_actorAsset->GetActor();
            if (actor)
            {
                return actor->GetNumNodes();
            }

            return 0;
        }

        //////////////////////////////////////////////////////////////////////////
        SkinningMethod EditorActorComponent::GetSkinningMethod() const
        {
            return m_skinningMethod;
        }
        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::LoadActorAsset()
        {
            // Queue actor asset load. Instantiation occurs in OnAssetReady.
            if (m_actorAsset.GetId().IsValid())
            {
                AZ::Data::AssetBus::Handler::BusDisconnect();
                AZ::Data::AssetBus::Handler::BusConnect(m_actorAsset.GetId());
                m_actorAsset.QueueLoad();
            }
            else
            {
                DestroyActorInstance();
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::DestroyActorInstance()
        {
            if (m_actorInstance)
            {
                ActorComponentNotificationBus::Event(
                    GetEntityId(),
                    &ActorComponentNotificationBus::Events::OnActorInstanceDestroyed,
                    m_actorInstance.get());
            }

            m_actorInstance = nullptr;
            m_renderActorInstance.reset();
        }

        //////////////////////////////////////////////////////////////////////////
        // EditorActorComponentRequestBus::Handler
        //////////////////////////////////////////////////////////////////////////
        const AZ::Data::AssetId& EditorActorComponent::GetActorAssetId()
        {
            return m_actorAsset.GetId();
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::OnEntityVisibilityChanged(bool visibility)
        {
            m_entityVisible = visibility;

            if (m_renderActorInstance)
            {
                m_renderActorInstance->SetIsVisible(m_entityVisible && m_renderCharacter);
            }
        }
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        AZ::Crc32 EditorActorComponent::OnAssetSelected()
        {
            LoadActorAsset();

            if (!m_actorAsset.GetId().IsValid())
            {
                m_materialPerLOD.clear();

                // Only need to refresh the values here.
                return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
            }

            return AZ::Edit::PropertyRefreshLevels::None;
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::OnMaterialChanged()
        {
            if (m_renderActorInstance)
            {
                m_renderActorInstance->SetMaterials(m_materialPerLOD);
            }
        }

        void EditorActorComponent::OnMaterialPerActorChanged()
        {
            if (m_actorInstance)
            {
                m_materialPerLOD.resize(m_actorInstance->GetActor()->GetNumLODLevels());
                for (auto& materialPath : m_materialPerLOD)
                {
                    materialPath.SetAssetPath(m_materialPerActor.GetAssetPath().c_str());
                }
            }
            OnMaterialChanged();
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::OnDebugDrawFlagChanged()
        {
            if (m_renderSkeleton || m_renderBounds || m_renderCharacter)
            {
                AZ::TickBus::Handler::BusConnect();
            }
            else
            {
                AZ::TickBus::Handler::BusDisconnect();
            }

            if (m_renderActorInstance)
            {
                m_renderActorInstance->SetIsVisible(m_entityVisible && m_renderCharacter);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::OnSkinningMethodChanged()
        {
            if (m_renderActorInstance)
            {
                m_renderActorInstance->SetSkinningMethod(m_skinningMethod);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        bool EditorActorComponent::AttachmentTargetVisibility()
        {
            return (m_attachmentType != AttachmentType::None);
        }

        //////////////////////////////////////////////////////////////////////////
        bool EditorActorComponent::AttachmentTargetJointVisibility()
        {
            return (m_attachmentType == AttachmentType::ActorAttachment);
        }

        //////////////////////////////////////////////////////////////////////////
        AZStd::string EditorActorComponent::AttachmentJointButtonText()
        {
            return m_attachmentJointName.empty() ?
                AZStd::string("(No joint selected)") : m_attachmentJointName;
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Crc32 EditorActorComponent::OnAttachmentTypeChanged()
        {
            if (m_attachmentType == AttachmentType::None)
            {
                m_attachmentTarget.SetInvalid();
                m_attachmentJointName.clear();
            }
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Crc32 EditorActorComponent::OnAttachmentTargetChanged()
        {
            if (!IsValidAttachment(GetEntityId(), m_attachmentTarget))
            {
                m_attachmentTarget.SetInvalid();
                AZ_Error("EMotionFX", false, "You cannot attach to yourself or create circular dependencies! Attachment cannot be performed.");
            }
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }

        //////////////////////////////////////////////////////////////////////////
        AZ::Crc32 EditorActorComponent::OnAttachmentTargetJointSelect()
        {
            // Grab actor instance and invoke UI for joint selection.
            EMotionFXPtr<ActorInstance> actorInstance;
            ActorComponentRequestBus::EventResult(
                actorInstance,
                m_attachmentTarget,
                &ActorComponentRequestBus::Events::GetActorInstance);

            AZ::Crc32 refreshLevel = AZ::Edit::PropertyRefreshLevels::None;

            if (actorInstance)
            {
                EMStudio::NodeSelectionWindow* nodeSelectWindow = new EMStudio::NodeSelectionWindow(nullptr, true);
                nodeSelectWindow->setWindowTitle(nodeSelectWindow->tr("Select Target Joint"));

                CommandSystem::SelectionList selection;

                // If a joint was previously selected, ensure it's pre-selected in the UI.
                if (!m_attachmentJointName.empty())
                {
                    Node* node = actorInstance->GetActor()->GetSkeleton()->FindNodeByName(m_attachmentJointName.c_str());
                    if (node)
                    {
                        selection.AddNode(node);
                    }
                }

                QObject::connect(nodeSelectWindow, &EMStudio::NodeSelectionWindow::accepted,
                    [this, nodeSelectWindow, &refreshLevel, &actorInstance]()
                    {
                        auto& selectedItems = nodeSelectWindow->GetNodeHierarchyWidget()->GetSelectedItems();
                        if (!selectedItems.empty())
                        {
                            const char* jointName = selectedItems[0].GetNodeName();
                            Node* node = actorInstance->GetActor()->GetSkeleton()->FindNodeByName(jointName);
                            if (node)
                            {
                                m_attachmentJointName = jointName;
                                m_attachmentJointIndex = node->GetNodeIndex();

                                refreshLevel = AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
                            }
                        }
                    });

                nodeSelectWindow->Update(actorInstance->GetID(), &selection);
                nodeSelectWindow->exec();
                delete nodeSelectWindow;
            }

            return refreshLevel;
        }

        void EditorActorComponent::OnBBoxConfigChanged()
        {
            if (m_actorInstance)
            {
                m_bboxConfig.SetAndUpdate(m_actorInstance.get());
            }
        }

        void EditorActorComponent::LaunchAnimationEditor(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType&)
        {
            // call to open must be done before LoadCharacter
            const char* panelName = EMStudio::MainWindow::GetEMotionFXPaneName();
            EBUS_EVENT(AzToolsFramework::EditorRequests::Bus, OpenViewPane, panelName);

            if (assetId.IsValid())
            {
                AZ::Data::AssetId animgraphAssetId;
                EditorAnimGraphComponentRequestBus::EventResult(animgraphAssetId, GetEntityId(), &EditorAnimGraphComponentRequestBus::Events::GetAnimGraphAssetId);
                AZ::Data::AssetId motionSetAssetId;
                EditorAnimGraphComponentRequestBus::EventResult(motionSetAssetId, GetEntityId(), &EditorAnimGraphComponentRequestBus::Events::GetMotionSetAssetId);

                EMStudio::MainWindow* mainWindow = EMStudio::GetMainWindow();
                if (mainWindow)
                {
                    mainWindow->LoadCharacter(assetId, animgraphAssetId, motionSetAssetId);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////

        void EditorActorComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            m_actorAsset = asset;
            AZ_Assert(m_actorAsset.IsReady() && m_actorAsset->GetActor(), "Actor asset should be loaded and actor valid.");

            CheckActorCreation();
        }

        void EditorActorComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            DestroyActorInstance();
            OnAssetReady(asset);
        }

        void EditorActorComponent::SetActorAsset(AZ::Data::Asset<ActorAsset> actorAsset)
        {
            m_actorAsset = actorAsset;

            Actor* actor = m_actorAsset->GetActor();
            if (actor)
            {
                CheckActorCreation();
            }
        }

        void EditorActorComponent::InitializeMaterial(ActorAsset& actorAsset)
        {
            if (!m_materialPerLOD.empty())
            {
                // If the materialPerLOD exist, it means that we previously stored the path to the material. Use it.
                m_materialPerActor.SetAssetPath(m_materialPerLOD[0].GetAssetPath().c_str());
            }
            else
            {
                // If a material exists next to the actor, pre - initialize LOD material slot with that material.
                // This is merely an accelerator for the user, and is isolated to tools-only code (the editor actor component).
                AZStd::string materialAssetPath;
                EBUS_EVENT_RESULT(materialAssetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, actorAsset.GetId());
                if (!materialAssetPath.empty())
                {
                    // Query the catalog for a material of the same name as the actor.
                    AzFramework::StringFunc::Path::ReplaceExtension(materialAssetPath, "mtl");
                    AZ::Data::AssetId materialAssetId;
                    EBUS_EVENT_RESULT(materialAssetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, materialAssetPath.c_str(), AZ::Data::s_invalidAssetType, false);

                    // If found, initialize all empty material slots with the material.
                    if (materialAssetId.IsValid())
                    {
                        m_materialPerActor.SetAssetPath(materialAssetPath.c_str());
                    }
                }
            }

            using namespace AzToolsFramework;
            ToolsApplicationEvents::Bus::Broadcast(&ToolsApplicationEvents::InvalidatePropertyDisplay, Refresh_EntireTree);
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world)
        {
            AZ_UNUSED(local);
            if (!m_actorInstance)
            {
                return;
            }

            m_actorInstance->SetLocalSpaceTransform(MCore::AzTransformToEmfxTransform(world));
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::SetPrimaryAsset(const AZ::Data::AssetId& assetId)
        {
            AZ::Data::Asset<ActorAsset> asset = AZ::Data::AssetManager::Instance().FindOrCreateAsset<ActorAsset>(assetId, m_actorAsset.GetAutoLoadBehavior());
            if (asset)
            {
                m_actorAsset = asset;
                OnAssetSelected();
            }
        }

        //////////////////////////////////////////////////////////////////////////
        void EditorActorComponent::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
        {
            if (!m_actorInstance)
            {
                return;
            }

            if (m_renderActorInstance)
            {
                m_renderActorInstance->OnTick(deltaTime);
                m_renderActorInstance->UpdateBounds();

                m_debugRenderFlags[RENDER_AABB] = m_renderBounds;
                m_debugRenderFlags[RENDER_SKELETON] = m_renderSkeleton;
                m_debugRenderFlags[RENDER_EMFX_DEBUG] = true;
                m_renderActorInstance->DebugDraw(m_debugRenderFlags);
            }
        }

        void EditorActorComponent::BuildGameEntity(AZ::Entity* gameEntity)
        {
            ActorComponent::Configuration cfg;
            cfg.m_actorAsset = m_actorAsset;
            cfg.m_materialPerLOD = m_materialPerLOD;
            cfg.m_renderSkeleton = m_renderSkeleton;
            cfg.m_renderCharacter = m_renderCharacter;
            cfg.m_renderBounds = m_renderBounds;
            cfg.m_attachmentType = m_attachmentType;
            cfg.m_attachmentTarget = m_attachmentTarget;
            cfg.m_attachmentJointIndex = m_attachmentJointIndex;
            cfg.m_lodLevel = m_lodLevel;
            cfg.m_skinningMethod = m_skinningMethod;
            cfg.m_bboxConfig = m_bboxConfig;
            cfg.m_forceUpdateJointsOOV = m_forceUpdateJointsOOV;

            gameEntity->AddComponent(aznew ActorComponent(&cfg));
        }

        AZ::EntityId EditorActorComponent::GetAttachedToEntityId() const
        {
            return m_attachmentTarget;
        }

        AZ::Aabb EditorActorComponent::GetEditorSelectionBoundsViewport(
            const AzFramework::ViewportInfo& /*viewportInfo*/)
        {
            return GetWorldBounds();
        }

        AZ::Aabb EditorActorComponent::GetWorldBounds()
        {
            if (m_renderActorInstance)
            {
                return m_renderActorInstance->GetWorldAABB();
            }

            return AZ::Aabb::CreateNull();
        }

        AZ::Aabb EditorActorComponent::GetLocalBounds()
        {
            if (m_renderActorInstance)
            {
                return m_renderActorInstance->GetLocalAABB();
            }

            return AZ::Aabb::CreateNull();
        }

        bool EditorActorComponent::EditorSelectionIntersectRayViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            const AZ::Vector3& src, const AZ::Vector3& dir, float& distance)
        {
            if (!m_actorAsset.Get() || !m_actorAsset.Get()->GetActor() || !m_actorInstance || !m_actorInstance->GetTransformData() || !m_renderCharacter)
            {
                return false;
            }

            distance = std::numeric_limits<float>::max();

            // Get the MCore::Ray used by Mesh::Intersects
            // Convert the input source position and direction to a line segment by using the frustum depth as line length.
            const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(viewportInfo.m_viewportId);
            const float frustumDepth = cameraState.m_farClip - cameraState.m_nearClip;
            const AZ::Vector3 dest = src + dir * frustumDepth;
            const MCore::Ray ray(src, dest);

            // Update the mesh deformers (apply software skinning and morphing) so the intersection test will hit the actor
            // if it is being animated by a motion component that is previewing the animation in the editor.
            m_actorInstance->UpdateMeshDeformers(0.0f, true);

            const TransformData* transformData = m_actorInstance->GetTransformData();
            const Pose* currentPose = transformData->GetCurrentPose();
            bool isHit = false;

            // Iterate through the meshes in the actor, looking for the closest hit
            const size_t lodLevel = m_actorInstance->GetLODLevel();
            Actor* actor = m_actorAsset.Get()->GetActor();
            const size_t numNodes = actor->GetNumNodes();
            for (size_t nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex)
            {
                Mesh* mesh = actor->GetMesh(lodLevel, nodeIndex);
                if (!mesh || mesh->GetIsCollisionMesh())
                {
                    continue;
                }

                // Use the actor instance transform for skinned meshes (as the vertices are pre-transformed and in model space) and the node world transform otherwise.
                const Transform meshTransform = currentPose->GetMeshNodeWorldSpaceTransform(lodLevel, nodeIndex);

                AZ::Vector3 hitPoint;
                if (mesh->Intersects(meshTransform, ray, &hitPoint))
                {
                    isHit = true;
                    float hitDistance = (src - hitPoint).GetLength();
                    if (hitDistance < distance)
                    {
                        distance = hitDistance;
                    }
                }
            }

            return isHit;
        }

        // Check if the given attachment is valid.
        bool EditorActorComponent::IsValidAttachment(const AZ::EntityId& attachment, const AZ::EntityId& attachTo) const
        {
            // Cannot attach to yourself.
            if (attachment == attachTo)
            {
                return false;
            }

            // Detect if attachTo is already in another circular chain.
            auto AttachmentStep = [](AZ::EntityId attach, int stride) -> AZ::EntityId
            {
                AZ_Assert(stride > 0, "Stride value has to be greater than 0.");

                if (attach.IsValid())
                {
                    for (int i = 0; i < stride; ++i)
                    {
                        AZ::EntityId next;
                        EditorActorComponentRequestBus::EventResult(next, attach, &EditorActorComponentRequestBus::Events::GetAttachedToEntityId);
                        if (!next.IsValid())
                        {
                            return next;
                        }
                        attach = next;
                    }
                    return attach;
                }
                else
                {
                    return attach;
                }
            };
            AZ::EntityId slowWalker = attachTo;
            AZ::EntityId fastWalker = attachTo;
            while (fastWalker.IsValid())
            {
                slowWalker = AttachmentStep(slowWalker, 1);
                fastWalker = AttachmentStep(fastWalker, 2);
                if (fastWalker.IsValid() && fastWalker == slowWalker)
                {
                    return false; // Cycle detected if slowWalker meets fastWalker.
                }
            }

            // Walk our way up to the root.
            AZ::EntityId resultId;
            EditorActorComponentRequestBus::EventResult(resultId, attachTo, &EditorActorComponentRequestBus::Events::GetAttachedToEntityId);
            while (resultId.IsValid())
            {
                AZ::EntityId localResult;
                EditorActorComponentRequestBus::EventResult(localResult, resultId, &EditorActorComponentRequestBus::Events::GetAttachedToEntityId);

                // We detected a loop.
                if (localResult == attachment)
                {
                    return false;
                }

                resultId = localResult;
            }

            return true;
        }

        // The entity has attached to the target.
        void EditorActorComponent::OnAttached(AZ::EntityId targetId)
        {
            const AZ::EntityId* busIdPtr = LmbrCentral::AttachmentComponentNotificationBus::GetCurrentBusId();
            if (busIdPtr)
            {
                const auto result = AZStd::find(m_attachments.begin(), m_attachments.end(), *busIdPtr);
                if (result == m_attachments.end())
                {
                    m_attachments.emplace_back(*busIdPtr);
                }
            }

            if (!m_actorInstance)
            {
                return;
            }

            ActorInstance* targetActorInstance = nullptr;
            ActorComponentRequestBus::EventResult(targetActorInstance, targetId, &ActorComponentRequestBus::Events::GetActorInstance);

            const char* jointName = nullptr;
            LmbrCentral::AttachmentComponentRequestBus::EventResult(jointName, GetEntityId(), &LmbrCentral::AttachmentComponentRequestBus::Events::GetJointName);
            if (targetActorInstance)
            {
                Node* node = jointName ? targetActorInstance->GetActor()->GetSkeleton()->FindNodeByName(jointName) : targetActorInstance->GetActor()->GetSkeleton()->GetNode(0);
                if (node)
                {
                    const size_t jointIndex = node->GetNodeIndex();
                    Attachment* attachment = AttachmentNode::Create(targetActorInstance, jointIndex, m_actorInstance.get(), true /* Managed externally, by this component. */);
                    targetActorInstance->AddAttachment(attachment);
                }
            }
        }

        // The entity is detaching from the target.
        void EditorActorComponent::OnDetached(AZ::EntityId targetId)
        {
            // Remove the targetId from the attachment list
            const AZ::EntityId* busIdPtr = LmbrCentral::AttachmentComponentNotificationBus::GetCurrentBusId();
            if (busIdPtr)
            {
                m_attachments.erase(AZStd::remove(m_attachments.begin(), m_attachments.end(), *busIdPtr), m_attachments.end());
            }

            if (!m_actorInstance)
            {
                return;
            }

            ActorInstance* targetActorInstance = nullptr;
            ActorComponentRequestBus::EventResult(targetActorInstance, targetId, &ActorComponentRequestBus::Events::GetActorInstance);
            if (targetActorInstance)
            {
                targetActorInstance->RemoveAttachment(m_actorInstance.get());
            }
        }

        bool EditorActorComponent::IsAtomDisabled() const
        {
            return false;
        }

        void EditorActorComponent::CheckActorCreation()
        {
            // Enable/disable debug drawing.
            OnDebugDrawFlagChanged();

            // Create actor instance.
            auto* actorAsset = m_actorAsset.GetAs<ActorAsset>();
            AZ_Error("EMotionFX", actorAsset, "Actor asset is not valid.");
            if (!actorAsset)
            {
                return;
            }

            if (m_actorInstance)
            {
                ActorComponentNotificationBus::Event(
                    GetEntityId(),
                    &ActorComponentNotificationBus::Events::OnActorInstanceDestroyed,
                    m_actorInstance.get());
            }

            m_actorInstance = actorAsset->CreateInstance(GetEntity());
            if (!m_actorInstance)
            {
                AZ_Error("EMotionFX", actorAsset, "Failed to create actor instance.");
                return;
            }

            // If we are loading the actor for the first time, automatically add the material
            // per lod information. If the amount of lods between different actors that are assigned
            // to this component differ, then reinit the materials.
            if (m_materialPerActor.GetAssetPath().empty())
            {
                InitializeMaterial(*actorAsset);
            }
            OnMaterialPerActorChanged();

            // Assign entity Id to user data field, so we can extract owning entity from an EMFX actor pointer.
            m_actorInstance->SetCustomData(reinterpret_cast<void*>(static_cast<AZ::u64>(GetEntityId())));

            // Notify listeners that an actor instance has been created.
            ActorComponentNotificationBus::Event(
                GetEntityId(),
                &ActorComponentNotificationBus::Events::OnActorInstanceCreated,
                m_actorInstance.get());

            // Setup initial transform and listen for transform changes.
            AZ::Transform transform;
            AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
            OnTransformChanged(transform, transform);
            AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());

            // Force an update of node transforms so we can get an accurate bounding box.
            m_actorInstance->UpdateTransformations(0.0f, true, false);
            OnBBoxConfigChanged(); // Apply BBox config

            // Creating the render actor AFTER both actor asset and mesh asset loaded.
            RenderBackend* renderBackend = AZ::Interface<RenderBackendManager>::Get()->GetRenderBackend();
            if (renderBackend)
            {
                m_actorAsset->InitRenderActor();

                // If there is already a RenderActorInstance, destroy it before creating the new one so there are not two instances potentially handling events for the same entityId
                m_renderActorInstance.reset(nullptr);
                // Create the new RenderActorInstance
                m_renderActorInstance.reset(renderBackend->CreateActorInstance(GetEntityId(),
                    m_actorInstance,
                    m_actorAsset,
                    m_materialPerLOD,
                    m_skinningMethod,
                    transform));

                if (m_renderActorInstance)
                {
                    m_renderActorInstance->SetIsVisible(m_entityVisible && m_renderCharacter);

                    m_renderActorInstance->SetOnMaterialChangedCallback([this](const AZStd::string& materialName)
                        {
                            m_materialPerLOD.clear();

                            if (!materialName.empty())
                            {
                                m_materialPerActor.SetAssetPath(materialName.c_str());
                            }
                            else
                            {
                                m_materialPerActor.SetAssetPath("");
                                InitializeMaterial(*m_actorAsset.GetAs<ActorAsset>());
                            }

                            // Update the rendernode and the property grid
                            OnMaterialPerActorChanged();
                            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                                &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay,
                                AzToolsFramework::Refresh_AttributesAndValues);
                        });
                }
            }

            // Reattach all attachments
            for (AZ::EntityId& attachment : m_attachments)
            {
                LmbrCentral::AttachmentComponentRequestBus::Event(attachment, &LmbrCentral::AttachmentComponentRequestBus::Events::Reattach, true);
            }
        }

        void EditorActorComponent::SetRenderFlag(ActorRenderFlagBitset renderFlags)
        {
            m_debugRenderFlags = renderFlags;
        }
    } //namespace Integration
} // namespace EMotionFX
