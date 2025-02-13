/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Vector3.h>
#include <SceneAPI/SceneBuilder/SceneSystem.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpTypeConverter.h>
#include <assimp/scene.h>


namespace AZ
{
    namespace SceneAPI
    {
        SceneSystem::SceneSystem() :
            m_unitSizeInMeters(1.0f),
            m_originalUnitSizeInMeters(1.0f),
            m_adjustTransform(nullptr),
            m_adjustTransformInverse(nullptr)
        {
        }

        void SceneSystem::Set(const SDKScene::SceneWrapperBase* scene)
        {
            // Get unit conversion factor to meter.
            if (!azrtti_istypeof<AssImpSDKWrapper::AssImpSceneWrapper>(scene))
            {
                return;
            }

            const AssImpSDKWrapper::AssImpSceneWrapper* assImpScene = azrtti_cast<const AssImpSDKWrapper::AssImpSceneWrapper*>(scene);

            /* Check if metadata has information about "UnitScaleFactor" or "OriginalUnitScaleFactor". 
             * This particular metadata is FBX format only. */
            if (assImpScene->GetAssImpScene()->mMetaData->HasKey("UnitScaleFactor") ||
                assImpScene->GetAssImpScene()->mMetaData->HasKey("OriginalUnitScaleFactor"))
            {
                // If either metadata piece is not available, the default of 1 will be used.
                assImpScene->GetAssImpScene()->mMetaData->Get("UnitScaleFactor", m_unitSizeInMeters);
                assImpScene->GetAssImpScene()->mMetaData->Get("OriginalUnitScaleFactor", m_originalUnitSizeInMeters);

                /* Conversion factor for converting from centimeters to meters.
                 * This applies to an FBX format in which the default unit is a centimeter. */
                m_unitSizeInMeters = m_unitSizeInMeters * .01f;
            }
            else
            {
                // Some file formats (like DAE) embed the scale in the root transformation, so extract that scale from here.
                auto rootTransform = 
                    AssImpSDKWrapper::AssImpTypeConverter::ToTransform(assImpScene->GetAssImpScene()->mRootNode->mTransformation);
                m_unitSizeInMeters = rootTransform.ExtractScale().GetMaxElement();
            }

            AZStd::pair<AssImpSDKWrapper::AssImpSceneWrapper::AxisVector, int32_t> upAxisAndSign = assImpScene->GetUpVectorAndSign();

            if (upAxisAndSign.second <= 0)
            {
                AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Negative scene orientation is not a currently supported orientation.");
                return;
            }

            AZStd::pair<AssImpSDKWrapper::AssImpSceneWrapper::AxisVector, int32_t> frontAxisAndSign = assImpScene->GetFrontVectorAndSign();

            if (upAxisAndSign.first != AssImpSDKWrapper::AssImpSceneWrapper::AxisVector::Z &&
                upAxisAndSign.first != AssImpSDKWrapper::AssImpSceneWrapper::AxisVector::Unknown)
            {
                AZ::Matrix4x4 currentCoordMatrix = AZ::Matrix4x4::CreateIdentity();
                //(UpVector = +Z, FrontVector = +Y, CoordSystem = -X(RightHanded))
                AZ::Matrix4x4 targetCoordMatrix = AZ::Matrix4x4::CreateFromColumns(
                    AZ::Vector4(-1, 0, 0, 0),
                    AZ::Vector4(0, 0, 1, 0),
                    AZ::Vector4(0, 1, 0, 0),
                    AZ::Vector4(0, 0, 0, 1));

                switch (upAxisAndSign.first)
                {
                case AssImpSDKWrapper::AssImpSceneWrapper::AxisVector::X: {
                    if (frontAxisAndSign.second == 1)
                    {
                        currentCoordMatrix = AZ::Matrix4x4::CreateFromColumns(
                            AZ::Vector4(0, -1, 0, 0),
                            AZ::Vector4(1, 0, 0, 0),
                            AZ::Vector4(0, 0, 1, 0),
                            AZ::Vector4(0, 0, 0, 1));
                    }
                    else
                    {
                        currentCoordMatrix = AZ::Matrix4x4::CreateFromColumns(
                            AZ::Vector4(0, 1, 0, 0),
                            AZ::Vector4(1, 0, 0, 0),
                            AZ::Vector4(0, 0, -1, 0),
                            AZ::Vector4(0, 0, 0, 1));
                    }
                }
                break;
                case AssImpSDKWrapper::AssImpSceneWrapper::AxisVector::Y: {
                    if (frontAxisAndSign.second == 1)
                    {
                        currentCoordMatrix = AZ::Matrix4x4::CreateFromColumns(
                            AZ::Vector4(1, 0, 0, 0),
                            AZ::Vector4(0, 1, 0, 0),
                            AZ::Vector4(0, 0, 1, 0),
                            AZ::Vector4(0, 0, 0, 1));
                    }
                    else
                    {
                        currentCoordMatrix = AZ::Matrix4x4::CreateFromColumns(
                            AZ::Vector4(-1, 0, 0, 0),
                            AZ::Vector4(0, 1, 0, 0),
                            AZ::Vector4(0, 0, -1, 0),
                            AZ::Vector4(0, 0, 0, 1));
                    }
                }
                break;
                }
                AZ::Matrix4x4 adjustmatrix = targetCoordMatrix * currentCoordMatrix.GetInverseTransform();
                m_adjustTransform.reset(new DataTypes::MatrixType(AssImpSDKWrapper::AssImpTypeConverter::ToTransform(adjustmatrix)));
                m_adjustTransformInverse.reset(new DataTypes::MatrixType(m_adjustTransform->GetInverseFull()));
            }
        }

        void SceneSystem::SwapVec3ForUpAxis(Vector3& swapVector) const
        {
            if (m_adjustTransform)
            {
                swapVector = *m_adjustTransform * swapVector;
            }
        }

        void SceneSystem::SwapTransformForUpAxis(DataTypes::MatrixType& inOutTransform) const
        {
            if (m_adjustTransform)
            {
                inOutTransform = (*m_adjustTransform * inOutTransform) * *m_adjustTransformInverse;
            }
        }

        void SceneSystem::ConvertUnit(Vector3& scaleVector) const
        {
            scaleVector *= m_unitSizeInMeters;
        }

        void SceneSystem::ConvertUnit(DataTypes::MatrixType& inOutTransform) const
        {
            Vector3 translation = inOutTransform.GetTranslation();
            translation *= m_unitSizeInMeters;
            inOutTransform.SetTranslation(translation);
        }

        void SceneSystem::ConvertBoneUnit(DataTypes::MatrixType& inOutTransform) const
        {


            // Need to scale translation explicitly as MultiplyByScale won't change the translation component
            // and we need to convert to meter unit
            Vector3 translation = inOutTransform.GetTranslation();
            translation *= m_unitSizeInMeters;
            inOutTransform.SetTranslation(translation);
        }
    }
}
