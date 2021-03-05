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
#include <AzCore/RTTI/RTTI.h>
namespace fbxsdk
{
    class FbxNode;
}
#ifdef ASSET_IMPORTER_SDK_SUPPORTED_TRAIT
struct aiNode;
#endif
namespace AZ
{
    namespace SDKNode
    {
        class NodeWrapper
        {
        public:
            AZ_RTTI(NodeWrapper, "{5EB0897B-9728-44B7-B056-BA34AAF14715}");

            NodeWrapper() = default;
            NodeWrapper(fbxsdk::FbxNode* fbxNode);
#ifdef ASSET_IMPORTER_SDK_SUPPORTED_TRAIT
            NodeWrapper(aiNode* aiNode);
#endif
            virtual ~NodeWrapper();

            enum CurveNodeComponent
            {
                Component_X,
                Component_Y,
                Component_Z
            };

            fbxsdk::FbxNode* GetFbxNode();
#ifdef ASSET_IMPORTER_SDK_SUPPORTED_TRAIT
            aiNode* GetAssImpNode();
#endif

            virtual const char* GetName() const;
            virtual AZ::u64 GetUniqueId() const;
            virtual int GetMaterialCount() const;

            virtual int GetChildCount()const;
            virtual const std::shared_ptr<NodeWrapper> GetChild(int childIndex) const;


            fbxsdk::FbxNode* m_fbxNode = nullptr;
#ifdef ASSET_IMPORTER_SDK_SUPPORTED_TRAIT
            aiNode* m_assImpNode = nullptr;
#endif
        };
    } //namespace Node
} //namespace AZ
