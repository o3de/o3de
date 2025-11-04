/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Task/TaskExecutor.h>
#include <AzCore/Task/TaskGraph.h>
#include <AzFramework/Scene/SceneSystemComponent.h>
#include <AzFramework/Visibility/OctreeSystemComponent.h>

#include <Atom/RPI.Public/Culling.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Common/RPITestFixture.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace RPI;

    // The CullingTests fixture sets up a culling scene for testing culling.
    // It also creates some views and a varying number of cullable objects visible in each view.
    // It does not register the cullables with the culling scene, so their properties can be overridden
    // before registering in order to test different scenarios
    class CullingTests : public RPITestFixture
    {
        void SetUp() override
        {
            RPITestFixture::SetUp();

            m_executor = aznew TaskExecutor{};
            TaskExecutor::SetInstance(m_executor);

            m_octreeSystemComponent = new AzFramework::OctreeSystemComponent;
            m_sceneSystemComponent = new AzFramework::SceneSystemComponent;
            m_testScene = Scene::CreateScene(SceneDescriptor{});
            m_cullingScene = m_testScene->GetCullingScene();
            m_cullingScene->Activate(m_testScene.get());

            CreateTestViews();
            CreateTestObjects();
        }

        void TearDown() override
        {
            m_views.clear();
            m_cullingScene->Deactivate();
            m_testScene = nullptr;
            delete m_octreeSystemComponent;
            delete m_sceneSystemComponent;

            if (&TaskExecutor::Instance() == m_executor) // if this test created the default instance unset it before destroying it
            {
                TaskExecutor::SetInstance(nullptr);
            }
            azdestroy(m_executor);

            RPITestFixture::TearDown();
        }

    protected:
        static constexpr size_t testCameraCount = 4;
        static constexpr size_t visibleObjectUserDataOffset = 100;
        using TestCameraList = AZStd::vector<ViewPtr>;

        void Cull(TestCameraList& views)
        {
            m_cullingScene->BeginCulling(*m_testScene, views);

            // Create and submit work to the culling scene in a similar style as RPI::Scene::PrepareRender
            static const TaskDescriptor processCullablesDescriptor{ "RPI::Scene::ProcessCullables", "Graphics" };
            TaskGraphEvent processCullablesTGEvent{ "ProcessCullables Wait" };
            TaskGraph processCullablesTG{ "ProcessCullables" };
            for (ViewPtr& viewPtr : views)
            {
                processCullablesTG.AddTask(
                    processCullablesDescriptor,
                    [this, &viewPtr, &processCullablesTGEvent]()
                    {
                        TaskGraph subTaskGraph{ "ProcessCullables Subgraph" };
                        m_cullingScene->ProcessCullablesTG(*m_testScene, *viewPtr, subTaskGraph, processCullablesTGEvent);
                        if (!subTaskGraph.IsEmpty())
                        {
                            subTaskGraph.Detach();
                            subTaskGraph.Submit(&processCullablesTGEvent);
                        }
                    });
            }

            processCullablesTG.Submit(&processCullablesTGEvent);
            processCullablesTGEvent.Wait();
            m_cullingScene->EndCulling(*m_testScene, views);

            for (ViewPtr& viewPtr : views)
            {
                viewPtr->FinalizeVisibleObjectList();
            }
        }

        enum ViewIndex
        {
            YPositive = 0,
            XNegative,
            YNegative,
            XPositive
        };

        // Create four test cameras
        // Top down view of the cameras
        //    ___        +y
        //    \0/        |  
        //  |1>X<3|      | 
        //    /2\        |_____+x
        //    ---
        //
        void CreateTestViews()
        {
            ViewPtr viewYPositive = View::CreateView(Name("TestViewYPositive"), RPI::View::UsageCamera);
            ViewPtr viewXNegative = View::CreateView(Name("TestViewXNegative"), RPI::View::UsageShadow);
            ViewPtr viewYNegative = View::CreateView(Name("TestViewYNegative"), RPI::View::UsageShadow);
            ViewPtr viewXPositive = View::CreateView(Name("TestViewXPositive"), RPI::View::UsageReflectiveCubeMap);

            // Render everything by default
            RHI::DrawListMask drawListMask;
            drawListMask.reset();
            drawListMask.flip();

            viewYPositive->SetDrawListMask(drawListMask);
            viewXNegative->SetDrawListMask(drawListMask);
            viewYNegative->SetDrawListMask(drawListMask);
            viewXPositive->SetDrawListMask(drawListMask);

            const float fovY = DegToRad(90.0f);
            const float aspectRatio = 1.0f;
            const float nearDist = 0.1f;
            const float farDist = 100.0f;

            // These rotations represent a right-handed rotation around the z-up axis.
            // Starting with a view pointed straight down the y-forward axis,
            // these will rotate counter clockwise
            viewYPositive->SetCameraTransform(Matrix3x4::CreateIdentity());
            viewXNegative->SetCameraTransform(Matrix3x4::CreateRotationZ(DegToRad(90.0f)));
            viewYNegative->SetCameraTransform(Matrix3x4::CreateRotationZ(DegToRad(180.0f)));
            viewXPositive->SetCameraTransform(Matrix3x4::CreateRotationZ(DegToRad(270.0f)));

            // Matrix4x4::CreateProjection creates a view pointing up the positive z-axis.
            // Combine that with the rotation matrices above to get the 4 views.
            Matrix4x4 viewToClipZPositive = Matrix4x4::CreateIdentity();
            bool reverseDepth = true;
            MakePerspectiveFovMatrixRH(viewToClipZPositive, fovY, aspectRatio, nearDist, farDist, reverseDepth);
            viewYPositive->SetViewToClipMatrix(viewToClipZPositive);
            viewXNegative->SetViewToClipMatrix(viewToClipZPositive);
            viewYNegative->SetViewToClipMatrix(viewToClipZPositive);
            viewXPositive->SetViewToClipMatrix(viewToClipZPositive);

            m_views.resize(testCameraCount);
            m_views[YPositive] = viewYPositive;
            m_views[XNegative] = viewXNegative;
            m_views[YNegative] = viewYNegative;
            m_views[XPositive] = viewXPositive;
        }

        static void InitializeCullableFromAabb(Cullable& cullable, const Aabb& aabb, size_t index)
        {
            cullable.m_cullData.m_boundingObb = Obb::CreateFromAabb(aabb);
            cullable.m_cullData.m_boundingSphere = Sphere::CreateFromAabb(aabb);
            cullable.m_cullData.m_visibilityEntry.m_boundingVolume = aabb;
            cullable.m_cullData.m_visibilityEntry.m_typeFlags = AzFramework::VisibilityEntry::TYPE_RPI_VisibleObjectList;

            // Set all bits in the draw list mask by default, so everything will be rendered
            cullable.m_cullData.m_drawListMask.reset();
            cullable.m_cullData.m_drawListMask.flip();

            cullable.m_cullData.m_visibilityEntry.m_userData = &cullable;
            cullable.m_lodData.m_lodSelectionRadius = 0.5f * aabb.GetExtents().GetMaxElement();
            Cullable::LodData::Lod lod;
            lod.m_screenCoverageMin = 0.0f;
            lod.m_screenCoverageMax = 1.0f;

            // We're not actually using the user data for anything, but it needs to be non-null or the
            // VisibleObjectContext will assert. We'll use the index here, which could potentially be
            // used for validation in the tests (e.g., validate the Nth object was culled/visible).
            // However, we must also add an offset, or the 0th object will be treated as a nullptr.
            lod.m_visibleObjectUserData = reinterpret_cast<void*>(index + visibleObjectUserDataOffset);
            cullable.m_lodData.m_lods.push_back(lod);
        }

        // Create test objects visible to the cameras
        // (objects represented as dots in the diagram below)
        // Top down view of the cameras:
        //        ....
        //        ___         +y
        //        \0/         |  
        // ... |1> X <3| .    | 
        //        /2\         |_____+x
        //        ---
        //        ..
        //
        void CreateTestObjects()
        {
            // Four objects visible by the first camera
            Aabb aabb = Aabb::CreateCenterRadius(Vector3::CreateAxisY(10.0), 1.0);
            for (size_t i = 0; i < 10; ++i)
            {
                if (i == 4)
                {
                    // Three objects visible by the second camera
                    aabb = Aabb::CreateCenterRadius(Vector3::CreateAxisX(-10.0), 1.0);
                }
                if (i == 7)
                {
                    // Two objects visible by the third camera
                    aabb = Aabb::CreateCenterRadius(Vector3::CreateAxisY(-10.0), 1.0);
                }
                if (i == 9)
                {
                    // One object visible by the final camera
                    aabb = Aabb::CreateCenterRadius(Vector3::CreateAxisX(10.0), 1.0);
                }

                // We're initializing these cullables in place because the copy constructor for RPI::Cullbable is implicitely deleted
                InitializeCullableFromAabb(m_testObjects[i], aabb, i);
            }
        }

        TaskExecutor* m_executor;
        AzFramework::OctreeSystemComponent* m_octreeSystemComponent;
        AzFramework::SceneSystemComponent* m_sceneSystemComponent;
        ScenePtr m_testScene;
        CullingScene* m_cullingScene;
        AZStd::vector<ViewPtr> m_views;
        AZStd::array<Cullable, 10> m_testObjects;
    };

    TEST_F(CullingTests, VisibleObjectListTest)
    {
        for (Cullable& object : m_testObjects)
        {
            m_cullingScene->RegisterOrUpdateCullable(object);
        }

        Cull(m_views);

        EXPECT_EQ(m_views[YPositive]->GetVisibleObjectList().size(), 4);
        EXPECT_EQ(m_views[XNegative]->GetVisibleObjectList().size(), 3);
        EXPECT_EQ(m_views[YNegative]->GetVisibleObjectList().size(), 2);
        EXPECT_EQ(m_views[XPositive]->GetVisibleObjectList().size(), 1);

        for (Cullable& object : m_testObjects)
        {
            m_cullingScene->UnregisterCullable(object);
        }
    }
}
