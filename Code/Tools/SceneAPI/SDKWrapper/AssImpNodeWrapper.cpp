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
#include <AzCore/Math/Sha1.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <assimp/scene.h>


namespace AZ
{
    namespace AssImpSDKWrapper
    {
        AssImpNodeWrapper::AssImpNodeWrapper(aiNode* fbxNode)
            :SDKNode::NodeWrapper(fbxNode)
        {
            AZ_Assert(m_assImpNode, "Asset Importer Node cannot be null");
        }

        AssImpNodeWrapper::~AssImpNodeWrapper()
        {
        }
        const char* AssImpNodeWrapper::GetName() const
        {
            return m_assImpNode->mName.C_Str();
        }

        AZ::u64 AssImpNodeWrapper::GetUniqueId() const
        {
            AZStd::string fingerprintString;
            fingerprintString.append(GetName());
            fingerprintString.append(m_assImpNode->mParent ? m_assImpNode->mParent->mName.C_Str() : "");
            AZStd::string extraInformation = AZStd::string::format("%i%i", m_assImpNode->mNumChildren, m_assImpNode->mNumMeshes);
            fingerprintString.append(extraInformation);
            AZ::Sha1 sha;
            sha.ProcessBytes(fingerprintString.data(), fingerprintString.size());
            AZ::u32 digest[5]; //sha1 populate a 5 element array of AZ:u32
            sha.GetDigest(digest);
            return (static_cast<AZ::u64>(digest[0]) << 32) | digest[1];
        }

        int AssImpNodeWrapper::GetChildCount() const
        {
            return m_assImpNode->mNumChildren;
        }

        const std::shared_ptr<SDKNode::NodeWrapper> AssImpNodeWrapper::GetChild(int childIndex) const
        {
            aiNode* child = m_assImpNode->mChildren[childIndex];
            AZ_Error("SDKWrapper", child, "Cannot get child assImpNode at index %d", childIndex);
            return child ? std::shared_ptr<SDKNode::NodeWrapper>(new AssImpNodeWrapper(child)) : nullptr;
        }

        const bool AssImpNodeWrapper::ContainsMesh()
        {
            return m_assImpNode->mNumMeshes != 0;
        }

        int AssImpNodeWrapper::GetMaterialCount() const
        {
            // one mesh uses only a single material
            return m_assImpNode->mNumMeshes;
        }
    } // namespace AssImpSDKWrapper
} // namespace AZ
