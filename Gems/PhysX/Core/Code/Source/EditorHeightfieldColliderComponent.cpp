/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/EditorHeightfieldColliderComponent.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/MathStringConversions.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/SimulatedBodies/StaticRigidBody.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <Editor/ColliderComponentMode.h>
#include <Source/HeightfieldColliderComponent.h>
#include <Source/Shape.h>
#include <Source/Utils.h>
#include <System/PhysXSystem.h>

#include <utility>
#include <Pipeline/HeightFieldAssetHandler.h>

namespace PhysX
{
    AZ_CVAR(float, physx_heightfieldDebugDrawDistance, 50.0f, nullptr,
        AZ::ConsoleFunctorFlags::Null, "Distance for PhysX Heightfields debug visualization.");
    AZ_CVAR(bool, physx_heightfieldDebugDrawBoundingBox, false,
        nullptr, AZ::ConsoleFunctorFlags::Null, "Draw the bounding box used for heightfield debug visualization.");

    void EditorHeightfieldColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorHeightfieldColliderComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("ColliderConfiguration", &EditorHeightfieldColliderComponent::m_colliderConfig)
                ->Field("DebugDrawSettings", &EditorHeightfieldColliderComponent::m_colliderDebugDraw)
                ->Field("UseBakedHeightfield", &EditorHeightfieldColliderComponent::m_useBakedHeightfield)
                ->Field("BakedHeightfieldRelativePath", &EditorHeightfieldColliderComponent::m_bakedHeightfieldRelativePath)
                ->Field("BakedHeightfieldAsset", &EditorHeightfieldColliderComponent::m_bakedHeightfieldAsset)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorHeightfieldColliderComponent>(
                    "PhysX Heightfield Collider", "Creates geometry in the PhysX simulation based on an attached heightfield component")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/PhysXHeightfieldCollider.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/PhysXHeightfieldCollider.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(
                            AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/physx/heightfield-collider/")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorHeightfieldColliderComponent::m_colliderConfig, "Collider configuration",
                        "Configuration of the collider")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorHeightfieldColliderComponent::OnConfigurationChanged)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &EditorHeightfieldColliderComponent::m_colliderDebugDraw,
                        "Debug draw settings",
                        "Debug draw settings")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorHeightfieldColliderComponent::m_useBakedHeightfield, "Use Baked Heightfield",
                        "Selects between a dynamically generated heightfield or a prebaked one. "
                        "A prebaked one will remain unchanged at game time even if the heightfield provider changes its data. "
                        "A dynamic one will change with heightfield provider changes.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorHeightfieldColliderComponent::OnToggleBakedHeightfield)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &EditorHeightfieldColliderComponent::IsHeightfieldInvalid)

                    ->DataElement(
                        AZ::Edit::UIHandlers::MultiLineEdit, &EditorHeightfieldColliderComponent::m_bakedHeightfieldRelativePath,
                        "Baked Heightfield Relative Path", "Path to the baked heightfield asset")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &EditorHeightfieldColliderComponent::GetBakedHeightfieldVisibilitySetting)

                    ->UIElement(AZ::Edit::UIHandlers::Button, "Bake Heightfield", "Bake Heightfield")
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->Attribute(AZ::Edit::Attributes::ButtonText, "Bake Heightfield")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorHeightfieldColliderComponent::RequestHeightfieldBaking)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &EditorHeightfieldColliderComponent::GetBakedHeightfieldVisibilitySetting)
                    ;
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<EditorHeightfieldColliderRequestBus>("EditorHeightfieldColliderRequestBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Module, "physics")
                    ->Event("RequestHeightfieldBaking", &EditorHeightfieldColliderInterface::RequestHeightfieldBaking);

            }
        }
    }

    AZ::u32 EditorHeightfieldColliderComponent::GetBakedHeightfieldVisibilitySetting() const
    {
        // Controls that are specific to baked heightfields call this to determine their visibility
        // they are visible when the mode is set to baked, otherwise hidden
        return m_useBakedHeightfield ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    void EditorHeightfieldColliderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysicsWorldBodyService"));
        provided.push_back(AZ_CRC_CE("PhysicsColliderService"));
        provided.push_back(AZ_CRC_CE("PhysicsHeightfieldColliderService"));
    }

    void EditorHeightfieldColliderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("PhysicsHeightfieldProviderService"));
    }

    void EditorHeightfieldColliderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PhysicsColliderService"));
        // Incompatible with other rigid bodies because it handles its own rigid body
        // internally and it would conflict if another rigid body is added to the entity.
        incompatible.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
    }

    EditorHeightfieldColliderComponent::EditorHeightfieldColliderComponent()
        : m_physXConfigChangedHandler(
              []([[maybe_unused]] const AzPhysics::SystemConfiguration* config)
              {
                  AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                      &AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh,
                      AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
              })
        , m_heightfieldAssetBakingJob(this)
    {
        // By default, disable heightfield collider debug drawing. This doesn't need to be viewed in the common case.
        m_colliderDebugDraw.SetDisplayFlag(false);

        // Heightfields don't support the following:
        // - Offset:  There shouldn't be a need to offset the data, since the heightfield provider is giving a physics representation
        // - IsTrigger:  PhysX heightfields don't support acting as triggers
        // - MaterialSelection:  The heightfield provider provides per-vertex material selection
        m_colliderConfig->SetPropertyVisibility(Physics::ColliderConfiguration::Offset, false);
        m_colliderConfig->SetPropertyVisibility(Physics::ColliderConfiguration::IsTrigger, false);
        m_colliderConfig->SetPropertyVisibility(Physics::ColliderConfiguration::MaterialSelection, false);
    }

    EditorHeightfieldColliderComponent ::~EditorHeightfieldColliderComponent()
    {
    }

    // AZ::Component
    void EditorHeightfieldColliderComponent::Activate()
    {
        EditorHeightfieldColliderRequestBus::Handler::BusConnect(GetEntityId());

        m_bakingCompletion.Reset(true /*isClearDependent*/);
        m_heightfieldAssetBakingJob.Reset(true);

        AzPhysics::SceneHandle sceneHandle = AzPhysics::InvalidSceneHandle;
        if (auto sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneHandle = sceneInterface->GetSceneHandle(AzPhysics::EditorPhysicsSceneName);
        }

        m_heightfieldCollider = AZStd::make_unique<HeightfieldCollider>(
            GetEntityId(), GetEntity()->GetName(), sceneHandle, m_colliderConfig,
            m_shapeConfig, HeightfieldCollider::DataSource::GenerateNewHeightfield);

        AzToolsFramework::Components::EditorComponentBase::Activate();

        const AZ::EntityId entityId = GetEntityId();

        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusConnect(entityId);

        // Debug drawing
        m_colliderDebugDraw.Connect(entityId);
        m_colliderDebugDraw.SetDisplayCallback(this);
    }

    void EditorHeightfieldColliderComponent::Deactivate()
    {
        m_colliderDebugDraw.Disconnect();
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();
        EditorHeightfieldColliderRequestBus::Handler::BusDisconnect();

        if (m_useBakedHeightfield)
        {
            // Wait for any in progress heightfield asset baking job to complete
            FinishHeightfieldBakingJob();
        }

        m_heightfieldCollider.reset();
    }

    void EditorHeightfieldColliderComponent::BlockOnPendingJobs()
    {
        if (m_heightfieldCollider)
        {
            m_heightfieldCollider->BlockOnPendingJobs();
        }
    }

    void EditorHeightfieldColliderComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        auto* heightfieldColliderComponent = gameEntity->CreateComponent<HeightfieldColliderComponent>();
        heightfieldColliderComponent->SetColliderConfiguration(*m_colliderConfig);
        heightfieldColliderComponent->SetBakedHeightfieldAsset(m_bakedHeightfieldAsset);
    }

    AZ::u32 EditorHeightfieldColliderComponent::OnConfigurationChanged()
    {
        m_heightfieldCollider->RefreshHeightfield(Physics::HeightfieldProviderNotifications::HeightfieldChangeMask::Settings);
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    bool EditorHeightfieldColliderComponent::SaveHeightfieldAssetToDisk()
    {
        auto assetType = AZ::AzTypeInfo<PhysX::Pipeline::HeightFieldAsset>::Uuid();
        auto assetHandler = const_cast<AZ::Data::AssetHandler*>(AZ::Data::AssetManager::Instance().GetHandler(assetType));
        if (assetHandler)
        {
            char projectPath[AZ_MAX_PATH_LEN];
            AZ::IO::FileIOBase::GetInstance()->ResolvePath("@projectroot@", projectPath, AZ_MAX_PATH_LEN);

            AZStd::string heightfieldFullPath;
            AzFramework::StringFunc::Path::ConstructFull(projectPath, m_bakedHeightfieldRelativePath.c_str(), heightfieldFullPath, true);

            AZ::IO::FileIOStream fileStream(heightfieldFullPath.c_str(), AZ::IO::OpenMode::ModeWrite);
            if (fileStream.IsOpen())
            {
                if (!assetHandler->SaveAssetData(m_bakedHeightfieldAsset, &fileStream))
                {
                    AZ_Error("PhysX", false, "Unable to save heightfield asset %s", heightfieldFullPath.c_str());
                    return false;
                }
            }
            else
            {
                AZ_Error("PhysX", false, "Unable to open heightfield asset file %s", heightfieldFullPath.c_str());
                return false;
            }
        }
        return true;
    }

    void EditorHeightfieldColliderComponent::StartHeightfieldBakingJob()
    {
        FinishHeightfieldBakingJob();

        m_bakingCompletion.Reset(true /*isClearDependent*/);
        m_heightfieldAssetBakingJob.Reset(true);

        m_heightfieldAssetBakingJob.SetDependent(&m_bakingCompletion);
        m_heightfieldAssetBakingJob.Start();
    }

    bool EditorHeightfieldColliderComponent::IsHeightfieldInvalid() const
    {
        return m_shapeConfig->GetCachedNativeHeightfield() == nullptr;
    }

    void EditorHeightfieldColliderComponent::FinishHeightfieldBakingJob()
    {
        m_bakingCompletion.StartAndWaitForCompletion();
    }

    bool EditorHeightfieldColliderComponent::CheckHeightfieldPathExists()
    {
        if (!m_bakedHeightfieldRelativePath.empty())
        {
            char projectPath[AZ_MAX_PATH_LEN];
            AZ::IO::FileIOBase::GetInstance()->ResolvePath("@projectroot@", projectPath, AZ_MAX_PATH_LEN);

            // retrieve the source heightfield path from the configuration
            // we need to make sure to use the same source heightfield for each baking

            // test to see if the heightfield file is actually there, if it was removed we need to
            // generate a new filename, otherwise it will cause an error in the asset system
            AZStd::string fullPath;
            AzFramework::StringFunc::Path::Join(projectPath,
                m_bakedHeightfieldRelativePath.c_str(), fullPath,
                /*bCaseInsensitive*/true, /*bNormalize*/true);

            if (!AZ::IO::FileIOBase::GetInstance()->Exists(fullPath.c_str()))
            {
                // clear it to force the generation of a new filename
                m_bakedHeightfieldRelativePath.clear();
            }
        }

        return !m_bakedHeightfieldRelativePath.empty();
    }

    void EditorHeightfieldColliderComponent::GenerateHeightfieldAsset()
    {
        AZStd::string entityName = GetEntity()->GetName();

        // the file name is a combination of the entity name and a UUID
        AZ::Uuid uuid = AZ::Uuid::CreateRandom();
        AZStd::string uuidString;
        uuid.ToString(uuidString);
        m_bakedHeightfieldRelativePath = "Heightfields/" + entityName + "_" + uuidString;

        // replace any invalid filename characters
        auto invalidCharacters = [](char letter)
        {
            return
                letter == ':' || letter == '"' || letter == '\'' ||
                letter == '{' || letter == '}' ||
                letter == '<' || letter == '>';
        };
        AZStd::replace_if(m_bakedHeightfieldRelativePath.begin(), m_bakedHeightfieldRelativePath.end(), invalidCharacters, '_');
        m_bakedHeightfieldRelativePath += AZ_FILESYSTEM_EXTENSION_SEPARATOR;
        m_bakedHeightfieldRelativePath += Pipeline::HeightFieldAssetHandler::s_assetFileExtension;

        {
            AZ::Data::AssetId generatedAssetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(generatedAssetId,
                &AZ::Data::AssetCatalogRequests::GenerateAssetIdTEMP, m_bakedHeightfieldRelativePath.c_str());

            AZ::Data::Asset<Pipeline::HeightFieldAsset> asset = AZ::Data::AssetManager::Instance().FindAsset(generatedAssetId, AZ::Data::AssetLoadBehavior::Default);
            if (!asset.GetId().IsValid())
            {
                asset = AZ::Data::AssetManager::Instance().CreateAsset<PhysX::Pipeline::HeightFieldAsset>(generatedAssetId);
            }

            m_bakedHeightfieldAsset = asset;

            physx::PxHeightField* pxHeightfield = static_cast<physx::PxHeightField*>(m_shapeConfig->GetCachedNativeHeightfield());

            // Since PxHeightfield will have shared ownership in both HeightfieldAsset and HeightfieldShapeConfiguration,
            // we need to increment the reference counter here. Both of these places call release() in destructors,
            // so we need to avoid double deletion this way.
            pxHeightfield->acquireReference();

            m_bakedHeightfieldAsset.Get()->SetHeightField(pxHeightfield);
            m_bakedHeightfieldAsset.Get()->SetMinHeight(m_shapeConfig->GetMinHeightBounds());
            m_bakedHeightfieldAsset.Get()->SetMaxHeight(m_shapeConfig->GetMaxHeightBounds());
        }
    }

    bool EditorHeightfieldColliderComponent::CheckoutHeightfieldAsset() const
    {
        char projectPath[AZ_MAX_PATH_LEN];
        AZ::IO::FileIOBase::GetInstance()->ResolvePath("@projectroot@", projectPath, AZ_MAX_PATH_LEN);

        AZStd::string heightfieldFullPath;
        AzFramework::StringFunc::Path::ConstructFull(projectPath, m_bakedHeightfieldRelativePath.c_str(), heightfieldFullPath, true);

        // make sure the folder is created
        AZStd::string heightfieldCaptureFolderPath;
        AzFramework::StringFunc::Path::GetFolderPath(heightfieldFullPath.data(), heightfieldCaptureFolderPath);
        AZ::IO::SystemFile::CreateDir(heightfieldCaptureFolderPath.c_str());

        // check out the file in source control                
        bool checkedOutSuccessfully = false;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            checkedOutSuccessfully,
            &AzToolsFramework::ToolsApplicationRequestBus::Events::RequestEditForFileBlocking,
            heightfieldFullPath.c_str(),
            "Checking out for edit...",
            AzToolsFramework::ToolsApplicationRequestBus::Events::RequestEditProgressCallback());

        if (!checkedOutSuccessfully)
        {
            AZ_Error("PhysX", false, "Source control checkout failed for file [%s]", heightfieldFullPath.c_str());
            return false;
        }

        return true;
    }

    void EditorHeightfieldColliderComponent::RequestHeightfieldBaking()
    {
        if (IsHeightfieldInvalid())
        {
            AZ_Error("PhysX", false,
                "Unable to start heightfield baking for entity [%s]. Invalid heightfield.",
                GetEntity()->GetName().c_str());
            return;
        }

        if (!CheckHeightfieldPathExists())
        {
            GenerateHeightfieldAsset();
        }

        if (CheckoutHeightfieldAsset())
        {
            StartHeightfieldBakingJob();
        }
    }

    AZ::u32 EditorHeightfieldColliderComponent::OnToggleBakedHeightfield()
    {
        if (m_useBakedHeightfield)
        {
            RequestHeightfieldBaking();
        }

        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    // AzToolsFramework::EntitySelectionEvents
    void EditorHeightfieldColliderComponent::OnSelected()
    {
        if (auto* physXSystem = GetPhysXSystem())
        {
            if (!m_physXConfigChangedHandler.IsConnected())
            {
                physXSystem->RegisterSystemConfigurationChangedEvent(m_physXConfigChangedHandler);
            }
        }
    }

    // AzToolsFramework::EntitySelectionEvents
    void EditorHeightfieldColliderComponent::OnDeselected()
    {
        m_physXConfigChangedHandler.Disconnect();
    }

    // DisplayCallback
    void EditorHeightfieldColliderComponent::Display(const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        const AzPhysics::SimulatedBody* simulatedBody = m_heightfieldCollider->GetSimulatedBody();
        if (!simulatedBody)
        {
            return;
        }

        const AzPhysics::StaticRigidBody* staticRigidBody = azrtti_cast<const AzPhysics::StaticRigidBody*>(simulatedBody);
        if (!staticRigidBody)
        {
            return;
        }

        // Calculate the center of a box in front of the camera - this will be the area to draw
        const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(viewportInfo.m_viewportId);
        const AZ::Vector3 boundsAabbCenter = cameraState.m_position + cameraState.m_forward * physx_heightfieldDebugDrawDistance * 0.5f;

        const AZ::Vector3 bodyPosition = staticRigidBody->GetPosition();
        const AZ::Vector3 aabbCenterLocalBody = boundsAabbCenter - bodyPosition;

        const AZ::u32 shapeCount = staticRigidBody->GetShapeCount();
        for (AZ::u32 shapeIndex = 0; shapeIndex < shapeCount; ++shapeIndex)
        {
            const AZStd::shared_ptr<const Physics::Shape> shape = staticRigidBody->GetShape(shapeIndex);
            m_colliderDebugDraw.DrawHeightfield(debugDisplay, aabbCenterLocalBody,
                physx_heightfieldDebugDrawDistance, shape);
        }

        if (physx_heightfieldDebugDrawBoundingBox)
        {
            const AZ::Aabb boundsAabb = AZ::Aabb::CreateCenterRadius(aabbCenterLocalBody, physx_heightfieldDebugDrawDistance);
            if (boundsAabb.IsValid())
            {
                debugDisplay.DrawWireBox(boundsAabb.GetMin(), boundsAabb.GetMax());
            }
        }
    }

    void EditorHeightfieldColliderComponent::HeightfieldBakingJob::Process()
    {
        if (m_owner)
        {
            m_owner->SaveHeightfieldAssetToDisk();
        }
    }

    EditorHeightfieldColliderComponent::HeightfieldBakingJob::HeightfieldBakingJob(EditorHeightfieldColliderComponent* owner) : AZ::Job(false, nullptr), m_owner(owner)
    {

    }
} // namespace PhysX
