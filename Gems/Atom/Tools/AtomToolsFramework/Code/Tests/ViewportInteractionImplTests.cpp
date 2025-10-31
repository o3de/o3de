/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <AtomToolsFramework/Viewport/RenderViewportWidget.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Viewport/ScreenGeometry.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <Tests/Utils/Printers.h>

namespace UnitTest
{
    class ViewportInteractionImplFixture : public ::testing::Test
    {
    public:
        static inline constexpr AzFramework::ViewportId TestViewportId = 1234;
        static inline constexpr AzFramework::ScreenSize ScreenDimensions = AzFramework::ScreenSize(1280, 720);

        static AzFramework::ScreenPoint ScreenCenter()
        {
            const auto halfScreenDimensions = ScreenDimensions * 0.5f;
            return AzFramework::ScreenPoint(halfScreenDimensions.m_width, halfScreenDimensions.m_height);
        }

        void SetUp() override
        {
            AZ::NameDictionary::Create();

            m_view = AZ::RPI::View::CreateView(AZ::Name("TestView"), AZ::RPI::View::UsageCamera);

            const auto aspectRatio = aznumeric_cast<float>(ScreenDimensions.m_width) / aznumeric_cast<float>(ScreenDimensions.m_height);

            AZ::Matrix4x4 viewToClipMatrix;
            AZ::MakePerspectiveFovMatrixRH(viewToClipMatrix, AZ::DegToRad(60.0f), aspectRatio, 0.1f, 1000.f, true);
            m_view->SetViewToClipMatrix(viewToClipMatrix);

            m_viewportInteractionImpl = AZStd::make_unique<AtomToolsFramework::ViewportInteractionImpl>(m_view);

            m_viewportInteractionImpl->m_deviceScalingFactorFn = []
            {
                return 1.0f;
            };
            m_viewportInteractionImpl->m_screenSizeFn = []
            {
                return ScreenDimensions;
            };

            m_viewportInteractionImpl->Connect(TestViewportId);
        }

        void TearDown() override
        {
            m_viewportInteractionImpl->Disconnect();
            m_viewportInteractionImpl.reset();

            m_view.reset();

            AZ::NameDictionary::Destroy();
        }

        AZ::RPI::ViewPtr m_view;
        AZStd::unique_ptr<AtomToolsFramework::ViewportInteractionImpl> m_viewportInteractionImpl;
    };

    // transform a point from screen space to world space, and then from world space back to screen space
    AzFramework::ScreenPoint ScreenToWorldToScreen(
        const AzFramework::ScreenPoint& screenPoint,
        AzToolsFramework::ViewportInteraction::ViewportInteractionRequests& viewportInteractionRequests)
    {
        const auto worldResult = viewportInteractionRequests.ViewportScreenToWorld(screenPoint);
        return viewportInteractionRequests.ViewportWorldToScreen(worldResult);
    }

#if AZ_TRAIT_DISABLE_FAILED_ARM64_TESTS
    TEST_F(ViewportInteractionImplFixture, DISABLED_ViewportInteractionRequestsMapsFromScreenToWorldAndBack)
#else
    TEST_F(ViewportInteractionImplFixture, ViewportInteractionRequestsMapsFromScreenToWorldAndBack)
#endif // AZ_TRAIT_DISABLE_FAILED_ARM64_TESTS
    {
        using AzFramework::ScreenPoint;

        m_view->SetCameraTransform(AZ::Matrix3x4::CreateFromMatrix3x3AndTranslation(
            AZ::Matrix3x3::CreateRotationZ(AZ::DegToRad(90.0f)), AZ::Vector3(10.0f, 0.0f, 5.0f)));

        {
            const auto expectedScreenPoint = ScreenPoint{ 600, 450 };
            const auto resultScreenPoint = ScreenToWorldToScreen(expectedScreenPoint, *m_viewportInteractionImpl);
            EXPECT_EQ(resultScreenPoint, expectedScreenPoint);
        }

        {
            auto expectedScreenPoint = ScreenCenter();
            const auto resultScreenPoint = ScreenToWorldToScreen(expectedScreenPoint, *m_viewportInteractionImpl);
            EXPECT_EQ(resultScreenPoint, expectedScreenPoint);
        }

        {
            const auto expectedScreenPoint = ScreenPoint{ 0, 0 };
            const auto resultScreenPoint = ScreenToWorldToScreen(expectedScreenPoint, *m_viewportInteractionImpl);
            EXPECT_EQ(resultScreenPoint, expectedScreenPoint);
        }

        {
            const auto expectedScreenPoint = ScreenPoint{ ScreenDimensions.m_width, ScreenDimensions.m_height };
            const auto resultScreenPoint = ScreenToWorldToScreen(expectedScreenPoint, *m_viewportInteractionImpl);
            EXPECT_EQ(resultScreenPoint, expectedScreenPoint);
        }
    }

    TEST_F(ViewportInteractionImplFixture, ScreenToWorldReturnsPositionOnNearClipPlaneInWorldSpace)
    {
        m_view->SetCameraTransform(AZ::Matrix3x4::CreateFromMatrix3x3AndTranslation(
            AZ::Matrix3x3::CreateRotationZ(AZ::DegToRad(-90.0f)), AZ::Vector3(20.0f, 0.0f, 0.0f)));

        const auto worldResult = m_viewportInteractionImpl->ViewportScreenToWorld(ScreenCenter());
        EXPECT_THAT(worldResult, IsClose(AZ::Vector3(20.1f, 0.0f, 0.0f)));
    }

    // note: values produced by reproducing in the editor viewport
    TEST_F(ViewportInteractionImplFixture, WorldToScreenGivesExpectedScreenCoordinates)
    {
        using AzFramework::ScreenPoint;

        {
            m_view->SetCameraTransform(AZ::Matrix3x4::CreateFromMatrix3x3AndTranslation(
                AZ::Matrix3x3::CreateRotationZ(AZ::DegToRad(160.0f)) * AZ::Matrix3x3::CreateRotationX(AZ::DegToRad(-18.0f)),
                AZ::Vector3(-21.0f, 2.5f, 6.0f)));

            const auto screenResult = m_viewportInteractionImpl->ViewportWorldToScreen(AZ::Vector3(-21.0f, -1.5f, 5.0f));
            EXPECT_EQ(screenResult, ScreenPoint(420, 326));
        }

        {
            m_view->SetCameraTransform(AZ::Matrix3x4::CreateFromMatrix3x3AndTranslation(
                AZ::Matrix3x3::CreateRotationZ(AZ::DegToRad(175.0f)) * AZ::Matrix3x3::CreateRotationX(AZ::DegToRad(-90.0f)),
                AZ::Vector3(-10.0f, -11.0f, 2.5f)));

            const auto screenResult = m_viewportInteractionImpl->ViewportWorldToScreen(AZ::Vector3(-10.0f, -10.5f, 0.5f));
            EXPECT_EQ(screenResult, ScreenPoint(654, 515));
        }

        {
            m_view->SetCameraTransform(AZ::Matrix3x4::CreateFromMatrix3x3AndTranslation(
                AZ::Matrix3x3::CreateRotationZ(AZ::DegToRad(70.0f)) * AZ::Matrix3x3::CreateRotationX(AZ::DegToRad(65.0f)),
                AZ::Vector3(-22.5f, -10.0f, 1.5f)));

            const auto screenResult = m_viewportInteractionImpl->ViewportWorldToScreen(AZ::Vector3(-23.0f, -9.5f, 3.0f));
            EXPECT_EQ(screenResult, ScreenPoint(754, 340));
        }
    }

    TEST_F(ViewportInteractionImplFixture, ScreenToWorldRayGivesGivesExpectedOriginAndDirection)
    {
        using AzFramework::ScreenPoint;

        m_view->SetCameraTransform(AZ::Matrix3x4::CreateFromMatrix3x3AndTranslation(
            AZ::Matrix3x3::CreateRotationZ(AZ::DegToRad(34.0f)) * AZ::Matrix3x3::CreateRotationX(AZ::DegToRad(-24.0f)),
            AZ::Vector3(-9.3f, -9.8f, 4.0f)));

        const auto ray = m_viewportInteractionImpl->ViewportScreenToWorldRay(ScreenPoint(832, 226));

        float unused;
        auto intersection =
            AZ::Intersect::IntersectRaySphere(ray.m_origin, ray.m_direction, AZ::Vector3(-14.0f, 5.7f, 0.75f), 0.5f, unused);

        EXPECT_EQ(intersection, AZ::Intersect::SphereIsectTypes::ISECT_RAY_SPHERE_ISECT);
    }

    TEST_F(ViewportInteractionImplFixture, ViewportInteractionRequestsReturnsNewViewWhenItIsChanged)
    {
        // Given
        const auto primaryViewTransform = AZ::Matrix3x4::CreateFromMatrix3x3AndTranslation(
            AZ::Matrix3x3::CreateRotationZ(AZ::DegToRad(90.0f)) * AZ::Matrix3x3::CreateRotationX(AZ::DegToRad(-45.0f)),
            AZ::Vector3(-10.0f, -15.0f, 20.0f));

        m_view->SetCameraTransform(primaryViewTransform);

        AZ::RPI::ViewPtr secondaryView = AZ::RPI::View::CreateView(AZ::Name("SecondaryView"), AZ::RPI::View::UsageCamera);

        const auto secondaryViewTransform = AZ::Matrix3x4::CreateFromMatrix3x3AndTranslation(
            AZ::Matrix3x3::CreateRotationZ(AZ::DegToRad(-90.0f)) * AZ::Matrix3x3::CreateRotationX(AZ::DegToRad(30.0f)),
            AZ::Vector3(-50.0f, -25.0f, 10.0f));

        secondaryView->SetCameraTransform(secondaryViewTransform);

        // When
        AZ::RPI::ViewportContextIdNotificationBus::Event(
            TestViewportId, &AZ::RPI::ViewportContextIdNotificationBus::Events::OnViewportDefaultViewChanged, secondaryView);

        // retrieve updated camera transform
        AzFramework::CameraState cameraState;
        AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::EventResult(
            cameraState, TestViewportId, &AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::Events::GetCameraState);

        const auto cameraTransform = AzFramework::CameraTransform(cameraState);

        // Then
        // camera transform matches that of the secondary view
        EXPECT_THAT(cameraTransform, IsClose(secondaryViewTransform));
    }
} // namespace UnitTest
