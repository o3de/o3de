/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

#include <AzFramework/Application/Application.h>
#include <AzFramework/Components/TransformComponent.h>

#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <Prefab/PrefabTestFixture.h>

#include <AZTestShared/Math/MathTestHelpers.h>

using namespace AZ;
using namespace AzFramework;

namespace UnitTest
{
    // Fixture base class for AzFramework::TransformComponent tests.
    class TransformComponentApplication
        : public LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            ComponentApplication::Descriptor desc;
            desc.m_useExistingAllocator = true;

            AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_loadSettingsRegistry = false;
            m_app.Start(desc, startupParameters);

            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
        }

        void TearDown() override
        {
            m_app.Stop();
            LeakDetectionFixture::TearDown();
        }

        AzFramework::Application m_app;
    };


    // Runs a series of tests on TransformComponent.
    class TransformComponentUberTest
        : public TransformComponentApplication
        , public TransformNotificationBus::Handler
    {
    public:

        //////////////////////////////////////////////////////////////////////////////
        // TransformNotificationBus
        void OnTransformChanged(const Transform& local, const Transform& world) override
        {
            AZ_TEST_ASSERT(m_checkWorldTM == world);
            AZ_TEST_ASSERT(m_checkLocalTM == local);
        }

        /// Called when the parent of an entity has changed. When the old/new parent are invalid the !EntityId.IsValid
        void OnParentChanged(EntityId oldParent, EntityId newParent) override
        {
            AZ_TEST_ASSERT(m_checkOldParentId == oldParent);
            AZ_TEST_ASSERT(m_checkNewParentId == newParent);
        }

        //////////////////////////////////////////////////////////////////////////////

        void run()
        {
            m_checkWorldTM = Transform::CreateIdentity();
            m_checkLocalTM = Transform::CreateIdentity();

            // Create test entity
            Entity childEntity, parentEntity;
            TransformComponent* childTransformComponent = childEntity.CreateComponent<TransformComponent>();
            TransformComponent* parentTranformComponent = parentEntity.CreateComponent<TransformComponent>();
            (void)parentTranformComponent;

            TransformNotificationBus::Handler::BusConnect(childEntity.GetId());

            childEntity.Init();
            parentEntity.Init();

            // We bind transform interface only when entity is activated
            AZ_TEST_ASSERT(childEntity.GetTransform() == nullptr);
            childEntity.Activate();
            TransformInterface* childTransform = childEntity.GetTransform();

            parentEntity.Activate();
            TransformInterface* parentTransform = parentEntity.GetTransform();
            parentTransform->SetWorldTM(Transform::CreateTranslation(Vector3(1.0f, 0.0f, 0.0f)));

            // Check validity of transform interface and initial transforms
            AZ_TEST_ASSERT(childTransform == static_cast<TransformInterface*>(childTransformComponent));
            AZ_TEST_ASSERT(childTransform->GetWorldTM() == m_checkWorldTM);
            AZ_TEST_ASSERT(childTransform->GetLocalTM() == m_checkLocalTM);
            AZ_TEST_ASSERT(childTransform->GetParentId() == m_checkNewParentId);

            // Modify the local (and world) matrix
            m_checkLocalTM = Transform::CreateTranslation(Vector3(5.0f, 0.0f, 0.0f));
            m_checkWorldTM = m_checkLocalTM;
            childTransform->SetWorldTM(m_checkWorldTM);

            // Parent the child object
            m_checkNewParentId = parentEntity.GetId();
            m_checkLocalTM *= parentTransform->GetWorldTM().GetInverse(); // the set parent will move the child object into parent space
            childTransform->SetParent(m_checkNewParentId);

            // Deactivate the parent (this essentially removes the parent)
            m_checkNewParentId.SetInvalid();
            m_checkOldParentId = parentEntity.GetId();
            m_checkLocalTM = m_checkWorldTM; // we will remove the parent
            parentEntity.Deactivate();

            TransformNotificationBus::Handler::BusDisconnect(childEntity.GetId());

            // now we should we without a parent
            childEntity.Deactivate();
        }

        Transform m_checkWorldTM;
        Transform m_checkLocalTM;
        EntityId m_checkOldParentId;
        EntityId m_checkNewParentId;
    };

    TEST_F(TransformComponentUberTest, Test)
    {
        run();
    }

    class TransformComponentChildNotificationTest
        : public TransformComponentApplication
        , public TransformNotificationBus::Handler
    {
    public:
        void OnChildAdded(EntityId child) override
        {
            AZ_TEST_ASSERT(child == m_checkChildId);
            m_onChildAddedCount++;
        }

        void OnChildRemoved(EntityId child) override
        {
            AZ_TEST_ASSERT(child == m_checkChildId);
            m_onChildRemovedCount++;
        }

        void run()
        {
            // Create ID for parent and begin listening for child add/remove notifications
            AZ::EntityId parentId = Entity::MakeId();
            TransformNotificationBus::Handler::BusConnect(parentId);

            Entity childEntity;
            AZ::TransformConfig transformConfig;
            transformConfig.m_isStatic = false;
            childEntity.CreateComponent<TransformComponent>()->SetConfiguration(transformConfig);

            m_checkChildId = childEntity.GetId();

            childEntity.Init();
            childEntity.Activate();
            TransformInterface* childTransform = childEntity.GetTransform();

            // Expected number of notifications to OnChildAdded and OnChildRemoved
            int checkAddCount = 0;
            int checkRemoveCount = 0;

            // Changing to target parentId should notify add
            AZ_TEST_ASSERT(m_onChildAddedCount == checkAddCount);
            childTransform->SetParent(parentId);
            checkAddCount++;
            AZ_TEST_ASSERT(m_onChildAddedCount == checkAddCount);

            // Deactivating child should notify removal
            AZ_TEST_ASSERT(m_onChildRemovedCount == checkRemoveCount);
            childEntity.Deactivate();
            checkRemoveCount++;
            AZ_TEST_ASSERT(m_onChildRemovedCount == checkRemoveCount);

            // Activating child (while parentId is set) should notify add
            AZ_TEST_ASSERT(m_onChildAddedCount == checkAddCount);
            childEntity.Activate();
            checkAddCount++;
            AZ_TEST_ASSERT(m_onChildAddedCount == checkAddCount);

            // Setting parent invalid should notify removal
            AZ_TEST_ASSERT(m_onChildRemovedCount == checkRemoveCount);
            childTransform->SetParent(EntityId());
            checkRemoveCount++;
            AZ_TEST_ASSERT(m_onChildRemovedCount == checkRemoveCount);

            TransformNotificationBus::Handler::BusDisconnect(parentId);
            childEntity.Deactivate();
        }

        EntityId m_checkChildId;
        int m_onChildAddedCount = 0;
        int m_onChildRemovedCount = 0;
    };

    TEST_F(TransformComponentChildNotificationTest, Test)
    {
        run();
    }

    class LookAtTransformTest
        : public ::testing::Test
    {
    public:
        void run()
        {
            // CreateLookAt
            AZ::Vector3 lookAtEye(1.0f, 2.0f, 3.0f);
            AZ::Vector3 lookAtTarget(10.0f, 5.0f, -5.0f);
            AZ::Transform t1 = AZ::Transform::CreateLookAt(lookAtEye, lookAtTarget);
            AZ_TEST_ASSERT(t1.GetBasisY().IsClose((lookAtTarget - lookAtEye).GetNormalized()));
            AZ_TEST_ASSERT(t1.GetTranslation() == lookAtEye);
            AZ_TEST_ASSERT(t1.IsOrthogonal());

            AZ_TEST_START_TRACE_SUPPRESSION;
            t1 = AZ::Transform::CreateLookAt(lookAtEye, lookAtEye); //degenerate direction
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            AZ_TEST_ASSERT(t1.IsOrthogonal());
            AZ_TEST_ASSERT(t1 == AZ::Transform::CreateIdentity());

            t1 = AZ::Transform::CreateLookAt(lookAtEye, lookAtEye + AZ::Vector3::CreateAxisZ()); //degenerate with up direction
            AZ_TEST_ASSERT(t1.GetBasisY().IsClose(AZ::Vector3::CreateAxisZ()));
            AZ_TEST_ASSERT(t1.GetTranslation() == lookAtEye);
            AZ_TEST_ASSERT(t1.IsOrthogonal());
        }
    };

    TEST_F(LookAtTransformTest, Test)
    {
        run();
    }

    // Test TransformComponent's methods of modifying/retrieving underlying translation, rotation and scale transform component.
    class TransformComponentTransformMatrixSetGet
        : public TransformComponentApplication
    {
    protected:
        void SetUp() override
        {
            TransformComponentApplication::SetUp();

            m_parentEntity = aznew Entity("Parent");
            m_parentId = m_parentEntity->GetId();
            m_parentEntity->Init();
            m_parentEntity->CreateComponent<TransformComponent>();

            m_childEntity = aznew Entity("Child");
            m_childId = m_childEntity->GetId();
            m_childEntity->Init();
            m_childEntity->CreateComponent<TransformComponent>();

            m_parentEntity->Activate();
            m_childEntity->Activate();

            TransformBus::Event(m_childId, &TransformBus::Events::SetParent, m_parentId);
        }

        void TearDown() override
        {
            m_childEntity->Deactivate();
            m_parentEntity->Deactivate();
            delete m_childEntity;
            delete m_parentEntity;

            TransformComponentApplication::TearDown();
        }

        Entity* m_parentEntity = nullptr;
        EntityId m_parentId = EntityId();
        Entity* m_childEntity = nullptr;
        EntityId m_childId = EntityId();
    };

    TEST_F(TransformComponentTransformMatrixSetGet, SetLocalX_SimpleValues_Set)
    {
        float tx = 123.123f;
        TransformBus::Event(m_childId, &TransformBus::Events::SetLocalX, tx);
        Transform tm;
        TransformBus::EventResult(tm, m_childId, &TransformBus::Events::GetLocalTM);
        EXPECT_NEAR(tx, tm.GetTranslation().GetX(), AZ::Constants::Tolerance);
    }

    TEST_F(TransformComponentTransformMatrixSetGet, GetLocalX_SimpleValues_Set)
    {
        Transform tm;
        tm.SetTranslation(AZ::Vector3::CreateAxisX(432.456f));
        TransformBus::Event(m_childId, &TransformBus::Events::SetLocalTM, tm);
        float tx = 0;
        TransformBus::EventResult(tx, m_childId, &TransformBus::Events::GetLocalX);
        EXPECT_NEAR(tx, tm.GetTranslation().GetX(), AZ::Constants::Tolerance);
    }

    TEST_F(TransformComponentTransformMatrixSetGet, SetLocalY_SimpleValues_Set)
    {
        float ty = 435.676f;
        TransformBus::Event(m_childId, &TransformBus::Events::SetLocalY, ty);
        Transform tm;
        TransformBus::EventResult(tm, m_childId, &TransformBus::Events::GetLocalTM);
        EXPECT_NEAR(ty, tm.GetTranslation().GetY(), AZ::Constants::Tolerance);
    }

    TEST_F(TransformComponentTransformMatrixSetGet, GetLocalY_SimpleValues_Set)
    {
        Transform tm;
        tm.SetTranslation(AZ::Vector3::CreateAxisY(154.754f));
        TransformBus::Event(m_childId, &TransformBus::Events::SetLocalTM, tm);
        float ty = 0;
        TransformBus::EventResult(ty, m_childId, &TransformBus::Events::GetLocalY);
        EXPECT_NEAR(ty, tm.GetTranslation().GetY(), AZ::Constants::Tolerance);
    }

    TEST_F(TransformComponentTransformMatrixSetGet, SetLocalZ_SimpleValues_Set)
    {
        float tz = 987.456f;
        TransformBus::Event(m_childId, &TransformBus::Events::SetLocalZ, tz);
        Transform tm;
        TransformBus::EventResult(tm, m_childId, &TransformBus::Events::GetLocalTM);
        EXPECT_NEAR(tz, tm.GetTranslation().GetZ(), AZ::Constants::Tolerance);
    }

    TEST_F(TransformComponentTransformMatrixSetGet, GetLocalZ_SimpleValues_Set)
    {
        Transform tm;
        tm.SetTranslation(AZ::Vector3::CreateAxisZ(453.894f));
        TransformBus::Event(m_childId, &TransformBus::Events::SetLocalTM, tm);
        float tz = 0;
        TransformBus::EventResult(tz, m_childId, &TransformBus::Events::GetLocalZ);
        EXPECT_NEAR(tz, tm.GetTranslation().GetZ(), AZ::Constants::Tolerance);
    }

    TEST_F(TransformComponentTransformMatrixSetGet, SetLocalRotation_SimpleValues_Set)
    {
        // add some scale first
        float scale = 1.23f;
        Transform tm = Transform::CreateUniformScale(scale);
        TransformBus::Event(m_childId, &TransformBus::Events::SetLocalTM, tm);

        float rx = 42.435f;
        float ry = 19.454f;
        float rz = 98.356f;
        Vector3 angles(rx, ry, rz);
        TransformBus::Event(m_childId, &TransformBus::Events::SetLocalRotation, angles);

        TransformBus::EventResult(tm, m_childId, &TransformBus::Events::GetLocalTM);
        Matrix3x3 rotateZ = Matrix3x3::CreateRotationZ(rz);
        Matrix3x3 rotateY = Matrix3x3::CreateRotationY(ry);
        Matrix3x3 rotateX = Matrix3x3::CreateRotationX(rx);
        Matrix3x3 finalRotate = rotateX * rotateY * rotateZ;

        Vector3 basisX = tm.GetBasisX();
        Vector3 expectedBasisX = finalRotate.GetBasisX() * scale;
        EXPECT_TRUE(basisX.IsClose(expectedBasisX));
        Vector3 basisY = tm.GetBasisY();
        Vector3 expectedBasisY = finalRotate.GetBasisY() * scale;
        EXPECT_TRUE(basisY.IsClose(expectedBasisY));
        Vector3 basisZ = tm.GetBasisZ();
        Vector3 expectedBasisZ = finalRotate.GetBasisZ() * scale;
        EXPECT_TRUE(basisZ.IsClose(expectedBasisZ));
    }

    TEST_F(TransformComponentTransformMatrixSetGet, GetLocalRotation_SimpleValues_Return)
    {
        float rx = 0.66f;
        float ry = 1.23f;
        float rz = 0.23f;
        Matrix3x3 rotateZ = Matrix3x3::CreateRotationZ(rz);
        Matrix3x3 rotateY = Matrix3x3::CreateRotationY(ry);
        Matrix3x3 rotateX = Matrix3x3::CreateRotationX(rx);
        Matrix3x3 finalRotate = rotateX * rotateY * rotateZ;
        Transform tm = Transform::CreateFromMatrix3x3(finalRotate);
        TransformBus::Event(m_childId, &TransformBus::Events::SetLocalTM, tm);

        Vector3 angles;
        TransformBus::EventResult(angles, m_childId, &TransformBus::Events::GetLocalRotation);

        EXPECT_TRUE(angles.IsClose(Vector3(rx, ry, rz)));
    }

    TEST_F(TransformComponentTransformMatrixSetGet, SetLocalRotationQuaternion_SimpleValues_Set)
    {
        float rx = 42.435f;
        float ry = 19.454f;
        float rz = 98.356f;
        Quaternion quatX = Quaternion::CreateRotationX(rx);
        Quaternion quatY = Quaternion::CreateRotationY(ry);
        Quaternion quatZ = Quaternion::CreateRotationZ(rz);
        Quaternion finalQuat = quatX * quatY * quatZ;
        TransformBus::Event(m_childId, &TransformBus::Events::SetLocalRotationQuaternion, finalQuat);

        Transform tm;
        TransformBus::EventResult(tm, m_childId, &TransformBus::Events::GetLocalTM);
        Matrix3x3 rotateZ = Matrix3x3::CreateRotationZ(rz);
        Matrix3x3 rotateY = Matrix3x3::CreateRotationY(ry);
        Matrix3x3 rotateX = Matrix3x3::CreateRotationX(rx);
        Matrix3x3 finalRotate = rotateX * rotateY * rotateZ;

        Vector3 basisX = tm.GetBasisX();
        Vector3 expectedBasisX = finalRotate.GetBasisX();
        EXPECT_TRUE(basisX.IsClose(expectedBasisX));
        Vector3 basisY = tm.GetBasisY();
        Vector3 expectedBasisY = finalRotate.GetBasisY();
        EXPECT_TRUE(basisY.IsClose(expectedBasisY));
        Vector3 basisZ = tm.GetBasisZ();
        Vector3 expectedBasisZ = finalRotate.GetBasisZ();
        EXPECT_TRUE(basisZ.IsClose(expectedBasisZ));
    }

    TEST_F(TransformComponentTransformMatrixSetGet, GetLocalRotationQuaternion_SimpleValues_Return)
    {
        float rx = 0.66f;
        float ry = 1.23f;
        float rz = 0.23f;
        Matrix3x3 rotateZ = Matrix3x3::CreateRotationZ(rz);
        Matrix3x3 rotateY = Matrix3x3::CreateRotationY(ry);
        Matrix3x3 rotateX = Matrix3x3::CreateRotationX(rx);
        Matrix3x3 finalRotate = rotateX * rotateY * rotateZ;
        Transform tm = Transform::CreateFromMatrix3x3(finalRotate);
        TransformBus::Event(m_childId, &TransformBus::Events::SetLocalTM, tm);

        Quaternion quatX = Quaternion::CreateRotationX(rx);
        Quaternion quatY = Quaternion::CreateRotationY(ry);
        Quaternion quatZ = Quaternion::CreateRotationZ(rz);
        Quaternion expectedQuat = quatX * quatY * quatZ;

        Quaternion resultQuat;
        TransformBus::EventResult(resultQuat, m_childId, &TransformBus::Events::GetLocalRotationQuaternion);

        EXPECT_TRUE(resultQuat.IsClose(expectedQuat));
    }

    TEST_F(TransformComponentTransformMatrixSetGet, RotateAroundLocalX_SimpleValues_Set)
    {
        float rx = 1.43f;
        TransformBus::Event(m_childId, &TransformBus::Events::RotateAroundLocalX, rx);
        Vector3 localRotation;
        TransformBus::EventResult(localRotation, m_childId, &TransformBus::Events::GetLocalRotation);
        EXPECT_TRUE(localRotation.IsClose(Vector3(rx, 0.0f, 0.0f)));
    }

    TEST_F(TransformComponentTransformMatrixSetGet, RotateAroundLocalX_RepeatCallingThisFunctionDoesNotSkewScale)
    {
        // test numeric stability
        float rx = 1.43f;
        for (int i = 0; i < 100; ++i)
        {
            TransformBus::Event(m_childId, &TransformBus::Events::RotateAroundLocalX, rx);
        }
        float localScale = FLT_MAX;
        TransformBus::EventResult(localScale, m_childId, &TransformBus::Events::GetLocalUniformScale);
        EXPECT_NEAR(localScale, 1.0f, AZ::Constants::Tolerance);
    }

    TEST_F(TransformComponentTransformMatrixSetGet, RotateAroundLocalX_ScaleDoesNotSkewRotation)
    {
        float expectedScale = 42.564f;
        TransformBus::Event(m_childId, &TransformBus::Events::SetLocalUniformScale, expectedScale);

        float rx = 1.43f;
        TransformBus::Event(m_childId, &TransformBus::Events::RotateAroundLocalX, rx);
        Vector3 localRotation;
        TransformBus::EventResult(localRotation, m_childId, &TransformBus::Events::GetLocalRotation);
        EXPECT_TRUE(localRotation.IsClose(Vector3(rx, 0.0f, 0.0f)));
    }

    TEST_F(TransformComponentTransformMatrixSetGet, RotateAroundLocalY_SimpleValue_Set)
    {
        float ry = 1.43f;
        TransformBus::Event(m_childId, &TransformBus::Events::RotateAroundLocalY, ry);
        Vector3 localRotation;
        TransformBus::EventResult(localRotation, m_childId, &TransformBus::Events::GetLocalRotation);
        EXPECT_TRUE(localRotation.IsClose(Vector3(0.0f, ry, 0.0f)));
    }

    TEST_F(TransformComponentTransformMatrixSetGet, RotateAroundLocalY_RepeatCallingThisFunctionDoesNotSkewScale)
    {
        // test numeric stability
        float ry = 1.43f;
        for (int i = 0; i < 100; ++i)
        {
            TransformBus::Event(m_childId, &TransformBus::Events::RotateAroundLocalY, ry);
        }
        float localScale = FLT_MAX;
        TransformBus::EventResult(localScale, m_childId, &TransformBus::Events::GetLocalUniformScale);
        EXPECT_NEAR(localScale, 1.0f, AZ::Constants::Tolerance);
    }

    TEST_F(TransformComponentTransformMatrixSetGet, RotateAroundLocalY_ScaleDoesNotSkewRotation)
    {
        float expectedScale = 42.564f;
        TransformBus::Event(m_childId, &TransformBus::Events::SetLocalUniformScale, expectedScale);

        float ry = 1.43f;
        TransformBus::Event(m_childId, &TransformBus::Events::RotateAroundLocalY, ry);
        Vector3 localRotation;
        TransformBus::EventResult(localRotation, m_childId, &TransformBus::Events::GetLocalRotation);
        EXPECT_TRUE(localRotation.IsClose(Vector3(0.0f, ry, 0.0f)));
    }

    TEST_F(TransformComponentTransformMatrixSetGet, RotateAroundLocalZ_SimpleValues_Set)
    {
        float rz = 1.43f;
        TransformBus::Event(m_childId, &TransformBus::Events::RotateAroundLocalZ, rz);
        Vector3 localRotation;
        TransformBus::EventResult(localRotation, m_childId, &TransformBus::Events::GetLocalRotation);
        EXPECT_TRUE(localRotation.IsClose(Vector3(0.0f, 0.0f, rz)));
    }

    TEST_F(TransformComponentTransformMatrixSetGet, RotateAroundLocalZ_RepeatCallingThisFunctionDoesNotSkewScale)
    {
        // test numeric stability
        float rz = 1.43f;
        for (int i = 0; i < 100; ++i)
        {
            TransformBus::Event(m_childId, &TransformBus::Events::RotateAroundLocalZ, rz);
        }
        float localScale = FLT_MAX;
        TransformBus::EventResult(localScale, m_childId, &TransformBus::Events::GetLocalUniformScale);
        EXPECT_NEAR(localScale, 1.0f, AZ::Constants::Tolerance);
    }

    TEST_F(TransformComponentTransformMatrixSetGet, RotateAroundLocalZ_ScaleDoesNotSkewRotation)
    {
        float expectedScale = 42.564f;
        TransformBus::Event(m_childId, &TransformBus::Events::SetLocalUniformScale, expectedScale);

        float rz = 1.43f;
        TransformBus::Event(m_childId, &TransformBus::Events::RotateAroundLocalZ, rz);
        Vector3 localRotation;
        TransformBus::EventResult(localRotation, m_childId, &TransformBus::Events::GetLocalRotation);
        EXPECT_TRUE(localRotation.IsClose(Vector3(0.0f, 0.0f, rz)));
    }

    TEST_F(TransformComponentTransformMatrixSetGet, SetLocalScale_SimpleValues_Set)
    {
        float expectedScale = 42.564f;
        TransformBus::Event(m_childId, &TransformBus::Events::SetLocalUniformScale, expectedScale);

        Transform tm;
        TransformBus::EventResult(tm, m_childId, &TransformBus::Events::GetLocalTM);
        float scale = tm.GetUniformScale();
        EXPECT_NEAR(scale, expectedScale, AZ::Constants::Tolerance);
    }

    TEST_F(TransformComponentTransformMatrixSetGet, GetLocalScale_SimpleValues_Return)
    {
        float expectedScale = 43.463f;
        Transform scaleTM = Transform::CreateUniformScale(expectedScale);
        TransformBus::Event(m_childId, &TransformBus::Events::SetLocalTM, scaleTM);

        float scale;
        TransformBus::EventResult(scale, m_childId, &TransformBus::Events::GetLocalUniformScale);
        EXPECT_NEAR(scale, expectedScale, AZ::Constants::Tolerance);
    }

    TEST_F(TransformComponentTransformMatrixSetGet, GetWorldScale_ChildHasNoScale_ReturnScaleSameAsParent)
    {
        float expectedScale = 43.463f;
        Transform scaleTM = Transform::CreateUniformScale(expectedScale);
        TransformBus::Event(m_parentId, &TransformBus::Events::SetLocalTM, scaleTM);

        float scale = FLT_MAX;
        TransformBus::EventResult(scale, m_childId, &TransformBus::Events::GetWorldUniformScale);
        EXPECT_NEAR(scale, expectedScale, AZ::Constants::Tolerance);
    }

    TEST_F(TransformComponentTransformMatrixSetGet, GetWorldScale_ChildHasScale_ReturnCompoundScale)
    {
        float parentScale = 4.463f;
        Transform parentScaleTM = Transform::CreateUniformScale(parentScale);
        TransformBus::Event(m_parentId, &TransformBus::Events::SetLocalTM, parentScaleTM);

        float childScale = 1.64f;
        Transform childScaleTM = Transform::CreateUniformScale(childScale);
        TransformBus::Event(m_childId, &TransformBus::Events::SetLocalTM, childScaleTM);

        float scale = FLT_MAX;
        TransformBus::EventResult(scale, m_childId, &TransformBus::Events::GetWorldUniformScale);
        EXPECT_NEAR(scale, parentScale * childScale, AZ::Constants::Tolerance);
    }

    class TransformComponentHierarchy
        : public TransformComponentApplication
    {
    protected:
        void SetUp() override
        {
            TransformComponentApplication::SetUp();

            m_parentEntity = aznew Entity("Parent");
            m_parentId = m_parentEntity->GetId();
            m_parentEntity->Init();
            m_parentEntity->CreateComponent<TransformComponent>();

            m_childEntity = aznew Entity("Child");
            m_childId = m_childEntity->GetId();
            m_childEntity->Init();
            m_childEntity->CreateComponent<TransformComponent>();

            m_parentEntity->Activate();
            m_childEntity->Activate();
        }

        void TearDown() override
        {
            m_childEntity->Deactivate();
            m_parentEntity->Deactivate();
            delete m_childEntity;
            delete m_parentEntity;

            TransformComponentApplication::TearDown();
        }

        Entity* m_parentEntity = nullptr;
        EntityId m_parentId = EntityId();
        Entity* m_childEntity = nullptr;
        EntityId m_childId = EntityId();
    };

    TEST_F(TransformComponentHierarchy, SetParent_NormalValue_SetKeepWorldTransform)
    {
        AZ::Vector3 childLocalPos(20.45f, 46.14f, 93.65f);
        TransformBus::Event(m_childId, &TransformBus::Events::SetLocalTranslation, childLocalPos);
        AZ::Vector3 expectedChildWorldPos = childLocalPos;

        AZ::Vector3 parentLocalPos(65.24f, 10.65f, 37.87f);
        TransformBus::Event(m_parentId, &TransformBus::Events::SetLocalTranslation, parentLocalPos);

        TransformBus::Event(m_childId, &TransformBus::Events::SetParent, m_parentId);

        AZ::Vector3 childWorldPos;
        TransformBus::EventResult(childWorldPos, m_childId, &TransformBus::Events::GetWorldTranslation);
        EXPECT_TRUE(childWorldPos == expectedChildWorldPos);
    }

    TEST_F(TransformComponentHierarchy, SetParentRelative_NormalValue_SetKeepLocalTransform)
    {
        AZ::Vector3 expectedChildLocalPos(22.45f, 42.14f, 97.45f);
        TransformBus::Event(m_childId, &TransformBus::Events::SetLocalTranslation, expectedChildLocalPos);
        AZ::Vector3 parentLocalPos(15.64f, 12.65f, 29.87f);
        TransformBus::Event(m_parentId, &TransformBus::Events::SetLocalTranslation, parentLocalPos);

        TransformBus::Event(m_childId, &TransformBus::Events::SetParentRelative, m_parentId);

        AZ::Vector3 childLocalPos;
        TransformBus::EventResult(childLocalPos, m_childId, &TransformBus::Events::GetLocalTranslation);
        EXPECT_TRUE(childLocalPos == expectedChildLocalPos);
    }

    TEST_F(TransformComponentHierarchy, SetParent_Null_SetKeepWorldTransform)
    {
        AZ::Vector3 childLocalPos(28.45f, 56.14f, 43.65f);
        TransformBus::Event(m_childId, &TransformBus::Events::SetLocalTranslation, childLocalPos);
        AZ::Vector3 parentLocalPos(85.24f, 12.65f, 33.87f);
        TransformBus::Event(m_parentId, &TransformBus::Events::SetLocalTranslation, parentLocalPos);

        TransformBus::Event(m_childId, &TransformBus::Events::SetParentRelative, m_parentId);

        AZ::Vector3 expectedChildWorldPos;
        TransformBus::EventResult(expectedChildWorldPos, m_childId, &TransformBus::Events::GetWorldTranslation);

        TransformBus::Event(m_childId, &TransformBus::Events::SetParent, AZ::EntityId());

        AZ::Vector3 childWorldPos;
        TransformBus::EventResult(childWorldPos, m_childId, &TransformBus::Events::GetWorldTranslation);
        EXPECT_TRUE(childWorldPos == expectedChildWorldPos);

        // child entity doesn't have a parent now, its world position should equal its local one
        AZ::Vector3 actualChildLocalPos;
        TransformBus::EventResult(actualChildLocalPos, m_childId, &TransformBus::Events::GetLocalTranslation);
        EXPECT_TRUE(actualChildLocalPos == expectedChildWorldPos);
    }

    TEST_F(TransformComponentHierarchy, SetParentRelative_Null_SetKeepLocalTransform)
    {
        AZ::Vector3 childLocalPos(28.45f, 49.14f, 94.65f);
        TransformBus::Event(m_childId, &TransformBus::Events::SetLocalTranslation, childLocalPos);
        AZ::Vector3 parentLocalPos(66.24f, 19.65f, 32.87f);
        TransformBus::Event(m_parentId, &TransformBus::Events::SetLocalTranslation, parentLocalPos);

        TransformBus::Event(m_childId, &TransformBus::Events::SetParent, m_parentId);

        AZ::Vector3 expectedChildLocalPos;
        TransformBus::EventResult(expectedChildLocalPos, m_childId, &TransformBus::Events::GetLocalTranslation);

        TransformBus::Event(m_childId, &TransformBus::Events::SetParentRelative, AZ::EntityId());

        TransformBus::EventResult(childLocalPos, m_childId, &TransformBus::Events::GetLocalTranslation);
        EXPECT_TRUE(childLocalPos == expectedChildLocalPos);

        // child entity doesn't have a parent now, its world position should equal its local one
        AZ::Vector3 actualChildWorldPos;
        TransformBus::EventResult(actualChildWorldPos, m_childId, &TransformBus::Events::GetWorldTranslation);
        EXPECT_TRUE(actualChildWorldPos == expectedChildLocalPos);
    }

    // Fixture provides TransformComponent that is static (or not static) on an entity that has been activated.
    template<bool IsStatic>
    class StaticOrMovableTransformComponent
        : public TransformComponentApplication
    {
    protected:
        void SetUp() override
        {
            TransformComponentApplication::SetUp();

            m_entity = aznew Entity(IsStatic ? "Static Entity" : "Movable Entity");

            AZ::TransformConfig transformConfig;
            transformConfig.m_isStatic = IsStatic;

            auto transformComponent = m_entity->CreateComponent<TransformComponent>();
            transformComponent->SetConfiguration(transformConfig);
            m_transformInterface = transformComponent;

            m_entity->Init();
            m_entity->Activate();
        }

        Entity* m_entity;
        TransformInterface* m_transformInterface = nullptr;
    };

    class MovableTransformComponent : public StaticOrMovableTransformComponent<false> {};
    class StaticTransformComponent : public StaticOrMovableTransformComponent<true> {};

    TEST_F(StaticTransformComponent, SanityCheck)
    {
        ASSERT_NE(m_entity, nullptr);
        ASSERT_NE(m_transformInterface, nullptr);
        EXPECT_EQ(m_entity->GetState(), Entity::State::Active);
    }

    TEST_F(MovableTransformComponent, IsStaticTransform_False)
    {
        EXPECT_FALSE(m_transformInterface->IsStaticTransform());
    }

    TEST_F(StaticTransformComponent, IsStaticTransform_True)
    {
        EXPECT_TRUE(m_transformInterface->IsStaticTransform());
    }

    TEST_F(MovableTransformComponent, SetWorldTM_MovesEntity)
    {
        [[maybe_unused]] Transform previousTM = m_transformInterface->GetWorldTM();
        Transform nextTM = Transform::CreateTranslation(Vector3(1.f, 2.f, 3.f));
        m_transformInterface->SetWorldTM(nextTM);
        EXPECT_TRUE(m_transformInterface->GetWorldTM().IsClose(nextTM));
    }

    TEST_F(StaticTransformComponent, SetWorldTM_DoesNothing)
    {
        Transform previousTM = m_transformInterface->GetWorldTM();
        Transform nextTM = Transform::CreateTranslation(Vector3(1.f, 2.f, 3.f));
        m_transformInterface->SetWorldTM(nextTM);
        EXPECT_TRUE(m_transformInterface->GetWorldTM().IsClose(previousTM));
    }

    TEST_F(MovableTransformComponent, SetLocalTM_MovesEntity)
    {
        [[maybe_unused]] Transform previousTM = m_transformInterface->GetLocalTM();
        Transform nextTM = Transform::CreateTranslation(Vector3(1.f, 2.f, 3.f));
        m_transformInterface->SetLocalTM(nextTM);
        EXPECT_TRUE(m_transformInterface->GetLocalTM().IsClose(nextTM));
    }

    TEST_F(StaticTransformComponent, SetLocalTM_DoesNothing)
    {
        Transform previousTM = m_transformInterface->GetLocalTM();
        Transform nextTM = Transform::CreateTranslation(Vector3(1.f, 2.f, 3.f));
        m_transformInterface->SetLocalTM(nextTM);
        EXPECT_TRUE(m_transformInterface->GetLocalTM().IsClose(previousTM));
    }

    TEST_F(StaticTransformComponent, SetLocalTmOnDeactivatedEntity_MovesEntity)
    {
        // when static transform component is deactivated, it should allow movement
        [[maybe_unused]] Transform previousTM = m_transformInterface->GetLocalTM();
        m_entity->Deactivate();
        Transform nextTM = Transform::CreateTranslation(Vector3(1.f, 2.f, 3.f));
        m_transformInterface->SetLocalTM(nextTM);
        EXPECT_TRUE(m_transformInterface->GetLocalTM().IsClose(nextTM));
    }

    // Sets up a parent/child relationship between two static transform components
    class ParentedStaticTransformComponent
        : public TransformComponentApplication
    {
    protected:
        void SetUp() override
        {
            TransformComponentApplication::SetUp();

            m_parentEntity = aznew Entity("Parent");
            m_parentEntity->Init();

            AZ::TransformConfig parentConfig{ AZ::Transform::CreateTranslation(AZ::Vector3(5.f, 5.f, 5.f)) };
            parentConfig.m_isStatic = true;
            m_parentEntity->CreateComponent<TransformComponent>()->SetConfiguration(parentConfig);

            m_childEntity = aznew Entity("Child");
            m_childEntity->Init();

            AZ::TransformConfig childConfig{ AZ::Transform::CreateTranslation(AZ::Vector3(5.f, 5.f, 5.f)) };
            childConfig.m_isStatic = true;
            childConfig.m_parentId = m_parentEntity->GetId();
            childConfig.m_parentActivationTransformMode = AZ::TransformConfig::ParentActivationTransformMode::MaintainOriginalRelativeTransform;
            m_childEntity->CreateComponent<TransformComponent>()->SetConfiguration(childConfig);
        }

        Entity* m_parentEntity = nullptr;
        Entity* m_childEntity = nullptr;
    };

    // we do expect a static entity to move if its parent is activated after itself
    TEST_F(ParentedStaticTransformComponent, ParentActivatesLast_OffsetObeyed)
    {
        m_childEntity->Activate();

        Transform previousWorldTM;
        TransformBus::EventResult(previousWorldTM, m_childEntity->GetId(), &TransformBus::Events::GetWorldTM);

        m_parentEntity->Activate();

        Transform nextWorldTM;
        TransformBus::EventResult(nextWorldTM, m_childEntity->GetId(), &TransformBus::Events::GetWorldTM);

        EXPECT_FALSE(previousWorldTM.IsClose(nextWorldTM));
    }

    // Fixture that loads a TransformComponent from a buffer.
    // Useful for testing version converters.
    class TransformComponentVersionConverter
        : public TransformComponentApplication
    {
    public:
        void SetUp() override
        {
            TransformComponentApplication::SetUp();

            m_transformComponent.reset(AZ::Utils::LoadObjectFromBuffer<TransformComponent>(m_objectStreamBuffer, strlen(m_objectStreamBuffer) + 1));
            m_transformInterface = m_transformComponent.get();
        }

        void TearDown() override
        {
            m_transformComponent.reset();

            TransformComponentApplication::TearDown();
        }

        AZStd::unique_ptr<TransformComponent> m_transformComponent;
        TransformInterface* m_transformInterface = nullptr;
        const char* m_objectStreamBuffer = nullptr;
    };

    ///////////////////////////////////////////////////////////////////////////
    // TransformConfig

    static bool operator==(const TransformConfig& lhs, const TransformConfig& rhs)
    {
        return lhs.m_parentId == rhs.m_parentId
            && lhs.m_parentActivationTransformMode == rhs.m_parentActivationTransformMode
            && lhs.m_isStatic == rhs.m_isStatic
            && lhs.m_localTransform == rhs.m_localTransform
            && lhs.m_worldTransform == rhs.m_worldTransform
            ;
    }

    class TransformConfigTest
        : public TransformComponentApplication
    {
    public:
        // creates a transform with a random rotation and a translation with each component uniformly sampled in the
        // range [0, 1)
        AZ::Transform CreateRandomTransform()
        {
            AZ::Vector3 translation(m_random.GetRandomFloat(), m_random.GetRandomFloat(), m_random.GetRandomFloat());
            return AZ::Transform::CreateFromQuaternionAndTranslation(CreateRandomQuaternion(m_random), translation);
        }

        TransformConfig GetRandomConfig()
        {
            TransformConfig config;
            config.m_worldTransform = CreateRandomTransform();
            config.m_localTransform = CreateRandomTransform();
            config.m_parentId = Entity::MakeId();
            config.m_parentActivationTransformMode = (TransformConfig::ParentActivationTransformMode)(m_random.GetRandom() % 2);
            config.m_netSyncEnabled = (m_random.GetRandom() % 2) == 1;
            config.m_interpolatePosition = (m_random.GetRandom() % 2) == 1 ? InterpolationMode::NoInterpolation : InterpolationMode::LinearInterpolation;
            config.m_interpolateRotation = (m_random.GetRandom() % 2) == 1 ? InterpolationMode::NoInterpolation : InterpolationMode::LinearInterpolation;
            config.m_isStatic = (m_random.GetRandom() % 2) == 1;

            return config;
        }

        void SetUp() override
        {
            TransformComponentApplication::SetUp();

            m_random.SetSeed(static_cast<AZ::u64>(m_app.GetTimeAtCurrentTick().GetMilliseconds()));
        }

        AZ::SimpleLcgRandom m_random;
    };

    TEST_F(TransformConfigTest, SetConfiguration_Succeeds)
    {
        TransformComponent component;
        TransformConfig config = GetRandomConfig();
        EXPECT_TRUE(component.SetConfiguration(config));
    }

    TEST_F(TransformConfigTest, GetConfiguration_Succeeds)
    {
        TransformComponent component;
        TransformConfig config;
        EXPECT_TRUE(component.GetConfiguration(config));
    }

    TEST_F(TransformConfigTest, SetThenGet_ConfigsMatches)
    {
        TransformComponent component;
        TransformConfig originalConfig = GetRandomConfig();
        EXPECT_TRUE(component.SetConfiguration(originalConfig));

        TransformConfig retrievedConfig;
        EXPECT_TRUE(component.GetConfiguration(retrievedConfig));

        EXPECT_TRUE(originalConfig == retrievedConfig);
    }

    TEST_F(TransformConfigTest, ConfigDefaultsComparedToComponentDefaults_Same)
    {
        // A default-constructed TransformConfig should be equivalent
        // to a configuration fetched from a default-constructed TransformComponent.
        TransformConfig defaultConfig;
        TransformConfig retrievedConfig;
        TransformComponent component;

        EXPECT_TRUE(component.GetConfiguration(retrievedConfig));
        EXPECT_TRUE(defaultConfig == retrievedConfig);
    }

    ///////////////////////////////////////////////////////////////////////////
    // AzToolsFramework::Components::TransformComponent

    // Fixture base class for AzToolsFramework::Components::TransformComponent tests
    class OldEditorTransformComponentTest
        : public UnitTest::LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_loadSettingsRegistry = false;
            m_app.Start(AZ::ComponentApplication::Descriptor(), startupParameters);

            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
        }

        void TearDown() override
        {
            m_app.Stop();
        }

        AzToolsFramework::ToolsApplication m_app;
    };

    // Old TransformComponents used to store "Slice Root" entity Id, which could be its own Id.
    // The version-converter could end up making an entity into its own transform parent.
    // The EditorEntityFixupComponent should fix this up during slice instantiation.
    TEST_F(OldEditorTransformComponentTest, OldSliceRoots_ShouldHaveNoParent)
    {
        const char kSliceData[] =
R"DELIMITER(<ObjectStream version="1">
    <Class name="PrefabComponent" field="element" version="1" type="{AFD304E4-1773-47C8-855A-8B622398934F}">
        <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
            <Class name="AZ::u64" field="Id" value="3561916384376604258" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
        </Class>
        <Class name="AZStd::vector" field="Entities" type="{2BADE35A-6F1B-4698-B2BC-3373D010020C}">
            <Class name="AZ::Entity" field="element" version="2" type="{75651658-8663-478D-9090-2432DFCAFA44}">
                <Class name="EntityId" field="Id" version="1" type="{6383F1D3-BB27-4E6B-A49A-6409B2059EAA}">
                    <Class name="AZ::u64" field="id" value="15464031792689993220" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                </Class>
                <Class name="AZStd::string" field="Name" value="MrRootEntity" type="{EF8FF807-DDEE-4EB0-B678-4CA3A2C490A4}"/>
                <Class name="bool" field="IsDependencyReady" value="true" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                <Class name="AZStd::vector" field="Components" type="{2BADE35A-6F1B-4698-B2BC-3373D010020C}">
                    <Class name="TransformComponent" field="element" version="5" type="{27F1E1A1-8D9D-4C3B-BD3A-AFB9762449C0}">
                        <Class name="EditorComponentBase" field="BaseClass1" version="1" type="{D5346BD4-7F20-444E-B370-327ACD03D4A0}">
                            <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
                                <Class name="AZ::u64" field="Id" value="3107681419974783222" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                            </Class>
                        </Class>
                        <Class name="EntityId" field="Parent Entity" version="1" type="{6383F1D3-BB27-4E6B-A49A-6409B2059EAA}">
                            <Class name="AZ::u64" field="id" value="4294967295" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                        </Class>
                        <Class name="EditorTransform" field="Transform Data" version="1" type="{B02B7063-D238-4F40-A724-405F7A6D68CB}">
                            <Class name="Vector3" field="Translate" value="0.0000000 0.0000000 0.0000000" type="{8379EB7D-01FA-4538-B64B-A6543B4BE73D}"/>
                            <Class name="Vector3" field="Rotate" value="0.0000000 0.0000000 0.0000000" type="{8379EB7D-01FA-4538-B64B-A6543B4BE73D}"/>
                            <Class name="Vector3" field="Scale" value="1.0000000 1.0000000 1.0000000" type="{8379EB7D-01FA-4538-B64B-A6543B4BE73D}"/>
                        </Class>
                        <Class name="Transform" field="Slice Transform" value="1.0000000 0.0000000 0.0000000 0.0000000 1.0000000 0.0000000 0.0000000 0.0000000 1.0000000 0.0000000 0.0000000 0.0000000" type="{5D9958E9-9F1E-4985-B532-FFFDE75FEDFD}"/>
                        <Class name="EntityId" field="Slice Root" version="1" type="{6383F1D3-BB27-4E6B-A49A-6409B2059EAA}">
                            <Class name="AZ::u64" field="id" value="15464031792689993220" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                        </Class>
                        <Class name="Transform" field="Cached World Transform" value="1.0000000 0.0000000 0.0000000 0.0000000 1.0000000 0.0000000 0.0000000 0.0000000 1.0000000 0.0000000 0.0000000 0.0000000" type="{5D9958E9-9F1E-4985-B532-FFFDE75FEDFD}"/>
                    </Class>
                </Class>
            </Class>
        </Class>
        <Class name="AZStd::list" field="Prefabs" type="{B845AD64-B5A0-4CCD-A86B-3477A36779BE}"/>
        <Class name="bool" field="IsDynamic" value="true" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
    </Class>
</ObjectStream>)DELIMITER";

        AZStd::unique_ptr<SliceComponent> slice{ AZ::Utils::LoadObjectFromBuffer<AZ::SliceComponent>(kSliceData, strlen(kSliceData) + 1) };
        EXPECT_NE(slice.get(), nullptr);
        if (slice)
        {
            AZStd::vector<AZ::Entity*> entities;
            slice->GetEntities(entities);
            EXPECT_FALSE(entities.empty());
            if (!entities.empty())
            {
                auto editorTransformComponent = entities[0]->FindComponent<AzToolsFramework::Components::TransformComponent>();
                EXPECT_NE(editorTransformComponent, nullptr);
                if (editorTransformComponent)
                {
                    // EditorEntityFixupComponent should have fixed this
                    EXPECT_EQ(editorTransformComponent->GetParentId(), AZ::EntityId());
                }
            }
        }
    }

    // Fixture provides a root prefab with Transform component and listens for TransformNotificationBus.
    class TransformComponentActivationTest
        : public PrefabTestFixture
        , public TransformNotificationBus::Handler
    {
    protected:
        void SetUpEditorFixtureImpl() override
        {
            PrefabTestFixture::SetUpEditorFixtureImpl();
        }

        void TearDownEditorFixtureImpl() override
        {
            AZ::TransformNotificationBus::Handler::BusDisconnect();
            
            PrefabTestFixture::TearDownEditorFixtureImpl();
        }
        
        void OnTransformChanged(const Transform& /*local*/, const Transform& /*world*/) override
        {
            m_transformUpdated = true;
        }

        void MoveEntity(AZ::EntityId entityId)
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("Move Entity");
            TransformBus::Event(entityId, &TransformInterface::SetWorldTranslation, Vector3(1.f, 0.f, 0.f));
        }
        
        bool m_transformUpdated = false;
    };
    
    TEST_F(TransformComponentActivationTest, TransformChangedEventIsSentWhenEntityIsActivatedViaUndoRedo)
    {
        AZ::EntityId entityId = CreateEditorEntityUnderRoot("Entity");
        MoveEntity(entityId);
        ProcessDeferredUpdates();
        AZ::TransformNotificationBus::Handler::BusConnect(entityId);

        // verify that undoing/redoing move operations fires TransformChanged event
        Undo();
        EXPECT_TRUE(m_transformUpdated);
        m_transformUpdated = false;

        Redo();
        EXPECT_TRUE(m_transformUpdated);
        m_transformUpdated = false;
    }

    TEST_F(TransformComponentActivationTest, TransformChangedEventIsNotSentWhenEntityIsDeactivatedAndActivated)
    {
        AZ::EntityId entityId = CreateEditorEntityUnderRoot("Entity");
        AZ::TransformNotificationBus::Handler::BusConnect(entityId);

        // verify that simply activating/deactivating an entity does not fire TransformChanged event
        Entity* entity = nullptr;
        ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
        entity->Deactivate();
        entity->Activate();
        EXPECT_FALSE(m_transformUpdated);
    }
} // namespace UnitTest
