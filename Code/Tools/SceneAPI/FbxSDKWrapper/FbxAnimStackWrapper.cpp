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
#include <AzCore/std/smart_ptr/make_shared.h>
#include <SceneAPI/FbxSDKWrapper/FbxAnimStackWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxAnimLayerWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxTimeSpanWrapper.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        FbxAnimStackWrapper::FbxAnimStackWrapper(FbxAnimStack* fbxAnimStack)
            : m_fbxAnimStack(fbxAnimStack)
        {
            AZ_Assert(fbxAnimStack, "Invalid FbxAnimStack input to initialize FbxAnimStackWrapper");
        }

        FbxAnimStackWrapper::~FbxAnimStackWrapper()
        {
            m_fbxAnimStack = nullptr;
        }

        const char* FbxAnimStackWrapper::GetName() const
        {
            return m_fbxAnimStack->GetName();
        }

        int FbxAnimStackWrapper::GetAnimationLayerCount() const
        {
            return m_fbxAnimStack->GetMemberCount<FbxAnimLayer>();
        }

        const AZStd::shared_ptr<FbxAnimLayerWrapper> FbxAnimStackWrapper::GetAnimationLayerAt(int index) const
        {
            AZ_Assert(index < GetAnimationLayerCount(), "Invalid animation layer index %d for layer count %d", index, GetAnimationLayerCount());
            return AZStd::make_shared<FbxAnimLayerWrapper>(m_fbxAnimStack->GetMember<FbxAnimLayer>(index));
        }

        FbxTimeSpanWrapper FbxAnimStackWrapper::GetLocalTimeSpan() const
        {
            return FbxTimeSpanWrapper(m_fbxAnimStack->GetLocalTimeSpan());
        }
    } // namespace FbxSDKWrapper
} // namespace AZ
