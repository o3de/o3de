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

#include <SceneAPI/FbxSDKWrapper/FbxTimeWrapper.h>
#include <fbxsdk.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class FbxTimeSpanWrapper
        {
        public:
            FbxTimeSpanWrapper(const FbxTimeSpan& fbxTimeSpan);
            virtual ~FbxTimeSpanWrapper() = default;
            
            virtual FbxTimeWrapper GetStartTime() const;
            virtual FbxTimeWrapper GetStopTime() const;
            virtual double GetFrameRate() const;
            int64_t GetNumFrames() const;

        protected:
            FbxTimeSpan m_fbxTimeSpan;
        };
    } // namespace FbxSDKWrapper
} // namespace AZ
