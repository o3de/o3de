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
        class FbxNodeWrapper;
        class FbxMeshWrapper;

        class FbxBlendShapeChannelWrapper
        {
        public:
            FbxBlendShapeChannelWrapper(FbxBlendShapeChannel* fbxBlendShapeChannel);
            virtual ~FbxBlendShapeChannelWrapper();

            virtual const char* GetName() const;
            virtual int GetTargetShapeCount() const;

            //While not strictly true that the target shapes are meshes, for the purposes of the engine's
            //current runtime they must be meshes. 
            virtual AZStd::shared_ptr<const FbxMeshWrapper> GetTargetShape(int index) const;

        protected:
            FbxBlendShapeChannelWrapper() = default;
            FbxBlendShapeChannel* m_fbxBlendShapeChannel;
        };
    } // namespace FbxSDKWrapper
} // namespace AZ
