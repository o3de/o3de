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
        Result StartObject() override;
        Result EndObject(AZ::u64 attributeCount) override;
        Result Key(AZ::Name key) override;
        Result RawKey(AZStd::string_view key, Lifetime lifetime) override;
        Result StartArray() override;
        Result EndArray(AZ::u64 elementCount) override;
        Result StartNode(AZ::Name name) override;
        Result RawStartNode(AZStd::string_view name, Lifetime lifetime) override;
        Result EndNode(AZ::u64 attributeCount, AZ::u64 elementCount) override;

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
            AZStd::vector<Value> m_elements;
            AZStd::vector<AZStd::pair<AZ::Name, Value>> m_attributes;
        };

        ValueBuffer& GetValueBuffer();

        Value& m_result;
        AZStd::stack<ValueInfo> m_entryStack;
        AZStd::vector<ValueBuffer> m_valueBuffers;
    };
} // namespace AZ::Dom
