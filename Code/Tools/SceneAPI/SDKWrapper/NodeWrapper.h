/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/RTTI/RTTI.h>
namespace fbxsdk
{
    class FbxNode;
}

struct aiNode;

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
            NodeWrapper(aiNode* aiNode);
            virtual ~NodeWrapper();

            enum CurveNodeComponent
            {
                Component_X,
                Component_Y,
                Component_Z
            };

            fbxsdk::FbxNode* GetFbxNode();
            aiNode* GetAssImpNode();

            virtual const char* GetName() const;
            virtual AZ::u64 GetUniqueId() const;
            virtual int GetMaterialCount() const;

            virtual int GetChildCount()const;
            virtual const std::shared_ptr<NodeWrapper> GetChild(int childIndex) const;


            fbxsdk::FbxNode* m_fbxNode = nullptr;
            aiNode* m_assImpNode = nullptr;
        };
    } //namespace Node
} //namespace AZ
