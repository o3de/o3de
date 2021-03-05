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
        class FbxAnimCurveNodeWrapper;

        class FbxAnimLayerWrapper
        {
            friend class FbxNodeWrapper;
        public:
            FbxAnimLayerWrapper(FbxAnimLayer* fbxAnimLayer);
            ~FbxAnimLayerWrapper() = default;
            const char* GetName() const;
            u32 GetCurveNodeCount() const ;
            FbxAnimLayer* GetFbxLayer() const;
            AZStd::shared_ptr<const FbxAnimCurveNodeWrapper> GetCurveNodeWrapper(u32 index) const;

        protected:
            FbxAnimLayer* m_fbxAnimLayer;
        };
    } // namespace FbxSDKWrapper
} // namespace AZ
