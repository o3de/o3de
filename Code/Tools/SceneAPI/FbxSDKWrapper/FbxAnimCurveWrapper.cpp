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

#include <SceneAPI/FbxSDKWrapper/FbxAnimCurveWrapper.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        FbxAnimCurveWrapper::FbxAnimCurveWrapper(FbxAnimCurve* fbxAnimCurve)
            : m_fbxAnimCurve(fbxAnimCurve)
        {
        }

        const char* FbxAnimCurveWrapper::GetName() const
        {
            return m_fbxAnimCurve->GetName();
        }

        float FbxAnimCurveWrapper::Evaluate(FbxTimeWrapper& time) const
        {
            return m_fbxAnimCurve->Evaluate(time.m_fbxTime);
        }
    } // namespace FbxSDKWrapper
} // namespace AZ