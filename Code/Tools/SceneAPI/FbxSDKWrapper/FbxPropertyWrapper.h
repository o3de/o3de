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
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class FbxPropertyWrapper
        {
        public:
            FbxPropertyWrapper(FbxProperty* fbxProperty);
            virtual ~FbxPropertyWrapper();

            virtual bool IsValid() const;
            virtual Vector3 GetFbxVector3() const;
            virtual int GetFbxInt() const;
            virtual AZStd::string GetFbxString() const;
            virtual const char* GetEnumValue(int index) const;

        protected:
            FbxProperty* m_fbxProperty;
        };
    } // namespace FbxSDKWrapper
} // namespace AZ
