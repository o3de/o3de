/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContextUtilities.h>

#include <AzCore/Math/Uuid.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace BehaviorContextUtilitiesCPP
{
    using namespace AZ;

    const u32 k_typeModMask = BehaviorParameter::Traits::TR_POINTER | BehaviorParameter::Traits::TR_CONST | BehaviorParameter::Traits::TR_REFERENCE;
    
    u32 CleanTraits(u32 dirt)
    {
        return dirt & k_typeModMask;
    }

    struct TypeHasher
    {
        using argument_type = const BehaviorParameter*;
        using result_type = size_t;
        result_type operator()(const argument_type& value) const 
        {
            if (value)
            { 
                result_type result = AZStd::hash<Uuid>()(value->m_typeId);
                AZStd::hash_combine(result, CleanTraits(value->m_traits));
                return result;
            }
            else
            {
                return 0;
            }
        }
    };

    struct TypeEqual
    {
        bool operator()(const BehaviorParameter* left, const BehaviorParameter* right) const
        {
            return (left == nullptr && right == nullptr)
                || (left != nullptr
                    && right != nullptr
                    && left->m_typeId == right->m_typeId
                    && CleanTraits(left->m_traits) == CleanTraits(right->m_traits));
        }
    };
    
    using TypeSet = AZStd::unordered_set<const BehaviorParameter*, TypeHasher, TypeEqual>;

    AZ::ExplicitOverloadInfo GetExplicitOverloads(AZ::ExplicitOverloadInfo& explicitOverloadInfo)
    {
        using namespace AZ;
        BehaviorContext* behaviorContext = nullptr;
        ComponentApplicationBus::BroadcastResult(behaviorContext, &ComponentApplicationRequests::GetBehaviorContext);

        if (behaviorContext)
        {
            auto iter = behaviorContext->m_explicitOverloads.find(explicitOverloadInfo);
            if (iter != behaviorContext->m_explicitOverloads.end())
            {
                explicitOverloadInfo = *iter;
            }
        }
        else
        {
            AZ_Warning("Script Canvas", false, "BehaviorContext is required!");
        }

        return explicitOverloadInfo;
    }
}

namespace AZ
{
    using namespace BehaviorContextUtilitiesCPP;

    AZ::BehaviorContext* GetDefaultBehaviorContext()
    {
        AZ::BehaviorContext* behaviorContext(nullptr);
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        AZ_Assert(behaviorContext, "BehaviorContext is required");
        return behaviorContext;
    }

    ExplicitOverloadInfo GetExplicitOverloads(const AZStd::string& name)
    {
        AZ::ExplicitOverloadInfo explicitOverloadInfo;
        explicitOverloadInfo.m_name = name;
        return BehaviorContextUtilitiesCPP::GetExplicitOverloads(explicitOverloadInfo);
    }

    ExplicitOverloadInfo GetExplicitOverloads(const BehaviorMethod& method)
    {
        ExplicitOverloadInfo explicitOverloadInfo;
        if (Attribute* attribute = FindAttribute(ScriptCanvasAttributes::ExplicitOverloadCrc, method.m_attributes))
        {
            AttributeReader(nullptr, attribute).Read<ExplicitOverloadInfo>(explicitOverloadInfo);
        }
        return BehaviorContextUtilitiesCPP::GetExplicitOverloads(explicitOverloadInfo);      
    }

    AZStd::vector<AZStd::pair<const BehaviorMethod*, const BehaviorClass*>> OverloadsToVector(const BehaviorMethod& method, const BehaviorClass* behaviorClass)
    {
        AZStd::vector<AZStd::pair<const BehaviorMethod*, const BehaviorClass*>> overloads;
        const ExplicitOverloadInfo explicitOverloads = GetExplicitOverloads(method);

        if (!explicitOverloads.m_overloads.empty())
        {
            for (const auto& methodAndClass : explicitOverloads.m_overloads)
            {
                overloads.emplace_back(methodAndClass.first, methodAndClass.second);
            }
        }
        else
        {
            auto overload = &method;

            do
            {
                overloads.emplace_back(overload, behaviorClass);
                overload = overload->m_overload;
            }
            while (overload);
        }

        return overloads;
    }

    AZStd::string GetOverloadName(const BehaviorMethod& overload, size_t overloadIndex, const OverloadVariance& variance, AZStd::string nameOverride)
    {
        AZStd::string overloadName = nameOverride.empty() ? overload.m_name : nameOverride;

        for (size_t argIndex = 0, argSentinel = overload.GetNumArguments(); argIndex < argSentinel; ++argIndex)
        {
            auto overloadedArgIter = variance.m_input.find(argIndex);
            if (overloadedArgIter != variance.m_input.end() && overloadedArgIter->second[overloadIndex])
            {
                // if this doesn't work try the type name
                overloadName += ReplaceCppArtifacts(overloadedArgIter->second[overloadIndex]->m_name);
            }
        }

        AZ_Warning("BehaviorConvext", overloadName != overload.m_name, "overload naming failure");
        return overloadName;
    }

    AZStd::string GetOverloadName(const BehaviorMethod& method, size_t overloadIndex, AZStd::string nameOverride)
    {
        auto names = GetOverloadNames(method, nameOverride);
        return overloadIndex < names.size() ? names[overloadIndex] : nameOverride;
    }
    
    AZStd::vector<AZStd::string> GetOverloadNames(const BehaviorMethod& method, AZStd::string nameOverride)
    {
        AZStd::vector<AZStd::string> names;
        
        auto overloads = OverloadsToVector(method, nullptr);
        auto variance = GetOverloadVariance(method, overloads, VariantOnThis::Yes);
        
        for (size_t overloadIndex(0), sentinel = overloads.size(); overloadIndex < sentinel; ++overloadIndex)
        {
            names.push_back(GetOverloadName(method, overloadIndex, variance, nameOverride));
        }

        return names;
    }

    OverloadVariance GetOverloadVariance(const BehaviorMethod& method, const AZStd::vector<AZStd::pair<const BehaviorMethod*, const BehaviorClass*>>& overloads, VariantOnThis onThis)
    {
        OverloadVariance variance;

        // input 0, possible this pointer
        if (method.GetNumArguments() > 0)
        {
            TypeSet types;
            AZStd::vector<const BehaviorParameter*> stripedArgs;

            bool oneArgIsThisPointer = false;

            for (size_t overloadIndex = 0, overloadSentinel = overloads.size(); overloadIndex < overloadSentinel; ++overloadIndex)
            {
                auto argument = overloads[overloadIndex].first->GetArgument(0);

                if (argument)
                {
                    const bool isThisPointer
                        = (argument->m_traits & AZ::BehaviorParameter::Traits::TR_THIS_PTR) != 0
                        || AZ::FindAttribute(AZ::Script::Attributes::TreatAsMemberFunction, overloads[overloadIndex].first->m_attributes);

                    oneArgIsThisPointer = oneArgIsThisPointer || isThisPointer;
                }

                types.insert(argument);
                stripedArgs.emplace_back(argument);
            }

            if (types.size() == overloads.size())
            {
                variance.m_unambiguousInput.insert(0);
            }

            if (types.size() > 1 && (onThis == VariantOnThis::Yes || !oneArgIsThisPointer))
            {
                variance.m_input.insert(AZStd::make_pair(0, stripedArgs));
            }
        }

        // input 1..N
        for (size_t argIndex = 1, argSentinel = method.GetNumArguments(); argIndex < argSentinel; ++argIndex)
        {
            TypeSet types;
            AZStd::vector<const BehaviorParameter*> stripedArgs;

            for (size_t overloadIndex = 0, overloadSentinel = overloads.size(); overloadIndex < overloadSentinel; ++overloadIndex)
            {
                auto argument = overloads[overloadIndex].first->GetArgument(argIndex);
                types.insert(argument);
                stripedArgs.emplace_back(argument);
            }

            if (types.size() == overloads.size())
            {
                variance.m_unambiguousInput.insert(0);
            }

            if (types.size() > 1)
            {
                variance.m_input.insert(AZStd::make_pair(argIndex, stripedArgs));
            }
        }

        // output
        if (overloads[0].first->HasResult())
        {
            TypeSet types;
            AZStd::vector<const BehaviorParameter*> returnValues;

            for (size_t overloadIndex = 0, overloadSentinel = overloads.size(); overloadIndex < overloadSentinel; ++overloadIndex)
            {
                auto result = overloads[overloadIndex].first->GetResult();
                types.insert(result);
                returnValues.emplace_back(result);
            }

            if (types.size() > 1)
            {
                variance.m_output = returnValues;
            }
        }

        return variance;
    }

    void RemovePropertyGetterNameArtifacts(AZStd::string& name)
    {
        if (name.ends_with(k_PropertyNameGetterSuffix))
        {
            AZ::StringFunc::Replace(name, k_PropertyNameGetterSuffix, "");
        }
    }

    void RemovePropertySetterNameArtifacts(AZStd::string& name)
    {
        if (name.ends_with(k_PropertyNameSetterSuffix))
        {
            AZ::StringFunc::Replace(name, k_PropertyNameSetterSuffix, "");
        }
    }

    void RemovePropertyNameArtifacts(AZStd::string& name)
    {
        RemovePropertyGetterNameArtifacts(name);
        RemovePropertySetterNameArtifacts(name);
    }

    AZStd::string ReplaceCppArtifacts(AZStd::string_view sourceName)
    {
        using namespace AZ::StringFunc;

        AZStd::string newName = sourceName;


        // remove common things like "allocator" and "AZStd::"
        Replace(newName, "AZStd::", "", true);
        Replace(newName, "allocator", "", true);

        // deal with *,&,<,>,etc.
        Replace(newName, "*", "Ptr", true);
        //Replace(newName, "&&", "Rval", true); we don't support RValues in script
        Replace(newName, "&", "Ref", true);
        Replace(newName, "<", "_", true); // template open
        Replace(newName, ">", "", true); // template close
        Replace(newName, ",", "_", true); // template or function parameters
        Replace(newName, "::", "_", true); // namespace

        // strip all other non alphanumeric or '_' characters
        AZStd::erase_if(newName, [](AZStd::string::value_type ch) {
            return !AZStd::is_alnum(ch) && ch != '_';  // allow alpha numeric and '_'
            });

        if (newName != sourceName)
        {
            // remove trailing "_"
            while (!newName.empty() && newName.at(newName.length() - 1) == '_')
            {
                newName.pop_back();
            }
        }
        return newName;
    }

    void StripQualifiers(AZStd::string& name)
    {
        using namespace AZ::StringFunc;

        Replace(name, "&", " ", true);
        Replace(name, "*", " ", true);
        Replace(name, "const ", "", true);
        Replace(name, " ", "", true);
    }
    
    bool TypeCompare(const BehaviorParameter& a, const BehaviorParameter& b)
    {
        return BehaviorContextHelper::IsStringParameter(a) && BehaviorContextHelper::IsStringParameter(b)
            || (CleanTraits(a.m_traits) == CleanTraits(b.m_traits) && a.m_typeId == b.m_typeId);
    }

}
