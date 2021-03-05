#pragma once

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

#include "fbxsdk.h"
#include "FbxTimeWrapper.h"
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class FbxAnimCurveWrapper;

        class FbxAnimCurveNodeWrapper
        {
        public:
            FbxAnimCurveNodeWrapper(FbxAnimCurveNode* fbxAnimCurveNode);
            ~FbxAnimCurveNodeWrapper() = default;
            const char* GetName() const;
            int GetChannelCount() const;
            int GetCurveCount(int channelID) const;

            AZStd::shared_ptr<const FbxAnimCurveWrapper> GetCurveWrapper(int channelID, int index) const;

        protected:
            FbxAnimCurveNode* m_fbxAnimCurveNode;
        };
    }
}