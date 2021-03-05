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

namespace EMotionFX
{
    class Parameter
    {
    public:
        virtual ~Parameter() = default;
        
        AZ_RTTI(Parameter, "{4AF0BAFC-98F8-4EA3-8946-4AD87D7F2A6C}")
        MOCK_CONST_METHOD0(GetTypeDisplayName, const AZStd::string&());
        
        MOCK_CONST_METHOD0(GetName, const AZStd::string&());
        MOCK_METHOD1(SetName, void(const AZStd::string&));

        MOCK_CONST_METHOD0(GetDescription, const AZStd::string&());
        MOCK_METHOD1(SetDescription, void(const AZStd::string&));
    };
} // namespace EMotionFX

