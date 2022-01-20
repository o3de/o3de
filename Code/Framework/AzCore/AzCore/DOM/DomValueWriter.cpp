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

    Visitor::Result ValueWriter::RefCountedString(AZStd::shared_ptr<const AZStd::vector<char>> value, [[maybe_unused]] Lifetime lifetime)
    {
        CurrentValue().SetString(AZStd::move(value));
        return FinishWrite();
    }

    Visitor::Result ValueWriter::StartObject()
    {
        CurrentValue().SetObject();

        m_entryStack.emplace(CurrentValue());
        return VisitorSuccess();
    }

    template <class T, class A>
    void MoveVectorMemory(AZStd::vector<T, A>& dest, AZStd::vector<T, A>& source)
    {
        dest.resize_no_construct(source.size());
        const size_t size = sizeof(T) * source.size();
        memcpy(dest.data(), source.data(), size);
        memset(source.data(), 0, size);
        source.resize(0);
    }

    Visitor::Result ValueWriter::EndContainer(Type containerType, AZ::u64 attributeCount, AZ::u64 elementCount)
    {
        const char* endMethodName;
        switch (containerType)
        {
        case Type::Object:
            endMethodName = "EndObject";
            break;
        case Type::Array:
            endMethodName = "EndArray";
            break;
        case Type::Node:
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
        Value& container = topEntry.m_container;
        ValueBuffer& buffer = GetValueBuffer();

        if (container.GetType() != containerType)
        {
            return VisitorFailure(
                VisitorErrorCode::InternalError,
                AZStd::string::format("AZ::Dom::ValueWriter: %s called from within a different container type", endMethodName));
        }

        if (aznumeric_cast<AZ::u64>(buffer.m_attributes.size()) != attributeCount)
        {
            return VisitorFailure(
                VisitorErrorCode::InternalError,
                AZStd::string::format(
                    "AZ::Dom::ValueWriter: %s expected %llu attributes but received %zu attributes instead", endMethodName, attributeCount,
                    buffer.m_attributes.size()));
        }

        if (aznumeric_cast<AZ::u64>(buffer.m_elements.size()) != elementCount)
        {
            return VisitorFailure(
                VisitorErrorCode::InternalError,
                AZStd::string::format(
                    "AZ::Dom::ValueWriter: %s expected %llu elements but received %zu elements instead", endMethodName, elementCount,
                    buffer.m_elements.size()));
        }

        if (buffer.m_attributes.size() > 0)
        {
            MoveVectorMemory(container.GetMutableObject(), buffer.m_attributes);
        }

        if(buffer.m_elements.size() > 0)
        {
            MoveVectorMemory(container.GetMutableArray(), buffer.m_elements);
        }

        m_entryStack.pop();
        return FinishWrite();
    }

    ValueWriter::ValueBuffer& ValueWriter::GetValueBuffer()
    {
        if (m_entryStack.size() <= m_valueBuffers.size())
        {
            return m_valueBuffers[m_entryStack.size() - 1];
        }

        m_valueBuffers.resize(m_entryStack.size());
        return m_valueBuffers[m_entryStack.size() - 1];
    }

    Visitor::Result ValueWriter::EndObject(AZ::u64 attributeCount)
    {
        return EndContainer(Type::Object, attributeCount, 0);
    }

    Visitor::Result ValueWriter::Key(AZ::Name key)
    {
        AZ_Assert(!m_entryStack.empty(), "Attempmted to push a key with no object");
        AZ_Assert(!m_entryStack.top().m_container.IsArray(), "Attempted to push a key to an array");
        m_entryStack.top().m_key = AZStd::move(key);
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
        return EndContainer(Type::Array, 0, elementCount);
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
        return EndContainer(Type::Node, attributeCount, elementCount);
    }

    Visitor::Result ValueWriter::OpaqueValue(OpaqueType& value)
    {
        CurrentValue().SetOpaqueValue(value);
        return FinishWrite();
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
            GetValueBuffer().m_attributes.emplace_back(AZStd::move(newEntry.m_key), AZStd::move(value));
            newEntry.m_key = AZ::Name();
        }
        else
        {
            GetValueBuffer().m_elements.emplace_back(AZStd::move(value));
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
