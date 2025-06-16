/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SceneWrapper.h"

#include <AzCore/Math/Vector3.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <SceneAPI/SceneBuilder/SceneSystem.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>


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
            m_unitSizeInMeters = scene->GetUnitSizeInMeters();
            if (auto forcedRootTransform = scene->UseForcedRootTransform())
            {
                m_adjustTransform.reset(new DataTypes::MatrixType(*forcedRootTransform));
                m_adjustTransformInverse.reset(new DataTypes::MatrixType(m_adjustTransform->GetInverseFull()));
                return;
            }

            AZStd::pair<SDKScene::SceneWrapperBase::AxisVector, int32_t> upAxisAndSign = scene->GetUpVectorAndSign();

            if (upAxisAndSign.second <= 0)
            {
                AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Negative scene orientation is not a currently supported orientation.");
                return;
            }

            AZStd::pair<SDKScene::SceneWrapperBase::AxisVector, int32_t> frontAxisAndSign = scene->GetFrontVectorAndSign();

            if (upAxisAndSign.first != SDKScene::SceneWrapperBase::AxisVector::Z &&
                upAxisAndSign.first != SDKScene::SceneWrapperBase::AxisVector::Unknown)
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
                case SDKScene::SceneWrapperBase::AxisVector::X: {
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
                case SDKScene::SceneWrapperBase::AxisVector::Y: {
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
                m_adjustTransform.reset(new DataTypes::MatrixType(SDKScene::SceneTypeConverter::ToTransform(adjustmatrix)));
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
