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

#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <gmock/gmock.h>

namespace Physics
{
    class MockPhysicsSystem
        : public Physics::SystemRequestBus::Handler
        , AZ::Interface<Physics::System>::Registrar
    {
    public:
        MockPhysicsSystem()
        {
            BusConnect();
        }
        ~MockPhysicsSystem()
        {
            BusDisconnect();
        }
        MOCK_CONST_METHOD0(GetDefaultWorldConfiguration, const Physics::WorldConfiguration&());
        MOCK_METHOD1(SetDefaultWorldConfiguration, void(const Physics::WorldConfiguration& worldConfiguration));
        MOCK_METHOD1(CreateWorld, AZStd::shared_ptr<Physics::World>(AZ::Crc32 id));
        MOCK_METHOD2(CreateWorldCustom, AZStd::shared_ptr<Physics::World>(AZ::Crc32 id, const Physics::WorldConfiguration& settings));
        MOCK_METHOD1(CreateStaticRigidBody, AZStd::unique_ptr<Physics::RigidBodyStatic>(const Physics::WorldBodyConfiguration& configuration));
        MOCK_METHOD1(CreateRigidBody, AZStd::unique_ptr<Physics::RigidBody>(const Physics::RigidBodyConfiguration& configuration));
        MOCK_METHOD2(CreateShape, AZStd::shared_ptr<Physics::Shape>(const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& configuration));
        MOCK_METHOD4(AddColliderComponentToEntity, void(AZ::Entity* entity, const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& shapeConfiguration, bool addEditorComponents));
        MOCK_METHOD1(ReleaseNativeMeshObject, void(void* nativeMeshObject));
        MOCK_METHOD1(CreateMaterial, AZStd::shared_ptr<Physics::Material>(const Physics::MaterialConfiguration& materialConfiguration));
        MOCK_METHOD0(GetDefaultMaterial, AZStd::shared_ptr<Physics::Material>());
        MOCK_METHOD1(CreateMaterialsFromLibrary, AZStd::vector<AZStd::shared_ptr<Physics::Material>>(const Physics::MaterialSelection& materialSelection));
        MOCK_METHOD0(LoadDefaultMaterialLibrary, bool());
        MOCK_METHOD2(UpdateMaterialSelection, bool(const Physics::ShapeConfiguration& shapeConfiguration, Physics::ColliderConfiguration& colliderConfiguration));
        MOCK_METHOD0(GetSupportedJointTypes, AZStd::vector<AZ::TypeId>());
        MOCK_METHOD1(CreateJointLimitConfiguration, AZStd::shared_ptr<Physics::JointLimitConfiguration>(AZ::TypeId jointType));
        MOCK_METHOD3(CreateJoint, AZStd::shared_ptr<Physics::Joint>(const AZStd::shared_ptr<Physics::JointLimitConfiguration>& configuration, Physics::WorldBody* parentBody, Physics::WorldBody* childBody));
        MOCK_METHOD10(GenerateJointLimitVisualizationData, void(const Physics::JointLimitConfiguration& configuration, const AZ::Quaternion& parentRotation, const AZ::Quaternion& childRotation, float scale, AZ::u32 angularSubdivisions, AZ::u32 radialSubdivisions, AZStd::vector<AZ::Vector3>& vertexBufferOut, AZStd::vector<AZ::u32>& indexBufferOut, AZStd::vector<AZ::Vector3>& lineBufferOut, AZStd::vector<bool>& lineValidityBufferOut));
        MOCK_METHOD5(ComputeInitialJointLimitConfiguration, AZStd::unique_ptr<Physics::JointLimitConfiguration>(const AZ::TypeId& jointLimitTypeId, const AZ::Quaternion& parentWorldRotation, const AZ::Quaternion& childWorldRotation, const AZ::Vector3& axis, const AZStd::vector<AZ::Quaternion>& exampleLocalRotations));
        MOCK_METHOD3(CookConvexMeshToFile, bool(const AZStd::string& filePath, const AZ::Vector3* vertices, AZ::u32 vertexCount));
        MOCK_METHOD3(CookConvexMeshToMemory, bool(const AZ::Vector3* vertices, AZ::u32 vertexCount, AZStd::vector<AZ::u8>& result));
        MOCK_METHOD5(CookTriangleMeshToFile, bool(const AZStd::string& filePath, const AZ::Vector3* vertices, AZ::u32 vertexCount, const AZ::u32* indices, AZ::u32 indexCount));
        MOCK_METHOD5(CookTriangleMeshToMemory, bool(const AZ::Vector3* vertices, AZ::u32 vertexCount, const AZ::u32* indices, AZ::u32 indexCount, AZStd::vector<AZ::u8>& result));
    };

    //Mocked of the AzPhysics System Interface. To keep things simple just mocked functions that have a return value.
    class MockPhysicsInterface
        : AZ::Interface<AzPhysics::SystemInterface>::Registrar
    {
    public:
        void Initialize([[maybe_unused]]const AzPhysics::SystemConfiguration* config) override {}
        void Reinitialize() override {}
        void Shutdown() override {}
        void Simulate([[maybe_unused]] float deltaTime) override {}
        void UpdateConfiguration([[maybe_unused]] const AzPhysics::SystemConfiguration* newConfig, [[maybe_unused]] bool forceReinitialization = false) override {}
        void UpdateDefaultMaterialLibrary([[maybe_unused]] const AZ::Data::Asset<Physics::MaterialLibraryAsset>& materialLibrary) override {}
        void UpdateDefaultSceneConfiguration([[maybe_unused]] const AzPhysics::SceneConfiguration& sceneConfiguration) override {}
        void RemoveScene([[maybe_unused]] AzPhysics::SceneHandle handle) override {}
        void RemoveScenes([[maybe_unused]] const AzPhysics::SceneHandleList& handles) override {}
        void RemoveAllScenes() override {}

        MOCK_METHOD1(AddScene, AzPhysics::SceneHandle(const AzPhysics::SceneConfiguration& config));
        MOCK_METHOD1(AddScenes, AzPhysics::SceneHandleList(const AzPhysics::SceneConfigurationList& configs));
        MOCK_METHOD1(GetScene, AzPhysics::Scene* (AzPhysics::SceneHandle handle));
        MOCK_METHOD1(GetScenes, AzPhysics::SceneList (const AzPhysics::SceneHandleList& handles));
        MOCK_METHOD0(GetAllScenes, AzPhysics::SceneList& ());
        MOCK_CONST_METHOD0(GetConfiguration, const AzPhysics::SystemConfiguration* ());
        MOCK_CONST_METHOD0(GetDefaultMaterialLibrary, const AZ::Data::Asset<Physics::MaterialLibraryAsset>& ());
        MOCK_CONST_METHOD0(GetDefaultSceneConfiguration, const AzPhysics::SceneConfiguration& ());
    };
} // namespace Physics
