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

#include <stdint.h>
#include <fbxsdk.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class FbxTimeWrapper
        {
            friend class FbxNodeWrapper;
            friend class FbxAnimCurveWrapper;
        public:
            enum TimeMode
            {
                defaultMode,
                frames60,
                frames30,
                frames24,
                eModesCount
            };

            FbxTimeWrapper();
            FbxTimeWrapper(const FbxTime& fbxTime);
            virtual ~FbxTimeWrapper();
            
            virtual void SetFrame(int64_t frames, TimeMode timeMode = defaultMode);
            virtual void SetTime(double time);
            virtual double GetFrameRate() const;
            virtual int64_t GetFrameCount() const;
            virtual double GetTime() const;

        protected:
            FbxTime::EMode GetFbxTimeMode(TimeMode timeMode) const;

            FbxTime m_fbxTime;
        };
    } // namespace FbxSDKWrapper
} // namespace AZ
