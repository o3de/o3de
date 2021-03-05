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

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ
{
    struct OverloadInfo
    {
        size_t m_index = 0;
        size_t m_matchCount = 0;
    };

    struct OverloadVariance
    {
        AZStd::unordered_map<size_t, AZStd::vector<const BehaviorParameter*>> m_input;
        AZStd::vector<const BehaviorParameter*> m_output;
    };


    AZ::BehaviorContext* GetDefaultBehaviorContext();

    ExplicitOverloadInfo GetExplicitOverloads(const AZStd::string& name);

    ExplicitOverloadInfo GetExplicitOverloads(const BehaviorMethod& method);

    AZStd::string GetOverloadName(const BehaviorMethod&, size_t overloadIndex, AZStd::string nameOverride = {});

    AZStd::string GetOverloadName(const BehaviorMethod&, size_t overloadIndex, const OverloadVariance& variance, AZStd::string nameOverride = {});    

    AZStd::vector<AZStd::string> GetOverloadNames(const BehaviorMethod& method, AZStd::string nameOverride = {});

    /**
    /// returns a map of 0 based argument index --> vector<BehaviorParameter*>>. no two rows should have the same type

    e.g.:
    
    // overloaded argument index = 1
    void myOverload(EntityId, float);   // overload index 0
    void myOverload(EntityId, bool);    // overload index 1
    void myOverload(EntityId, string);  // overload index 2

    would yield:
    1 -> { BP*(float), BP*(bool),  BP*(string), 

    The argument index (0) is omitted, since it is EntityId in all cases, and never overloaded

    */
    enum class VariantOnThis
    {
        No,
        Yes
    };
    OverloadVariance GetOverloadVariance(const BehaviorMethod& method, const AZStd::vector<AZStd::pair<const BehaviorMethod*, const BehaviorClass*>>& overloads, VariantOnThis onThis);

    AZStd::vector<AZStd::pair<const BehaviorMethod*, const BehaviorClass*>> OverloadsToVector(const BehaviorMethod&, const BehaviorClass*);

    AZStd::string ReplaceCppArtifacts(AZStd::string_view sourceName);

    void StripQualifiers(AZStd::string& name);

    /// returns true iff a and b have the same type and traits
    bool TypeCompare(const BehaviorParameter& a, const BehaviorParameter& b);

}
