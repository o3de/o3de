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
#include <SceneAPI/SDKWrapper/SceneWrapper.h>

namespace AZ
{
   namespace SDKScene
   {
      const char* SceneWrapperBase::s_defaultSceneName = "myScene";

      SceneWrapperBase::SceneWrapperBase(fbxsdk::FbxScene* fbxScene)
          :m_fbxScene(fbxScene)
#ifdef ASSET_IMPORTER_SDK_SUPPORTED_TRAIT
          , m_assImpScene(nullptr)
#endif
      {
      }
#ifdef ASSET_IMPORTER_SDK_SUPPORTED_TRAIT
      SceneWrapperBase::SceneWrapperBase(aiScene* aiScene)
          :m_fbxScene(nullptr)
          , m_assImpScene(aiScene)
      {
      }
#endif

      bool SceneWrapperBase::LoadSceneFromFile([[maybe_unused]] const char* fileName)
      {
          return false;
      }
      bool SceneWrapperBase::LoadSceneFromFile([[maybe_unused]] const AZStd::string& fileName)
      {
          return LoadSceneFromFile(fileName.c_str());
      }

      const std::shared_ptr<SDKNode::NodeWrapper> SceneWrapperBase::GetRootNode() const
      {
          return {};
      }
      std::shared_ptr<SDKNode::NodeWrapper> SceneWrapperBase::GetRootNode()
      {
          return {};
      }

      void SceneWrapperBase::Clear()
      {
      }

      fbxsdk::FbxScene* SceneWrapperBase::GetFbxScene() const
      {
          return m_fbxScene;
      }

#ifdef ASSET_IMPORTER_SDK_SUPPORTED_TRAIT
      const aiScene* SceneWrapperBase::GetAssImpScene() const
      {
          return m_assImpScene;
      }
#endif


   } //namespace Scene
}// namespace AZ
