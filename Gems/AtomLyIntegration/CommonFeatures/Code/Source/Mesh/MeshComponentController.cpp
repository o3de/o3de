/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Mesh/MeshComponentController.h>

#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentConstants.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshHandleStateBus.h>

#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/RHIUtils.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/Scene.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Scene/Scene.h>
#include <AzFramework/Scene/SceneSystemInterface.h>

#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace Render
    {
        static AZ::Name s_CLOTH_DATA_Name = AZ::Name::FromStringLiteral("CLOTH_DATA", AZ::Interface<AZ::NameDictionary>::Get());

        namespace Internal
        {
            struct MeshComponentNotificationBusHandler final
                : public MeshComponentNotificationBus::Handler
                , public AZ::BehaviorEBusHandler
            {
                AZ_EBUS_BEHAVIOR_BINDER(
                    MeshComponentNotificationBusHandler, "{8B8F4977-817F-4C7C-9141-0E5FF899E1BC}", AZ::SystemAllocator, OnModelReady);

                void OnModelReady(
                    [[maybe_unused]] const Data::Asset<RPI::ModelAsset>& modelAsset,
                    [[maybe_unused]] const Data::Instance<RPI::Model>& model) override
                {
                    Call(FN_OnModelReady);
                }
            };
        } // namespace Internal

        namespace MeshComponentControllerVersionUtility
        {
            bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
            {
                if (classElement.GetVersion() < 2)
                {
                    RPI::Cullable::LodOverride lodOverride = aznumeric_cast<RPI::Cullable::LodOverride>(classElement.FindElement(AZ_CRC_CE("LodOverride")));
                    static constexpr uint8_t old_NoLodOverride = AZStd::numeric_limits <RPI::Cullable::LodOverride>::max();
                    if (lodOverride == old_NoLodOverride)
                    {
                        classElement.AddElementWithData(context, "LodType", RPI::Cullable::LodType::SpecificLod);
                    }
                }
                return true;
            }
        } // namespace MeshComponentControllerVersionUtility

        void MeshComponentConfig::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<MeshComponentConfig>()
                    ->Version(4, &MeshComponentControllerVersionUtility::VersionConverter)
                    ->Field("ModelAsset", &MeshComponentConfig::m_modelAsset)
                    ->Field("SortKey", &MeshComponentConfig::m_sortKey)
                    ->Field("ExcludeFromReflectionCubeMaps", &MeshComponentConfig::m_excludeFromReflectionCubeMaps)
                    ->Field("UseForwardPassIBLSpecular", &MeshComponentConfig::m_useForwardPassIblSpecular)
                    ->Field("IsRayTracingEnabled", &MeshComponentConfig::m_isRayTracingEnabled)
                    ->Field("IsAlwaysDynamic", &MeshComponentConfig::m_isAlwaysDynamic)
                    ->Field("SupportRayIntersection", &MeshComponentConfig::m_enableRayIntersection)
                    ->Field("LodType", &MeshComponentConfig::m_lodType)
                    ->Field("LodOverride", &MeshComponentConfig::m_lodOverride)
                    ->Field("MinimumScreenCoverage", &MeshComponentConfig::m_minimumScreenCoverage)
                    ->Field("QualityDecayRate", &MeshComponentConfig::m_qualityDecayRate)
                    ->Field("LightingChannelConfig", &MeshComponentConfig::m_lightingChannelConfig);
            }
        }

        bool MeshComponentConfig::IsAssetSet()
        {
            return m_modelAsset.GetId().IsValid();
        }

        bool MeshComponentConfig::LodTypeIsScreenCoverage()
        {
            return m_lodType == RPI::Cullable::LodType::ScreenCoverage;
        }

        bool MeshComponentConfig::LodTypeIsSpecificLOD()
        {
            return m_lodType == RPI::Cullable::LodType::SpecificLod;
        }

        bool MeshComponentConfig::ShowLodConfig()
        {
            return LodTypeIsScreenCoverage() || LodTypeIsSpecificLOD();
        }

        AZStd::vector<AZStd::pair<RPI::Cullable::LodOverride, AZStd::string>> MeshComponentConfig::GetLodOverrideValues()
        {
            AZStd::vector<AZStd::pair<RPI::Cullable::LodOverride, AZStd::string>> values;
            uint32_t lodCount = 0;
            if (IsAssetSet())
            {
                if (m_modelAsset.IsReady())
                {
                    lodCount = static_cast<uint32_t>(m_modelAsset->GetLodCount());
                }
                else
                {
                    // If the asset isn't loaded, it's still possible it exists in the instance database.
                    Data::Instance<RPI::Model> model =
                        Data::InstanceDatabase<RPI::Model>::Instance().Find(Data::InstanceId::CreateFromAsset(m_modelAsset));
                    if (model)
                    {
                        lodCount = static_cast<uint32_t>(model->GetLodCount());
                    }
                }
            }

            values.reserve(lodCount + 1);
            values.push_back({ aznumeric_cast<RPI::Cullable::LodOverride>(0), "Default LOD 0 (Highest Detail)" });

            for (uint32_t i = 1; i < lodCount; ++i)
            {
                AZStd::string enumDescription = AZStd::string::format("LOD %i", i);
                values.push_back({ aznumeric_cast<RPI::Cullable::LodOverride>(i), enumDescription.c_str() });
            }

            return values;
        }

        MeshComponentController::~MeshComponentController()
        {
            // Release memory, disconnect from buses in the right order and broadcast events so that other components are aware.
            Deactivate();
        }

        void MeshComponentController::Reflect(ReflectContext* context)
        {
            MeshComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<MeshComponentController>()
                    ->Version(1)
                    ->Field("Configuration", &MeshComponentController::m_configuration);
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->ConstantProperty("DefaultLodOverride", BehaviorConstant(0))
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "render")
                    ->Attribute(AZ::Script::Attributes::Module, "render");

                behaviorContext->ConstantProperty("DefaultLodType", BehaviorConstant(RPI::Cullable::LodType::Default))
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "render")
                    ->Attribute(AZ::Script::Attributes::Module, "render");

                behaviorContext->EBus<MeshComponentRequestBus>("RenderMeshComponentRequestBus")
                    ->Event("GetModelAssetId", &MeshComponentRequestBus::Events::GetModelAssetId)
                    ->Event("SetModelAssetId", &MeshComponentRequestBus::Events::SetModelAssetId)
                    ->Event("GetModelAssetPath", &MeshComponentRequestBus::Events::GetModelAssetPath)
                    ->Event("SetModelAssetPath", &MeshComponentRequestBus::Events::SetModelAssetPath)
                    ->Event("SetSortKey", &MeshComponentRequestBus::Events::SetSortKey)
                    ->Event("GetSortKey", &MeshComponentRequestBus::Events::GetSortKey)
                    ->Event("SetIsAlwaysDynamic", &MeshComponentRequestBus::Events::SetIsAlwaysDynamic)
                    ->Event("GetIsAlwaysDynamic", &MeshComponentRequestBus::Events::GetIsAlwaysDynamic)
                    ->Event("SetLodType", &MeshComponentRequestBus::Events::SetLodType)
                    ->Event("GetLodType", &MeshComponentRequestBus::Events::GetLodType)
                    ->Event("SetLodOverride", &MeshComponentRequestBus::Events::SetLodOverride)
                    ->Event("GetLodOverride", &MeshComponentRequestBus::Events::GetLodOverride)
                    ->Event("SetMinimumScreenCoverage", &MeshComponentRequestBus::Events::SetMinimumScreenCoverage)
                    ->Event("GetMinimumScreenCoverage", &MeshComponentRequestBus::Events::GetMinimumScreenCoverage)
                    ->Event("SetQualityDecayRate", &MeshComponentRequestBus::Events::SetQualityDecayRate)
                    ->Event("GetQualityDecayRate", &MeshComponentRequestBus::Events::GetQualityDecayRate)
                    ->Event("SetRayTracingEnabled", &MeshComponentRequestBus::Events::SetRayTracingEnabled)
                    ->Event("GetExcludeFromReflectionCubeMaps", &MeshComponentRequestBus::Events::GetExcludeFromReflectionCubeMaps)
                    ->Event("SetExcludeFromReflectionCubeMaps", &MeshComponentRequestBus::Events::SetExcludeFromReflectionCubeMaps)
                    ->Event("GetRayTracingEnabled", &MeshComponentRequestBus::Events::GetRayTracingEnabled)
                    ->Event("SetVisibility", &MeshComponentRequestBus::Events::SetVisibility)
                    ->Event("GetVisibility", &MeshComponentRequestBus::Events::GetVisibility)
                    ->VirtualProperty("ModelAssetId", "GetModelAssetId", "SetModelAssetId")
                    ->VirtualProperty("ModelAssetPath", "GetModelAssetPath", "SetModelAssetPath")
                    ->VirtualProperty("SortKey", "GetSortKey", "SetSortKey")
                    ->VirtualProperty("IsAlwaysDynamic", "GetIsAlwaysDynamic", "SetIsAlwaysDynamic")
                    ->VirtualProperty("LodType", "GetLodType", "SetLodType")
                    ->VirtualProperty("LodOverride", "GetLodOverride", "SetLodOverride")
                    ->VirtualProperty("MinimumScreenCoverage", "GetMinimumScreenCoverage", "SetMinimumScreenCoverage")
                    ->VirtualProperty("QualityDecayRate", "GetQualityDecayRate", "SetQualityDecayRate")
                    ->VirtualProperty("RayTracingEnabled", "GetRayTracingEnabled", "SetRayTracingEnabled")
                    ->VirtualProperty("ExcludeFromReflectionCubeMaps", "GetExcludeFromReflectionCubeMaps", "SetExcludeFromReflectionCubeMaps")
                    ->VirtualProperty("Visibility", "GetVisibility", "SetVisibility")
                    ;
                
                behaviorContext->EBus<MeshComponentNotificationBus>("MeshComponentNotificationBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "render")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Handler<Internal::MeshComponentNotificationBusHandler>();
            }
        }

        void MeshComponentController::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC_CE("TransformService"));
            dependent.push_back(AZ_CRC_CE("NonUniformScaleService"));
        }

        void MeshComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("MaterialConsumerService"));
            provided.push_back(AZ_CRC_CE("MeshService"));
        }

        void MeshComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("MaterialConsumerService"));
            incompatible.push_back(AZ_CRC_CE("MeshService"));
        }

        MeshComponentController::MeshComponentController(const MeshComponentConfig& config)
            : m_configuration(config)
        {
        }

        static AzFramework::EntityContextId FindOwningContextId(const AZ::EntityId entityId)
        {
            AzFramework::EntityContextId contextId = AzFramework::EntityContextId::CreateNull();
            AzFramework::EntityIdContextQueryBus::EventResult(
                contextId, entityId, &AzFramework::EntityIdContextQueries::GetOwningContextId);
            return contextId;
        }

        void MeshComponentController::Activate(const AZ::EntityComponentIdPair& entityComponentIdPair)
        {
            const AZ::EntityId entityId = entityComponentIdPair.GetEntityId();
            m_entityComponentIdPair = entityComponentIdPair;

            m_transformInterface = TransformBus::FindFirstHandler(entityId);
            AZ_Warning(
                "MeshComponentController", m_transformInterface,
                "Unable to attach to a TransformBus handler. This mesh will always be rendered at the origin.");

            m_meshFeatureProcessor = RPI::Scene::GetFeatureProcessorForEntity<MeshFeatureProcessorInterface>(entityId);
            AZ_Error("MeshComponentController", m_meshFeatureProcessor, "Unable to find a MeshFeatureProcessorInterface on the entityId.");

            m_cachedNonUniformScale = AZ::Vector3::CreateOne();
            AZ::NonUniformScaleRequestBus::EventResult(m_cachedNonUniformScale, entityId, &AZ::NonUniformScaleRequests::GetScale);
            AZ::NonUniformScaleRequestBus::Event(
                entityId, &AZ::NonUniformScaleRequests::RegisterScaleChangedEvent, m_nonUniformScaleChangedHandler);

            const auto entityContextId = FindOwningContextId(entityId);
            MeshComponentRequestBus::Handler::BusConnect(entityId);
            MeshHandleStateRequestBus::Handler::BusConnect(entityId);
            AtomImGuiTools::AtomImGuiMeshCallbackBus::Handler::BusConnect(entityId);
            TransformNotificationBus::Handler::BusConnect(entityId);
            MaterialConsumerRequestBus::Handler::BusConnect(entityId);
            MaterialComponentNotificationBus::Handler::BusConnect(entityId);
            AzFramework::BoundsRequestBus::Handler::BusConnect(entityId);
            AzFramework::VisibleGeometryRequestBus::Handler::BusConnect(entityId);
            AzFramework::RenderGeometry::IntersectionRequestBus::Handler::BusConnect({ entityId, entityContextId });
            AzFramework::RenderGeometry::IntersectionNotificationBus::Bind(m_intersectionNotificationBus, entityContextId);

            LightingChannelMaskChanged();

            // Buses must be connected before RegisterModel in case requests are made as a result of HandleModelChange
            RegisterModel();
        }

        void MeshComponentController::Deactivate()
        {
            // Buses must be disconnected after unregistering the model, otherwise they can't deliver the events during the process.
            UnregisterModel();

            AzFramework::RenderGeometry::IntersectionRequestBus::Handler::BusDisconnect();
            AzFramework::VisibleGeometryRequestBus::Handler::BusDisconnect();
            AzFramework::BoundsRequestBus::Handler::BusDisconnect();
            MaterialComponentNotificationBus::Handler::BusDisconnect();
            MaterialConsumerRequestBus::Handler::BusDisconnect();
            TransformNotificationBus::Handler::BusDisconnect();
            MeshComponentRequestBus::Handler::BusDisconnect();
            MeshHandleStateRequestBus::Handler::BusDisconnect();
            AtomImGuiTools::AtomImGuiMeshCallbackBus::Handler::BusDisconnect();

            m_nonUniformScaleChangedHandler.Disconnect();

            m_meshFeatureProcessor = nullptr;
            m_transformInterface = nullptr;
            m_entityComponentIdPair = AZ::EntityComponentIdPair(AZ::EntityId(), AZ::InvalidComponentId);
            m_configuration.m_modelAsset.Release();
        }

        void MeshComponentController::SetConfiguration(const MeshComponentConfig& config)
        {
            m_configuration = config;
        }

        const MeshComponentConfig& MeshComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void MeshComponentController::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
        {
            if (m_meshFeatureProcessor)
            {
                m_meshFeatureProcessor->SetTransform(m_meshHandle, world, m_cachedNonUniformScale);
            }

            // ensure the render geometry is kept in sync with any changes to the entity the mesh is on
            AzFramework::RenderGeometry::IntersectionNotificationBus::Event(
                m_intersectionNotificationBus, &AzFramework::RenderGeometry::IntersectionNotificationBus::Events::OnGeometryChanged,
                m_entityComponentIdPair.GetEntityId());
        }

        void MeshComponentController::HandleNonUniformScaleChange(const AZ::Vector3& nonUniformScale)
        {
            m_cachedNonUniformScale = nonUniformScale;
            if (m_meshFeatureProcessor)
            {
                m_meshFeatureProcessor->SetTransform(m_meshHandle, m_transformInterface->GetWorldTM(), m_cachedNonUniformScale);
            }
        }

        void MeshComponentController::LightingChannelMaskChanged()
        {
            if (m_meshFeatureProcessor)
            {
                m_meshFeatureProcessor->SetLightingChannelMask(m_meshHandle, m_configuration.m_lightingChannelConfig.GetLightingChannelMask());
            }
        }

        MaterialAssignmentLabelMap MeshComponentController::GetMaterialLabels() const
        {
            return GetMaterialSlotLabelsFromModelAsset(GetModelAsset());
        }

        MaterialAssignmentId MeshComponentController::FindMaterialAssignmentId(
            const MaterialAssignmentLodIndex lod, const AZStd::string& label) const
        {
            return GetMaterialSlotIdFromModelAsset(GetModelAsset(), lod, label);
        }

        MaterialAssignmentMap MeshComponentController::GetDefaultMaterialMap() const
        {
            return GetDefaultMaterialMapFromModelAsset(GetModelAsset());
        }

        AZStd::unordered_set<AZ::Name> MeshComponentController::GetModelUvNames() const
        {
            const Data::Instance<RPI::Model> model = GetModel();
            return model ? model->GetUvNames() : AZStd::unordered_set<AZ::Name>();
        }

        void MeshComponentController::OnMaterialsUpdated(const MaterialAssignmentMap& materials)
        {
            if (m_meshFeatureProcessor)
            {
                m_meshFeatureProcessor->SetCustomMaterials(m_meshHandle, ConvertToCustomMaterialMap(materials));
            }
        }

        void MeshComponentController::OnMaterialPropertiesUpdated([[maybe_unused]] const MaterialAssignmentMap& materials)
        {
            if (m_meshFeatureProcessor)
            {
                m_meshFeatureProcessor->SetRayTracingDirty(m_meshHandle);
            }
        }

        bool MeshComponentController::RequiresCloning(const Data::Asset<RPI::ModelAsset>& modelAsset)
        {
            // Is the model asset containing a cloth buffer? If yes, we need to clone the model asset for instancing.
            const AZStd::span<const AZ::Data::Asset<AZ::RPI::ModelLodAsset>> lodAssets = modelAsset->GetLodAssets();
            for (const AZ::Data::Asset<AZ::RPI::ModelLodAsset>& lodAsset : lodAssets)
            {
                const AZStd::span<const AZ::RPI::ModelLodAsset::Mesh> meshes = lodAsset->GetMeshes();
                for (const AZ::RPI::ModelLodAsset::Mesh& mesh : meshes)
                {
                    if (mesh.GetSemanticBufferAssetView(s_CLOTH_DATA_Name) != nullptr)
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        void MeshComponentController::HandleModelChange(const AZ::Data::Instance<AZ::RPI::Model>& model)
        {
            Data::Asset<RPI::ModelAsset> modelAsset = m_meshFeatureProcessor->GetModelAsset(m_meshHandle);
            if (model && modelAsset.IsReady())
            {
                const AZ::EntityId entityId = m_entityComponentIdPair.GetEntityId();
                m_configuration.m_modelAsset = modelAsset;
                MeshComponentNotificationBus::Event(entityId, &MeshComponentNotificationBus::Events::OnModelReady, m_configuration.m_modelAsset, model);
                MaterialConsumerNotificationBus::Event(entityId, &MaterialConsumerNotificationBus::Events::OnMaterialAssignmentSlotsChanged);
                AZ::Interface<AzFramework::IEntityBoundsUnion>::Get()->RefreshEntityLocalBoundsUnion(entityId);
                AzFramework::RenderGeometry::IntersectionNotificationBus::Event(
                    m_intersectionNotificationBus, &AzFramework::RenderGeometry::IntersectionNotificationBus::Events::OnGeometryChanged,
                    m_entityComponentIdPair.GetEntityId());

                MeshHandleStateNotificationBus::Event(entityId, &MeshHandleStateNotificationBus::Events::OnMeshHandleSet, &m_meshHandle);
            }
        }

        void MeshComponentController::HandleObjectSrgCreate(const Data::Instance<RPI::ShaderResourceGroup>& objectSrg)
        {
            MeshComponentNotificationBus::Event(m_entityComponentIdPair.GetEntityId(), &MeshComponentNotificationBus::Events::OnObjectSrgCreated, objectSrg);
        }

        void MeshComponentController::RegisterModel()
        {
            if (m_meshFeatureProcessor && m_configuration.m_modelAsset.GetId().IsValid())
            {
                const AZ::EntityId entityId = m_entityComponentIdPair.GetEntityId();

                MaterialAssignmentMap materials;
                MaterialComponentRequestBus::EventResult(materials, entityId, &MaterialComponentRequests::GetMaterialMap);

                m_meshFeatureProcessor->ReleaseMesh(m_meshHandle);
                MeshHandleDescriptor meshDescriptor;
                meshDescriptor.m_entityId = m_entityComponentIdPair.GetEntityId();
                meshDescriptor.m_modelAsset = m_configuration.m_modelAsset;
                meshDescriptor.m_customMaterials = ConvertToCustomMaterialMap(materials);
                meshDescriptor.m_useForwardPassIblSpecular = m_configuration.m_useForwardPassIblSpecular;
                meshDescriptor.m_requiresCloneCallback = RequiresCloning;
                meshDescriptor.m_isRayTracingEnabled = m_configuration.m_isRayTracingEnabled;
                meshDescriptor.m_excludeFromReflectionCubeMaps = m_configuration.m_excludeFromReflectionCubeMaps;
                meshDescriptor.m_isAlwaysDynamic = m_configuration.m_isAlwaysDynamic;
                meshDescriptor.m_supportRayIntersection = m_configuration.m_enableRayIntersection || m_configuration.m_editorRayIntersection;
                meshDescriptor.m_modelChangedEventHandler = m_modelChangedEventHandler;
                meshDescriptor.m_objectSrgCreatedHandler = m_objectSrgCreatedHandler;
                m_meshHandle = m_meshFeatureProcessor->AcquireMesh(meshDescriptor);

                const AZ::Transform& transform =
                    m_transformInterface ? m_transformInterface->GetWorldTM() : AZ::Transform::CreateIdentity();

                m_meshFeatureProcessor->SetTransform(m_meshHandle, transform, m_cachedNonUniformScale);
                m_meshFeatureProcessor->SetSortKey(m_meshHandle, m_configuration.m_sortKey);
                m_meshFeatureProcessor->SetLightingChannelMask(m_meshHandle, m_configuration.m_lightingChannelConfig.GetLightingChannelMask());
                m_meshFeatureProcessor->SetMeshLodConfiguration(m_meshHandle, GetMeshLodConfiguration());
                m_meshFeatureProcessor->SetVisible(m_meshHandle, m_isVisible);
                m_meshFeatureProcessor->SetRayTracingEnabled(m_meshHandle, meshDescriptor.m_isRayTracingEnabled);
            }
            else
            {
                // If there is no model asset to be loaded then we need to invalidate the material slot configuration
                MaterialConsumerNotificationBus::Event(
                    m_entityComponentIdPair.GetEntityId(), &MaterialConsumerNotificationBus::Events::OnMaterialAssignmentSlotsChanged);
            }
        }

        void MeshComponentController::UnregisterModel()
        {
            if (m_meshFeatureProcessor && m_meshHandle.IsValid())
            {
                MeshComponentNotificationBus::Event(
                    m_entityComponentIdPair.GetEntityId(), &MeshComponentNotificationBus::Events::OnModelPreDestroy);
                m_meshFeatureProcessor->ReleaseMesh(m_meshHandle);

                MeshHandleStateNotificationBus::Event(
                    m_entityComponentIdPair.GetEntityId(), &MeshHandleStateNotificationBus::Events::OnMeshHandleSet, &m_meshHandle);

                // Model has been released which invalidates the material slot configuration
                MaterialConsumerNotificationBus::Event(
                    m_entityComponentIdPair.GetEntityId(), &MaterialConsumerNotificationBus::Events::OnMaterialAssignmentSlotsChanged);
            }
        }

        void MeshComponentController::RefreshModelRegistration()
        {
            // [GFX TODO][ATOM-13364] Without the Suspend / Resume calls below, a model refresh will trigger an asset unload and reload
            // that breaks Material Thumbnail Previews in the Editor.  The asset unload/reload itself is undesirable, but the flow should
            // get investigated further to determine what state management and notifications need to be modified, since the previews ought
            // to still work even if a full asset reload were to occur here.

            // The unregister / register combination can cause the asset reference to get released, which could trigger a full reload
            // of the asset.  Tell the Asset Manager not to release any asset references until after the registration is complete.
            // This will ensure that if we're reusing the same model, it remains loaded.
            Data::AssetManager::Instance().SuspendAssetRelease();
            UnregisterModel();
            RegisterModel();
            Data::AssetManager::Instance().ResumeAssetRelease();
        }

        void MeshComponentController::SetModelAssetId(Data::AssetId modelAssetId)
        {
            SetModelAsset(Data::Asset<RPI::ModelAsset>(modelAssetId, azrtti_typeid<RPI::ModelAsset>()));
        }

        void MeshComponentController::SetModelAsset(Data::Asset<RPI::ModelAsset> modelAsset)
        {
            if (m_configuration.m_modelAsset != modelAsset)
            {
                m_configuration.m_modelAsset = modelAsset;
                m_configuration.m_modelAsset.SetAutoLoadBehavior(Data::AssetLoadBehavior::PreLoad);
                RefreshModelRegistration();
            }
        }

        Data::Asset<const RPI::ModelAsset> MeshComponentController::GetModelAsset() const
        {
            return m_configuration.m_modelAsset;
        }

        Data::AssetId MeshComponentController::GetModelAssetId() const
        {
            return m_configuration.m_modelAsset.GetId();
        }

        void MeshComponentController::SetModelAssetPath(const AZStd::string& modelAssetPath)
        {
            AZ::Data::AssetId assetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, modelAssetPath.c_str(), AZ::RPI::ModelAsset::RTTI_Type(), false);
            SetModelAssetId(assetId);
        }

        AZStd::string MeshComponentController::GetModelAssetPath() const
        {
            AZStd::string assetPathString;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPathString, &AZ::Data::AssetCatalogRequests::GetAssetPathById, m_configuration.m_modelAsset.GetId());
            return assetPathString;
        }

        Data::Instance<RPI::Model> MeshComponentController::GetModel() const
        {
            return m_meshFeatureProcessor ? m_meshFeatureProcessor->GetModel(m_meshHandle) : Data::Instance<RPI::Model>();
        }
        
        const RPI::MeshDrawPacketLods* MeshComponentController::GetDrawPackets() const
        {
            return m_meshFeatureProcessor ? &m_meshFeatureProcessor->GetDrawPackets(m_meshHandle) : nullptr;
        }

        void MeshComponentController::SetSortKey(RHI::DrawItemSortKey sortKey)
        {
            m_configuration.m_sortKey = sortKey; // Save for serialization
            m_meshFeatureProcessor->SetSortKey(m_meshHandle, sortKey);
        }

        RHI::DrawItemSortKey MeshComponentController::GetSortKey() const
        {
            return m_meshFeatureProcessor->GetSortKey(m_meshHandle);
        }

        void MeshComponentController::SetIsAlwaysDynamic(bool isAlwaysDynamic)
        {
            m_configuration.m_isAlwaysDynamic = isAlwaysDynamic; // Save for serialization
            m_meshFeatureProcessor->SetIsAlwaysDynamic(m_meshHandle, isAlwaysDynamic);
        }

        bool MeshComponentController::GetIsAlwaysDynamic() const
        {
            return m_meshFeatureProcessor->GetIsAlwaysDynamic(m_meshHandle);
        }

        RPI::Cullable::LodConfiguration MeshComponentController::GetMeshLodConfiguration() const
        {
            return {
                m_configuration.m_lodType,
                m_configuration.m_lodOverride,
                m_configuration.m_minimumScreenCoverage,
                m_configuration.m_qualityDecayRate
            };
        }
        // -----------------------
        void MeshComponentController::SetLodType(RPI::Cullable::LodType lodType)
        {
            RPI::Cullable::LodConfiguration lodConfig = GetMeshLodConfiguration();
            lodConfig.m_lodType = lodType;
            m_meshFeatureProcessor->SetMeshLodConfiguration(m_meshHandle, lodConfig);
        }

        RPI::Cullable::LodType MeshComponentController::GetLodType() const
        {
            RPI::Cullable::LodConfiguration lodConfig = m_meshFeatureProcessor->GetMeshLodConfiguration(m_meshHandle);
            return lodConfig.m_lodType;
        }

        void MeshComponentController::SetLodOverride(RPI::Cullable::LodOverride lodOverride)
        {
            RPI::Cullable::LodConfiguration lodConfig = GetMeshLodConfiguration();
            lodConfig.m_lodOverride = lodOverride;
            m_meshFeatureProcessor->SetMeshLodConfiguration(m_meshHandle, lodConfig);
        }

        RPI::Cullable::LodOverride MeshComponentController::GetLodOverride() const
        {
            RPI::Cullable::LodConfiguration lodConfig = m_meshFeatureProcessor->GetMeshLodConfiguration(m_meshHandle);
            return lodConfig.m_lodOverride;
        }

        void MeshComponentController::SetMinimumScreenCoverage(float minimumScreenCoverage)
        {
            RPI::Cullable::LodConfiguration lodConfig = GetMeshLodConfiguration();
            lodConfig.m_minimumScreenCoverage = minimumScreenCoverage;
            m_meshFeatureProcessor->SetMeshLodConfiguration(m_meshHandle, lodConfig);
        }

        float MeshComponentController::GetMinimumScreenCoverage() const
        {
            RPI::Cullable::LodConfiguration lodConfig = m_meshFeatureProcessor->GetMeshLodConfiguration(m_meshHandle);
            return lodConfig.m_minimumScreenCoverage;
        }

        void MeshComponentController::SetQualityDecayRate(float qualityDecayRate)
        {
            RPI::Cullable::LodConfiguration lodConfig = GetMeshLodConfiguration();
            lodConfig.m_qualityDecayRate = qualityDecayRate;
            m_meshFeatureProcessor->SetMeshLodConfiguration(m_meshHandle, lodConfig);
        }

        float MeshComponentController::GetQualityDecayRate() const
        {
            RPI::Cullable::LodConfiguration lodConfig = m_meshFeatureProcessor->GetMeshLodConfiguration(m_meshHandle);
            return lodConfig.m_qualityDecayRate;
        }

        void MeshComponentController::SetVisibility(bool visible)
        {
            if (m_isVisible != visible)
            {
                if (m_meshFeatureProcessor)
                {
                    m_meshFeatureProcessor->SetVisible(m_meshHandle, visible);
                }
                m_isVisible = visible;
            }
        }

        bool MeshComponentController::GetVisibility() const
        {
            return m_isVisible;
        }

        void MeshComponentController::SetRayTracingEnabled(bool enabled)
        {
            if (m_meshHandle.IsValid() && m_meshFeatureProcessor)
            {
                m_meshFeatureProcessor->SetRayTracingEnabled(m_meshHandle, enabled);
                m_configuration.m_isRayTracingEnabled = enabled;
            }
        }

        bool MeshComponentController::GetRayTracingEnabled() const
        {
            if (m_meshHandle.IsValid() && m_meshFeatureProcessor)
            {
                return m_meshFeatureProcessor->GetRayTracingEnabled(m_meshHandle);
            }

            return false;
        }

        void MeshComponentController::SetExcludeFromReflectionCubeMaps(bool excludeFromReflectionCubeMaps)
        {
            m_configuration.m_excludeFromReflectionCubeMaps = excludeFromReflectionCubeMaps;
            if (m_meshFeatureProcessor)
            {
                m_meshFeatureProcessor->SetExcludeFromReflectionCubeMaps(m_meshHandle, excludeFromReflectionCubeMaps);
            }
        }

        bool MeshComponentController::GetExcludeFromReflectionCubeMaps() const
        {
            if (m_meshHandle.IsValid() && m_meshFeatureProcessor)
            {
                return m_meshFeatureProcessor->GetExcludeFromReflectionCubeMaps(m_meshHandle);
            }

            return false;
        }

        Aabb MeshComponentController::GetWorldBounds() const
        {
            if (const AZ::Aabb localBounds = GetLocalBounds(); localBounds.IsValid())
            {
                return localBounds.GetTransformedAabb(m_transformInterface->GetWorldTM());
            }

            return AZ::Aabb::CreateNull();
        }

        Aabb MeshComponentController::GetLocalBounds() const
        {
            if (m_meshHandle.IsValid() && m_meshFeatureProcessor)
            {
                if (Aabb aabb = m_meshFeatureProcessor->GetLocalAabb(m_meshHandle); aabb.IsValid())
                {
                    aabb.MultiplyByScale(m_cachedNonUniformScale);
                    return aabb;
                }
            }

            return Aabb::CreateNull();
        }

        void MeshComponentController::BuildVisibleGeometry(
            const AZ::Aabb& bounds, AzFramework::VisibleGeometryContainer& geometryContainer) const
        {
            // Only include data for this entity if it is within bounds. This could possibly be done per sub mesh.
            if (bounds.IsValid())
            {
                const AZ::Aabb worldBounds = GetWorldBounds();
                if (worldBounds.IsValid() && !worldBounds.Overlaps(bounds))
                {
                    return;
                }
            }

            // The draw list tag is needed to search material shaders and determine if they are transparent.
            AZ::RHI::DrawListTag transparentDrawListTag =
                AZ::RHI::RHISystemInterface::Get()->GetDrawListTagRegistry()->AcquireTag(AZ::Name("transparent"));

            // Retrieve the map of material overrides from the material component. If any mesh has a material override, that must be checked
            // for transparency instead of the material included with the model asset.
            MaterialAssignmentMap materials;
            MaterialComponentRequestBus::EventResult(
                materials, m_entityComponentIdPair.GetEntityId(), &MaterialComponentRequests::GetMaterialMap);

            // Attempt to copy the triangle list geometry data out of the model asset into the visible geometry structure
            const auto& modelAsset = GetModelAsset();
            if (!modelAsset.IsReady() || modelAsset->GetLodAssets().empty())
            {
                AZ_Warning("MeshComponentController", false, "Unable to get geometry because mesh asset is not ready or empty.");
                return;
            }

            // This will only extract data from the first LOD. It might be necessary to make the LOD selectable.
            const int lodIndex = 0;
            const auto& lodAsset = modelAsset->GetLodAssets().front();
            if (!lodAsset.IsReady())
            {
                AZ_Warning("MeshComponentController", false, "Unable to get geometry because selected LOD asset is not ready.");
                return;
            }

            const AZ::Name positionName{ "POSITION" };
            for (const auto& mesh : lodAsset->GetMeshes())
            {
                // Get the index buffer data, confirming that the asset is valid and indices are 32 bit integers. Other formats are
                // currently not supported.
                const AZ::RPI::BufferAssetView& indexBufferView = mesh.GetIndexBufferAssetView();
                const AZ::RHI::BufferViewDescriptor& indexBufferViewDesc = indexBufferView.GetBufferViewDescriptor();
                const AZ::Data::Asset<AZ::RPI::BufferAsset> indexBufferAsset = indexBufferView.GetBufferAsset();
                if (!indexBufferAsset.IsReady() || indexBufferViewDesc.m_elementSize != sizeof(uint32_t))
                {
                    AZ_Warning("MeshComponentController", false, "Unable to get geometry for mesh because index buffer asset is not ready or is an incompatible format.");
                    continue;
                }

                // Get the position buffer data, if it exists with the expected name.
                const AZ::RPI::BufferAssetView* positionBufferView = mesh.GetSemanticBufferAssetView(positionName);
                if (!positionBufferView)
                {
                    AZ_Warning("MeshComponentController", false, "Unable to get geometry for mesh because position buffer data was not found.");
                    continue;
                }

                // Confirm that the position buffer is valid and contains 3 32 bit floats for each position. Other formats are currently not
                // supported.
                constexpr uint32_t ElementsPerVertex = 3;
                const AZ::RHI::BufferViewDescriptor& positionBufferViewDesc = positionBufferView->GetBufferViewDescriptor();
                const AZ::Data::Asset<AZ::RPI::BufferAsset> positionBufferAsset = positionBufferView->GetBufferAsset();
                if (!positionBufferAsset.IsReady() || positionBufferViewDesc.m_elementSize != sizeof(float) * ElementsPerVertex)
                {
                    AZ_Warning("MeshComponentController", false, "Unable to get geometry for mesh because position buffer asset is not ready or is an incompatible format.");
                    continue;
                }

                const AZStd::span<const uint8_t> indexRawBuffer = indexBufferAsset->GetBuffer();
                const uint32_t* indexPtr = reinterpret_cast<const uint32_t*>(
                    indexRawBuffer.data() + (indexBufferViewDesc.m_elementOffset * indexBufferViewDesc.m_elementSize));

                const AZStd::span<const uint8_t> positionRawBuffer = positionBufferAsset->GetBuffer();
                const float* positionPtr = reinterpret_cast<const float*>(
                    positionRawBuffer.data() + (positionBufferViewDesc.m_elementOffset * positionBufferViewDesc.m_elementSize));

                // Copy the index and position data into the visible geometry structure.
                AzFramework::VisibleGeometry visibleGeometry;
                visibleGeometry.m_transform = AZ::Matrix4x4::CreateFromTransform(m_transformInterface->GetWorldTM());
                visibleGeometry.m_transform *= AZ::Matrix4x4::CreateScale(m_cachedNonUniformScale);

                // Reserve space for indices and copy data, assuming stride between elements is 0.
                visibleGeometry.m_indices.resize_no_construct(indexBufferViewDesc.m_elementCount);

                AZ_Assert(
                    (sizeof(visibleGeometry.m_indices[0]) * visibleGeometry.m_indices.size()) >=
                    (indexBufferViewDesc.m_elementSize * indexBufferViewDesc.m_elementCount),
                    "Index buffer size exceeds memory allocated for visible geometry indices.");

                memcpy(&visibleGeometry.m_indices[0], indexPtr, indexBufferViewDesc.m_elementCount * indexBufferViewDesc.m_elementSize);

                // Reserve space for vertices and copy data, assuming stride between elements is 0.
                visibleGeometry.m_vertices.resize_no_construct(positionBufferViewDesc.m_elementCount * ElementsPerVertex);

                AZ_Assert(
                    (sizeof(visibleGeometry.m_vertices[0]) * visibleGeometry.m_vertices.size()) >=
                    (positionBufferViewDesc.m_elementSize * positionBufferViewDesc.m_elementCount),
                    "Position buffer size exceeds memory allocated for visible geometry vertices.");

                memcpy(&visibleGeometry.m_vertices[0], positionPtr, positionBufferViewDesc.m_elementCount * positionBufferViewDesc.m_elementSize);

                // Inspect the material assigned to this mesh to determine if it should be considered transparent.
                const auto& materialSlotId = mesh.GetMaterialSlotId();
                const auto& materialSlot = modelAsset->FindMaterialSlot(materialSlotId);

                // The material asset assigned by the model will be used by default.
                AZ::Data::Instance<AZ::RPI::Material> material = AZ::RPI::Material::FindOrCreate(materialSlot.m_defaultMaterialAsset);

                // Materials provided by the material component take priority over materials provided by the model asset.
                const MaterialAssignmentId id(lodIndex, materialSlotId);
                const MaterialAssignmentId ignoreLodId(DefaultCustomMaterialLodIndex, materialSlotId);
                for (const auto& currentId : { id, ignoreLodId, DefaultMaterialAssignmentId })
                {
                    if (auto itr = materials.find(currentId); itr != materials.end() && itr->second.m_materialInstance)
                    {
                        material = itr->second.m_materialInstance;
                        break;
                    }
                }

                // Once the active material has been resolved, determine if it has any shaders with the transparent tag.
                visibleGeometry.m_transparent = DoesMaterialUseDrawListTag(material, transparentDrawListTag);

                geometryContainer.emplace_back(AZStd::move(visibleGeometry));
            }

            // Release the draw list tag acquired at the top of the function to determine material transparency.
            AZ::RHI::RHISystemInterface::Get()->GetDrawListTagRegistry()->ReleaseTag(transparentDrawListTag);
        }

        bool MeshComponentController::DoesMaterialUseDrawListTag(
            const AZ::Data::Instance<AZ::RPI::Material> material, const AZ::RHI::DrawListTag searchDrawListTag) const
        {
            bool foundTag = false;

            if (material)
            {
                material->ForAllShaderItems(
                    [&](const AZ::Name&, const AZ::RPI::ShaderCollection::Item& shaderItem)
                    {
                        if (shaderItem.IsEnabled())
                        {
                            // Get the DrawListTag. Use the explicit draw list override if exists.
                            AZ::RHI::DrawListTag drawListTag = shaderItem.GetDrawListTagOverride();

                            if (drawListTag.IsNull())
                            {
                                drawListTag = AZ::RHI::RHISystemInterface::Get()->GetDrawListTagRegistry()->FindTag(
                                    shaderItem.GetShaderAsset()->GetDrawListName());
                            }

                            // If this shader has a matching tag end the search.
                            if (drawListTag == searchDrawListTag)
                            {
                                foundTag = true;
                                return false;
                            }
                        }

                        // Continue iterating until all shaders have been checked or a matching tag is found. 
                        return true;
                    });
            }

            return foundTag;
        }

        AzFramework::RenderGeometry::RayResult MeshComponentController::RenderGeometryIntersect(
            const AzFramework::RenderGeometry::RayRequest& ray) const
        {
            AzFramework::RenderGeometry::RayResult result;
            if (const Data::Instance<RPI::Model> model = GetModel())
            {
                float t;
                AZ::Vector3 normal;
                if (model->RayIntersection(
                        m_transformInterface->GetWorldTM(), m_cachedNonUniformScale, ray.m_startWorldPosition,
                        ray.m_endWorldPosition - ray.m_startWorldPosition, t, normal))
                {
                    // fill in ray result structure after successful intersection
                    const auto intersectionLine = (ray.m_endWorldPosition - ray.m_startWorldPosition);
                    result.m_uv = AZ::Vector2::CreateZero();
                    result.m_worldPosition = ray.m_startWorldPosition + intersectionLine * t;
                    result.m_worldNormal = normal;
                    result.m_distance = intersectionLine.GetLength() * t;
                    result.m_entityAndComponent = m_entityComponentIdPair;
                }
            }

            return result;
        }

        const MeshFeatureProcessorInterface::MeshHandle* MeshComponentController::GetMeshHandle() const
        {
            return &m_meshHandle;
        }
    } // namespace Render
} // namespace AZ
