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

#pragma once

#include <SceneAPI/FbxSDKWrapper/FbxTimeWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxMeshWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxPropertyWrapper.h>
#include <SceneAPI/SceneCore/DataTypes/MatrixType.h>
#include <SceneAPI/SDKWrapper/NodeWrapper.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class FbxMaterialWrapper;
        class FbxAnimLayerWrapper;
        class FbxAnimCurveWrapper;

        class FbxNodeWrapper : public SDKNode::NodeWrapper
        {
        public:
            AZ_RTTI(FbxNodeWrapper, "{5F1C09D1-791C-41CA-94DB-D7DD2810C859}", SDKNode::NodeWrapper);
            FbxNodeWrapper(FbxNode* fbxNode);
            ~FbxNodeWrapper() override;

            int GetMaterialCount() const override;
            virtual const char* GetMaterialName(int index) const;
            virtual const std::shared_ptr<FbxMaterialWrapper> GetMaterial(int index) const;
            virtual const std::shared_ptr<FbxMeshWrapper> GetMesh() const;
            virtual const std::shared_ptr<FbxPropertyWrapper> FindProperty(const char* name) const;
            virtual bool IsBone() const;
            virtual bool IsMesh() const;
            const char* GetName() const override;
            AZ::u64 GetUniqueId() const override;

            virtual SceneAPI::DataTypes::MatrixType EvaluateGlobalTransform();
            virtual Vector3 EvaluateLocalTranslation();
            virtual Vector3 EvaluateLocalTranslation(FbxTimeWrapper& time);
            virtual SceneAPI::DataTypes::MatrixType EvaluateLocalTransform();
            virtual SceneAPI::DataTypes::MatrixType EvaluateLocalTransform(FbxTimeWrapper& time);
            virtual Vector3 EvaluateLocalRotation();
            virtual Vector3 EvaluateLocalRotation(FbxTimeWrapper& time);
            virtual Vector3 GetGeometricTranslation() const;
            virtual Vector3 GetGeometricScaling() const;
            virtual Vector3 GetGeometricRotation() const;
            virtual SceneAPI::DataTypes::MatrixType GetGeometricTransform() const;
            virtual const AZStd::shared_ptr<FbxAnimCurveWrapper> GetLocalTranslationCurve(const AZStd::shared_ptr<FbxAnimLayerWrapper>& layer, CurveNodeComponent component) const;
            virtual const AZStd::shared_ptr<FbxAnimCurveWrapper> GetLocalRotationCurve(const AZStd::shared_ptr<FbxAnimLayerWrapper>& layer, CurveNodeComponent component) const;

            int GetChildCount() const override;
            const std::shared_ptr<NodeWrapper> GetChild(int childIndex) const override;

            virtual bool IsAnimated() const;

        protected:
            FbxNodeWrapper() = default;
        };
    } // namespace FbxSDKWrapper
} // namespace AZ
