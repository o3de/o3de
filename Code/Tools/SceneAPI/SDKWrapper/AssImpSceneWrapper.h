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
#include <SceneAPI/SDKWrapper/SceneWrapper.h>
#include <assimp/Importer.hpp>

struct aiScene;

namespace AZ
{
    namespace AssImpSDKWrapper
    {
        class AssImpSceneWrapper : public SDKScene::SceneWrapperBase
        {
        public:
            AZ_RTTI(AssImpSceneWrapper, "{43A61F62-DCD4-4132-B80B-F2FBC80740BC}", SDKScene::SceneWrapperBase);
            AssImpSceneWrapper();
            AssImpSceneWrapper(aiScene* aiScene);
            ~AssImpSceneWrapper();

            bool LoadSceneFromFile(const char* fileName) override;
            bool LoadSceneFromFile(const AZStd::string& fileName) override;

            const std::shared_ptr<SDKNode::NodeWrapper> GetRootNode() const override;
            std::shared_ptr<SDKNode::NodeWrapper> GetRootNode() override;
            void Clear() override;

            enum class AxisVector
            {
                X = 0,
                Y = 1,
                Z = 2,
                Unknown
            };

            AZStd::pair<AxisVector, int32_t> GetUpVectorAndSign() const;
            AZStd::pair<AxisVector, int32_t> GetFrontVectorAndSign() const;
        protected:

            Assimp::Importer m_importer;
        };

    } // namespace AssImpSDKWrapper
} // namespace AZ
