/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Math/Spline.h>
#include <AzCore/Math/VertexContainerInterface.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzFramework/Application/Application.h>
#include <AzFramework/Components/TransformComponent.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <Shape/SplineComponent.h>

namespace UnitTest
{
    class SplineComponentTests
        : public LeakDetectionFixture
    {
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_splineComponentDescriptor;

    public:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();

            m_transformComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(AzFramework::TransformComponent::CreateDescriptor());
            m_transformComponentDescriptor->Reflect(&(*m_serializeContext));
            m_splineComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(LmbrCentral::SplineComponent::CreateDescriptor());
            m_splineComponentDescriptor->Reflect(&(*m_serializeContext));
        }

        void TearDown() override
        {
            m_transformComponentDescriptor.reset();
            m_splineComponentDescriptor.reset();
            m_serializeContext.reset();
            LeakDetectionFixture::TearDown();
        }

        void Spline_AddUpdate() const
        {
            AZ::Entity entity;
            entity.CreateComponent<LmbrCentral::SplineComponent>();
            entity.CreateComponent<AzFramework::TransformComponent>();

            entity.Init();
            entity.Activate();

            LmbrCentral::SplineComponentRequestBus::Event(entity.GetId(), &LmbrCentral::SplineComponentRequests::AddVertex, AZ::Vector3(0.0f, 0.0f, 0.0f));
            LmbrCentral::SplineComponentRequestBus::Event(entity.GetId(), &LmbrCentral::SplineComponentRequests::AddVertex, AZ::Vector3(0.0f, 10.0f, 0.0f));
            LmbrCentral::SplineComponentRequestBus::Event(entity.GetId(), &LmbrCentral::SplineComponentRequests::AddVertex, AZ::Vector3(10.0f, 10.0f, 0.0f));
            LmbrCentral::SplineComponentRequestBus::Event(entity.GetId(), &LmbrCentral::SplineComponentRequests::AddVertex, AZ::Vector3(10.0f, 0.0f, 0.0f));

            AZ::ConstSplinePtr spline;
            LmbrCentral::SplineComponentRequestBus::EventResult(spline, entity.GetId(), &LmbrCentral::SplineComponentRequests::GetSpline);

            AZ_TEST_ASSERT(spline->GetVertexCount() == 4);

            LmbrCentral::SplineComponentRequestBus::Event(entity.GetId(), &LmbrCentral::SplineComponentRequests::RemoveVertex, 0);
            AZ_TEST_ASSERT(spline->GetVertexCount() == 3);

            LmbrCentral::SplineComponentRequestBus::Event(entity.GetId(), &LmbrCentral::SplineComponentRequests::UpdateVertex, 0, AZ::Vector3(10.0f, 10.0f, 10.0f));
            AZ_TEST_ASSERT(spline->GetVertex(0).IsClose(AZ::Vector3(10.0f, 10.0f, 10.0f)));

            LmbrCentral::SplineComponentRequestBus::Event(entity.GetId(), &LmbrCentral::SplineComponentRequests::InsertVertex, 1, AZ::Vector3(20.0f, 20.0f, 20.0f));
            AZ_TEST_ASSERT(spline->GetVertex(1).IsClose(AZ::Vector3(20.0f, 20.0f, 20.0f)));
            AZ_TEST_ASSERT(spline->GetVertexCount() == 4);
            AZ_TEST_ASSERT(spline->GetVertex(2).IsClose(AZ::Vector3(10.0f, 10.0f, 0.0f)));
        }

        void Spline_CopyModify() const
        {
            AZ::Entity entity;
            // add spline component - default to Linear
            entity.CreateComponent<LmbrCentral::SplineComponent>();
            entity.CreateComponent<AzFramework::TransformComponent>();

            entity.Init();
            entity.Activate();

            // set vertices via vertex container bus
            LmbrCentral::SplineComponentRequestBus::Event(entity.GetId(), &LmbrCentral::SplineComponentRequests::SetVertices, AZStd::vector<AZ::Vector3>{ AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(0.0f, 10.0f, 0.0f), AZ::Vector3(10.0f, 10.0f, 0.0f), AZ::Vector3(10.0f, 0.0f, 0.0f) });

            // get linear spline from entity
            AZ::ConstSplinePtr linearSplinePtr;
            LmbrCentral::SplineComponentRequestBus::EventResult(linearSplinePtr, entity.GetId(), &LmbrCentral::SplineComponentRequests::GetSpline);
            AZ_TEST_ASSERT(linearSplinePtr->GetVertexCount() == 4);

            // clear vertices via vertex container bus
            LmbrCentral::SplineComponentRequestBus::Event(entity.GetId(), &LmbrCentral::SplineComponentRequests::ClearVertices);
            AZ_TEST_ASSERT(linearSplinePtr->GetVertexCount() == 0);

            // set vertices via vertex container bus
            LmbrCentral::SplineComponentRequestBus::Event(entity.GetId(), &LmbrCentral::SplineComponentRequests::SetVertices, AZStd::vector<AZ::Vector3>{ AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(0.0f, 5.0f, 0.0f), AZ::Vector3(5.0f, 5.0f, 0.0f), AZ::Vector3(5.0f, 0.0f, 0.0f) });
            AZ_TEST_ASSERT(linearSplinePtr->GetVertexCount() == 4);

            // change spline type to Bezier
            LmbrCentral::SplineComponentRequestBus::Event(entity.GetId(), &LmbrCentral::SplineComponentRequests::ChangeSplineType, LmbrCentral::SplineType::BEZIER);

            // check data was created after change correctly
            AZ::ConstSplinePtr bezierSplinePtr;
            LmbrCentral::SplineComponentRequestBus::EventResult(bezierSplinePtr, entity.GetId(), &LmbrCentral::SplineComponentRequests::GetSpline);
            if (const AZ::BezierSpline* bezierSpline = azrtti_cast<const AZ::BezierSpline*>(bezierSplinePtr.get()))
            {
                AZ_TEST_ASSERT(bezierSpline->GetBezierData().size() == 4);
                AZ_TEST_ASSERT(bezierSpline->GetVertexCount() == 4);
            }
            else
            {
                AZ_TEST_ASSERT(false);
            }

            // check copy constructor
            {
                AZ::SplinePtr newBezierSplinePtr = AZStd::make_shared<AZ::BezierSpline>(*bezierSplinePtr.get());
                if (const AZ::BezierSpline* bezierSpline = azrtti_cast<const AZ::BezierSpline*>(newBezierSplinePtr.get()))
                {
                    AZ_TEST_ASSERT(bezierSpline->GetBezierData().size() == 4);
                    AZ_TEST_ASSERT(bezierSpline->GetVertexCount() == 4);
                }
                else
                {
                    AZ_TEST_ASSERT(false);
                }
            }

            // check assignment operator
            {
                if (azrtti_cast<const AZ::BezierSpline*>(bezierSplinePtr.get()))
                {
                    AZ::BezierSpline newBezierSpline;
                    newBezierSpline = *bezierSplinePtr.get();

                    AZ_TEST_ASSERT(newBezierSpline.GetBezierData().size() == 4);
                    AZ_TEST_ASSERT(newBezierSpline.GetVertexCount() == 4);
                }
                else
                {
                    AZ_TEST_ASSERT(false);
                }
            }

            // set vertices for Bezier spline
            LmbrCentral::SplineComponentRequestBus::Event(entity.GetId(), &LmbrCentral::SplineComponentRequests::SetVertices, AZStd::vector<AZ::Vector3>{ AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(0.0f, 0.0f, 10.0f) });
            if (const AZ::BezierSpline* bezierSpline = azrtti_cast<const AZ::BezierSpline*>(bezierSplinePtr.get()))
            {
                AZ_TEST_ASSERT(bezierSpline->GetBezierData().size() == 2);
                AZ_TEST_ASSERT(bezierSpline->GetVertexCount() == 2);
            }
            else
            {
                AZ_TEST_ASSERT(false);
            }

            // change spline type to CatmullRom
            LmbrCentral::SplineComponentRequestBus::Event(entity.GetId(), &LmbrCentral::SplineComponentRequests::ChangeSplineType, LmbrCentral::SplineType::CATMULL_ROM);
            
            AZ::ConstSplinePtr catmullRomSplinePtr;
            LmbrCentral::SplineComponentRequestBus::EventResult(catmullRomSplinePtr, entity.GetId(), &LmbrCentral::SplineComponentRequests::GetSpline);
            if (const AZ::CatmullRomSpline* catmullRomSpline = azrtti_cast<const AZ::CatmullRomSpline*>(catmullRomSplinePtr.get()))
            {
                AZ_TEST_ASSERT(catmullRomSpline->GetVertexCount() == 2);
            }
            else
            {
                AZ_TEST_ASSERT(false);
            }
        }
    };

    TEST_F(SplineComponentTests, Spline_AddUpdate)
    {
        Spline_AddUpdate();
    }

    TEST_F(SplineComponentTests, Spline_CopyModify)
    {
        Spline_CopyModify();
    }
}
