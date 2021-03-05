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

#include <SceneAPI/SDKWrapper/NodeWrapper.h>

namespace AZ
{
    namespace SDKNode
    {
        NodeWrapper::NodeWrapper(fbxsdk::FbxNode* fbxNode)
            :m_fbxNode(fbxNode)
#ifdef ASSET_IMPORTER_SDK_SUPPORTED_TRAIT
            , m_assImpNode(nullptr)
#endif
        {
        }
#ifdef ASSET_IMPORTER_SDK_SUPPORTED_TRAIT
        NodeWrapper::NodeWrapper(aiNode* aiNode)
            :m_fbxNode(nullptr)
            , m_assImpNode(aiNode)
        {
        }
#endif

        NodeWrapper::~NodeWrapper()
        {
            m_fbxNode = nullptr;
#ifdef ASSET_IMPORTER_SDK_SUPPORTED_TRAIT
            m_assImpNode = nullptr;
#endif
        }

        fbxsdk::FbxNode* NodeWrapper::GetFbxNode()
        {
            return m_fbxNode;
        }
#ifdef ASSET_IMPORTER_SDK_SUPPORTED_TRAIT
        aiNode* NodeWrapper::GetAssImpNode()
        {
            return m_assImpNode;
        }
#endif

        const char* NodeWrapper::GetName() const
        {
            return "";
        }
        AZ::u64 NodeWrapper::GetUniqueId() const
        {
            return 0;
        }

        int NodeWrapper::GetMaterialCount() const
        {
            return -1;
        }

        int NodeWrapper::GetChildCount()const
        {
            return -1;
        }
        const std::shared_ptr<NodeWrapper> NodeWrapper::GetChild([[maybe_unused]] int childIndex) const
        {
            return {};
        }

    }// namespace Node
}//namespace AZ
