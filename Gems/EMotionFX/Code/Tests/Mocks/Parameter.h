/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

