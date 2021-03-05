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

#include <AzCore/Casting/numeric_cast.h>
#include <SceneAPI/FbxSDKWrapper/FbxPropertyWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxTypeConverter.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        FbxPropertyWrapper::FbxPropertyWrapper(FbxProperty* fbxProperty)
            : m_fbxProperty(fbxProperty)
        {
            AZ_Assert(fbxProperty, "Invalid FbxProperty to initialize FbxPropertyWrapper");
        }

        FbxPropertyWrapper::~FbxPropertyWrapper()
        {
            m_fbxProperty = nullptr;
        }

        bool FbxPropertyWrapper::IsValid() const
        {
            return m_fbxProperty->IsValid();
        }

        Vector3 FbxPropertyWrapper::GetFbxVector3() const
        {
            return FbxTypeConverter::ToVector3(m_fbxProperty->Get<FbxVector4>());
        }

        int FbxPropertyWrapper::GetFbxInt() const
        {
            return aznumeric_caster(m_fbxProperty->Get<FbxInt>());
        }

        AZStd::string FbxPropertyWrapper::GetFbxString() const
        {
            return AZStd::string(m_fbxProperty->Get<FbxString>().Buffer());
        }

        const char* FbxPropertyWrapper::GetEnumValue(int index) const
        {
            return m_fbxProperty->GetEnumValue(index);
        }
    } // namespace FbxSDKWrapper
} // namespace AZ
