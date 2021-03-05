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
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/FbxSDKWrapper/FbxSceneWrapper.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        FbxSceneWrapper::FbxSceneWrapper()
            : SceneWrapperBase()
            , m_fbxManager(nullptr)
            , m_fbxImporter(nullptr)
            , m_fbxIOSettings(nullptr)
        {
        }

        FbxSceneWrapper::FbxSceneWrapper(FbxScene* fbxScene)
            : SceneWrapperBase(fbxScene)
            , m_fbxManager(nullptr)
            , m_fbxImporter(nullptr)
            , m_fbxIOSettings(nullptr)
        {
            AZ_Assert(fbxScene, "FbxSceneWrapper should have a valid scene to initialize.");
        }

        FbxSceneWrapper::~FbxSceneWrapper()
        {
            if (m_fbxScene)
            {
                m_fbxScene->Destroy();
                m_fbxScene = nullptr;
            }

            if (m_fbxImporter)
            {
                m_fbxImporter->Destroy();
                m_fbxImporter = nullptr;
            }

            if (m_fbxIOSettings)
            {
                m_fbxIOSettings->Destroy();
                m_fbxIOSettings = nullptr;
            }

            if (m_fbxManager)
            {
                m_fbxManager->Destroy();
                m_fbxManager = nullptr;
            }
        }

        AZStd::shared_ptr<FbxAxisSystemWrapper> FbxSceneWrapper::GetAxisSystem() const
        {
            return AZStd::make_shared<FbxAxisSystemWrapper>(m_fbxScene->GetGlobalSettings().GetAxisSystem());
        }

        AZStd::shared_ptr<FbxSystemUnitWrapper> FbxSceneWrapper::GetSystemUnit() const
        {
            return AZStd::make_shared<FbxSystemUnitWrapper>(m_fbxScene->GetGlobalSettings().GetSystemUnit());
        }

        FbxTimeWrapper FbxSceneWrapper::GetTimeLineDefaultDuration() const
        {
            FbxTimeSpan timeSpan;
            m_fbxScene->GetGlobalSettings().GetTimelineDefaultTimeSpan(timeSpan);
            return FbxTimeWrapper(timeSpan.GetDuration());
        }

        const char* FbxSceneWrapper::GetLastSavedApplicationName() const
        {
            if (m_fbxScene->GetDocumentInfo())
            {
                return m_fbxScene->GetDocumentInfo()->LastSaved_ApplicationName.Get().Buffer();
            }
            AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Cannot get valid document info from FbxScene");
            return nullptr;
        }

        const char* FbxSceneWrapper::GetLastSavedApplicationVersion() const
        {
            if (m_fbxScene->GetDocumentInfo())
            {
                return m_fbxScene->GetDocumentInfo()->LastSaved_ApplicationVersion.Get().Buffer();
            }
            AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Cannot get valid document info from FbxScene");
            return nullptr;
        }

        const std::shared_ptr<SDKNode::NodeWrapper> FbxSceneWrapper::GetRootNode() const
        {
            return std::shared_ptr<FbxNodeWrapper>(new FbxNodeWrapper(m_fbxScene->GetRootNode()));
        }

        std::shared_ptr<SDKNode::NodeWrapper> FbxSceneWrapper::GetRootNode()
        {
            return std::shared_ptr<FbxNodeWrapper>(new FbxNodeWrapper(m_fbxScene->GetRootNode()));
        }

        int FbxSceneWrapper::GetAnimationStackCount() const
        {
            return m_fbxScene->GetSrcObjectCount<FbxAnimStack>();
        }

        const std::shared_ptr<FbxAnimStackWrapper> FbxSceneWrapper::GetAnimationStackAt(int index) const
        {
            return std::shared_ptr<FbxAnimStackWrapper>(new FbxAnimStackWrapper(FbxCast<FbxAnimStack>(m_fbxScene->GetSrcObject<FbxAnimStack>(index))));
        }

        bool FbxSceneWrapper::LoadSceneFromFile(const char* fileName)
        {
            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "FbxSceneWrapper::LoadSceneFromFile %s", fileName);
            AZ_TraceContext("Filename", fileName);

            if (!m_fbxManager)
            {
                m_fbxManager = FbxManager::Create();
                if (!m_fbxManager)
                {
                    AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Failed to create FbxManager");
                    return false;
                }
            }

            if (!m_fbxIOSettings)
            {
                m_fbxIOSettings = FbxIOSettings::Create(m_fbxManager, IOSROOT);
                if (!m_fbxIOSettings)
                {
                    AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Failed to create FbxIOSettings");
                    return false;
                }
            }

            m_fbxManager->SetIOSettings(m_fbxIOSettings);

            if (!m_fbxImporter)
            {
                m_fbxImporter = FbxImporter::Create(m_fbxManager, "");
                if (!m_fbxImporter)
                {
                    AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Failed to create FbxImporter with FbxManager");
                    return false;
                }
            }

            if (!m_fbxImporter->Initialize(fileName, -1, m_fbxManager->GetIOSettings()))
            {
                FbxString error = m_fbxImporter->GetStatus().GetErrorString();
                AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Failed to initialize FbxImporter with fbx file. Error returned: %s", error.Buffer());
                return false;
            }

            // Create a new FBX scene so it can be populated by the imported file.
            m_fbxScene = FbxScene::Create(m_fbxManager, s_defaultSceneName);

            if (!m_fbxScene)
            {
                AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Failed to create FbxScene");
                return false;
            }

            if (!m_fbxImporter->Import(m_fbxScene))
            {
                FbxString error = m_fbxImporter->GetStatus().GetErrorString();
                AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Failed to import FbxScene. Error returned: %s", error.Buffer());
                return false;
            }

            return true;
        }

        bool FbxSceneWrapper::LoadSceneFromFile(const AZStd::string& fileName)
        {
            return LoadSceneFromFile(fileName.c_str());
        }

        void FbxSceneWrapper::Clear()
        {
            if (m_fbxScene)
            {
                m_fbxScene->Clear();
            }
        }
    }//namespace FbxSDKWrapper

} // namespace AZ
