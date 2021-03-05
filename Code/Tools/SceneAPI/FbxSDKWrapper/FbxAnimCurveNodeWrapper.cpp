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
#include "FbxAnimCurveNodeWrapper.h"
#include "FbxAnimCurveWrapper.h"
#include "fbxsdk.h"

namespace AZ
{
    namespace FbxSDKWrapper
    {
        FbxAnimCurveNodeWrapper::FbxAnimCurveNodeWrapper(FbxAnimCurveNode* fbxAnimCurveNode)
            : m_fbxAnimCurveNode(fbxAnimCurveNode)
        {
        }

        const char* FbxAnimCurveNodeWrapper::GetName() const
        {
            return m_fbxAnimCurveNode->GetName();
        }

        int FbxAnimCurveNodeWrapper::GetChannelCount() const
        {
            return m_fbxAnimCurveNode->GetChannelsCount();
        }

        int FbxAnimCurveNodeWrapper::GetCurveCount(int channelID) const 
        {
            return m_fbxAnimCurveNode->GetCurveCount(channelID);
        }

        AZStd::shared_ptr<const FbxAnimCurveWrapper> FbxAnimCurveNodeWrapper::GetCurveWrapper(int channelID, int index) const
        {
            return AZStd::make_shared<const FbxAnimCurveWrapper>(static_cast<FbxAnimCurve*>(m_fbxAnimCurveNode->GetCurve(channelID, index)));
        }
    }
}