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
#include <SceneAPI/FbxSDKWrapper/FbxTimeWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxTimeSpanWrapper.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        FbxTimeSpanWrapper::FbxTimeSpanWrapper(const FbxTimeSpan& fbxTimeSpan)
            : m_fbxTimeSpan(fbxTimeSpan)
        {
        }

        FbxTimeWrapper FbxTimeSpanWrapper::GetStartTime() const
        {
            return FbxTimeWrapper(m_fbxTimeSpan.GetStart());
        }

        FbxTimeWrapper FbxTimeSpanWrapper::GetStopTime() const
        {
            return FbxTimeWrapper(m_fbxTimeSpan.GetStop());
        }

        double FbxTimeSpanWrapper::GetFrameRate() const
        {
            return FbxTimeWrapper(m_fbxTimeSpan.GetStart()).GetFrameRate();
        }

        int64_t FbxTimeSpanWrapper::GetNumFrames() const
        {
            return GetStopTime().GetFrameCount() - GetStartTime().GetFrameCount() + 1;
        }
    } // namespace FbxSDKWrapper
} // namespace AZ
