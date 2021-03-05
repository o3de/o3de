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
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/FbxSDKWrapper/FbxTimeWrapper.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        FbxTimeWrapper::FbxTimeWrapper() : m_fbxTime(FBXSDK_TIME_INFINITE)
        {
        }

        FbxTimeWrapper::FbxTimeWrapper(const FbxTime& fbxTime) : m_fbxTime(fbxTime)
        {
        }

        FbxTimeWrapper::~FbxTimeWrapper()
        {
        }

        void FbxTimeWrapper::SetFrame(int64_t frames, TimeMode timeMode)
        {
            m_fbxTime.SetFrame(static_cast<FbxLongLong>(frames), GetFbxTimeMode(timeMode));
        }

        void FbxTimeWrapper::SetTime(double time)
        {
            m_fbxTime.SetSecondDouble(time);
        }

        double FbxTimeWrapper::GetFrameRate() const
        {
            return m_fbxTime.GetFrameRate(m_fbxTime.GetGlobalTimeMode());
        }

        int64_t FbxTimeWrapper::GetFrameCount() const
        {
            return static_cast<int64_t>(m_fbxTime.GetFrameCount());
        }

        double FbxTimeWrapper::GetTime() const
        {
            return m_fbxTime.GetSecondDouble();
        }

        FbxTime::EMode FbxTimeWrapper::GetFbxTimeMode(TimeMode timeMode) const
        {
            switch (timeMode)
            {
            case frames60:
                return FbxTime::eFrames60;
            case frames30:
                return FbxTime::eFrames30;
            case frames24:
                return FbxTime::eFrames24;
            case defaultMode:
                return FbxTime::eDefaultMode;
            default:
                AZ_TracePrintf(SceneAPI::Utilities::WarningWindow, "Unsupported frame rate");
                return FbxTime::eDefaultMode;
            }
        }
    } // namespace FbxSDKWrapper
} // namespace AZ
