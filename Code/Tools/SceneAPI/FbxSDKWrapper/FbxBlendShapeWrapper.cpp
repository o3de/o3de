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
#include <AzCore/std/smart_ptr/make_shared.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/FbxSDKWrapper/FbxMeshWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxBlendShapeWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxBlendShapeChannelWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxNodeWrapper.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        FbxBlendShapeWrapper::FbxBlendShapeWrapper(FbxBlendShape* fbxBlendShape)
            : m_fbxBlendShape(fbxBlendShape)
        {
            AZ_Assert(fbxBlendShape, "Invalid FbxBlendShape input to initialize FbxBlendShapeWrapper");
        }

        FbxBlendShapeWrapper::~FbxBlendShapeWrapper()
        {
            m_fbxBlendShape = nullptr;
        }

        const char* FbxBlendShapeWrapper::GetName() const
        {
            return m_fbxBlendShape->GetName();
        }

        //Technically Fbx returns a FbxGeometry off this interface, but we only support meshes in the engine runtime.
        AZStd::shared_ptr<const FbxMeshWrapper> FbxBlendShapeWrapper::GetGeometry() const
        {
            FbxGeometry* fbxGeom = m_fbxBlendShape->GetGeometry();

            if (fbxGeom && fbxGeom->GetAttributeType() == FbxNodeAttribute::EType::eMesh )
            {
                return AZStd::make_shared<FbxMeshWrapper>(static_cast<FbxMesh*>(fbxGeom));
            }
            else
            {
                return nullptr;
            }
        }

        int FbxBlendShapeWrapper::GetBlendShapeChannelCount() const
        {
            return m_fbxBlendShape->GetBlendShapeChannelCount();
        }

        AZStd::shared_ptr<const FbxBlendShapeChannelWrapper> FbxBlendShapeWrapper::GetBlendShapeChannel(int index) const
        {
            FbxBlendShapeChannel* fbxBlendShapeChannel = m_fbxBlendShape->GetBlendShapeChannel(index);
            if (fbxBlendShapeChannel)
            {
                return AZStd::make_shared<FbxBlendShapeChannelWrapper>(fbxBlendShapeChannel);
            }
            else
            {
                return nullptr;
            }
        }
    } // namespace FbxSDKWrapper
} // namespace AZ
