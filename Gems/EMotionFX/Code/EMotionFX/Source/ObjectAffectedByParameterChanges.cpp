/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/ObjectAffectedByParameterChanges.h>

namespace EMotionFX
{
    void ObjectAffectedByParameterChanges::AddRequiredParameters(AZStd::vector<AZStd::string>& parameterNames) const
    {
        AZ_UNUSED(parameterNames);
    }

    AZStd::vector<AZStd::string> ObjectAffectedByParameterChanges::GetParameters() const
    {
        return {};
    }

    AnimGraph* ObjectAffectedByParameterChanges::GetParameterAnimGraph() const
    {
        return nullptr;
    }

    void ObjectAffectedByParameterChanges::ParameterMaskChanged(const AZStd::vector<AZStd::string>& newParameterMask)
    {
        AZ_UNUSED(newParameterMask);
    }

    void ObjectAffectedByParameterChanges::ParameterAdded(const AZStd::string& newParameterName)
    {
        AZ_UNUSED(newParameterName);
    }

    void ObjectAffectedByParameterChanges::ParameterRenamed(const AZStd::string& oldParameterName, const AZStd::string& newParameterName)
    {
        AZ_UNUSED(oldParameterName);
        AZ_UNUSED(newParameterName);
    }

    void ObjectAffectedByParameterChanges::ParameterOrderChanged(const ValueParameterVector& beforeChange, const ValueParameterVector& afterChange)
    {
        AZ_UNUSED(beforeChange);
        AZ_UNUSED(afterChange);
    }

    void ObjectAffectedByParameterChanges::ParameterRemoved(const AZStd::string& oldParameterName)
    {
        AZ_UNUSED(oldParameterName);
    }

    void ObjectAffectedByParameterChanges::SortAndRemoveDuplicates(AnimGraph* animGraph, AZStd::vector<AZStd::string>& parameterNames)
    {
        // get the list of value parameters. Then iterate over them and build another vector with the ones contained in ports
        // This way we sort them based in value parameter and remove port duplicates
        AZStd::vector<AZStd::string> sortedPorts;
        const ValueParameterVector& valueParameters = animGraph->RecursivelyGetValueParameters();
        for (const ValueParameter* valueParameter : valueParameters)
        {
            if (AZStd::find(parameterNames.begin(), parameterNames.end(), valueParameter->GetName()) != parameterNames.end())
            {
                sortedPorts.push_back(valueParameter->GetName());
            }
        }
        parameterNames.swap(sortedPorts);
    }
} // namespace EMotionFX
