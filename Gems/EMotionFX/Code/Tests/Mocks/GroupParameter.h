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
    class GroupParameter
        : public Parameter
    {
    public:
        AZ_RTTI(GroupParameter, "{6B42666E-82D7-431E-807E-DA789C53AF05}", Parameter)

        MOCK_CONST_METHOD0(GetTypeDisplayName, const AZStd::string&());
        MOCK_CONST_METHOD0(GetNumParameters, size_t());
        MOCK_CONST_METHOD0(GetNumValueParameters, size_t());
        MOCK_CONST_METHOD1(FindParameter, const Parameter*(size_t index));
        MOCK_CONST_METHOD0(RecursivelyGetChildParameters, ParameterVector());
        MOCK_CONST_METHOD0(RecursivelyGetChildGroupParameters, GroupParameterVector());
        MOCK_CONST_METHOD0(RecursivelyGetChildValueParameters, ValueParameterVector());
        MOCK_CONST_METHOD0(GetChildParameters, const ParameterVector&());
        MOCK_CONST_METHOD0(GetChildValueParameters, ValueParameterVector());
        MOCK_CONST_METHOD1(FindParameterByName, const Parameter*(const AZStd::string& parameterName));
        MOCK_CONST_METHOD1(FindGroupParameterByName, const GroupParameter*(const AZStd::string& groupParameterName));
        MOCK_CONST_METHOD1(FindParentGroupParameter, const GroupParameter*(const Parameter* parameter));
        MOCK_CONST_METHOD1(FindParameterIndexByName, AZ::Outcome<size_t>(const AZStd::string& parameterName));
        MOCK_CONST_METHOD1(FindValueParameterIndexByName, AZ::Outcome<size_t>(const AZStd::string& parameterName));
        MOCK_CONST_METHOD1(FindParameterIndex, AZ::Outcome<size_t>(const Parameter* parameter));
        MOCK_CONST_METHOD1(FindValueParameterIndex, AZ::Outcome<size_t>(const Parameter* valueParameter));
        MOCK_CONST_METHOD1(FindRelativeParameterIndex, AZ::Outcome<size_t>(const Parameter* parameter));
        MOCK_METHOD2(AddParameter, bool(Parameter* parameter, const GroupParameter* parent));
        MOCK_METHOD3(InsertParameter, bool(size_t index, Parameter* parameter, const GroupParameter* parent));
        MOCK_METHOD1(RemoveParameter, bool(Parameter* parameter));
        MOCK_METHOD1(TakeParameterFromParent, bool(const Parameter* parameter));
    };
} // namespace EMotionFX

