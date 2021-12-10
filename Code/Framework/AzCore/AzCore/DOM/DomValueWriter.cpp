/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomValueWriter.h>

namespace AZ::Dom
{
    ValueWriter::ValueWriter(Value& outputValue)
        : m_result(outputValue)
    {
    }

    VisitorFlags ValueWriter::GetVisitorFlags() const
    {
        return VisitorFlags::SupportsRawKeys | VisitorFlags::SupportsArrays | VisitorFlags::SupportsObjects | VisitorFlags::SupportsNodes;
    }

    ValueWriter::ValueInfo::ValueInfo(Value& container)
        : m_container(container)
    {
    }

    Visitor::Result ValueWriter::Null()
    {
        CurrentValue().SetNull();
        return FinishWrite();
    }

    Visitor::Result ValueWriter::Bool(bool value)
    {
        CurrentValue().SetBool(value);
        return FinishWrite();
    }

    Visitor::Result ValueWriter::Int64(AZ::s64 value)
    {
        CurrentValue().SetInt64(value);
        return FinishWrite();
    }

    Visitor::Result ValueWriter::Uint64(AZ::u64 value)
    {
        CurrentValue().SetUint64(value);
        return FinishWrite();
    }

    Visitor::Result ValueWriter::Double(double value)
    {
        CurrentValue().SetDouble(value);
        return FinishWrite();
    }

    Visitor::Result ValueWriter::String(AZStd::string_view value, Lifetime lifetime)
    {
        if (lifetime == Lifetime::Persistent)
        {
            CurrentValue().SetString(value);
        }
        else
        {
            CurrentValue().CopyFromString(value);
        }
        return FinishWrite();
    }

    Visitor::Result ValueWriter::StartObject()
    {
        CurrentValue().SetObject();

        m_entryStack.emplace(CurrentValue());
        return VisitorSuccess();
    }

    Visitor::Result ValueWriter::EndContainer(Type containerType, AZ::u64 attributeCount, AZ::u64 elementCount)
    {
        const char* endMethodName;
        switch (containerType)
        {
        case Type::ObjectType:
            endMethodName = "EndObject";
            break;
        case Type::ArrayType:
            endMethodName = "EndArray";
            break;
        case Type::NodeType:
            endMethodName = "EndNode";
            break;
        default:
            AZ_Assert(false, "Invalid container type specified");
            return VisitorFailure(VisitorErrorCode::InternalError, "AZ::Dom::ValueWriter: EndContainer called with invalid container type");
        }

        if (m_entryStack.empty())
        {
            return VisitorFailure(
                VisitorErrorCode::InternalError,
                AZStd::string::format("AZ::Dom::ValueWriter: %s called without a matching call", endMethodName));
        }

        const ValueInfo& topEntry = m_entryStack.top();
        if (topEntry.m_container.GetType() != containerType)
        {
            return VisitorFailure(
                VisitorErrorCode::InternalError,
                AZStd::string::format("AZ::Dom::ValueWriter: %s called from within a different container type", endMethodName));
        }

        if (topEntry.m_attributeCount != attributeCount)
        {
            return VisitorFailure(
                VisitorErrorCode::InternalError,
                AZStd::string::format(
                    "AZ::Dom::ValueWriter: %s expected %llu attributes but received %llu attributes instead", endMethodName, attributeCount,
                    topEntry.m_attributeCount));
        }

        if (topEntry.m_elementCount != elementCount)
        {
            return VisitorFailure(
                VisitorErrorCode::InternalError,
                AZStd::string::format(
                    "AZ::Dom::ValueWriter: %s expected %llu elements but received %llu elements instead", endMethodName, elementCount,
                    topEntry.m_elementCount));
        }

        m_entryStack.pop();
        return FinishWrite();
    }

    Visitor::Result ValueWriter::EndObject(AZ::u64 attributeCount)
    {
        return EndContainer(Type::ObjectType, attributeCount, 0);
    }

    Visitor::Result ValueWriter::Key(AZ::Name key)
    {
        AZ_Assert(!m_entryStack.empty(), "Attempmted to push a key with no object");
        AZ_Assert(!m_entryStack.top().m_container.IsArray(), "Attempted to push a key to an array");
        m_entryStack.top().m_key = key;
        return VisitorSuccess();
    }

    Visitor::Result ValueWriter::RawKey(AZStd::string_view key, [[maybe_unused]] Lifetime lifetime)
    {
        return Key(AZ::Name(key));
    }

    Visitor::Result ValueWriter::StartArray()
    {
        CurrentValue().SetArray();

        m_entryStack.emplace(CurrentValue());
        return VisitorSuccess();
    }

    Visitor::Result ValueWriter::EndArray(AZ::u64 elementCount)
    {
        return EndContainer(Type::ArrayType, 0, elementCount);
    }

    Visitor::Result ValueWriter::StartNode(AZ::Name name)
    {
        CurrentValue().SetNode(name);

        m_entryStack.emplace(CurrentValue());
        return VisitorSuccess();
    }

    Visitor::Result ValueWriter::RawStartNode(AZStd::string_view name, [[maybe_unused]] Lifetime lifetime)
    {
        return StartNode(AZ::Name(name));
    }

    Visitor::Result ValueWriter::EndNode(AZ::u64 attributeCount, AZ::u64 elementCount)
    {
        return EndContainer(Type::NodeType, attributeCount, elementCount);
    }

    Visitor::Result ValueWriter::FinishWrite()
    {
        if (m_entryStack.empty())
        {
            return VisitorSuccess();
        }

        Value value;
        m_entryStack.top().m_value.Swap(value);
        ValueInfo& newEntry = m_entryStack.top();

        if (!newEntry.m_key.IsEmpty())
        {
            newEntry.m_container.AddMember(newEntry.m_key, AZStd::move(value));
            newEntry.m_key = AZ::Name();
            ++newEntry.m_attributeCount;
        }
        else
        {
            newEntry.m_container.PushBack(AZStd::move(value));
            ++newEntry.m_elementCount;
        }

        return VisitorSuccess();
    }

    Value& ValueWriter::CurrentValue()
    {
        if (m_entryStack.empty())
        {
            return m_result;
        }
        return m_entryStack.top().m_value;
    }
} // namespace AZ::Dom
