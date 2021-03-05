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
#include <AzCore/Casting/numeric_cast.h>
#include <SceneAPI/FbxSDKWrapper/FbxAnimLayerWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxAnimCurveNodeWrapper.h>

#include "fbxsdk.h"

namespace AZ
{
    namespace FbxSDKWrapper
    {
        FbxAnimLayerWrapper::FbxAnimLayerWrapper(FbxAnimLayer* fbxAnimLayer)
            : m_fbxAnimLayer(fbxAnimLayer)
        {
        }

        const char* FbxAnimLayerWrapper::GetName() const
        {
            return m_fbxAnimLayer->GetName();
        }

        u32 FbxAnimLayerWrapper::GetCurveNodeCount() const
        {
            return static_cast<u32>(m_fbxAnimLayer->GetMemberCount());
        }

        FbxAnimLayer* FbxAnimLayerWrapper::GetFbxLayer() const
        {
            return m_fbxAnimLayer;
        }

        AZStd::shared_ptr<const FbxAnimCurveNodeWrapper> FbxAnimLayerWrapper::GetCurveNodeWrapper(u32 index) const
        {
            return AZStd::make_shared<const FbxAnimCurveNodeWrapper>(static_cast<FbxAnimCurveNode*>(m_fbxAnimLayer->GetMember(aznumeric_cast<int>(index))));
        }
    } // namespace FbxSDKWrapper
} // namespace AZ
