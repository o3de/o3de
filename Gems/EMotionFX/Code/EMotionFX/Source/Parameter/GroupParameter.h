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

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Outcome/Outcome.h>

#include "Parameter.h"
#include "ValueParameter.h"

namespace EMotionFX
{
    class GroupParameter;
    typedef AZStd::vector<GroupParameter*> GroupParameterVector;

    /**
    * GroupParameter inherits from Parameter and is the type that allows to nest parameters.
    */
    class GroupParameter
        : public Parameter
    {
    public:
        AZ_RTTI(GroupParameter, "{6B42666E-82D7-431E-807E-DA789C53AF05}", Parameter)
        AZ_CLASS_ALLOCATOR_DECL

        GroupParameter() = default;
        explicit GroupParameter(
            AZStd::string name,
            AZStd::string description = {}
        )
            : Parameter(AZStd::move(name), AZStd::move(description))
        {
        }

        ~GroupParameter() override;

        static void Reflect(AZ::ReflectContext* context);

        const char* GetTypeDisplayName() const override;
        
        /**
        * Get the total number of parameters inside this group. This will be the number of parameters from all child group parameters
        * counting the groups (therefore is the amount of parameters that have a value)
        * @result The number of parameters.
        */
        size_t GetNumParameters() const;

        /**
        * Get the total number of value parameters inside this group. This will be the number of parameters from all group parameters.
        * without counting the groups.
        * @result The number of value parameters.
        */
        size_t GetNumValueParameters() const;

        /**
        * Get a pointer to the given parameter.
        * @param[in] index The index of the parameter to return (index based on all parameters, returned parameter could be a group).
        * @result A pointer to the parameter.
        */
        const Parameter* FindParameter(size_t index) const;

        /**
        * Get all the parameters contained by this group, recursively. This means that if a group contains more value
        * parameters, those are returned as well.
        * @result The value parameters contained in this group, recursively.
        */
        ParameterVector RecursivelyGetChildParameters() const;

        /**
        * Get all the group parameters contained in this group, recursively. This means that if a group is contained in
        * another group within this group, it is returned as well.
        * @result The group parameters contained in this group, recursively.
        */
        GroupParameterVector RecursivelyGetChildGroupParameters() const;

        /**
        * Get all the value parameters contained in this group, recursively. This means that if a group contains more value
        * parameters, those are returned as well.
        * @result The value parameters contained in this group, recursively.
        */
        ValueParameterVector RecursivelyGetChildValueParameters() const;

        /**
        * Get all the parameters contained directly by this group. This means that if this group contains a group which contains
        * more parameters, those are not returned (but the group is).
        * @result The parameters contained directly by this group.
        */
        const ParameterVector& GetChildParameters() const;

        /**
        * Get all the value parameters contained directly by this group. This means that if this group contains a group which contains
        * more value parameters, those are not returned.
        * @result The value parameters contained directly by this group.
        */
        ValueParameterVector GetChildValueParameters() const;
        
        /**
        * Find parameter by name.
        * @param[in] paramName The name of the parameter to search for.
        * @result A pointer to the parameter with the given name. nullptr will be returned in case it has not been found.
        */
        const Parameter* FindParameterByName(const AZStd::string& parameterName) const;

        /**
        * Find a group parameter by name.
        * @param groupName The group name to search for.
        * @result A pointer to the group parameter with the given name. nullptr will be returned in case it has not been found.
        */
        const GroupParameter* FindGroupParameterByName(const AZStd::string& groupParameterName) const;

        /**
        * Find the group parameter the given parameter is part of.
        * @param[in] parameter The parameter to search the group parameter for.
        * @result A pointer to the group parameter in which the given parameter is in. nullptr will be returned in case the parameter is not part of a group parameter.
        */
        const GroupParameter* FindParentGroupParameter(const Parameter* parameter) const;

        /**
        * Find parameter index by name.
        * @param[in] paramName The name of the parameter to search for.
        * @result The index of the parameter with the given name. AZ::Failure will be returned in case it has not been found.
        */
        AZ::Outcome<size_t> FindParameterIndexByName(const AZStd::string& parameterName) const;

        /**
        * Find value parameter index by name. Index is relative to other value parameters.
        * @param[in] paramName The name of the value parameter to search for.
        * @result The index of the parameter with the given name. AZ::Failure will be returned in case it has not been found.
        */
        AZ::Outcome<size_t> FindValueParameterIndexByName(const AZStd::string& parameterName) const;
        
        /**
        * Find parameter index by parameter.
        * @param[in] parameter The parameter to search for.
        * @result The index of the parameter. AZ::Failure will be returned in case it has not been found.
        */
        AZ::Outcome<size_t> FindParameterIndex(const Parameter* parameter) const;

        /**
        * Find value parameter index by parameter. Index is relative to other value parameters.
        * @param[in] parameter The parameter to search for.
        * @result The index of the parameter. AZ::Failure will be returned in case it has not been found.
        */
        AZ::Outcome<size_t> FindValueParameterIndex(const Parameter* valueParameter) const;
        
        /**
        * Find parameter index by parameter. Index is relative to its siblings.
        * @param[in] parameter The parameter to search for.
        * @result The index of the parameter. AZ::Failure will be returned in case it has not been found.
        */
        AZ::Outcome<size_t> FindRelativeParameterIndex(const Parameter* parameter) const;

        /**
        * Add the given parameter.
        * The parameter will be fully managed and destroyed by this group.
        * @param[in] parameter A pointer to the parameter to add.
        * #param[in] parent Parent group parameter. If nullptr, the parameter will be added to the root group parameter
        * @result success if the parameter was added, the returned index is the absolute index among all parameter values
        */
        bool AddParameter(Parameter* parameter, const GroupParameter* parent);

        /**
        * Insert the given parameter at the specified index. The index is relative to the parent.
        * The parameter will be fully managed and destroyed by this group.
        * @param[in] index The index at which position the new parameter shall be inserted.
        * @param[in] parameter A pointer to the attribute info to insert.
        * #param[in] parent Parent group parameter. If nullptr, the parameter will be added to the root group parameter
        * @result success if the parameter was added, the returned index is the absolute index among all parameter values
        */
        bool InsertParameter(size_t index, Parameter* parameter, const GroupParameter* parent);

        /**
        * Removes the parameter specified by index. The parameter will be deleted.
        * @param[in] parameter The parameter to remove.
        */
        bool RemoveParameter(Parameter* parameter);

        /**
        * Iterate over all group parameters and make sure the given parameter is not part of any of the groups anymore.
        * @param[in] parameterIndex The index of the parameter to be removed from all group parameters.
        */
        bool TakeParameterFromParent(const Parameter* parameter);

    private:
        ParameterVector m_childParameters;
    };
}
