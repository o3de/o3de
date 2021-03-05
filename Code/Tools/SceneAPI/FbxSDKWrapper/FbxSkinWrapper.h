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

#include <fbxsdk.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class FbxNodeWrapper;

        class FbxSkinWrapper
        {
        public:
            FbxSkinWrapper(FbxSkin* fbxSkin);
            virtual ~FbxSkinWrapper();

            virtual const char* GetName() const;
            virtual int GetClusterCount() const;
            virtual int GetClusterControlPointIndicesCount(int index) const;
            virtual int GetClusterControlPointIndex(int clusterIndex, int pointIndex) const;
            virtual double GetClusterControlPointWeight(int clusterIndex, int pointIndex) const;
            virtual AZStd::shared_ptr<const FbxNodeWrapper> GetClusterLink(int index) const;

        protected:
            FbxSkinWrapper() = default;
            FbxSkin* m_fbxSkin;
        };
    } // namespace FbxSDKWrapper
} // namespace AZ
