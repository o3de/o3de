/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>
#include <SceneAPI/SceneUI/RowWidgets/TransformRowWidget.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            class TransformRowWidgetTest
                : public ::testing::Test
            {
            public:
                ExpandedTransform m_expanded;
                Transform m_transform;

                Vector3 m_translation = Vector3(10.0f, 20.0f, 30.0f);
                Vector3 m_rotation = Vector3(30.0f, 45.0f, 60.0f);
                float m_scale = 3.0f;
            };

            TEST_F(TransformRowWidgetTest, GetTranslation_TranslationInMatrix_TranslationCanBeRetrievedDirectly)
            {
                m_transform = Transform::CreateTranslation(m_translation);
                m_expanded.SetTransform(m_transform);

                const Vector3& returned = m_expanded.GetTranslation();
                EXPECT_NEAR(m_translation.GetX(), returned.GetX(), 0.1f);
                EXPECT_NEAR(m_translation.GetY(), returned.GetY(), 0.1f);
                EXPECT_NEAR(m_translation.GetZ(), returned.GetZ(), 0.1f);
            }

            TEST_F(TransformRowWidgetTest, GetTranslation_TranslationInMatrix_TranslationCanBeRetrievedFromTransform)
            {
                m_transform = Transform::CreateTranslation(m_translation);
                m_expanded.SetTransform(m_transform);

                Transform rebuild;
                m_expanded.GetTransform(rebuild);
                Vector3 returned = rebuild.GetTranslation();
                EXPECT_NEAR(m_translation.GetX(), returned.GetX(), 0.1f);
                EXPECT_NEAR(m_translation.GetY(), returned.GetY(), 0.1f);
                EXPECT_NEAR(m_translation.GetZ(), returned.GetZ(), 0.1f);
            }

            TEST_F(TransformRowWidgetTest, DISABLED_GetRotation_RotationInMatrix_RotationCanBeRetrievedDirectly)
            {
                m_transform = AZ::ConvertEulerDegreesToTransform(m_rotation);
                m_expanded.SetTransform(m_transform);

                const Vector3& returned = m_expanded.GetRotation();
                EXPECT_NEAR(m_rotation.GetX(), returned.GetX(), 1.0f);
                EXPECT_NEAR(m_rotation.GetY(), returned.GetY(), 1.0f);
                EXPECT_NEAR(m_rotation.GetZ(), returned.GetZ(), 1.0f);
            }

            TEST_F(TransformRowWidgetTest, DISABLED_GetRotation_RotationInMatrix_RotationCanBeRetrievedFromTransform)
            {
                m_transform.SetFromEulerDegrees(m_rotation);
                m_expanded.SetTransform(m_transform);

                Transform rebuild;
                m_expanded.GetTransform(rebuild);
                Vector3 returned = rebuild.GetEulerDegrees();
                EXPECT_NEAR(m_rotation.GetX(), returned.GetX(), 1.0f);
                EXPECT_NEAR(m_rotation.GetY(), returned.GetY(), 1.0f);
                EXPECT_NEAR(m_rotation.GetZ(), returned.GetZ(), 1.0f);
            }

            TEST_F(TransformRowWidgetTest, GetScale_ScaleInMatrix_ScaleCanBeRetrievedDirectly)
            {
                m_transform = Transform::CreateUniformScale(m_scale);
                m_expanded.SetTransform(m_transform);

                const float returned = m_expanded.GetScale();
                EXPECT_NEAR(m_scale, returned, 0.1f);
            }

            TEST_F(TransformRowWidgetTest, GetScale_ScaleInMatrix_ScaleCanBeRetrievedFromTransform)
            {
                m_transform = Transform::CreateUniformScale(m_scale);
                m_expanded.SetTransform(m_transform);

                Transform rebuild;
                m_expanded.GetTransform(rebuild);
                float returned = rebuild.GetUniformScale();
                EXPECT_NEAR(m_scale, returned, 0.1f);
            }

            TEST_F(TransformRowWidgetTest, GetTransform_RotateAndTranslateInMatrix_ReconstructedTransformMatchesOriginal)
            {
                Quaternion quaternion = AZ::ConvertEulerDegreesToQuaternion(m_rotation);
                m_transform = Transform::CreateFromQuaternionAndTranslation(quaternion, m_translation);
                m_expanded.SetTransform(m_transform);

                Transform rebuild;
                m_expanded.GetTransform(rebuild);

                EXPECT_TRUE(m_transform.IsClose(rebuild, 0.001f));
            }
            
            TEST_F(TransformRowWidgetTest, GetTransform_RotateTranslateAndScaleInMatrix_ReconstructedTransformMatchesOriginal)
            {
                Quaternion quaternion = AZ::ConvertEulerDegreesToQuaternion(m_rotation);
                m_transform = Transform::CreateFromQuaternionAndTranslation(quaternion, m_translation);
                m_transform.MultiplyByUniformScale(m_scale);
                m_expanded.SetTransform(m_transform);

                Transform rebuild;
                m_expanded.GetTransform(rebuild);

                EXPECT_TRUE(m_transform.IsClose(rebuild, 0.001f));
            }
        } // namespace SceneUI
    } // namespace SceneAPI
} // namespace AZ
