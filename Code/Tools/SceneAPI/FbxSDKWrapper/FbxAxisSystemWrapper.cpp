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

#include <AzCore/Debug/Trace.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/FbxSDKWrapper/FbxAxisSystemWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxTypeConverter.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        FbxAxisSystemWrapper::FbxAxisSystemWrapper(const FbxAxisSystem& fbxAxisSystem)
            : m_fbxAxisSystem(fbxAxisSystem)
        {
        }

        FbxAxisSystemWrapper::UpVector FbxAxisSystemWrapper::GetUpVector(int& sign) const
        {
            switch (m_fbxAxisSystem.GetUpVector(sign))
            {
                case FbxAxisSystem::eXAxis:
                    return X;
                case FbxAxisSystem::eYAxis:
                    return Y;
                case FbxAxisSystem::eZAxis:
                    return Z;
                default:
                    AZ_TraceContext("Unknown value", m_fbxAxisSystem.GetUpVector(sign));
                    AZ_TracePrintf(SceneAPI::Utilities::WarningWindow, "Unrecognized axis up vector type");
                    return Unknown;
            }
        }

        SceneAPI::DataTypes::MatrixType FbxAxisSystemWrapper::CalculateConversionTransform(UpVector targetUpAxis)
        {
            FbxAxisSystem targetSystem;
            switch (targetUpAxis)
            {
                case Y:
                    // Maya YUp coordinate system (UpVector = +Y, FrontVector = +Z, CoordSystem = +X (RightHanded))
                    targetSystem = FbxAxisSystem::MayaYUp;
                    break;
                case Z:
                    // CryTek ZUp coordinate system (UpVector = +Z, FrontVector = +Y, CoordSystem = -X (RightHanded))
                    targetSystem = FbxAxisSystem(FbxAxisSystem::eZAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded);
                    break;
                case X:
                    // Default XUp coordinate system (UpVector = +X, FrontVector = +Z, CoordSystem = -Y (RightHanded))
                    targetSystem = FbxAxisSystem(FbxAxisSystem::eXAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded);
                    break;
                default:
                    AZ_TraceContext("Unknown value", targetUpAxis);
                    AZ_TracePrintf(SceneAPI::Utilities::WarningWindow, "Unrecognized axis conversion target axis type");
                    return SceneAPI::DataTypes::MatrixType::CreateIdentity();
            }
            FbxAMatrix targetMatrix;
            targetSystem.GetMatrix(targetMatrix);
            FbxAMatrix currentMatrix;
            m_fbxAxisSystem.GetMatrix(currentMatrix);
            FbxAMatrix adjustMatrix = targetMatrix * currentMatrix.Inverse();
            return FbxTypeConverter::ToTransform(adjustMatrix);
        }
    } // namespace FbxSDKWrapper
} // namespace AZ
