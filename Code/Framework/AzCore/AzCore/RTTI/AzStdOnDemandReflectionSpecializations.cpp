/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Math/Uuid.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/string.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <AzCore/RTTI/AzStdOnDemandReflection.inl>
#include <AzCore/RTTI/AzStdOnDemandReflectionLuaFunctions.inl>

namespace AZ::CommonOnDemandReflections
{
    template<class T>
    void ReflectCommonStringAPI(ReflectContext* context)
    {
        using ContainerType = T;
        using SizeType = typename ContainerType::size_type;
        using ValueType = typename ContainerType::value_type;
        if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
        {
            behaviorContext->Class<ContainerType>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->template Constructor<typename ContainerType::value_type*>()
                    ->Attribute(AZ::Script::Attributes::ConstructorOverride, &OnDemandLuaFunctions::ConstructBasicString<ContainerType>)
                    ->Attribute(AZ::Script::Attributes::ReaderWriterOverride, ScriptContext::CustomReaderWriter(&OnDemandLuaFunctions::StringTypeToLua<ContainerType>, &OnDemandLuaFunctions::StringTypeFromLua<ContainerType>))
                ->template WrappingMember<const char*>(&ContainerType::c_str)
                ->Method("c_str", &ContainerType::c_str)
                ->Method("Length", [](ContainerType* thisPtr) { return aznumeric_cast<int>(thisPtr->length()); })
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Length)
                ->Method("Equal", [](const ContainerType& lhs, const ContainerType& rhs)
                {
                    return lhs == rhs;
                })
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)
                ->Method("Find", [](ContainerType* thisPtr, const ContainerType& stringToFind, const int& startPos)
                {
                    return aznumeric_cast<int>(thisPtr->find(stringToFind, startPos));
                })
                ->Method("Substring", [](ContainerType* thisPtr, const int& pos, const int& len)
                {
                    return thisPtr->substr(pos, len);
                })
                ->Method("Replace", [](ContainerType* thisPtr, const ContainerType& stringToReplace, const ContainerType& replacementString)
                {
                    SizeType startPos = 0;
                    while ((startPos = thisPtr->find(stringToReplace, startPos)) != ContainerType::npos && !stringToReplace.empty())
                    {
                        thisPtr->replace(startPos, stringToReplace.length(), replacementString);
                        startPos += replacementString.length();
                    }

                    return *thisPtr;
                })
                ->Method("ReplaceByIndex", [](ContainerType* thisPtr, const int& beginIndex, const int& endIndex, const ContainerType& replacementString)
                {
                    thisPtr->replace(beginIndex, endIndex - beginIndex + 1, replacementString);
                    return *thisPtr;
                })
                ->Method("Add", [](ContainerType* thisPtr, const ContainerType& addend)
                {
                    return *thisPtr + addend;
                })
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Concat)
                ->Method("TrimLeft", [](ContainerType* thisPtr)
                {
                    auto wsfront = AZStd::find_if_not(thisPtr->begin(), thisPtr->end(), [](char c) {return AZStd::is_space(c);});
                    thisPtr->erase(thisPtr->begin(), wsfront);
                    return *thisPtr;
                })
                ->Method("TrimRight", [](ContainerType* thisPtr)
                {
                    auto wsend = AZStd::find_if_not(thisPtr->rbegin(), thisPtr->rend(), [](char c) {return AZStd::is_space(c);});
                    thisPtr->erase(wsend.base(), thisPtr->end());
                    return *thisPtr;
                })
                ->Method("ToLower", [](ContainerType* thisPtr)
                {
                    ContainerType toLowerString;
                    for (auto itr = thisPtr->begin(); itr < thisPtr->end(); itr++)
                    {
                        toLowerString.push_back(static_cast<ValueType>(tolower(*itr)));
                    }
                    return toLowerString;
                })
                ->Method("ToUpper", [](ContainerType* thisPtr)
                {
                    ContainerType toUpperString;
                    for (auto itr = thisPtr->begin(); itr < thisPtr->end(); itr++)
                    {
                        toUpperString.push_back(static_cast<ValueType>(toupper(*itr)));
                    }
                    return toUpperString;
                })
                ->Method("Join", [](AZStd::vector<ContainerType>* stringsToJoinPtr, const ContainerType& joinStr)
                {
                    ContainerType joinString;
                    AZ::StringFunc::Join(joinString, *stringsToJoinPtr, joinStr);
                    return joinString;
                })
                ->Method("Split", [](ContainerType* thisPtr, const ContainerType& splitter)
                {
                    AZStd::vector<ContainerType> splitStringList;
                    auto SplitString = [&splitStringList](AZStd::string_view token)
                    {
                        splitStringList.emplace_back(token);
                    };
                    AZ::StringFunc::TokenizeVisitor(*thisPtr, SplitString, splitter);
                    return splitStringList;
                });
        }
    }

    void ReflectCommonString(ReflectContext* context)
    {
        ReflectCommonStringAPI<AZStd::string>(context);
    }
    void ReflectCommonFixedString(ReflectContext* context)
    {
        ReflectCommonStringAPI<AZ::IO::FixedMaxPathString>(context);
    }

    void ReflectCommonStringView(ReflectContext* context)
    {
        using ContainerType = AZStd::string_view;

        if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
        {
            behaviorContext->Class<ContainerType>()
                ->Attribute(AZ::Script::Attributes::Category, "Core")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->template Constructor<typename ContainerType::value_type*>()
                ->Attribute(AZ::Script::Attributes::ConstructorOverride, &OnDemandLuaFunctions::ConstructStringView<ContainerType::value_type, ContainerType::traits_type>)
                ->Attribute(AZ::Script::Attributes::ReaderWriterOverride, ScriptContext::CustomReaderWriter(&OnDemandLuaFunctions::StringTypeToLua<ContainerType>, &OnDemandLuaFunctions::StringTypeFromLua<ContainerType>))
                ->Method("ToString", [](const ContainerType& stringView) { return stringView.data(); }, { { { "Reference", "String view object being converted to string" } } })
                ->Attribute(AZ::Script::Attributes::ToolTip, "Converts string_view to string")
                ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ->template WrappingMember<const char*>(&ContainerType::data)
                ->Method("data", &ContainerType::data)
                ->Attribute(AZ::Script::Attributes::ToolTip, "Returns reference to raw string data")
                ->Method("length", [](ContainerType* thisPtr) { return aznumeric_cast<int>(thisPtr->length()); }, { { { "This", "Reference to the object the method is being performed on" } } })
                ->Attribute(AZ::Script::Attributes::ToolTip, "Returns length of string view")
                ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Length)
                ->Method("size", [](ContainerType* thisPtr) { return aznumeric_cast<int>(thisPtr->size()); }, { { { "This", "Reference to the object the method is being performed on" }} })
                ->Attribute(AZ::Script::Attributes::ToolTip, "Returns length of string view")
                ->Method("find", [](ContainerType* thisPtr, ContainerType stringToFind, int startPos)
                {
                    return aznumeric_cast<int>(thisPtr->find(stringToFind, startPos));
                }, { { { "This", "Reference to the object the method is being performed on" }, { "View", "View to search " }, { "Position", "Index in view to start search" }} })
                ->Attribute(AZ::Script::Attributes::ToolTip, "Searches for supplied string within this string")
                ->Method("substr", [](ContainerType* thisPtr, int pos, int len)
                {
                    return thisPtr->substr(pos, len);
                }, { {{"This", "Reference to the object the method is being performed on"}, {"Position", "Index in view that indicates the beginning of the sub string"}, {"Count", "Length of characters that sub string view occupies" }} })
                ->Attribute(AZ::Script::Attributes::ToolTip, "Creates a sub view of this string view. The string data is not actually modified")
                ->Method("remove_prefix", [](ContainerType* thisPtr, int n) {thisPtr->remove_prefix(n); },
                { { { "This", "Reference to the object the method is being performed on" }, { "Count", "Number of characters to remove from start of view" }} })
                ->Attribute(AZ::Script::Attributes::ToolTip, "Moves the supplied number of characters from the beginning of this sub view")
                ->Method("remove_suffix", [](ContainerType* thisPtr, int n) {thisPtr->remove_suffix(n); },
                { { { "This", "Reference to the object the method is being performed on" } ,{ "Count", "Number of characters to remove from end of view" }} })
                ->Attribute(AZ::Script::Attributes::ToolTip, "Moves the supplied number of characters from the end of this sub view")
                ;
        }
    }


    void ReflectVoidOutcome(ReflectContext* context)
    {
        using OutcomeType = AZ::Outcome<void, void>;

        if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
        {
            // note we can reflect iterator types and support iterators, as of know we want to keep it simple
            behaviorContext->Class<OutcomeType>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::ScriptCanvasAttributes::AllowInternalCreation, true)
                ->Attribute(AZ::ScriptCanvasAttributes::PrettyName, &ScriptCanvasOnDemandReflection::OnDemandPrettyName<OutcomeType>::Get)
                ->Attribute(AZ::Script::Attributes::ToolTip, &ScriptCanvasOnDemandReflection::OnDemandToolTip<OutcomeType>::Get)
                ->Attribute(AZ::Script::Attributes::Category, &ScriptCanvasOnDemandReflection::OnDemandCategoryName<OutcomeType>::Get)
                ->Attribute(AZ::ScriptCanvasAttributes::AllowInternalCreation, AttributeIsValid::IfPresent)
                ->Attribute(AZ::ScriptCanvasAttributes::VariableCreationForbidden, AttributeIsValid::IfPresent)
                ->Method("Failure", []() -> OutcomeType { return AZ::Failure(); })
                ->Method("Success", []() -> OutcomeType { return AZ::Success(); })
                ->Method("IsSuccess", &OutcomeType::IsSuccess)
                ;
        }
    }

    void ReflectStdAny(ReflectContext* context)
    {
        if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
        {
            behaviorContext->Class<AZStd::any>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(
                    Script::Attributes::Ignore, true) // Don't reflect any type to script (there should never be an any instance in script)
                ->Attribute(
                    Script::Attributes::ReaderWriterOverride,
                    ScriptContext::CustomReaderWriter(&AZ::OnDemandLuaFunctions::AnyToLua, &OnDemandLuaFunctions::AnyFromLua));
        }
    }

} // namespace AZ
