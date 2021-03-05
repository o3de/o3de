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
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SDKWrapper/NodeWrapper.h>
namespace fbxsdk
{
    class FbxScene;
}
#ifdef ASSET_IMPORTER_SDK_SUPPORTED_TRAIT
struct aiScene;
#endif

namespace AZ
{
    namespace SDKScene
    {
        class SceneWrapperBase
        {
        public:
            AZ_RTTI(SceneWrapperBase, "{703CD344-2C75-4F30-8CE2-6BDEF2511AFD}");
            SceneWrapperBase() = default;
            SceneWrapperBase(fbxsdk::FbxScene* fbxScene);
            virtual ~SceneWrapperBase() = default;
#ifdef ASSET_IMPORTER_SDK_SUPPORTED_TRAIT
            SceneWrapperBase(aiScene* aiScene);
#endif

            virtual bool LoadSceneFromFile(const char* fileName);
            virtual bool LoadSceneFromFile(const AZStd::string& fileName);

            virtual const std::shared_ptr<SDKNode::NodeWrapper> GetRootNode() const;
            virtual std::shared_ptr<SDKNode::NodeWrapper> GetRootNode();

            virtual void Clear();
            
            virtual fbxsdk::FbxScene* GetFbxScene() const;
#ifdef ASSET_IMPORTER_SDK_SUPPORTED_TRAIT
            virtual const aiScene* GetAssImpScene() const;
#endif

            fbxsdk::FbxScene* m_fbxScene = nullptr;
#ifdef ASSET_IMPORTER_SDK_SUPPORTED_TRAIT
            const aiScene* m_assImpScene = nullptr;
#endif

            static const char* s_defaultSceneName;
        };

    } //namespace Scene
} //namespace AZ
