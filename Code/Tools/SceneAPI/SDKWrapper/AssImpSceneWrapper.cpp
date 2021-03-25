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
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace AZ
{
    namespace AssImpSDKWrapper
    {
        AssImpSceneWrapper::AssImpSceneWrapper()
            : SDKScene::SceneWrapperBase()
        {
        }
        AssImpSceneWrapper::AssImpSceneWrapper(aiScene* aiScene)
            : SDKScene::SceneWrapperBase(aiScene)
        {
        }

        AssImpSceneWrapper::~AssImpSceneWrapper()
        {
        }

        bool AssImpSceneWrapper::LoadSceneFromFile(const char* fileName)
        {
            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "AssImpSceneWrapper::LoadSceneFromFile %s", fileName);
            AZ_TraceContext("Filename", fileName);
            m_importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
            m_sceneFileName = fileName;
            m_assImpScene = m_importer.ReadFile(fileName,
                aiProcess_Triangulate //Triangulates all faces of all meshes
                | aiProcess_JoinIdenticalVertices //Identifies and joins identical vertex data sets for the imported meshes
                | aiProcess_GenNormals); //Generate normals for meshes
            if (!m_assImpScene)
            {
                AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Failed to import Asset Importer Scene. Error returned: %s", m_importer.GetErrorString());
                return false;
            }

            return true;
        }

        bool AssImpSceneWrapper::LoadSceneFromFile(const AZStd::string& fileName)
        {
            return LoadSceneFromFile(fileName.c_str());
        }

        const std::shared_ptr<SDKNode::NodeWrapper> AssImpSceneWrapper::GetRootNode() const
        {
            return std::shared_ptr<SDKNode::NodeWrapper>(new AssImpNodeWrapper(m_assImpScene->mRootNode));
        }
        std::shared_ptr<SDKNode::NodeWrapper> AssImpSceneWrapper::GetRootNode()
        {
            return std::shared_ptr<SDKNode::NodeWrapper>(new AssImpNodeWrapper(m_assImpScene->mRootNode));
        }

        void AssImpSceneWrapper::Clear()
        {
            m_importer.FreeScene();
        }

        AZStd::pair<AssImpSceneWrapper::AxisVector, int32_t> AssImpSceneWrapper::GetUpVectorAndSign() const
        {
            AZStd::pair<AssImpSceneWrapper::AxisVector, int32_t> result(AxisVector::Z, 1);
            int32_t upVectorRead(static_cast<int32_t>(result.first));
            m_assImpScene->mMetaData->Get("UpAxis", upVectorRead);
            m_assImpScene->mMetaData->Get("UpAxisSign", result.second);
            result.first = static_cast<AssImpSceneWrapper::AxisVector>(upVectorRead);
            return result;
        }

        AZStd::pair<AssImpSceneWrapper::AxisVector, int32_t> AssImpSceneWrapper::GetFrontVectorAndSign() const
        {
            AZStd::pair<AssImpSceneWrapper::AxisVector, int32_t> result(AxisVector::Y, 1);
            int32_t frontVectorRead(static_cast<int32_t>(result.first));
            m_assImpScene->mMetaData->Get("FrontAxis", frontVectorRead);
            m_assImpScene->mMetaData->Get("FrontAxisSign", result.second);
            result.first = static_cast<AssImpSceneWrapper::AxisVector>(frontVectorRead);
            return result;
        }
    }//namespace AssImpSDKWrapper

} // namespace AZ
