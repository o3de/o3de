/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Utilities.h"

#include <AzFramework/StringFunc/StringFunc.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace String
        {
            const char* Split::k_defaultDelimiter = " ";
            const char* Join::k_defaultSeparator = " ";

            void ReplaceStringUtilityNodeOutputSlot(const Node* oldNode, const Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap)
            {
                auto newDataOutSlots = replacementNode->GetSlotsByType(ScriptCanvas::CombinedSlotType::DataOut);
                auto oldDataOutSlots = oldNode->GetSlotsByType(ScriptCanvas::CombinedSlotType::DataOut);
                if (newDataOutSlots.size() == oldDataOutSlots.size())
                {
                    for (size_t index = 0; index < newDataOutSlots.size(); index++)
                    {
                        outSlotIdMap.emplace(oldDataOutSlots[index]->GetId(), AZStd::vector<SlotId>{ newDataOutSlots[index]->GetId() });
                    }
                }
            }

            void StartsWith::OnInputSignal(const SlotId&)
            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::StartsWith::OnInputSignal");

                AZStd::string sourceString = StartsWithProperty::GetSource(this);
                AZStd::string patternString = StartsWithProperty::GetPattern(this);

                bool caseSensitive = StartsWithProperty::GetCaseSensitive(this);

                if (AzFramework::StringFunc::StartsWith(sourceString.c_str(), patternString.c_str(), caseSensitive))
                {
                    SignalOutput(GetSlotId("True"));
                }
                else
                {
                    SignalOutput(GetSlotId("False"));
                }
            }

            void EndsWith::OnInputSignal(const SlotId&)
            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::EndssWith::OnInputSignal");

                AZStd::string sourceString = EndsWithProperty::GetSource(this);
                AZStd::string patternString = EndsWithProperty::GetPattern(this);

                bool caseSensitive = EndsWithProperty::GetCaseSensitive(this);

                if (AzFramework::StringFunc::EndsWith(sourceString.c_str(), patternString.c_str(), caseSensitive))
                {
                    SignalOutput(GetSlotId("True"));
                }
                else
                {
                    SignalOutput(GetSlotId("False"));
                }
            }

            void Split::CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const
            {
                ReplaceStringUtilityNodeOutputSlot(this, replacementNode, outSlotIdMap);
            }

            void Split::OnInputSignal(const SlotId&)
            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Split::OnInputSignal");

                AZStd::string sourceString = SplitProperty::GetSource(this);
                AZStd::string delimiterString = SplitProperty::GetDelimiters(this);

                AZStd::vector<AZStd::string> stringArray;

                AzFramework::StringFunc::Tokenize(sourceString.c_str(), stringArray, delimiterString.c_str(), false, false);

                const SlotId outputResultSlotId = SplitProperty::GetStringArraySlotId(this);
                if (auto* outputSlot = GetSlot(outputResultSlotId))
                {
                    ScriptCanvas::Datum output(ScriptCanvas::Data::FromAZType(azrtti_typeid<AZStd::vector<AZStd::string>>()), ScriptCanvas::Datum::eOriginality::Original, &stringArray, azrtti_typeid<AZStd::vector<AZStd::string>>());
                    PushOutput(output, *outputSlot);
                }

                SignalOutput(GetSlotId("Out"));
            }

            void Join::CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const
            {
                ReplaceStringUtilityNodeOutputSlot(this, replacementNode, outSlotIdMap);
            }

            void Join::OnInputSignal(const SlotId&)
            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Join::OnInputSignal");

                AZStd::vector<AZStd::string> sourceArray = JoinProperty::GetStringArray(this);
                AZStd::string separatorString = JoinProperty::GetSeparator(this);

                AZStd::string result;
                size_t index = 0;
                const size_t length = sourceArray.size();
                for (auto string : sourceArray)
                {
                    if (index < length - 1)
                    {
                        result.append(AZStd::string::format("%s%s", string.c_str(), separatorString.c_str()));
                    }
                    else
                    {
                        result.append(string);
                    }
                    ++index;
                }

                const SlotId outputResultSlotId = JoinProperty::GetStringSlotId(this);
                if (auto* outputSlot = GetSlot(outputResultSlotId))
                {
                    ScriptCanvas::Datum output(result);
                    PushOutput(output, *outputSlot);
                }

                SignalOutput(GetSlotId("Out"));
            }

        }
    }
}
