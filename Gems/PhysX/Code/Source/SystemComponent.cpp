/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Source/SystemComponent.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <PhysX/MeshAsset.h>
#include <PhysX/HeightFieldAsset.h>
#include <Source/Utils.h>
#include <Source/Collision.h>
#include <Source/Shape.h>
#include <Source/Pipeline/MeshAssetHandler.h>
#include <Source/Pipeline/HeightFieldAssetHandler.h>
#include <Source/PhysXCharacters/API/CharacterUtils.h>
#include <Source/PhysXCharacters/API/CharacterController.h>
#include <Source/WindProvider.h>

#include <PhysX/Debug/PhysXDebugInterface.h>
#include <System/PhysXSystem.h>

namespace PhysX
{
    bool SystemComponent::VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement)
    {
        using GlobalCollisionDebugState = Debug::DebugDisplayData::GlobalCollisionDebugState;

        if (classElement.GetVersion() <= 1)
        {
            const int pvdTransportTypeElemIndex = classElement.FindElement(AZ_CRC("PvdTransportType", 0x91e0b21e));

            if (pvdTransportTypeElemIndex >= 0)
            {
                Debug::PvdTransportType pvdTransportTypeValue;
                AZ::SerializeContext::DataElementNode& pvdTransportElement = classElement.GetSubElement(pvdTransportTypeElemIndex);
                pvdTransportElement.GetData<Debug::PvdTransportType>(pvdTransportTypeValue);

                if (pvdTransportTypeValue == static_cast<Debug::PvdTransportType>(2))
                {
                    // version 2 removes Disabled (2) value default to Network instead.
                    const bool success = pvdTransportElement.SetData<Debug::PvdTransportType>(context, Debug::PvdTransportType::Network);
                    if (!success)
                    {
                        return false;
                    }
                }
            }
        }

        if (classElement.GetVersion() <= 2)
        {
            const int globalColliderDebugDrawElemIndex = classElement.FindElement(AZ_CRC("GlobalColliderDebugDraw", 0xca73ed43));

            if (globalColliderDebugDrawElemIndex >= 0)
            {
                bool oldGlobalColliderDebugDrawElemDebug = false;
                AZ::SerializeContext::DataElementNode& globalColliderDebugDrawElem = classElement.GetSubElement(globalColliderDebugDrawElemIndex);
                // Previously globalColliderDebugDraw was a bool indicating whether to always draw debug or to manually set on the element
                if (!globalColliderDebugDrawElem.GetData<bool>(oldGlobalColliderDebugDrawElemDebug))
                {
                    return false;
                }
                classElement.RemoveElement(globalColliderDebugDrawElemIndex);
                const GlobalCollisionDebugState newValue = oldGlobalColliderDebugDrawElemDebug ? GlobalCollisionDebugState::AlwaysOn : GlobalCollisionDebugState::Manual;
                classElement.AddElementWithData(context, "GlobalColliderDebugDraw", newValue);
            }
        }

        if (classElement.GetVersion() <= 3)
        {
            classElement.AddElementWithData(context, "ShowJointHierarchy", true);
            classElement.AddElementWithData(context, "JointHierarchyLeadColor",
                Debug::DebugDisplayData::JointLeadColor::Aquamarine);
            classElement.AddElementWithData(context, "JointHierarchyFollowerColor",
                Debug::DebugDisplayData::JointFollowerColor::Magenta);
            classElement.AddElementWithData(context, "jointHierarchyDistanceThreshold", 1.0f);
        }

        return true;
    }

    void SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        Pipeline::MeshAsset::Reflect(context);

        PhysX::ReflectionUtils::ReflectPhysXOnlyApi(context);

        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SystemComponent, AZ::Component>()
                ->Version(1)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("AssetBuilder") }))
                ->Field("Enabled", &SystemComponent::m_enabled)
            ;

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<SystemComponent>("PhysX", "Global PhysX physics configuration.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SystemComponent::m_enabled,
                    "Enabled", "Enables the PhysX system component.")
                ;
            }
        }
    }

    void SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("PhysXService", 0x75beae2d));
    }

    void SystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("PhysXService", 0x75beae2d));
    }

    void SystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void SystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("AssetDatabaseService"));
        dependent.push_back(AZ_CRC_CE("AssetCatalogService"));
    }

    SystemComponent::SystemComponent()
        : m_enabled(true)
        , m_onSystemInitializedHandler(
            [this](const AzPhysics::SystemConfiguration* config)
            {
                EnableAutoManagedPhysicsTick(config->m_autoManageSimulationUpdate);
            })
        , m_onSystemConfigChangedHandler(
            [this](const AzPhysics::SystemConfiguration* config)
            {
                EnableAutoManagedPhysicsTick(config->m_autoManageSimulationUpdate);
            })
    {
    }

    SystemComponent::~SystemComponent()
    {
        if (m_collisionRequests.Get() == this)
        {
            m_collisionRequests.Unregister(this);
        }
        if (m_physicsSystem.Get() == this)
        {
            m_physicsSystem.Unregister(this);
        }
    }

    // AZ::Component interface implementation
    void SystemComponent::Init()
    {
        //Old API interfaces
        if (m_collisionRequests.Get() == nullptr)
        {
            m_collisionRequests.Register(this);
        }
        if (m_physicsSystem.Get() == nullptr)
        {
            m_physicsSystem.Register(this);
        }
    }

    template<typename AssetHandlerT, typename AssetT>
    void RegisterAsset(AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler>>& assetHandlers)
    {
        AssetHandlerT* handler = aznew AssetHandlerT();
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::EnableCatalogForAsset, AZ::AzTypeInfo<AssetT>::Uuid());
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddExtension, AssetHandlerT::s_assetFileExtension);
        assetHandlers.emplace_back(handler);
    }

    void SystemComponent::Activate()
    {
        if (!m_enabled)
        {
            return;
        }

        m_materialManager.Connect();
        m_defaultWorldComponent.Activate();

        // Assets related work
        auto* materialAsset = aznew AzFramework::GenericAssetHandler<Physics::MaterialLibraryAsset>("Physics Material", "Physics", "physmaterial");
        materialAsset->Register();
        m_assetHandlers.emplace_back(materialAsset);

        // Add asset types and extensions to AssetCatalog. Uses "AssetCatalogService".
        RegisterAsset<Pipeline::MeshAssetHandler, Pipeline::MeshAsset>(m_assetHandlers);
        RegisterAsset<Pipeline::HeightFieldAssetHandler, Pipeline::HeightFieldAsset>(m_assetHandlers);

        // Connect to relevant buses
        Physics::SystemRequestBus::Handler::BusConnect();
        PhysX::SystemRequestsBus::Handler::BusConnect();
        Physics::CollisionRequestBus::Handler::BusConnect();

        ActivatePhysXSystem();
    }

    void SystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        Physics::CollisionRequestBus::Handler::BusDisconnect();
        PhysX::SystemRequestsBus::Handler::BusDisconnect();
        Physics::SystemRequestBus::Handler::BusDisconnect();

        // Reset material manager
        m_materialManager.ReleaseAllMaterials();

        m_defaultWorldComponent.Deactivate();
        m_materialManager.Disconnect();

        m_windProvider.reset();

        m_onSystemInitializedHandler.Disconnect();
        m_onSystemConfigChangedHandler.Disconnect();
        if (m_physXSystem != nullptr)
        {
            m_physXSystem->Shutdown();
            m_physXSystem = nullptr;
        }
        m_assetHandlers.clear(); //this need to be after m_physXSystem->Shutdown(); For it will drop the default material library reference.
    }

    physx::PxConvexMesh* SystemComponent::CreateConvexMesh(const void* vertices, AZ::u32 vertexNum, AZ::u32 vertexStride)
    {
        physx::PxConvexMeshDesc desc;
        desc.points.data = vertices;
        desc.points.count = vertexNum;
        desc.points.stride = vertexStride;
        // we provide points only, therefore the PxConvexFlag::eCOMPUTE_CONVEX flag must be specified
        desc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX;

        //TEMP until this in moved over
        physx::PxConvexMesh* convex = m_physXSystem->GetPxCooking()->createConvexMesh(desc,
            m_physXSystem->GetPxPhysics()->getPhysicsInsertionCallback());
        AZ_Error("PhysX", convex, "Error. Unable to create convex mesh");

        return convex;
    }

    physx::PxHeightField* SystemComponent::CreateHeightField(const physx::PxHeightFieldSample* samples, AZ::u32 numRows, AZ::u32 numColumns)
    {
        physx::PxHeightFieldDesc desc;
        desc.format = physx::PxHeightFieldFormat::eS16_TM;
        desc.nbColumns = numColumns;
        desc.nbRows = numRows;
        desc.samples.data = samples;
        desc.samples.stride = sizeof(physx::PxHeightFieldSample);

        physx::PxHeightField* heightfield =
            m_physXSystem->GetPxCooking()->createHeightField(desc, m_physXSystem->GetPxPhysics()->getPhysicsInsertionCallback());
        AZ_Error("PhysX", heightfield, "Error. Unable to create heightfield");

        return heightfield;
    }

    bool SystemComponent::CookConvexMeshToFile(const AZStd::string& filePath, const AZ::Vector3* vertices, AZ::u32 vertexCount)
    {
        AZStd::vector<AZ::u8> physxData;

        if (CookConvexMeshToMemory(vertices, vertexCount, physxData))
        {
            return Utils::WriteCookedMeshToFile(filePath, physxData, 
                Physics::CookedMeshShapeConfiguration::MeshType::Convex);
        }

        AZ_Error("PhysX", false, "CookConvexMeshToFile. Convex cooking failed for %s.", filePath.c_str());
        return false;
    }

    bool SystemComponent::CookTriangleMeshToFile(const AZStd::string& filePath, const AZ::Vector3* vertices, AZ::u32 vertexCount,
        const AZ::u32* indices, AZ::u32 indexCount)
    {   
        AZStd::vector<AZ::u8> physxData;

        if (CookTriangleMeshToMemory(vertices, vertexCount, indices, indexCount, physxData))
        {
            return Utils::WriteCookedMeshToFile(filePath, physxData,
                Physics::CookedMeshShapeConfiguration::MeshType::TriangleMesh);
        }

        AZ_Error("PhysX", false, "CookTriangleMeshToFile. Mesh cooking failed for %s.", filePath.c_str());
        return false;
    }

    bool SystemComponent::CookConvexMeshToMemory(const AZ::Vector3* vertices, AZ::u32 vertexCount, AZStd::vector<AZ::u8>& result)
    {
        physx::PxDefaultMemoryOutputStream memoryStream;
        
        bool cookingResult = Utils::CookConvexToPxOutputStream(vertices, vertexCount, memoryStream);
        
        if(cookingResult)
        {
            result.insert(result.end(), memoryStream.getData(), memoryStream.getData() + memoryStream.getSize());
        }
        
        return cookingResult;
    }

    bool SystemComponent::CookTriangleMeshToMemory(const AZ::Vector3* vertices, AZ::u32 vertexCount,
        const AZ::u32* indices, AZ::u32 indexCount, AZStd::vector<AZ::u8>& result)
    {
        physx::PxDefaultMemoryOutputStream memoryStream;
        bool cookingResult = Utils::CookTriangleMeshToToPxOutputStream(vertices, vertexCount, indices, indexCount, memoryStream);

        if (cookingResult)
        {
            result.insert(result.end(), memoryStream.getData(), memoryStream.getData() + memoryStream.getSize());
        }

        return cookingResult;
    }

    physx::PxConvexMesh* SystemComponent::CreateConvexMeshFromCooked(const void* cookedMeshData, AZ::u32 bufferSize)
    {
        physx::PxDefaultMemoryInputData inpStream(reinterpret_cast<physx::PxU8*>(const_cast<void*>(cookedMeshData)), bufferSize);
        //TEMP until this in moved over
        return m_physXSystem->GetPxPhysics()->createConvexMesh(inpStream);
    }

    physx::PxTriangleMesh* SystemComponent::CreateTriangleMeshFromCooked(const void* cookedMeshData, AZ::u32 bufferSize)
    {
        physx::PxDefaultMemoryInputData inpStream(reinterpret_cast<physx::PxU8*>(const_cast<void*>(cookedMeshData)), bufferSize);
        //TEMP until this in moved over
        return m_physXSystem->GetPxPhysics()->createTriangleMesh(inpStream);
    }

    AZStd::shared_ptr<Physics::Shape> SystemComponent::CreateShape(const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& configuration)
    {
        auto shapePtr = AZStd::make_shared<PhysX::Shape>(colliderConfiguration, configuration);

        if (shapePtr->GetPxShape())
        {
            return shapePtr;
        }

        AZ_Error("PhysX", false, "SystemComponent::CreateShape error. Unable to create a shape from configuration.");

        return nullptr;
    }

    AZStd::shared_ptr<Physics::Material> SystemComponent::CreateMaterial(const Physics::MaterialConfiguration& materialConfiguration)
    {
        return AZStd::make_shared<PhysX::Material>(materialConfiguration);
    }

    void SystemComponent::ReleaseNativeHeightfieldObject(void* nativeHeightfieldObject)
    {
        if (nativeHeightfieldObject)
        {
            static_cast<physx::PxBase*>(nativeHeightfieldObject)->release();
        }
    }

    void SystemComponent::ReleaseNativeMeshObject(void* nativeMeshObject)
    {
        if (nativeMeshObject)
        {
            static_cast<physx::PxBase*>(nativeMeshObject)->release();
        }
    }

    AzPhysics::CollisionLayer SystemComponent::GetCollisionLayerByName(const AZStd::string& layerName)
    {
        return m_physXSystem->GetPhysXConfiguration().m_collisionConfig.m_collisionLayers.GetLayer(layerName);
    }

    AZStd::string SystemComponent::GetCollisionLayerName(const AzPhysics::CollisionLayer& layer)
    {
        return m_physXSystem->GetPhysXConfiguration().m_collisionConfig.m_collisionLayers.GetName(layer);
    }

    bool SystemComponent::TryGetCollisionLayerByName(const AZStd::string& layerName, AzPhysics::CollisionLayer& layer)
    {
        return m_physXSystem->GetPhysXConfiguration().m_collisionConfig.m_collisionLayers.TryGetLayer(layerName, layer);
    }

    AzPhysics::CollisionGroup SystemComponent::GetCollisionGroupByName(const AZStd::string& groupName)
    {
        return m_physXSystem->GetPhysXConfiguration().m_collisionConfig.m_collisionGroups.FindGroupByName(groupName);
    }

    bool SystemComponent::TryGetCollisionGroupByName(const AZStd::string& groupName, AzPhysics::CollisionGroup& group)
    {
        return m_physXSystem->GetPhysXConfiguration().m_collisionConfig.m_collisionGroups.TryFindGroupByName(groupName, group);
    }

    AZStd::string SystemComponent::GetCollisionGroupName(const AzPhysics::CollisionGroup& collisionGroup)
    {
        AZStd::string groupName;
        for (const auto& group : m_physXSystem->GetPhysXConfiguration().m_collisionConfig.m_collisionGroups.GetPresets())
        {
            if (group.m_group.GetMask() == collisionGroup.GetMask())
            {
                groupName = group.m_name;
                break;
            }
        }
        return groupName;
    }

    AzPhysics::CollisionGroup SystemComponent::GetCollisionGroupById(const AzPhysics::CollisionGroups::Id& groupId)
    {
        return m_physXSystem->GetPhysXConfiguration().m_collisionConfig.m_collisionGroups.FindGroupById(groupId);
    }

    void SystemComponent::SetCollisionLayerName(int index, const AZStd::string& layerName)
    {
        m_physXSystem->SetCollisionLayerName(aznumeric_cast<int>(index), layerName);
    }

    void SystemComponent::CreateCollisionGroup(const AZStd::string& groupName, const AzPhysics::CollisionGroup& group)
    {
        m_physXSystem->CreateCollisionGroup(groupName, group);
    }

    physx::PxFilterData SystemComponent::CreateFilterData(const AzPhysics::CollisionLayer& layer, const AzPhysics::CollisionGroup& group)
    {
        return PhysX::Collision::CreateFilterData(layer, group);
    }

    physx::PxCooking* SystemComponent::GetCooking()
    {
        //TEMP until this in moved over
        return m_physXSystem->GetPxCooking();
    }

    void SystemComponent::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (m_physXSystem)
        {
            m_physXSystem->Simulate(deltaTime);
        }
    }

    int SystemComponent::GetTickOrder()
    {
        return AZ::ComponentTickBus::TICK_PHYSICS_SYSTEM;
    }

    void SystemComponent::EnableAutoManagedPhysicsTick(bool shouldTick)
    {
        if (shouldTick && !m_isTickingPhysics)
        {
            AZ::TickBus::Handler::BusConnect();
        }
        else if (!shouldTick && m_isTickingPhysics)
        {
            AZ::TickBus::Handler::BusDisconnect();
        }
        m_isTickingPhysics = shouldTick;
    }

    void SystemComponent::ActivatePhysXSystem()
    {
        m_physXSystem = GetPhysXSystem();
        if (m_physXSystem)
        {
            m_physXSystem->RegisterSystemInitializedEvent(m_onSystemInitializedHandler);
            m_physXSystem->RegisterSystemConfigurationChangedEvent(m_onSystemConfigChangedHandler);
            const PhysXSettingsRegistryManager& registryManager = m_physXSystem->GetSettingsRegistryManager();
            if (AZStd::optional<PhysXSystemConfiguration> config = registryManager.LoadSystemConfiguration();
                config.has_value())
            {
                m_physXSystem->Initialize(&(*config));
            }
            else //load defaults if there is no config
            {
                const PhysXSystemConfiguration defaultConfig = PhysXSystemConfiguration::CreateDefault();
                m_physXSystem->Initialize(&defaultConfig);

                auto saveCallback = []([[maybe_unused]] const PhysXSystemConfiguration& config, [[maybe_unused]] PhysXSettingsRegistryManager::Result result)
                {
                    AZ_Warning("PhysX", result == PhysXSettingsRegistryManager::Result::Success,
                        "Unable to save the default PhysX configuration.");
                };
                registryManager.SaveSystemConfiguration(defaultConfig, saveCallback);
            }

            //Load the DefaultSceneConfig
            if (AZStd::optional<AzPhysics::SceneConfiguration> config = registryManager.LoadDefaultSceneConfiguration();
                config.has_value())
            {
                m_physXSystem->UpdateDefaultSceneConfiguration((*config));
            }
            else //load defaults if there is no config
            {
                const AzPhysics::SceneConfiguration defaultConfig = AzPhysics::SceneConfiguration::CreateDefault();
                m_physXSystem->UpdateDefaultSceneConfiguration(defaultConfig);

                auto saveCallback = []([[maybe_unused]] const AzPhysics::SceneConfiguration& config, [[maybe_unused]] PhysXSettingsRegistryManager::Result result)
                {
                    AZ_Warning("PhysX", result == PhysXSettingsRegistryManager::Result::Success,
                        "Unable to save the default Scene configuration.");
                };
                registryManager.SaveDefaultSceneConfiguration(defaultConfig, saveCallback);
            }

            //load the debug configuration and initialize the PhysX debug interface
            if (auto* debug = AZ::Interface<Debug::PhysXDebugInterface>::Get())
            {
                if (AZStd::optional<Debug::DebugConfiguration> config = registryManager.LoadDebugConfiguration();
                    config.has_value())
                {
                    debug->Initialize(*config);
                }
                else //load defaults if there is no config
                {
                    const Debug::DebugConfiguration defaultConfig = Debug::DebugConfiguration::CreateDefault();
                    debug->Initialize(defaultConfig);

                    auto saveCallback = []([[maybe_unused]] const Debug::DebugConfiguration& config, [[maybe_unused]] PhysXSettingsRegistryManager::Result result)
                    {
                        AZ_Warning("PhysX", result == PhysXSettingsRegistryManager::Result::Success,
                            "Unable to save the default PhysX Debug configuration.");
                    };
                    registryManager.SaveDebugConfiguration(defaultConfig, saveCallback);
                }
            }
        }

        m_windProvider = AZStd::make_unique<WindProvider>();
    }
} // namespace PhysX
