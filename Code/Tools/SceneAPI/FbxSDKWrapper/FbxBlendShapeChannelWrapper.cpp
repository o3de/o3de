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

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Debug/Trace.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/FbxSDKWrapper/FbxBlendShapeChannelWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxNodeWrapper.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        FbxBlendShapeChannelWrapper::FbxBlendShapeChannelWrapper(FbxBlendShapeChannel* fbxBlendShapeChannel)
            : m_fbxBlendShapeChannel(fbxBlendShapeChannel)
        {
            AZ_Assert(fbxBlendShapeChannel, "Invalid FbxSkin input to initialize FbxBlendShapeChannelWrapper");
        }

        FbxBlendShapeChannelWrapper::~FbxBlendShapeChannelWrapper()
        {
            m_fbxBlendShapeChannel = nullptr;
        }

        const char* FbxBlendShapeChannelWrapper::GetName() const
        {
            return m_fbxBlendShapeChannel->GetName();
        }

        //The engine currently only supports one target shape.  If there are more than one,
        //code will ultimately end up just using the max index returned by this function. 
        int FbxBlendShapeChannelWrapper::GetTargetShapeCount() const
        {
            return m_fbxBlendShapeChannel->GetTargetShapeCount();
        }

        //While not strictly true that the target shapes are meshes, for the purposes of the engine's
        //current runtime they must be meshes. 
        AZStd::shared_ptr<const FbxMeshWrapper> FbxBlendShapeChannelWrapper::GetTargetShape(int index) const
        {
            //we need to create a duplicate FbxMesh from the base mesh and the target data
            //FbxMeshWrapper needs an FbxMesh to point to so we are generating one
            //by cloning the mesh data and then replacing with the morph data.
            FbxShape* fbxShape = m_fbxBlendShapeChannel->GetTargetShape(index);
            if (!fbxShape)
            {
                return nullptr;
            }

            FbxGeometry* fbxGeom = fbxShape->GetBaseGeometry();
            if (fbxGeom && fbxGeom->GetAttributeType() == FbxNodeAttribute::EType::eMesh)
            {
                FbxMesh* fbxMesh = static_cast<FbxMesh*>(fbxGeom);
                FbxMesh* fbxBlendMesh = FbxMesh::Create(m_fbxBlendShapeChannel->GetScene(),"");
                fbxBlendMesh->Copy(*fbxMesh);
                //TODO: test that mesh is managed by the sdk
                const int count = fbxBlendMesh->GetControlPointsCount();
                //set control points from blend shape
                for (int i = 0; i < count; i++)
                {
                    fbxBlendMesh->SetControlPointAt(fbxShape->GetControlPointAt(i), i);
                }

                //source data
                FbxLayerElementArrayTemplate<FbxVector4>* normalsTemplate;
                if (fbxShape->GetNormals(&normalsTemplate))
                {
                    int normalCount = normalsTemplate->GetCount();

                    //destination data
                    FbxLayer* normalLayer = fbxBlendMesh->GetLayer(0, FbxLayerElement::eNormal);
                    if (normalLayer)
                    {
                        FbxLayerElementNormal* normals = normalLayer->GetNormals();
                        if (normals)
                        {
                            //set normal data from blend shape
                            for (int j = 0; j < normalCount; j++)
                            {
                                FbxVector4 normal = normalsTemplate->GetAt(j);
                                normals->GetDirectArray().SetAt(j, normal);
                            }
                        }
                    }
                }
                return AZStd::make_shared<FbxMeshWrapper>(fbxBlendMesh);
            }
            return nullptr;
        }
    }
}