/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Spline.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Math/MathTestHelpers.h>

using namespace AZ;

namespace UnitTest
{
    class MATH_SplineTest
        : public LeakDetectionFixture
    {
    public:
        void Linear_NearestAddressFromPosition()
        {
            {
                LinearSpline linearSpline;

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    PositionSplineQueryResult posSplineQueryResult = linearSpline.GetNearestAddressPosition(Vector3(7.5f, 2.5f, 0.0f));
                    EXPECT_EQ(posSplineQueryResult.m_splineAddress.m_segmentIndex, 1);
                    EXPECT_NEAR(posSplineQueryResult.m_splineAddress.m_segmentFraction, 0.5f, AZ::Constants::Tolerance);
                }

                {
                    PositionSplineQueryResult posSplineQueryResult = linearSpline.GetNearestAddressPosition(Vector3(-1.0f, -1.0f, 0.0f));
                    EXPECT_EQ(posSplineQueryResult.m_splineAddress.m_segmentIndex, 0);
                    EXPECT_NEAR(posSplineQueryResult.m_splineAddress.m_segmentFraction, 0.0f, AZ::Constants::Tolerance);
                }

                {
                    PositionSplineQueryResult posSplineQueryResult = linearSpline.GetNearestAddressPosition(Vector3(-1.0f, 6.0f, 0.0f));
                    EXPECT_EQ(posSplineQueryResult.m_splineAddress.m_segmentIndex, 2);
                    EXPECT_NEAR(posSplineQueryResult.m_splineAddress.m_segmentFraction, 1.0f, AZ::Constants::Tolerance);
                }

                {
                    PositionSplineQueryResult posSplineQueryResult = linearSpline.GetNearestAddressPosition(Vector3(2.5f, 6.0f, 0.0f));
                    EXPECT_EQ(posSplineQueryResult.m_splineAddress.m_segmentIndex, 2);
                    EXPECT_NEAR(posSplineQueryResult.m_splineAddress.m_segmentFraction, 0.5f, AZ::Constants::Tolerance);
                }
            }

            {
                LinearSpline linearSpline;
                linearSpline.SetClosed(true);

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    PositionSplineQueryResult posSplineQueryResult = linearSpline.GetNearestAddressPosition(Vector3(-2.0f, 2.5f, 0.0f));
                    EXPECT_EQ(posSplineQueryResult.m_splineAddress.m_segmentIndex, 3);
                    EXPECT_NEAR(posSplineQueryResult.m_splineAddress.m_segmentFraction, 0.5f, AZ::Constants::Tolerance);
                }

                {
                    PositionSplineQueryResult posSplineQueryResult = linearSpline.GetNearestAddressPosition(Vector3(-2.0f, 4.0f, 0.0f));
                    EXPECT_EQ(posSplineQueryResult.m_splineAddress.m_segmentIndex, 3);
                    EXPECT_NEAR(posSplineQueryResult.m_splineAddress.m_segmentFraction, 0.2f, AZ::Constants::Tolerance);
                }
            }
        }

        void Linear_NearestAddressFromDirection()
        {
            {
                LinearSpline linearSpline;

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    RaySplineQueryResult raySplineQueryResult = linearSpline.GetNearestAddressRay(Vector3(2.5f, -2.5f, 1.0f), Vector3(0.0f, 1.0f, 0.0f));
                    EXPECT_EQ(raySplineQueryResult.m_splineAddress.m_segmentIndex, 0);
                    EXPECT_NEAR(raySplineQueryResult.m_splineAddress.m_segmentFraction, 0.5f, AZ::Constants::Tolerance);
                }

                {
                    RaySplineQueryResult raySplineQueryResult = linearSpline.GetNearestAddressRay(Vector3(2.5f, -10.0f, 0.0f), Vector3(1.0f, 1.0f, 0.0f));
                    EXPECT_EQ(raySplineQueryResult.m_splineAddress.m_segmentIndex, 0);
                    EXPECT_NEAR(raySplineQueryResult.m_splineAddress.m_segmentFraction, 1.0f, AZ::Constants::Tolerance);
                }

                {
                    RaySplineQueryResult raySplineQueryResult = linearSpline.GetNearestAddressRay(Vector3(7.5f, 2.5f, 0.0f), Vector3(-1.0f, 1.0f, 0.0f));
                    EXPECT_EQ(raySplineQueryResult.m_splineAddress.m_segmentIndex, 1);
                    EXPECT_NEAR(raySplineQueryResult.m_splineAddress.m_segmentFraction, 1.0f, AZ::Constants::Tolerance);
                }

                {
                    RaySplineQueryResult raySplineQueryResult = linearSpline.GetNearestAddressRay(Vector3(-2.5f, 2.5f, -1.0f), Vector3(1.0f, 1.0f, 0.0f));
                    EXPECT_EQ(raySplineQueryResult.m_splineAddress.m_segmentIndex, 2);
                    EXPECT_NEAR(raySplineQueryResult.m_splineAddress.m_segmentFraction, 1.0f, AZ::Constants::Tolerance);
                }
            }

            {
                LinearSpline linearSpline;
                linearSpline.SetClosed(true);

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    RaySplineQueryResult raySplineQueryResult = linearSpline.GetNearestAddressRay(Vector3(-2.5f, 2.5f, 0.0f), Vector3(0.0f, 0.0f, -1.0f));
                    EXPECT_EQ(raySplineQueryResult.m_splineAddress.m_segmentIndex, 3);
                    EXPECT_NEAR(raySplineQueryResult.m_splineAddress.m_segmentFraction, 0.5f, AZ::Constants::Tolerance);
                }

                {
                    RaySplineQueryResult raySplineQueryResult = linearSpline.GetNearestAddressRay(Vector3(-2.5f, 2.5f, 0.0f), Vector3(1.0f, 0.0f, 0.0f));
                    EXPECT_EQ(raySplineQueryResult.m_splineAddress.m_segmentIndex, 3);
                    EXPECT_NEAR(raySplineQueryResult.m_splineAddress.m_segmentFraction, 0.5f, AZ::Constants::Tolerance);
                }
            }
        }

        void Linear_AddressByDistance()
        {
            {
                LinearSpline linearSpline;

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByDistance(7.5f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 1);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 0.5f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByDistance(15.0f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 2);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 1.0f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByDistance(0.0f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 0);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 0.0f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByDistance(11.0f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 2);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 0.2f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByDistance(3.0f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 0);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 0.6f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByDistance(20.0f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 2);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 1.0f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByDistance(-5.0f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 0);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 0.0f, AZ::Constants::Tolerance);
                }
            }

            {
                LinearSpline linearSpline;
                linearSpline.SetClosed(true);

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByDistance(17.5f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 3);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 0.5f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByDistance(20.0f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 3);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 1.0f, AZ::Constants::Tolerance);
                }
            }
        }

        void Linear_AddressByFraction()
        {
            {
                LinearSpline linearSpline;

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByFraction(0.5f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 1);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 0.5f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByFraction(1.0f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 2);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 1.0f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByFraction(0.0f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 0);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 0.0f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByFraction(0.6f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 1);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 0.8f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByFraction(0.2f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 0);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 0.6f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByFraction(1.5f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 2);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 1.0f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByFraction(-0.5f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 0);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 0.0f, AZ::Constants::Tolerance);
                }
            }

            {
                LinearSpline linearSpline;
                linearSpline.SetClosed(true);

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByFraction(0.5f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 2);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 0.0f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByFraction(1.0f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 3);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 1.0f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByFraction(0.8f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 3);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 0.2f, AZ::Constants::Tolerance);
                }
            }
        }

        void Linear_GetPosition()
        {
            {
                LinearSpline linearSpline;

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    Vector3 position = linearSpline.GetPosition(linearSpline.GetAddressByFraction(0.5f));
                    EXPECT_THAT(position, IsClose(Vector3(5.0f, 2.5f, 0.0f)));
                }

                {
                    Vector3 position = linearSpline.GetPosition(SplineAddress());
                    EXPECT_THAT(position, IsClose(Vector3(0.0f, 0.0f, 0.0f)));
                }

                {
                    Vector3 position = linearSpline.GetPosition(SplineAddress(linearSpline.GetSegmentCount() - 1, 1.0f));
                    EXPECT_THAT(position, IsClose(Vector3(0.0f, 5.0f, 0.0f)));
                }

                {
                    Vector3 position = linearSpline.GetPosition(linearSpline.GetAddressByDistance(4.0f));
                    EXPECT_THAT(position, IsClose(Vector3(4.0f, 0.0f, 0.0f)));
                }

                {
                    Vector3 position = linearSpline.GetPosition(SplineAddress(3, 0.0f));
                    EXPECT_THAT(position, IsClose(Vector3(0.0f, 5.0f, 0.0f)));
                }

                {
                    // out of bounds access (return position of last vertex - clamped)
                    Vector3 position = linearSpline.GetPosition(SplineAddress(5, 0.0f));
                    EXPECT_THAT(position, IsClose(Vector3(0.0f, 5.0f, 0.0f)));
                }
            }

            {
                LinearSpline linearSpline;
                linearSpline.SetClosed(true);

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    Vector3 position = linearSpline.GetPosition(linearSpline.GetAddressByFraction(0.5f));
                    EXPECT_THAT(position, IsClose(Vector3(5.0f, 5.0f, 0.0f)));
                }

                {
                    Vector3 position = linearSpline.GetPosition(linearSpline.GetAddressByDistance(20.0f));
                    EXPECT_THAT(position, IsClose(Vector3(0.0f, 0.0f, 0.0f)));
                }

                {
                    Vector3 position = linearSpline.GetPosition(linearSpline.GetAddressByDistance(18.0f));
                    EXPECT_THAT(position, IsClose(Vector3(0.0f, 2.0f, 0.0f)));
                }

                {
                    // out of bounds access (return position of last/first vertex - clamped)
                    Vector3 position = linearSpline.GetPosition(SplineAddress(5, 0.0f));
                    EXPECT_THAT(position, IsClose(Vector3(0.0f, 0.0f, 0.0f)));
                }
            }
        }

        void Linear_GetNormal()
        {
            {
                LinearSpline linearSpline;

                // n shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));

                {
                    Vector3 normal = linearSpline.GetNormal(linearSpline.GetAddressByFraction(0.5f));
                    EXPECT_THAT(normal, IsClose(Vector3(0.0f, 1.0f, 0.0f)));
                }

                {
                    Vector3 normal = linearSpline.GetNormal(SplineAddress());
                    EXPECT_THAT(normal, IsClose(Vector3(-1.0f, 0.0f, 0.0f)));
                }

                {
                    Vector3 normal = linearSpline.GetNormal(SplineAddress(linearSpline.GetSegmentCount() - 1, 1.0f));
                    EXPECT_THAT(normal, IsClose(Vector3(1.0f, 0.0f, 0.0f)));
                }

                {
                    Vector3 normal = linearSpline.GetNormal(linearSpline.GetAddressByDistance(4.0f));
                    EXPECT_THAT(normal, IsClose(Vector3(-1.0f, 0.0f, 0.0f)));
                }

                {
                    // out of bounds access (return normal of last vertex - clamped)
                    Vector3 normal = linearSpline.GetNormal(SplineAddress(5, 0.0f));
                    EXPECT_THAT(normal, IsClose(Vector3(1.0f, 0.0f, 0.0f)));
                }
            }

            {
                LinearSpline linearSpline;

                // n shape spline (yz plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 5.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 5.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                Vector3 normal = linearSpline.GetNormal(linearSpline.GetAddressByDistance(4.0f));
                EXPECT_THAT(normal, IsClose(Vector3(0.0f, 0.0f, 0.0f)));
            }

            {
                LinearSpline linearSpline;
                linearSpline.SetClosed(true);

                // n shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));

                {
                    Vector3 normal = linearSpline.GetNormal(linearSpline.GetAddressByDistance(20.0f));
                    EXPECT_THAT(normal, IsClose(Vector3(0.0f, -1.0f, 0.0f)));
                }

                {
                    Vector3 normal = linearSpline.GetNormal(linearSpline.GetAddressByFraction(0.8f));
                    EXPECT_THAT(normal, IsClose(Vector3(0.0f, -1.0f, 0.0f)));
                }
            }
        }

        void Linear_GetTangent()
        {
            {
                LinearSpline linearSpline;

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    Vector3 tangent = linearSpline.GetTangent(linearSpline.GetAddressByFraction(0.5f));
                    EXPECT_THAT(tangent, IsClose(Vector3(0.0f, 1.0f, 0.0f)));
                }

                {
                    Vector3 tangent = linearSpline.GetTangent(SplineAddress());
                    EXPECT_THAT(tangent, IsClose(Vector3(1.0f, 0.0f, 0.0f)));
                }

                {
                    Vector3 tangent = linearSpline.GetTangent(SplineAddress(linearSpline.GetSegmentCount() - 1, 1.0f));
                    EXPECT_THAT(tangent, IsClose(Vector3(-1.0f, 0.0f, 0.0f)));
                }

                {
                    Vector3 tangent = linearSpline.GetTangent(linearSpline.GetAddressByDistance(4.0f));
                    EXPECT_THAT(tangent, IsClose(Vector3(1.0f, 0.0f, 0.0f)));
                }
            }

            {
                LinearSpline linearSpline;
                linearSpline.SetClosed(true);

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    Vector3 tangent = linearSpline.GetTangent(SplineAddress(linearSpline.GetSegmentCount() - 1, 1.0f));
                    EXPECT_THAT(tangent, IsClose(Vector3(0.0f, -1.0f, 0.0f)));
                }

                {
                    Vector3 tangent = linearSpline.GetTangent(linearSpline.GetAddressByDistance(16.0f));
                    EXPECT_THAT(tangent, IsClose(Vector3(0.0f, -1.0f, 0.0f)));
                }
            }
        }

        void Linear_Length()
        {
            {
                LinearSpline linearSpline;

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    float splineLength = linearSpline.GetSplineLength();
                    EXPECT_NEAR(splineLength, 15.0f, AZ::Constants::Tolerance);
                }

                {
                    float segmentLength = linearSpline.GetSegmentLength(0);
                    EXPECT_NEAR(segmentLength, 5.0f, AZ::Constants::Tolerance);
                }

                {
                    float segmentLength = linearSpline.GetSegmentLength(2);
                    EXPECT_NEAR(segmentLength, 5.0f, AZ::Constants::Tolerance);
                }

                {
                    float segmentLength = linearSpline.GetSegmentLength(4);
                    EXPECT_NEAR(segmentLength, 0.0f, AZ::Constants::Tolerance);
                }
            }

            {
                LinearSpline linearSpline;

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(2.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(2.0f, 3.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(4.0f, 3.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(4.0f, 10.0f, 0.0f));

                {
                    float splineLength = linearSpline.GetSplineLength();
                    EXPECT_NEAR(splineLength, 14.0f, AZ::Constants::Tolerance);
                }

                {
                    float segmentLength = linearSpline.GetSegmentLength(0);
                    EXPECT_NEAR(segmentLength, 2.0f, AZ::Constants::Tolerance);
                }

                {
                    float segmentLength = linearSpline.GetSegmentLength(3);
                    EXPECT_NEAR(segmentLength, 7.0f, AZ::Constants::Tolerance);
                }
            }

            {
                LinearSpline linearSpline;
                linearSpline.SetClosed(true);

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    float splineLength = linearSpline.GetSplineLength();
                    EXPECT_NEAR(splineLength, 20.0f, AZ::Constants::Tolerance);
                }

                {
                    float segmentLength = linearSpline.GetSegmentLength(3);
                    EXPECT_NEAR(segmentLength, 5.0f, AZ::Constants::Tolerance);
                }
            }

            {
                LinearSpline linearSpline;
                linearSpline.SetClosed(true);

                // single line (y axis)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));

                {
                    float splineLength = linearSpline.GetSplineLength();
                    EXPECT_NEAR(splineLength, 20.0f, AZ::Constants::Tolerance);
                }

                {
                    float segmentLength = linearSpline.GetSegmentLength(1);
                    EXPECT_NEAR(segmentLength, 10.0f, AZ::Constants::Tolerance);
                }
            }
        }

        void Linear_Aabb()
        {
            {
                LinearSpline linearSpline;

                // slight n curve spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                AZ::Aabb aabb;
                linearSpline.GetAabb(aabb);

                EXPECT_THAT(aabb.GetMin(), IsClose(Vector3(0.0f, 0.0f, 0.0f)));
                EXPECT_THAT(aabb.GetMax(), IsClose(Vector3(10.0f, 10.0f, 0.0f)));
            }

            {
                LinearSpline linearSpline;
                linearSpline.SetClosed(true);

                // slight n curve spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                AZ::Aabb aabb;
                linearSpline.GetAabb(aabb);

                EXPECT_THAT(aabb.GetMin(), IsClose(Vector3(0.0f, 0.0f, 0.0f)));
                EXPECT_THAT(aabb.GetMax(), IsClose(Vector3(10.0f, 10.0f, 0.0f)));
            }

            {
                LinearSpline linearSpline;
                AZ::Transform translation = AZ::Transform::CreateFromMatrix3x3AndTranslation(Matrix3x3::CreateIdentity(), Vector3(25.0f, 25.0f, 0.0f));

                // slight n curve spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                AZ::Aabb aabb;
                linearSpline.GetAabb(aabb, translation);

                EXPECT_THAT(aabb.GetMin(), IsClose(Vector3(25.0f, 25.0f, 0.0f)));
                EXPECT_THAT(aabb.GetMax(), IsClose(Vector3(35.0f, 35.0f, 0.0f)));
            }
        }

        void Linear_GetLength()
        {
            {
                LinearSpline spline;

                spline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                spline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                spline.m_vertexContainer.AddVertex(Vector3(0.0f, 20.0f, 0.0f));

                auto distance = spline.GetLength(AZ::SplineAddress(1, 0.5f));
                EXPECT_NEAR(distance, 15.0f, 1e-4);

                distance = spline.GetLength(AZ::SplineAddress(0, 0.0f));
                EXPECT_NEAR(distance, 0.0f, 1e-4);

                distance = spline.GetLength(AZ::SplineAddress(1, 1.0f));
                EXPECT_NEAR(distance, 20.0f, 1e-4);

                distance = spline.GetLength(AZ::SplineAddress(5, 1.0f));
                EXPECT_NEAR(distance, spline.GetSplineLength(), 1e-4);
            }

            {
                LinearSpline spline;

                spline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                spline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                spline.m_vertexContainer.AddVertex(Vector3(20.0f, 20.0f, 10.0f));
                spline.m_vertexContainer.AddVertex(Vector3(10.0f, 15.0f, -10.0f));

                float length = spline.GetSplineLength();

                for (float t = 0.0f; t <= length; t += 0.5f)
                {
                    auto result = spline.GetLength(spline.GetAddressByDistance(t));
                    EXPECT_NEAR(result, t, 1e-4);
                }
            }

            {
                LinearSpline spline;

                auto result = spline.GetLength(AZ::SplineAddress(0, 0.0f));
                EXPECT_NEAR(result, 0.0f, 1e-4);

                spline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                result = spline.GetLength(AZ::SplineAddress(0, 0.0f));
                EXPECT_NEAR(result, 0.0f, 1e-4);
            }
        }

        void CatmullRom_NearestAddressFromPosition()
        {
            {
                CatmullRomSpline catmullRomSpline;

                // slight backward C curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    PositionSplineQueryResult posSplineQueryResult = catmullRomSpline.GetNearestAddressPosition(Vector3(7.5f, 2.5f, 0.0f));
                    EXPECT_EQ(posSplineQueryResult.m_splineAddress.m_segmentIndex, 1);
                    EXPECT_NEAR(posSplineQueryResult.m_splineAddress.m_segmentFraction, 0.5f, AZ::Constants::Tolerance);
                }

                {
                    PositionSplineQueryResult posSplineQueryResult = catmullRomSpline.GetNearestAddressPosition(Vector3(-1.0f, -1.0f, 0.0f));
                    EXPECT_EQ(posSplineQueryResult.m_splineAddress.m_segmentIndex, 1);
                    EXPECT_NEAR(posSplineQueryResult.m_splineAddress.m_segmentFraction, 0.0f, AZ::Constants::Tolerance);
                }

                {
                    PositionSplineQueryResult posSplineQueryResult = catmullRomSpline.GetNearestAddressPosition(Vector3(-1.0f, 6.0f, 0.0f));
                    EXPECT_EQ(posSplineQueryResult.m_splineAddress.m_segmentIndex, 1);
                    EXPECT_NEAR(posSplineQueryResult.m_splineAddress.m_segmentFraction, 1.0f, AZ::Constants::Tolerance);
                }

                {
                    PositionSplineQueryResult posSplineQueryResult = catmullRomSpline.GetNearestAddressPosition(Vector3(2.5f, 6.0f, 0.0f));
                    EXPECT_EQ(posSplineQueryResult.m_splineAddress.m_segmentIndex, 1);
                    EXPECT_NEAR(posSplineQueryResult.m_splineAddress.m_segmentFraction, 1.0f, AZ::Constants::Tolerance);
                }
            }

            {
                CatmullRomSpline catmullRomSpline;
                catmullRomSpline.SetClosed(true);

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    PositionSplineQueryResult posSplineQueryResult = catmullRomSpline.GetNearestAddressPosition(Vector3(5.0f, -2.5f, 0.0f));
                    EXPECT_EQ(posSplineQueryResult.m_splineAddress.m_segmentIndex, 3);
                    EXPECT_NEAR(posSplineQueryResult.m_splineAddress.m_segmentFraction, 0.5f, AZ::Constants::Tolerance);
                }
            }
        }

        void CatmullRom_NearestAddressFromDirection()
        {
            {
                CatmullRomSpline catmullRomSpline;

                // slight backward C curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    RaySplineQueryResult raySplineQueryResult = catmullRomSpline.GetNearestAddressRay(Vector3(2.5f, -2.5f, 1.0f), Vector3(0.0f, 1.0f, 0.0f));
                    EXPECT_EQ(raySplineQueryResult.m_splineAddress.m_segmentIndex, 1);
                    EXPECT_NEAR(raySplineQueryResult.m_splineAddress.m_segmentFraction, 0.0f, AZ::Constants::Tolerance);
                }

                {
                    RaySplineQueryResult raySplineQueryResult = catmullRomSpline.GetNearestAddressRay(Vector3(2.5f, -10.0f, 0.0f), Vector3(1.0f, 1.0f, 0.0f));
                    EXPECT_EQ(raySplineQueryResult.m_splineAddress.m_segmentIndex, 1);
                    EXPECT_NEAR(raySplineQueryResult.m_splineAddress.m_segmentFraction, 0.0f, AZ::Constants::Tolerance);
                }

                {
                    RaySplineQueryResult raySplineQueryResult = catmullRomSpline.GetNearestAddressRay(Vector3(7.5f, 2.5f, 0.0f), Vector3(-1.0f, 0.0f, 0.0f));
                    EXPECT_EQ(raySplineQueryResult.m_splineAddress.m_segmentIndex, 1);
                    EXPECT_NEAR(raySplineQueryResult.m_splineAddress.m_segmentFraction, 0.5f, AZ::Constants::Tolerance);
                }

                {
                    RaySplineQueryResult raySplineQueryResult = catmullRomSpline.GetNearestAddressRay(Vector3(-2.5f, 7.5f, -1.0f), Vector3(1.0f, 0.0f, 0.0f));
                    EXPECT_EQ(raySplineQueryResult.m_splineAddress.m_segmentIndex, 1);
                    EXPECT_NEAR(raySplineQueryResult.m_splineAddress.m_segmentFraction, 1.0f, AZ::Constants::Tolerance);
                }
            }

            {
                CatmullRomSpline catmullRomSpline;
                catmullRomSpline.SetClosed(true);

                // slight backward C curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    RaySplineQueryResult raySplineQueryResult = catmullRomSpline.GetNearestAddressRay(Vector3(-2.5f, 2.5f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
                    EXPECT_EQ(raySplineQueryResult.m_splineAddress.m_segmentIndex, 3);
                    EXPECT_NEAR(raySplineQueryResult.m_splineAddress.m_segmentFraction, 0.5f, AZ::Constants::Tolerance);
                }
            }
        }

        /*
            Catmull-Rom Sample Points - (0.0f, 0.0f, 0.0f), (0.0f, 10.0f, 0.0f), (10.0f, 10.0f, 0.0f), (10.0f, 0.0f, 0.0f)
            (calculated using https://www.desmos.com/calculator/552cpvzfxw)

            t = 0         pos = (0.0f, 10.0f, 0.0f)
            t = 0.125     pos = (0.83984375.0f, 10.546875.0f, 0.0f)
            t = 0.25      pos = (2.03125.0f, 10.9375.0f, 0.0f)
            t = 0.375     pos = (3.45703125.0f, 11.171875.0f, 0.0f)
            t = 0.5       pos = (5.0.0f, 11.25.0f, 0.0f)
            t = 0.625     pos = (6.54296875.0f, 11.171875.0f, 0.0f)
            t = 0.75      pos = (7.96875.0f, 10.9375.0f, 0.0f)
            t = 0.875     pos = (9.16015625.0f, 10.546875.0f, 0.0f)
            t = 1         pos = (10.0f, 10.0f)

            delta = (0.83984375.0f, 0.546875.0f, 0.0f)     length = 1.00220
            delta = (1.19140625.0f, 0.390625.0f, 0.0f)     length = 1.25381
            delta = (1.42578125.0f, 0.234375.0f, 0.0f)     length = 1.44492
            delta = (1.54296875.0f, 0.078125.0f, 0.0f)     length = 1.54495
            delta = (1.54296875.0f, -0.078125.0f, 0.0f)    length = 1.54495
            delta = (1.42578125.0f, -0.234375.0f, 0.0f)    length = 1.44492
            delta = (1.19140625.0f, -0.390625.0f, 0.0f)    length = 1.25381
            delta = (0.83984375.0f, -0.546875.0f, 0.0f)    length = 1.00220

            total = 10.49176
        */

        void CatmullRom_AddressByDistance()
        {
            {
                CatmullRomSpline catmullRomSpline;

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    SplineAddress splineAddress = catmullRomSpline.GetAddressByDistance(20.0f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 1);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 1.0f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = catmullRomSpline.GetAddressByDistance(0.0f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 1);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 0.0f, AZ::Constants::Tolerance);
                }

                {
                    // note: spline length is approx 10.49176
                    SplineAddress splineAddress = catmullRomSpline.GetAddressByDistance(5.24588f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 1);
                    EXPECT_NEAR(0.5f, splineAddress.m_segmentFraction, 0.0001f);
                }

                {
                    SplineAddress splineAddress = catmullRomSpline.GetAddressByDistance(-10.0f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 1);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 0.0f, AZ::Constants::Tolerance);
                }
            }

            {
                CatmullRomSpline catmullRomSpline;
                catmullRomSpline.SetClosed(true);

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    SplineAddress splineAddress = catmullRomSpline.GetAddressByDistance(50.0f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 3);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 1.0f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = catmullRomSpline.GetAddressByDistance(0.0f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 0);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 0.0f, AZ::Constants::Tolerance);
                }
            }
        }

        void CatmullRom_AddressByFraction()
        {
            {
                CatmullRomSpline catmullRomSpline;

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    SplineAddress splineAddress = catmullRomSpline.GetAddressByFraction(0.0f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 1);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 0.0f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = catmullRomSpline.GetAddressByFraction(1.0f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 1);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 1.0f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = catmullRomSpline.GetAddressByFraction(0.5f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 1);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 0.5f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = catmullRomSpline.GetAddressByFraction(0.75f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 1);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 0.75f, AZ::Constants::Tolerance);
                }
            }

            {
                CatmullRomSpline catmullRomSpline;
                catmullRomSpline.SetClosed(true);

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    SplineAddress splineAddress = catmullRomSpline.GetAddressByFraction(0.0f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 0);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 0.0f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = catmullRomSpline.GetAddressByFraction(1.0f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 3);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 1.0f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = catmullRomSpline.GetAddressByFraction(0.5f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 2);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 0.0f, AZ::Constants::Tolerance);
                }
            }
        }

        void CatmullRom_GetPosition()
        {
            {
                CatmullRomSpline catmullRomSpline;

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    Vector3 position = catmullRomSpline.GetPosition(SplineAddress(1, 0.125f));
                    EXPECT_THAT(position, IsClose(Vector3(0.83984375f, 10.546875f, 0.0f)));
                }

                {
                    Vector3 position = catmullRomSpline.GetPosition(SplineAddress(1, 0.625f));
                    EXPECT_THAT(position, IsClose(Vector3(6.54296875f, 11.171875f, 0.0f)));
                }

                {
                    Vector3 position = catmullRomSpline.GetPosition(SplineAddress(3, 0.5f));
                    EXPECT_THAT(position, IsClose(Vector3(10.0f, 10.0f, 0.0f)));
                }

                {
                    Vector3 position = catmullRomSpline.GetPosition(SplineAddress(0, 0.0f));
                    EXPECT_THAT(position, IsClose(Vector3(0.0f, 10.0f, 0.0f)));
                }

                {
                    Vector3 position = catmullRomSpline.GetPosition(SplineAddress(1, 0.0f));
                    EXPECT_THAT(position, IsClose(Vector3(0.0f, 10.0f, 0.0f)));
                }

                {
                    Vector3 position = catmullRomSpline.GetPosition(SplineAddress(2, 0.0f));
                    EXPECT_THAT(position, IsClose(Vector3(10.0f, 10.0f, 0.0f)));
                }

                {
                    // out of bounds access (return position of last vertex that is not a control point - clamped)
                    Vector3 position = catmullRomSpline.GetPosition(SplineAddress(5, 0.0f));
                    EXPECT_THAT(position, IsClose(Vector3(10.0f, 10.0f, 0.0f)));
                }
            }

            {
                CatmullRomSpline catmullRomSpline;
                catmullRomSpline.SetClosed(true);

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    Vector3 position = catmullRomSpline.GetPosition(SplineAddress(1, 0.125f));
                    EXPECT_THAT(position, IsClose(Vector3(0.83984375f, 10.546875f, 0.0f)));
                }

                {
                    Vector3 position = catmullRomSpline.GetPosition(SplineAddress(1, 0.625f));
                    EXPECT_THAT(position, IsClose(Vector3(6.54296875f, 11.171875f, 0.0f)));
                }

                {
                    Vector3 position = catmullRomSpline.GetPosition(SplineAddress(0, 0.0f));
                    EXPECT_THAT(position, IsClose(Vector3(0.0f, 0.0f, 0.0f)));
                }

                {
                    Vector3 position = catmullRomSpline.GetPosition(SplineAddress(3, 1.0f));
                    EXPECT_THAT(position, IsClose(Vector3(0.0f, 0.0f, 0.0f)));
                }

                {
                    Vector3 position = catmullRomSpline.GetPosition(SplineAddress(0, 0.5f));
                    EXPECT_THAT(position, IsClose(Vector3(-1.25f, 5.0f, 0.0f)));
                }

                {
                    // out of bounds access (return position of first/last vertex - clamped)
                    Vector3 position = catmullRomSpline.GetPosition(SplineAddress(5, 0.0f));
                    EXPECT_THAT(position, IsClose(Vector3(0.0f, 0.0f, 0.0f)));
                }
            }
        }

        void CatmullRom_GetNormal()
        {
            {
                CatmullRomSpline catmullRomSpline;

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    Vector3 normal = catmullRomSpline.GetNormal(SplineAddress());
                    EXPECT_THAT(normal, IsClose(Vector3(-5.0f, 5.0f, 0.0f).GetNormalized()));
                }

                {
                    Vector3 normal = catmullRomSpline.GetNormal(SplineAddress(1, 0.0f));
                    EXPECT_THAT(normal, IsClose(Vector3(-5.0f, 5.0f, 0.0f).GetNormalized()));
                }

                {
                    Vector3 normal = catmullRomSpline.GetNormal(SplineAddress(1, 1.0f));
                    EXPECT_THAT(normal, IsClose(Vector3(5.0f, 5.0f, 0.0f).GetNormalized()));
                }

                {
                    Vector3 normal = catmullRomSpline.GetNormal(SplineAddress(1, 0.5f));
                    EXPECT_THAT(normal, IsClose(Vector3(0.0f, 12.5f, 0.0f).GetNormalized()));
                }

                {
                    Vector3 normal = catmullRomSpline.GetNormal(SplineAddress(2, 0.0f));
                    EXPECT_THAT(normal, IsClose(Vector3(5.0f, 5.0f, 0.0f).GetNormalized()));
                }

                {
                    Vector3 normal = catmullRomSpline.GetNormal(SplineAddress(3, 0.5f));
                    EXPECT_THAT(normal, IsClose(Vector3(5.0f, 5.0f, 0.0f).GetNormalized()));
                }

                {
                    // out of bounds access (return normal of last vertex that is not a control point - clamped)
                    Vector3 normal = catmullRomSpline.GetNormal(SplineAddress(5, 0.0f));
                    EXPECT_THAT(normal, IsClose(Vector3(5.0f, 5.0f, 0.0f).GetNormalized()));
                }
            }

            {
                CatmullRomSpline catmullRomSpline;
                catmullRomSpline.SetClosed(true);

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    Vector3 normal = catmullRomSpline.GetNormal(SplineAddress());
                    EXPECT_THAT(normal, IsClose(Vector3(-5.0f, -5.0f, 0.0f).GetNormalized()));
                }

                {
                    Vector3 normal = catmullRomSpline.GetNormal(SplineAddress(1, 0.5f));
                    EXPECT_THAT(normal, IsClose(Vector3(0.0f, 1.0f, 0.0f)));
                }

                {
                    Vector3 normal = catmullRomSpline.GetNormal(SplineAddress(3, 0.5f));
                    EXPECT_THAT(normal, IsClose(Vector3(0.0f, -1.0f, 0.0f)));
                }

                {
                    // out of bounds access (return normal of last vertex that is not a control point - clamped)
                    Vector3 normal = catmullRomSpline.GetNormal(SplineAddress(5, 0.0f));
                    EXPECT_THAT(normal, IsClose(Vector3(-5.0f, -5.0f, 0.0f).GetNormalized()));
                }
            }
        }

        void CatmullRom_GetTangent()
        {
            {
                CatmullRomSpline catmullRomSpline;

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    Vector3 tangent = catmullRomSpline.GetTangent(SplineAddress(1, 0.0f));
                    EXPECT_THAT(tangent, IsClose(Vector3(5.0f, 5.0f, 0.0f).GetNormalized()));
                }

                {
                    Vector3 tangent = catmullRomSpline.GetTangent(SplineAddress(1, 1.0f));
                    EXPECT_THAT(tangent, IsClose(Vector3(5.0f, -5.0f, 0.0f).GetNormalized()));
                }

                {
                    Vector3 tangent = catmullRomSpline.GetTangent(SplineAddress(1, 0.5f));
                    EXPECT_THAT(tangent, IsClose(Vector3(12.5f, 0.0f, 0.0f).GetNormalized()));
                }

                {
                    Vector3 tangent = catmullRomSpline.GetTangent(SplineAddress(2, 0.0f));
                    EXPECT_THAT(tangent, IsClose(Vector3(5.0f, -5.0f, 0.0f).GetNormalized()));
                }

                {
                    Vector3 tangent = catmullRomSpline.GetTangent(SplineAddress(3, 0.5f));
                    EXPECT_THAT(tangent, IsClose(Vector3(5.0f, -5.0f, 0.0f).GetNormalized()));
                }
            }

            {
                CatmullRomSpline catmullRomSpline;
                catmullRomSpline.SetClosed(true);

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    Vector3 tangent = catmullRomSpline.GetTangent(SplineAddress(3, 0.5f));
                    EXPECT_THAT(tangent, IsClose(Vector3(-1.0f, 0.0f, 0.0f)));
                }

                {
                    Vector3 tangent = catmullRomSpline.GetTangent(SplineAddress(1, 0.5f));
                    EXPECT_THAT(tangent, IsClose(Vector3(1.0f, 0.0f, 0.0f)));
                }

                {
                    Vector3 tangent = catmullRomSpline.GetTangent(SplineAddress(3, 1.0f));
                    EXPECT_THAT(tangent, IsClose(Vector3(-5.0f, 5.0f, 0.0f).GetNormalized()));
                }
            }
        }

        void CatmullRom_Length()
        {
            {
                CatmullRomSpline catmullRomSpline;

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    float length = catmullRomSpline.GetSplineLength();
                    EXPECT_NEAR(10.49176f, length, 0.0001f);
                }

                {
                    float length = catmullRomSpline.GetSegmentLength(0);
                    EXPECT_NEAR(length, 0.0f, AZ::Constants::Tolerance);
                }

                {
                    float length = catmullRomSpline.GetSegmentLength(1);
                    EXPECT_NEAR(10.49176f, length, 0.0001f);
                }

                {
                    float length = catmullRomSpline.GetSegmentLength((size_t)-1);
                    EXPECT_NEAR(length, 0.0f, AZ::Constants::Tolerance);
                }

                {
                    float length = catmullRomSpline.GetSegmentLength(10);
                    EXPECT_NEAR(length, 0.0f, AZ::Constants::Tolerance);
                }
            }

            {
                CatmullRomSpline catmullRomSpline;
                catmullRomSpline.SetClosed(true);

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    float length = catmullRomSpline.GetSplineLength();
                    EXPECT_NEAR(41.96704, length, 0.0001f);
                }

                {
                    float length = catmullRomSpline.GetSegmentLength(3);
                    EXPECT_NEAR(10.49176f, length, 0.0001f);
                }
            }
        }

        void CatmullRom_Aabb()
        {
            {
                CatmullRomSpline catmullRomSpline;

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                AZ::Aabb aabb;
                catmullRomSpline.GetAabb(aabb);

                EXPECT_THAT(aabb.GetMin(), IsClose(Vector3(0.0f, 10.0f, 0.0f)));
                EXPECT_THAT(aabb.GetMax(), IsClose(Vector3(10.0f, 11.25f, 0.0f)));
            }

            {
                CatmullRomSpline catmullRomSpline;
                catmullRomSpline.SetClosed(true);

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                AZ::Aabb aabb;
                catmullRomSpline.GetAabb(aabb);

                EXPECT_THAT(aabb.GetMin(), IsClose(Vector3(-1.25f, -1.25f, 0.0f)));
                EXPECT_THAT(aabb.GetMax(), IsClose(Vector3(11.25f, 11.25f, 0.0f)));
            }
        }

        void CatmullRom_GetLength()
        {
            {
                CatmullRomSpline spline;

                spline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                spline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                spline.m_vertexContainer.AddVertex(Vector3(20.0f, 20.0f, 10.0f));
                spline.m_vertexContainer.AddVertex(Vector3(10.0f, 15.0f, -10.0f));

                float length = spline.GetSplineLength();

                for (float t = 0.0f; t <= length; t += 0.5f)
                {
                    auto result = spline.GetLength(spline.GetAddressByDistance(t));
                    EXPECT_NEAR(result, t, 1e-4);
                }

                auto distance = spline.GetLength(AZ::SplineAddress(3, 0.5f));
                EXPECT_NEAR(distance, spline.GetSplineLength(), 1e-4);

                distance = spline.GetLength(AZ::SplineAddress(4, 1.0f));
                EXPECT_NEAR(distance, spline.GetSplineLength(), 1e-4);

                distance = spline.GetLength(AZ::SplineAddress(5, 1.0f));
                EXPECT_NEAR(distance, spline.GetSplineLength(), 1e-4);
            }

            {
                CatmullRomSpline spline;

                auto result = spline.GetLength(AZ::SplineAddress(0, 0.0f));
                EXPECT_NEAR(result, 0.0f, 1e-4);

                spline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                result = spline.GetLength(AZ::SplineAddress(0, 0.0f));
                EXPECT_NEAR(result, 0.0f, 1e-4);
            }

            {
                CatmullRomSpline spline;
                spline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                spline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));

                auto result = spline.GetLength(AZ::SplineAddress(0, 0.0f));
                EXPECT_NEAR(result, 0.0f, 1e-4);
            }
        }

        void Bezier_NearestAddressFromPosition()
        {
            {
                BezierSpline bezierSpline;

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    PositionSplineQueryResult posSplineQueryResult = bezierSpline.GetNearestAddressPosition(Vector3(-1.0f, -1.0f, 0.0f));
                    EXPECT_EQ(posSplineQueryResult.m_splineAddress.m_segmentIndex, 0);
                    EXPECT_NEAR(posSplineQueryResult.m_splineAddress.m_segmentFraction, 0.0f, AZ::Constants::Tolerance);
                }

                {
                    PositionSplineQueryResult posSplineQueryResult = bezierSpline.GetNearestAddressPosition(Vector3(5.0f, 12.0f, 0.0f));
                    EXPECT_EQ(posSplineQueryResult.m_splineAddress.m_segmentIndex, 1);
                    EXPECT_NEAR(posSplineQueryResult.m_splineAddress.m_segmentFraction, 0.5f, AZ::Constants::Tolerance);
                }

                {
                    PositionSplineQueryResult posSplineQueryResult = bezierSpline.GetNearestAddressPosition(Vector3(12.0f, -1.0f, 0.0f));
                    EXPECT_EQ(posSplineQueryResult.m_splineAddress.m_segmentIndex, 2);
                    EXPECT_NEAR(posSplineQueryResult.m_splineAddress.m_segmentFraction, 1.0f, AZ::Constants::Tolerance);
                }
            }

            {
                BezierSpline bezierSpline;
                bezierSpline.SetClosed(true);

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    PositionSplineQueryResult posSplineQueryResult = bezierSpline.GetNearestAddressPosition(Vector3(5.0f, -12.0f, 0.0f));
                    EXPECT_EQ(posSplineQueryResult.m_splineAddress.m_segmentIndex, 3);
                    EXPECT_NEAR(posSplineQueryResult.m_splineAddress.m_segmentFraction, 0.5f, AZ::Constants::Tolerance);
                }

                {
                    PositionSplineQueryResult posSplineQueryResult = bezierSpline.GetNearestAddressPosition(Vector3(12.0f, 12.0f, 0.0f));
                    EXPECT_EQ(posSplineQueryResult.m_splineAddress.m_segmentIndex, 1);
                    EXPECT_NEAR(posSplineQueryResult.m_splineAddress.m_segmentFraction, 1.0f, AZ::Constants::Tolerance);
                }
            }
        }

        void Bezier_NearestAddressFromDirection()
        {
            {
                BezierSpline bezierSpline;

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    RaySplineQueryResult raySplineQueryResult = bezierSpline.GetNearestAddressRay(Vector3(-1.0f, -1.0f, 0.0f), Vector3(-1.0f, 1.0f, 0.0f));
                    EXPECT_EQ(raySplineQueryResult.m_splineAddress.m_segmentIndex, 0);
                    EXPECT_NEAR(raySplineQueryResult.m_splineAddress.m_segmentFraction, 0.0f, AZ::Constants::Tolerance);
                }

                {
                    RaySplineQueryResult raySplineQueryResult = bezierSpline.GetNearestAddressRay(Vector3(5.0f, 12.0f, 0.0f), Vector3(0.0f, -1.0f, 0.0f));
                    EXPECT_EQ(raySplineQueryResult.m_splineAddress.m_segmentIndex, 1);
                    EXPECT_NEAR(raySplineQueryResult.m_splineAddress.m_segmentFraction, 0.5f, AZ::Constants::Tolerance);
                }

                {
                    RaySplineQueryResult raySplineQueryResult = bezierSpline.GetNearestAddressRay(Vector3(12.0f, -1.0f, 0.0f), Vector3(1.0f, 1.0f, 0.0f));
                    EXPECT_EQ(raySplineQueryResult.m_splineAddress.m_segmentIndex, 2);
                    EXPECT_NEAR(raySplineQueryResult.m_splineAddress.m_segmentFraction, 1.0f, AZ::Constants::Tolerance);
                }
            }

            {
                BezierSpline bezierSpline;
                bezierSpline.SetClosed(true);

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    RaySplineQueryResult raySplineQueryResult = bezierSpline.GetNearestAddressRay(Vector3(-10.0f, -10.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f));
                    EXPECT_EQ(raySplineQueryResult.m_splineAddress.m_segmentIndex, 3);
                    EXPECT_NEAR(raySplineQueryResult.m_splineAddress.m_segmentFraction, 0.5f, AZ::Constants::Tolerance);
                }
            }
        }

        void Bezier_AddressByDistance()
        {
            {
                BezierSpline bezierSpline;

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    SplineAddress splineAddress = bezierSpline.GetAddressByDistance(0.0f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 0);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 0.0f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = bezierSpline.GetAddressByDistance(-5.0f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 0);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 0.0f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = bezierSpline.GetAddressByDistance(100.0f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 2);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 1.0f, AZ::Constants::Tolerance);
                }
            }

            {
                BezierSpline bezierSpline;
                bezierSpline.SetClosed(true);

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    SplineAddress splineAddress = bezierSpline.GetAddressByDistance(100.0f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 3);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 1.0f, AZ::Constants::Tolerance);
                }
            }
        }

        void Bezier_AddressByFraction()
        {
            {
                BezierSpline bezierSpline;

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    SplineAddress splineAddress = bezierSpline.GetAddressByFraction(0.0f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 0);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 0.0f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = bezierSpline.GetAddressByFraction(-0.5f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 0);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 0.0f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = bezierSpline.GetAddressByFraction(2.0f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 2);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 1.0f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = bezierSpline.GetAddressByFraction(1.0f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 2);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 1.0f, AZ::Constants::Tolerance);
                }

                {
                    SplineAddress splineAddress = bezierSpline.GetAddressByFraction(0.5f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 1);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 0.5f, Constants::Tolerance);
                }
            }

            {
                BezierSpline bezierSpline;
                bezierSpline.SetClosed(true);

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    SplineAddress splineAddress = bezierSpline.GetAddressByFraction(1.0f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 3);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 1.0f, Constants::Tolerance);
                }

                {
                    // testing a fraction far away from a boundary between segments as points right on
                    // a boundary could fall into either segment depending on floating point differences
                    SplineAddress splineAddress = bezierSpline.GetAddressByFraction(0.6f);
                    EXPECT_EQ(splineAddress.m_segmentIndex, 2);
                    EXPECT_NEAR(splineAddress.m_segmentFraction, 0.4f, Constants::Tolerance);
                }
            }
        }

        void Bezier_GetPosition()
        {
            {
                BezierSpline bezierSpline;

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    Vector3 position = bezierSpline.GetPosition(bezierSpline.GetAddressByFraction(1.0f));
                    EXPECT_THAT(position, IsClose(Vector3(10.0f, 0.0f, 0.0f)));
                }

                {
                    Vector3 position = bezierSpline.GetPosition(SplineAddress());
                    EXPECT_THAT(position, IsClose(Vector3(0.0f, 0.0f, 0.0f)));
                }

                {
                    Vector3 position = bezierSpline.GetPosition(SplineAddress(bezierSpline.GetSegmentCount() - 1, 1.0f));
                    EXPECT_THAT(position, IsClose(Vector3(10.0f, 0.0f, 0.0f)));
                }

                {
                    // out of bounds access (return position of last vertex - clamped)
                    Vector3 position = bezierSpline.GetPosition(SplineAddress(5, 0.5f));
                    EXPECT_THAT(position, IsClose(Vector3(10.0f, 0.0f, 0.0f)));
                }
            }

            {
                BezierSpline bezierSpline;
                bezierSpline.SetClosed(true);

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    Vector3 position = bezierSpline.GetPosition(SplineAddress());
                    EXPECT_THAT(position, IsClose(Vector3(0.0f, 0.0f, 0.0f)));
                }

                {
                    Vector3 position = bezierSpline.GetPosition(SplineAddress(bezierSpline.GetSegmentCount() - 1, 1.0f));
                    EXPECT_THAT(position, IsClose(Vector3(0.0f, 0.0f, 0.0f)));
                }

                {
                    // out of bounds access (return position of last/first vertex - clamped)
                    Vector3 position = bezierSpline.GetPosition(SplineAddress(5, 0.5f));
                    EXPECT_THAT(position, IsClose(Vector3(0.0f, 0.0f, 0.0f)));
                }
            }
        }

        void Bezier_GetNormal()
        {
            {
                BezierSpline bezierSpline;

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    Vector3 normal = bezierSpline.GetNormal(SplineAddress(1, 0.5f));
                    EXPECT_THAT(normal, IsClose(Vector3(0.0f, 1.0f, 0.0f)));
                }

                {
                    Vector3 normal = bezierSpline.GetNormal(SplineAddress(0, 0.0f));
                    EXPECT_THAT(normal, IsCloseTolerance(Vector3(-0.955f, -0.294f, 0.0f), 1e-3f));
                }

                {
                    Vector3 normal = bezierSpline.GetNormal(SplineAddress(3, 0.0f));
                    EXPECT_THAT(normal, IsCloseTolerance(Vector3(0.955f, -0.294f, 0.0f), 1e-3f));
                }

                {
                    // out of bounds access (return normal of last vertex - clamped)
                    Vector3 normal = bezierSpline.GetNormal(SplineAddress(5, 0.5f));
                    EXPECT_THAT(normal, IsCloseTolerance(Vector3(0.955f, -0.294f, 0.0f), 1e-3f));
                }
            }

            {
                BezierSpline bezierSpline;
                bezierSpline.SetClosed(true);

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    Vector3 normal = bezierSpline.GetNormal(SplineAddress(3, 0.5f));
                    EXPECT_THAT(normal, IsClose(Vector3(0.0f, -1.0f, 0.0f)));
                }

                {
                    Vector3 normal = bezierSpline.GetNormal(SplineAddress(0, 0.0f));
                    EXPECT_THAT(normal, IsClose(Vector3(-0.7071f, -0.7071f, 0.0f)));
                }

                {
                    Vector3 normal = bezierSpline.GetNormal(SplineAddress(3, 0.0f));
                    EXPECT_THAT(normal, IsClose(Vector3(0.7071f, -0.7071f, 0.0f)));
                }

                {
                    Vector3 normal = bezierSpline.GetNormal(SplineAddress(3, 1.0f));
                    EXPECT_THAT(normal, IsClose(Vector3(-0.7071f, -0.7071f, 0.0f)));
                }

                {
                    // out of bounds access (return normal of last vertex - clamped)
                    Vector3 normal = bezierSpline.GetNormal(SplineAddress(5, 0.5f));
                    EXPECT_THAT(normal, IsClose(Vector3(-0.7071f, -0.7071f, 0.0f)));
                }
            }
        }

        void Bezier_GetTangent()
        {
            {
                BezierSpline bezierSpline;

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    Vector3 tangent = bezierSpline.GetTangent(SplineAddress(1, 0.5f));
                    EXPECT_THAT(tangent, IsClose(Vector3(1.0f, 0.0f, 0.0f)));
                }
            }

            {
                BezierSpline bezierSpline;
                bezierSpline.SetClosed(true);

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    Vector3 tangent = bezierSpline.GetTangent(SplineAddress(3, 0.5f));
                    EXPECT_THAT(tangent, IsClose(Vector3(-1.0f, 0.0f, 0.0f)));
                }

                {
                    Vector3 tangent = bezierSpline.GetTangent(SplineAddress(2, 0.5f));
                    EXPECT_THAT(tangent, IsClose(Vector3(0.0f, -1.0f, 0.0f)));
                }
            }
        }

        void Bezier_Length()
        {
            {
                BezierSpline bezierSpline;

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    float splineLength = bezierSpline.GetSplineLength();
                    EXPECT_NEAR(splineLength, 31.8014f, 0.0001f);
                }

                // lengths should be exact as shape is symmetrical
                {
                    float segment0Length = bezierSpline.GetSegmentLength(0);
                    float segment2Length = bezierSpline.GetSegmentLength(2);
                    EXPECT_NEAR(segment0Length, segment2Length, Constants::Tolerance);
                }
            }

            {
                BezierSpline bezierSpline;
                bezierSpline.SetClosed(true);

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    float splineLength = bezierSpline.GetSplineLength();
                    EXPECT_NEAR(splineLength, 43.40867f, Constants::Tolerance);
                }

                {
                    float segment0Length = bezierSpline.GetSegmentLength(0);
                    float segment2Length = bezierSpline.GetSegmentLength(3);
                    EXPECT_NEAR(segment0Length, segment2Length, Constants::Tolerance);
                }
            }

            {
                BezierSpline bezierSpline;
                bezierSpline.SetClosed(true);

                // single line (y axis)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));

                {
                    float splineLength = bezierSpline.GetSplineLength();
                    EXPECT_NEAR(splineLength, 20.0f, AZ::Constants::Tolerance);
                }

                {
                    float segmentLength = bezierSpline.GetSegmentLength(1);
                    EXPECT_NEAR(segmentLength, 10.0f, AZ::Constants::Tolerance);
                }
            }
        }

        void Bezier_GetLength()
        {
            {
                BezierSpline bezierSpline;

                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 20.0f, 0.0f));

                auto distance = bezierSpline.GetLength(AZ::SplineAddress(1, 0.5f));
                EXPECT_NEAR(distance, 15.0f, 1e-4);

                distance = bezierSpline.GetLength(AZ::SplineAddress(0, 0.0f));
                EXPECT_NEAR(distance, 0.0f, 1e-4);

                distance = bezierSpline.GetLength(AZ::SplineAddress(1, 1.0f));
                EXPECT_NEAR(distance, 20.0f, 1e-4);

                distance = bezierSpline.GetLength(AZ::SplineAddress(5, 1.0f));
                EXPECT_NEAR(distance, bezierSpline.GetSplineLength(), 1e-4);
            }

            {
                BezierSpline bezierSpline;

                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(20.0f, 20.0f, 10.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 15.0f, -10.0f));

                float length = bezierSpline.GetSplineLength();

                for (float t = 0.0f; t <= length; t += 0.5f)
                {
                    auto result = bezierSpline.GetLength(bezierSpline.GetAddressByDistance(t));
                    EXPECT_NEAR(result, t, 1e-4);
                }
            }

            {
                BezierSpline bezierSpline;

                auto result = bezierSpline.GetLength(AZ::SplineAddress(0, 0.0f));
                EXPECT_NEAR(result, 0.0f, 1e-4);

                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                result = bezierSpline.GetLength(AZ::SplineAddress(0, 0.0f));
                EXPECT_NEAR(result, 0.0f, 1e-4);
            }
        }

        void Bezier_Aabb()
        {
            {
                BezierSpline bezierSpline;

                // slight n curve spline (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                AZ::Aabb aabb;
                bezierSpline.GetAabb(aabb);

                EXPECT_THAT(aabb.GetMin(), IsClose(Vector3(-1.2948f, 0.0f, 0.0f)));
                EXPECT_THAT(aabb.GetMax(), IsClose(Vector3(11.2948f, 11.7677f, 0.0f)));
            }

            {
                BezierSpline bezierSpline;
                bezierSpline.SetClosed(true);

                // slight n curve spline (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                AZ::Aabb aabb;
                bezierSpline.GetAabb(aabb);

                EXPECT_THAT(aabb.GetMin(), IsClose(Vector3(-1.7677f, -1.7677f, 0.0f)));
                EXPECT_THAT(aabb.GetMax(), IsClose(Vector3(11.7677f, 11.7677f, 0.0f)));
            }
        }

        void Spline_Invalid()
        {
            {
                LinearSpline linearSpline;

                PositionSplineQueryResult posSplineQuery = linearSpline.GetNearestAddressPosition(Vector3::CreateZero());
                EXPECT_EQ(posSplineQuery.m_splineAddress, SplineAddress());
                EXPECT_NEAR(posSplineQuery.m_distanceSq, std::numeric_limits<float>::max(), AZ::Constants::Tolerance);
                RaySplineQueryResult raySplineQuery = linearSpline.GetNearestAddressRay(Vector3::CreateZero(), Vector3::CreateZero());
                EXPECT_EQ(raySplineQuery.m_splineAddress, SplineAddress());
                EXPECT_NEAR(raySplineQuery.m_distanceSq, std::numeric_limits<float>::max(), AZ::Constants::Tolerance);
                EXPECT_NEAR(raySplineQuery.m_rayDistance, std::numeric_limits<float>::max(), AZ::Constants::Tolerance);
                SplineAddress linearAddressDistance = linearSpline.GetAddressByDistance(2.0f);
                EXPECT_EQ(linearAddressDistance, SplineAddress());
                SplineAddress linearAddressFraction = linearSpline.GetAddressByFraction(0.0f);
                EXPECT_EQ(linearAddressFraction, SplineAddress());
                Vector3 linearPosition = linearSpline.GetPosition(SplineAddress(0, 0.5f));
                EXPECT_THAT(linearPosition, IsClose(Vector3::CreateZero()));
                Vector3 linearNormal = linearSpline.GetNormal(SplineAddress(1, 0.5f));
                EXPECT_THAT(linearNormal, IsClose(Vector3::CreateAxisX()));
                Vector3 linearTangent = linearSpline.GetTangent(SplineAddress(2, 0.5f));
                EXPECT_THAT(linearTangent, IsClose(Vector3::CreateAxisX()));
                float linearSegmentLength = linearSpline.GetSegmentLength(2);
                EXPECT_NEAR(linearSegmentLength, 0.0f, AZ::Constants::Tolerance);
                float linearSplineLength = linearSpline.GetSplineLength();
                EXPECT_NEAR(linearSplineLength, 0.0f, AZ::Constants::Tolerance);
            }

            {
                CatmullRomSpline catmullRomSpline;

                PositionSplineQueryResult posSplineQuery = catmullRomSpline.GetNearestAddressPosition(Vector3::CreateZero());
                EXPECT_EQ(posSplineQuery.m_splineAddress, SplineAddress());
                EXPECT_NEAR(posSplineQuery.m_distanceSq, std::numeric_limits<float>::max(), AZ::Constants::Tolerance);
                RaySplineQueryResult raySplineQuery = catmullRomSpline.GetNearestAddressRay(Vector3::CreateZero(), Vector3::CreateZero());
                EXPECT_EQ(raySplineQuery.m_splineAddress, SplineAddress());
                EXPECT_NEAR(raySplineQuery.m_distanceSq, std::numeric_limits<float>::max(), AZ::Constants::Tolerance);
                EXPECT_NEAR(raySplineQuery.m_rayDistance, std::numeric_limits<float>::max(), AZ::Constants::Tolerance);
                SplineAddress catmullRomAddressDistance = catmullRomSpline.GetAddressByDistance(2.0f);
                EXPECT_EQ(catmullRomAddressDistance, SplineAddress());
                SplineAddress catmullRomAddressFraction = catmullRomSpline.GetAddressByFraction(0.0f);
                EXPECT_EQ(catmullRomAddressFraction, SplineAddress());
                Vector3 catmullRomPosition = catmullRomSpline.GetPosition(SplineAddress(0, 0.5f));
                EXPECT_THAT(catmullRomPosition, IsClose(Vector3::CreateZero()));
                Vector3 catmullRomNormal = catmullRomSpline.GetNormal(SplineAddress(1, 0.5f));
                EXPECT_THAT(catmullRomNormal, IsClose(Vector3::CreateAxisX()));
                Vector3 catmullRomTangent = catmullRomSpline.GetTangent(SplineAddress(2, 0.5f));
                EXPECT_THAT(catmullRomTangent, IsClose(Vector3::CreateAxisX()));
                float catmullRomSegmentLength = catmullRomSpline.GetSegmentLength(2);
                EXPECT_NEAR(catmullRomSegmentLength, 0.0f, AZ::Constants::Tolerance);
                float catmullRomSplineLength = catmullRomSpline.GetSplineLength();
                EXPECT_NEAR(catmullRomSplineLength, 0.0f, AZ::Constants::Tolerance);
            }

            {
                BezierSpline bezierSpline;

                PositionSplineQueryResult posSplineQuery = bezierSpline.GetNearestAddressPosition(Vector3::CreateZero());
                EXPECT_EQ(posSplineQuery.m_splineAddress, SplineAddress());
                EXPECT_NEAR(posSplineQuery.m_distanceSq, std::numeric_limits<float>::max(), AZ::Constants::Tolerance);
                RaySplineQueryResult raySplineQuery = bezierSpline.GetNearestAddressRay(Vector3::CreateZero(), Vector3::CreateZero());
                EXPECT_EQ(raySplineQuery.m_splineAddress, SplineAddress());
                EXPECT_NEAR(raySplineQuery.m_distanceSq, std::numeric_limits<float>::max(), AZ::Constants::Tolerance);
                EXPECT_NEAR(raySplineQuery.m_rayDistance, std::numeric_limits<float>::max(), AZ::Constants::Tolerance);
                SplineAddress bezierAddressDistance = bezierSpline.GetAddressByDistance(2.0f);
                EXPECT_EQ(bezierAddressDistance, SplineAddress());
                SplineAddress bezierAddressFraction = bezierSpline.GetAddressByFraction(0.0f);
                EXPECT_EQ(bezierAddressFraction, SplineAddress());
                Vector3 bezierPosition = bezierSpline.GetPosition(SplineAddress(0, 0.5f));
                EXPECT_THAT(bezierPosition, IsClose(Vector3::CreateZero()));
                Vector3 bezierNormal = bezierSpline.GetNormal(SplineAddress(1, 0.5f));
                EXPECT_THAT(bezierNormal, IsClose(Vector3::CreateAxisX()));
                Vector3 bezierTangent = bezierSpline.GetTangent(SplineAddress(2, 0.5f));
                EXPECT_THAT(bezierTangent, IsClose(Vector3::CreateAxisX()));
                float bezierSegmentLength = bezierSpline.GetSegmentLength(2);
                EXPECT_NEAR(bezierSegmentLength, 0.0f, AZ::Constants::Tolerance);
                float bezierSplineLength = bezierSpline.GetSplineLength();
                EXPECT_NEAR(bezierSplineLength, 0.0f, AZ::Constants::Tolerance);
            }
        }

        void Spline_Set()
        {
            LinearSpline linearSpline;

            // slight n curve spline (xy plane)
            linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
            linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
            linearSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
            linearSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

            AZStd::vector<Vector3> vertices{
                Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 10.0f, 0.0f), Vector3(10.0f, 10.0f, 0.0f), Vector3(10.0f, 0.0f, 0.0f)
            };

            LinearSpline linearSplineSetLValue;
            linearSplineSetLValue.m_vertexContainer.SetVertices(vertices);

            LinearSpline linearSplineSetRValue;
            linearSplineSetRValue.m_vertexContainer.SetVertices(AZStd::vector<Vector3>{ Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 10.0f, 0.0f), Vector3(10.0f, 10.0f, 0.0f), Vector3(10.0f, 0.0f, 0.0f) });

            LinearSpline linearSplineSetLValueCopy;
            linearSplineSetLValueCopy.m_vertexContainer.SetVertices(linearSplineSetLValue.GetVertices());

            EXPECT_EQ(linearSpline.GetVertexCount(), linearSplineSetLValue.GetVertexCount());
            EXPECT_EQ(linearSpline.GetVertexCount(), linearSplineSetRValue.GetVertexCount());
            EXPECT_EQ(linearSplineSetLValue.GetVertexCount(), linearSplineSetRValue.GetVertexCount());
            EXPECT_EQ(linearSplineSetLValueCopy.GetVertexCount(), vertices.size());

            BezierSpline bezierSpline;

            // slight n curve spline (xy plane)
            bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
            bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
            bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
            bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

            BezierSpline bezierSplineSetLValue;
            bezierSplineSetLValue.m_vertexContainer.SetVertices(vertices);

            BezierSpline bezierSplineSetRValue;
            bezierSplineSetRValue.m_vertexContainer.SetVertices(AZStd::vector<Vector3>{ Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 10.0f, 0.0f), Vector3(10.0f, 10.0f, 0.0f), Vector3(10.0f, 0.0f, 0.0f) });

            BezierSpline bezierSplineSetLValueCopy;
            bezierSplineSetLValueCopy.m_vertexContainer.SetVertices(bezierSplineSetLValue.GetVertices());

            EXPECT_EQ(bezierSpline.GetBezierData().size(), bezierSplineSetLValue.GetBezierData().size());
            EXPECT_EQ(bezierSplineSetLValue.GetBezierData().size(), 4);
            EXPECT_EQ(bezierSplineSetLValue.GetBezierData().size(), bezierSplineSetRValue.GetBezierData().size());
            EXPECT_EQ(bezierSplineSetLValueCopy.GetBezierData().size(), 4);
            EXPECT_EQ(bezierSplineSetLValueCopy.GetVertexCount(), vertices.size());
        }
    };

    TEST_F(MATH_SplineTest, Linear_NearestAddressFromPosition)
    {
        Linear_NearestAddressFromPosition();
    }

    TEST_F(MATH_SplineTest, Linear_NearestAddressFromDirection)
    {
        Linear_NearestAddressFromDirection();
    }

    TEST_F(MATH_SplineTest, Linear_AddressByDistance)
    {
        Linear_AddressByDistance();
    }

    TEST_F(MATH_SplineTest, Linear_AddressByFraction)
    {
        Linear_AddressByFraction();
    }

    TEST_F(MATH_SplineTest, Linear_GetPosition)
    {
        Linear_GetPosition();
    }

    TEST_F(MATH_SplineTest, Linear_GetNormal)
    {
        Linear_GetNormal();
    }

    TEST_F(MATH_SplineTest, Linear_GetTangent)
    {
        Linear_GetTangent();
    }

    TEST_F(MATH_SplineTest, Linear_Length)
    {
        Linear_Length();
    }

    TEST_F(MATH_SplineTest, Linear_Aabb)
    {
        Linear_Aabb();
    }

    TEST_F(MATH_SplineTest, Linear_GetLength)
    {
        Linear_GetLength();
    }

    TEST_F(MATH_SplineTest, CatmullRom_Length)
    {
        CatmullRom_Length();
    }

    TEST_F(MATH_SplineTest, CatmullRom_NearestAddressFromPosition)
    {
        CatmullRom_NearestAddressFromPosition();
    }

    TEST_F(MATH_SplineTest, CatmullRom_NearestAddressFromDirection)
    {
        CatmullRom_NearestAddressFromDirection();
    }

    TEST_F(MATH_SplineTest, CatmullRom_AddressByDistance)
    {
        CatmullRom_AddressByDistance();
    }

    TEST_F(MATH_SplineTest, CatmullRom_AddressByFraction)
    {
        CatmullRom_AddressByFraction();
    }

    TEST_F(MATH_SplineTest, CatmullRom_GetPosition)
    {
        CatmullRom_GetPosition();
    }

    TEST_F(MATH_SplineTest, CatmullRom_GetNormal)
    {
        CatmullRom_GetNormal();
    }

    TEST_F(MATH_SplineTest, CatmullRom_GetTangent)
    {
        CatmullRom_GetTangent();
    }

    TEST_F(MATH_SplineTest, CatmullRom_Aabb)
    {
        CatmullRom_Aabb();
    }

    TEST_F(MATH_SplineTest, CatmullRom_GetLength)
    {
        CatmullRom_GetLength();
    }

    TEST_F(MATH_SplineTest, Bezier_NearestAddressFromPosition)
    {
        Bezier_NearestAddressFromPosition();
    }

    TEST_F(MATH_SplineTest, Bezier_NearestAddressFromDirection)
    {
        Bezier_NearestAddressFromDirection();
    }

    TEST_F(MATH_SplineTest, Bezier_AddressByDistance)
    {
        Bezier_AddressByDistance();
    }

    TEST_F(MATH_SplineTest, Bezier_AddressByFraction)
    {
        Bezier_AddressByFraction();
    }

    TEST_F(MATH_SplineTest, Bezier_GetPosition)
    {
        Bezier_GetPosition();
    }

    TEST_F(MATH_SplineTest, Bezier_GetNormal)
    {
        Bezier_GetNormal();
    }

    TEST_F(MATH_SplineTest, Bezier_GetTangent)
    {
        Bezier_GetTangent();
    }

    TEST_F(MATH_SplineTest, Bezier_Length)
    {
        Bezier_Length();
    }

    TEST_F(MATH_SplineTest, Bezier_Aabb)
    {
        Bezier_Aabb();
    }

    TEST_F(MATH_SplineTest, Bezier_GetLength)
    {
        Bezier_GetLength();
    }

    TEST_F(MATH_SplineTest, Spline_Invalid)
    {
        Spline_Invalid();
    }

    TEST_F(MATH_SplineTest, Spline_Set)
    {
        Spline_Set();
    }
}
