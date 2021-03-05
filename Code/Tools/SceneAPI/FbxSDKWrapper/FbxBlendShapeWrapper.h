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

#include <fbxsdk.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class FbxBlendShapeChannelWrapper;
        class FbxNodeWrapper;
        class FbxMeshWrapper;

        class FbxBlendShapeWrapper
        {
        public:
            FbxBlendShapeWrapper(FbxBlendShape* fbxBlendShape);
            virtual ~FbxBlendShapeWrapper();

            virtual const char* GetName() const;
            //Technically Fbx returns a FbxGeometry off this interface, but we only support meshes in the engine runtime.
            virtual AZStd::shared_ptr<const FbxMeshWrapper> GetGeometry() const;
            virtual int GetBlendShapeChannelCount() const;
            virtual AZStd::shared_ptr<const FbxBlendShapeChannelWrapper> GetBlendShapeChannel(int index) const;

        protected:
            FbxBlendShapeWrapper() = default;
            FbxBlendShape* m_fbxBlendShape;
        };
    } // namespace FbxSDKWrapper
} // namespace AZ
