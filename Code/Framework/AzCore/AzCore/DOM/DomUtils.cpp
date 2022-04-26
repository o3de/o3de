/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomUtils.h>

#include <AzCore/IO/ByteContainerStream.h>

namespace AZ::Dom::Utils
{
    Visitor::Result ReadFromString(Backend& backend, AZStd::string_view string, AZ::Dom::Lifetime lifetime, Visitor& visitor)
    {
        return backend.ReadFromBuffer(string.data(), string.length(), lifetime, visitor);
    }

    Visitor::Result ReadFromStringInPlace(Backend& backend, AZStd::string& string, Visitor& visitor)
    {
        return backend.ReadFromBufferInPlace(string.data(), string.size(), visitor);
    }

    AZ::Outcome<Value, AZStd::string> SerializedStringToValue(
        Backend& backend, AZStd::string_view string, AZ::Dom::Lifetime lifetime)
    {
        return WriteToValue(
            [&](Visitor& visitor)
            {
                return backend.ReadFromBuffer(string.data(), string.size(), lifetime, visitor);
            });
    }

    AZ::Outcome<void, AZStd::string> ValueToSerializedString(Backend& backend, Dom::Value value, AZStd::string& buffer)
    {
        Dom::Visitor::Result result = backend.WriteToBuffer(
            buffer,
            [&](Visitor& visitor)
            {
                return value.Accept(visitor, false);
            });
        if (!result.IsSuccess())
        {
            return AZ::Failure(result.GetError().FormatVisitorErrorMessage());
        }
        return AZ::Success();
    }

    AZ::Outcome<Value, AZStd::string> WriteToValue(const Backend::WriteCallback& writeCallback)
    {
        Value value;
        AZStd::unique_ptr<Visitor> writer = value.GetWriteHandler();
        Visitor::Result result = writeCallback(*writer);
        if (!result.IsSuccess())
        {
            return AZ::Failure(result.GetError().FormatVisitorErrorMessage());
        }
        return AZ::Success(AZStd::move(value));
    }

    bool DeepCompareIsEqual(const Value& lhs, const Value& rhs)
    {
        const Value::ValueType& lhsValue = lhs.GetInternalValue();
        const Value::ValueType& rhsValue = rhs.GetInternalValue();

        if (lhs.IsString() && rhs.IsString())
        {
            // If we both hold the same ref counted string we don't need to do a full comparison
            if (AZStd::holds_alternative<Value::SharedStringType>(lhsValue) && lhsValue == rhsValue)
            {
                return true;
            }
            return lhs.GetString() == rhs.GetString();
        }

        return AZStd::visit(
            [&](auto&& ourValue) -> bool
            {
                using Alternative = AZStd::decay_t<decltype(ourValue)>;

                if constexpr (AZStd::is_same_v<Alternative, ObjectPtr>)
                {
                    if (!rhs.IsObject())
                    {
                        return false;
                    }
                    auto&& theirValue = AZStd::get<AZStd::remove_cvref_t<decltype(ourValue)>>(rhsValue);
                    if (ourValue == theirValue)
                    {
                        return true;
                    }

                    const Object::ContainerType& ourValues = ourValue->GetValues();
                    const Object::ContainerType& theirValues = theirValue->GetValues();

                    if (ourValues.size() != theirValues.size())
                    {
                        return false;
                    }

                    for (size_t i = 0; i < ourValues.size(); ++i)
                    {
                        const Object::EntryType& lhsChild = ourValues[i];
                        auto rhsIt = rhs.FindMember(lhsChild.first);
                        if (rhsIt == rhs.MemberEnd() || !DeepCompareIsEqual(lhsChild.second, rhsIt->second))
                        {
                            return false;
                        }
                    }

                    return true;
                }
                else if constexpr (AZStd::is_same_v<Alternative, ArrayPtr>)
                {
                    if (!rhs.IsArray())
                    {
                        return false;
                    }
                    auto&& theirValue = AZStd::get<AZStd::remove_cvref_t<decltype(ourValue)>>(rhsValue);
                    if (ourValue == theirValue)
                    {
                        return true;
                    }

                    const Array::ContainerType& ourValues = ourValue->GetValues();
                    const Array::ContainerType& theirValues = theirValue->GetValues();

                    if (ourValues.size() != theirValues.size())
                    {
                        return false;
                    }

                    for (size_t i = 0; i < ourValues.size(); ++i)
                    {
                        const Value& lhsChild = ourValues[i];
                        const Value& rhsChild = theirValues[i];
                        if (!DeepCompareIsEqual(lhsChild, rhsChild))
                        {
                            return false;
                        }
                    }

                    return true;
                }
                else if constexpr (AZStd::is_same_v<Alternative, NodePtr>)
                {
                    if (!rhs.IsNode())
                    {
                        return false;
                    }
                    auto&& theirValue = AZStd::get<AZStd::remove_cvref_t<decltype(ourValue)>>(rhsValue);
                    if (ourValue == theirValue)
                    {
                        return true;
                    }

                    const Node& ourNode = *ourValue;
                    const Node& theirNode = *theirValue;

                    const Object::ContainerType& ourProperties = ourNode.GetProperties();
                    const Object::ContainerType& theirProperties = theirNode.GetProperties();

                    if (ourProperties.size() != theirProperties.size())
                    {
                        return false;
                    }

                    for (size_t i = 0; i < ourProperties.size(); ++i)
                    {
                        const Object::EntryType& lhsChild = ourProperties[i];
                        auto rhsIt = rhs.FindMember(lhsChild.first);
                        if (rhsIt == rhs.MemberEnd() || !DeepCompareIsEqual(lhsChild.second, rhsIt->second))
                        {
                            return false;
                        }
                    }

                    const Array::ContainerType& ourChildren = ourNode.GetChildren();
                    const Array::ContainerType& theirChildren = theirNode.GetChildren();

                    for (size_t i = 0; i < ourChildren.size(); ++i)
                    {
                        const Value& lhsChild = ourChildren[i];
                        const Value& rhsChild = theirChildren[i];
                        if (!DeepCompareIsEqual(lhsChild, rhsChild))
                        {
                            return false;
                        }
                    }

                    return true;
                }
                else
                {
                    return lhs == rhs;
                }
            },
            lhsValue);
    }

    Value DeepCopy(const Value& value, bool copyStrings)
    {
        Value copiedValue;
        AZStd::unique_ptr<Visitor> writer = copiedValue.GetWriteHandler();
        value.Accept(*writer, copyStrings);
        return copiedValue;
    }
} // namespace AZ::Dom::Utils
