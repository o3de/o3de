/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomValue.h>
#include <AzCore/std/containers/stack.h>

namespace AZ::Dom
{
    //! Visitor that writes to a Value.
    //! Supports all Visitor operations.
    class ValueWriter : public Visitor
    {
    public:
        ValueWriter(Value& outputValue);

        VisitorFlags GetVisitorFlags() const override;
        Result Null() override;
        Result Bool(bool value) override;
        Result Int64(AZ::s64 value) override;
        Result Uint64(AZ::u64 value) override;
        Result Double(double value) override;

        Result String(AZStd::string_view value, Lifetime lifetime) override;
        Result RefCountedString(AZStd::shared_ptr<const AZStd::string> value, Lifetime lifetime) override;
        Result StartObject() override;
        Result EndObject(AZ::u64 attributeCount) override;
        Result Key(AZ::Name key) override;
        Result RawKey(AZStd::string_view key, Lifetime lifetime) override;
        Result StartArray() override;
        Result EndArray(AZ::u64 elementCount) override;
        Result StartNode(AZ::Name name) override;
        Result RawStartNode(AZStd::string_view name, Lifetime lifetime) override;
        Result EndNode(AZ::u64 attributeCount, AZ::u64 elementCount) override;
        Result OpaqueValue(OpaqueType& value) override;

    private:
        Result FinishWrite();
        Value& CurrentValue();
        Visitor::Result EndContainer(Type containerType, AZ::u64 attributeCount, AZ::u64 elementCount);

        struct ValueInfo
        {
            ValueInfo(Value& container);

            KeyType m_key;
            Value m_value;
            Value& m_container;
        };

        struct ValueBuffer
        {
            Array::ContainerType m_elements;
            Object::ContainerType m_attributes;
        };

        ValueBuffer& GetValueBuffer();

        Value& m_result;
        // Stores info about the current value being processed
        AZStd::stack<ValueInfo, AZStd::deque<ValueInfo, AZStdAlloc<ValueAllocator>>> m_entryStack;
        // Provides temporary storage for elements and attributes to prevent extra heap allocations
        // These buffers persist to be reused even as the entry stack changes
        AZStd::vector<ValueBuffer, AZStdAlloc<ValueAllocator>> m_valueBuffers;
    };
} // namespace AZ::Dom
