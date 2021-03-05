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

#include <AzCore/Math/Vector3.h>
#include <SceneAPI/FbxSDKWrapper/FbxSceneWrapper.h>
#include <SceneAPI/FbxSceneBuilder/FbxSceneSystem.h>

#ifdef ASSET_IMPORTER_SDK_SUPPORTED_TRAIT
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpTypeConverter.h>
#include <assimp/scene.h>
#endif


namespace AZ
{
    namespace SceneAPI
    {
        FbxSceneSystem::FbxSceneSystem() :
            m_unitSizeInMeters(1.0f),
            m_originalUnitSizeInMeters(1.0f),
            m_adjustTransform(nullptr),
            m_adjustTransformInverse(nullptr)
        {
        }

        void FbxSceneSystem::Set(const SDKScene::SceneWrapperBase* fbxScene)
        {
            // Get unit conversion factor to meter.
            if (azrtti_istypeof<FbxSDKWrapper::FbxSceneWrapper>(fbxScene))
            {
                const FbxSDKWrapper::FbxSceneWrapper* fbxSDKScene = azrtti_cast <const FbxSDKWrapper::FbxSceneWrapper*>(fbxScene);
                m_unitSizeInMeters = fbxSDKScene->GetSystemUnit()->GetConversionFactorTo(FbxSDKWrapper::FbxSystemUnitWrapper::m);
                const FbxGlobalSettings& globalSettings = fbxSDKScene->GetFbxScene()->GetGlobalSettings();
                m_originalUnitSizeInMeters = static_cast<float>(globalSettings.GetOriginalSystemUnit().GetConversionFactorTo(FbxSystemUnit::m));

                int sign = 0;
                FbxSDKWrapper::FbxAxisSystemWrapper::UpVector upVector = fbxSDKScene->GetAxisSystem()->GetUpVector(sign);

                if (upVector != FbxSDKWrapper::FbxAxisSystemWrapper::Z && upVector != FbxSDKWrapper::FbxAxisSystemWrapper::Unknown)
                {
                    m_adjustTransform.reset(new DataTypes::MatrixType(fbxSDKScene->GetAxisSystem()->CalculateConversionTransform(FbxSDKWrapper::FbxAxisSystemWrapper::Z)));
                    m_adjustTransformInverse.reset(new DataTypes::MatrixType(m_adjustTransform->GetInverseFull()));
                }
            }
#ifdef ASSET_IMPORTER_SDK_SUPPORTED_TRAIT
            else if (azrtti_istypeof<AssImpSDKWrapper::AssImpSceneWrapper>(fbxScene))
            {
                const AssImpSDKWrapper::AssImpSceneWrapper* assImpScene = azrtti_cast<const AssImpSDKWrapper::AssImpSceneWrapper*>(fbxScene);

                // If either meta data piece is not available, the default of 1 will be used.
                assImpScene->GetAssImpScene()->mMetaData->Get("UnitScaleFactor", m_unitSizeInMeters);
                assImpScene->GetAssImpScene()->mMetaData->Get("OriginalUnitScaleFactor", m_originalUnitSizeInMeters);

                /* Conversion factor for converting from centimeters to meters */
                m_unitSizeInMeters = m_unitSizeInMeters *.01f;

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
                    case AssImpSDKWrapper::AssImpSceneWrapper::AxisVector::X:
                    {
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
                        case AssImpSDKWrapper::AssImpSceneWrapper::AxisVector::Y:
                        {
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
                    AZ::Matrix4x4 inverse = currentCoordMatrix.GetInverseTransform();
                    AZ::Matrix4x4 adjustmatrix = targetCoordMatrix * currentCoordMatrix.GetInverseTransform();
                    m_adjustTransform.reset(new DataTypes::MatrixType(AssImpSDKWrapper::AssImpTypeConverter::ToTransform(adjustmatrix)));
                    m_adjustTransformInverse.reset(new DataTypes::MatrixType(m_adjustTransform->GetInverseFull()));
                }
            }
#endif
        }

        void FbxSceneSystem::SwapVec3ForUpAxis(Vector3& swapVector) const
        {
            if (m_adjustTransform)
            {
                swapVector = *m_adjustTransform * swapVector;
            }
        }

        void FbxSceneSystem::SwapTransformForUpAxis(DataTypes::MatrixType& inOutTransform) const
        {
            if (m_adjustTransform)
            {
                inOutTransform = (*m_adjustTransform * inOutTransform) * *m_adjustTransformInverse;
            }
        }

        void FbxSceneSystem::ConvertUnit(Vector3& scaleVector) const
        {
            scaleVector *= m_unitSizeInMeters;
        }

        void FbxSceneSystem::ConvertUnit(DataTypes::MatrixType& inOutTransform) const
        {
            Vector3 translation = inOutTransform.GetTranslation();
            translation *= m_unitSizeInMeters;
            inOutTransform.SetTranslation(translation);
        }

        void FbxSceneSystem::ConvertBoneUnit(DataTypes::MatrixType& inOutTransform) const
        {


            // Need to scale translation explicitly as MultiplyByScale won't change the translation component
            // and we need to convert to meter unit
            Vector3 translation = inOutTransform.GetTranslation();
            translation *= m_unitSizeInMeters;
            inOutTransform.SetTranslation(translation);
        }
    }
}
