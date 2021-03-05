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

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <fbxsdk.h>

namespace AZ
{
    namespace FbxSDKWrapper
    { 
        using FbxSDKLongLong = FbxLongLong;
        class FbxAnimLayerWrapper;
        class FbxTimeSpanWrapper;

        class FbxAnimStackWrapper
        {
        public:
            FbxAnimStackWrapper(FbxAnimStack* fbxAnimStack);
            virtual ~FbxAnimStackWrapper();

            virtual const char* GetName() const;
            virtual int GetAnimationLayerCount() const;
            virtual const AZStd::shared_ptr<FbxAnimLayerWrapper> GetAnimationLayerAt(int index) const;
            virtual FbxTimeSpanWrapper GetLocalTimeSpan() const;

        protected:
            FbxAnimStackWrapper() = default;
            FbxAnimStack* m_fbxAnimStack;
        };
    } // namespace FbxSDKWrapper
} // namespace AZ
