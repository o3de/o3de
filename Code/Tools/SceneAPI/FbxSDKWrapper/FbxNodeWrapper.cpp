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
#include <SceneAPI/FbxSDKWrapper/FbxNodeWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxMaterialWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxTypeConverter.h>
#include <SceneAPI/FbxSDKWrapper/FbxAnimLayerWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxAnimCurveWrapper.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        FbxNodeWrapper::FbxNodeWrapper(FbxNode* fbxNode)
            : NodeWrapper(fbxNode)
        {
            AZ_Assert(fbxNode, "Invalid FbxNode to initialize FbxNodeWrapper");
        }

        FbxNodeWrapper::~FbxNodeWrapper()
        {
        }


        int FbxNodeWrapper::GetMaterialCount() const
        {
            return m_fbxNode->GetMaterialCount();
        }

        const char* FbxNodeWrapper::GetMaterialName(int index) const
        {
            if (index < GetMaterialCount())
            {
                return m_fbxNode->GetMaterial(index)->GetName();
            }
            AZ_Assert(index < GetMaterialCount(), "Invalid material index %d", index);
            return nullptr;
        }

        const std::shared_ptr<FbxMeshWrapper> FbxNodeWrapper::GetMesh() const
        {
            FbxMesh* mesh = m_fbxNode->GetMesh();
            return mesh ? std::shared_ptr<FbxMeshWrapper>(new FbxMeshWrapper(mesh)) : nullptr;
        }

        const std::shared_ptr<FbxPropertyWrapper> FbxNodeWrapper::FindProperty(const char* name) const
        {
            FbxProperty propertyName = m_fbxNode->FindProperty(name);
            return std::shared_ptr<FbxPropertyWrapper>(new FbxPropertyWrapper(&propertyName));
        }

        bool FbxNodeWrapper::IsBone() const
        {
            return (m_fbxNode->GetSkeleton() != nullptr);
        }

        bool FbxNodeWrapper::IsMesh() const
        {
            return (m_fbxNode->GetMesh() != nullptr);
        }

        const char* FbxNodeWrapper::GetName() const
        {
            return m_fbxNode->GetName();
        }

        AZ::u64 FbxNodeWrapper::GetUniqueId() const
        {
            return m_fbxNode->GetUniqueID();
        }

        SceneAPI::DataTypes::MatrixType FbxNodeWrapper::EvaluateGlobalTransform()
        {
            return FbxTypeConverter::ToTransform(m_fbxNode->EvaluateGlobalTransform());
        }

        Vector3 FbxNodeWrapper::EvaluateLocalTranslation()
        {
            return FbxTypeConverter::ToVector3(m_fbxNode->EvaluateLocalTranslation());
        }

        Vector3 FbxNodeWrapper::EvaluateLocalTranslation(FbxTimeWrapper& time)
        {
            return FbxTypeConverter::ToVector3(m_fbxNode->EvaluateLocalTranslation(time.m_fbxTime));
        }

        SceneAPI::DataTypes::MatrixType FbxNodeWrapper::EvaluateLocalTransform()
        {
            return FbxTypeConverter::ToTransform(m_fbxNode->EvaluateLocalTransform());
        }

        SceneAPI::DataTypes::MatrixType FbxNodeWrapper::EvaluateLocalTransform(FbxTimeWrapper& time)
        {
            return FbxTypeConverter::ToTransform(m_fbxNode->EvaluateLocalTransform(time.m_fbxTime));
        }

        Vector3 FbxNodeWrapper::EvaluateLocalRotation()
        {
            return FbxTypeConverter::ToVector3(m_fbxNode->EvaluateLocalTransform().GetR());
        }

        Vector3 FbxNodeWrapper::EvaluateLocalRotation(FbxTimeWrapper& time)
        {
            return FbxTypeConverter::ToVector3(m_fbxNode->EvaluateLocalTransform(time.m_fbxTime).GetR());
        }

        Vector3 FbxNodeWrapper::GetGeometricTranslation() const
        {
            return FbxTypeConverter::ToVector3(m_fbxNode->GetGeometricTranslation(FbxNode::eSourcePivot));
        }

        Vector3 FbxNodeWrapper::GetGeometricScaling() const
        {
            return FbxTypeConverter::ToVector3(m_fbxNode->GetGeometricScaling(FbxNode::eSourcePivot));
        }

        Vector3 FbxNodeWrapper::GetGeometricRotation() const
        {
            return FbxTypeConverter::ToVector3(m_fbxNode->GetGeometricRotation(FbxNode::eSourcePivot));
        }

        SceneAPI::DataTypes::MatrixType FbxNodeWrapper::GetGeometricTransform() const
        {
            FbxAMatrix geoTransform(m_fbxNode->GetGeometricTranslation(FbxNode::eSourcePivot),
                m_fbxNode->GetGeometricRotation(FbxNode::eSourcePivot), m_fbxNode->GetGeometricScaling(FbxNode::eSourcePivot));
            return FbxTypeConverter::ToTransform(geoTransform);
        }

        const AZStd::shared_ptr<FbxAnimCurveWrapper> FbxNodeWrapper::GetLocalTranslationCurve(const AZStd::shared_ptr<FbxAnimLayerWrapper>& layer, 
            CurveNodeComponent component) const
        {
            if (!layer)
            {
                return nullptr;
            }
            const char* componentString = nullptr;
            switch (component)
            {
            case Component_X:
                componentString = FBXSDK_CURVENODE_COMPONENT_X;
                break;
            case Component_Y:
                componentString = FBXSDK_CURVENODE_COMPONENT_Y;
                break;
            case Component_Z:
                componentString = FBXSDK_CURVENODE_COMPONENT_Z;
                break;
            default:
                break;
            }
            // If there is available animation curve data of specified component/channel for translation, create and return a wrapped version of it.
            return m_fbxNode->LclTranslation.GetCurve(layer->m_fbxAnimLayer, componentString) ? 
                AZStd::make_shared<FbxAnimCurveWrapper>(m_fbxNode->LclTranslation.GetCurve(layer->m_fbxAnimLayer, componentString)) : nullptr;
        }

        const AZStd::shared_ptr<FbxAnimCurveWrapper> FbxNodeWrapper::GetLocalRotationCurve(const AZStd::shared_ptr<FbxAnimLayerWrapper>& layer, 
            CurveNodeComponent component) const
        {
            if (!layer)
            {
                return nullptr;
            }
            const char* componentString = nullptr;
            switch (component)
            {
            case Component_X:
                componentString = FBXSDK_CURVENODE_COMPONENT_X;
                break;
            case Component_Y:
                componentString = FBXSDK_CURVENODE_COMPONENT_Y;
                break;
            case Component_Z:
                componentString = FBXSDK_CURVENODE_COMPONENT_Z;
                break;
            default:
                break;
            }
            // If there is available animation curve data of specified component/channel for rotation, create and return a wrapped version of it.
            return m_fbxNode->LclRotation.GetCurve(layer->m_fbxAnimLayer, componentString) ?
                AZStd::make_shared<FbxAnimCurveWrapper>(m_fbxNode->LclRotation.GetCurve(layer->m_fbxAnimLayer, componentString)) : nullptr;
        }

        bool FbxNodeWrapper::IsAnimated() const
        {
            return FbxAnimUtilities::IsAnimated(m_fbxNode);
        }

        int FbxNodeWrapper::GetChildCount() const
        {
            return m_fbxNode->GetChildCount();
        }

        const std::shared_ptr<SDKNode::NodeWrapper> FbxNodeWrapper::GetChild(int childIndex) const
        {
            FbxNode* child = m_fbxNode->GetChild(childIndex);
            AZ_Assert(child, "Cannot get child FbxNode at index %d", childIndex);
            return child ? std::shared_ptr<SDKNode::NodeWrapper>(new FbxNodeWrapper(child)) : nullptr;
        }

        const std::shared_ptr<FbxMaterialWrapper> FbxNodeWrapper::GetMaterial(int index) const
        {
            if (index < GetMaterialCount())
            {
                return std::shared_ptr<FbxMaterialWrapper>(new FbxMaterialWrapper(m_fbxNode->GetMaterial(index)));
            }
            AZ_Assert(index < GetMaterialCount(), "Invalid material index %d", index);
            return nullptr;
        }
    } // namespace FbxSDKWrapper
} // namespace AZ
