/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace EMotionFX
{
    class ObjectAffectedByParameterChanges
    {
    public:
        AZ_RTTI(ObjectAffectedByParameterChanges, "{5B0BC730-F5FF-490F-93E6-706080058D08}")

        virtual ~ObjectAffectedByParameterChanges() = default;

        MOCK_CONST_METHOD1(AddRequiredParameters, void(AZStd::vector<AZStd::string>& parameterNames));
        MOCK_CONST_METHOD0(GetParameters, AZStd::vector<AZStd::string>());
        MOCK_CONST_METHOD0(GetParameterAnimGraph, AnimGraph*());
        MOCK_METHOD1(ParameterMaskChanged, void(const AZStd::vector<AZStd::string>& newParameterMask));
        MOCK_METHOD1(ParameterAdded, void(const AZStd::string& newParameterName));
        MOCK_METHOD2(ParameterRenamed, void(const AZStd::string& oldParameterName, const AZStd::string& newParameterName));
        MOCK_METHOD2(ParameterOrderChanged, void(const ValueParameterVector& beforeChange, const ValueParameterVector& afterChange));
        MOCK_METHOD1(ParameterRemoved, void(const AZStd::string& oldParameterName));
        MOCK_METHOD2(BuildParameterRemovedCommands, void(MCore::CommandGroup& commandGroup, const AZStd::string& oldParameterName));

        static void SortAndRemoveDuplicates(AnimGraph* animGraph, AZStd::vector<AZStd::string>& parameterNames);
    };
}
