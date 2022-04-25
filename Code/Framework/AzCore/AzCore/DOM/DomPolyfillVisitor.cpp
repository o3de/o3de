/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomPolyfillVisitor.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace AZ::Dom
{
    AZ::Name DomPolyfillVisitor::s_objectNodeName = AZ::Name::FromStringLiteral("o3de:Object");
    AZ::Name DomPolyfillVisitor::s_objectNodeKeyAttributeName = AZ::Name::FromStringLiteral("o3de:Key");
    AZ::Name DomPolyfillVisitor::s_arrayNodeName = AZ::Name::FromStringLiteral("o3de:Array");
    AZ::Name DomPolyfillVisitor::s_entryNodeName = AZ::Name::FromStringLiteral("o3de:Entry");

    DomPolyfillVisitor::DomPolyfillVisitor(AZStd::unique_ptr<Visitor>&& visitorToProxy)
        : m_proxiedVisitorOwningStorage(AZStd::move(visitorToProxy))
        , m_proxiedVisitor(m_proxiedVisitorOwningStorage.get())
    {
        Initialize();
    }

    DomPolyfillVisitor::DomPolyfillVisitor(Visitor& visitorToProxy)
        : m_proxiedVisitor(&visitorToProxy)
    {
        Initialize();
    }

    VisitorFlags DomPolyfillVisitor::GetVisitorFlags() const
    {
        return m_proxiedVisitor->GetVisitorFlags() | m_supportToPolyfill;
    }

    Visitor::Result DomPolyfillVisitor::Null()
    {
        return HandleValue(
            [&]()
            {
                return m_proxiedVisitor->Null();
            });
    }

    Visitor::Result DomPolyfillVisitor::Bool(bool value)
    {
        return HandleValue(
            [&]()
            {
                return m_proxiedVisitor->Bool(value);
            });
    }

    Visitor::Result DomPolyfillVisitor::Int64(AZ::s64 value)
    {
        return HandleValue(
            [&]()
            {
                return m_proxiedVisitor->Int64(value);
            });
    }

    Visitor::Result DomPolyfillVisitor::Uint64(AZ::u64 value)
    {
        return HandleValue(
            [&]()
            {
                return m_proxiedVisitor->Uint64(value);
            });
    }

    Visitor::Result DomPolyfillVisitor::Double(double value)
    {
        return HandleValue(
            [&]()
            {
                return m_proxiedVisitor->Double(value);
            });
    }

    Visitor::Result DomPolyfillVisitor::String(AZStd::string_view value, Lifetime lifetime)
    {
        if (!m_entryStack.empty())
        {
            EntryStackEntry& stack = m_entryStack.top();
            if (stack.m_nextStringIsKey)
            {
                stack.m_nextStringIsKey = false;
                return RawKey(value, lifetime);
            }
        }
        return HandleValue(
            [&]()
            {
                return m_proxiedVisitor->String(value, lifetime);
            });
    }

    Visitor::Result DomPolyfillVisitor::RawValue(AZStd::string_view value, Lifetime lifetime)
    {
        if (!m_entryStack.empty())
        {
            EntryStackEntry& stack = m_entryStack.top();
            if (stack.m_nextStringIsKey)
            {
                stack.m_nextStringIsKey = false;
                return RawKey(value, lifetime);
            }
        }
        if ((m_supportToPolyfill & VisitorFlags::SupportsRawValues) == VisitorFlags::Null)
        {
            return HandleValue(
                [&]()
                {
                    return m_proxiedVisitor->RawValue(value, lifetime);
                });
        }

        AZStd::cmatch matches;
        if (AZStd::regex_match(value.begin(), value.end(), m_quoteRegex))
        {
            return String(value.substr(1, value.length() - 2), lifetime);
        }
        else if (AZStd::regex_match(value.begin(), value.end(), m_trueRegex))
        {
            return Bool(true);
        }
        else if (AZStd::regex_match(value.begin(), value.end(), m_falseRegex))
        {
            return Bool(false);
        }
        else if (AZStd::regex_match(value.begin(), value.end(), m_nullRegex))
        {
            return Null();
        }
        else if (value.length() > 0 && value != "." && AZStd::regex_match(value.begin(), value.end(), matches, m_numberRegex))
        {
            if (matches[3].str().size() > 0)
            {
                return Double(strtod(value.data(), nullptr));
            }
            else if (matches[1].str().size() > 0)
            {
                return Int64(strtoll(value.data(), nullptr, 0));
            }
            else
            {
                return Uint64(strtoull(value.data(), nullptr, 0));
            }
        }
        return String(value, lifetime);
    }

    Visitor::Result DomPolyfillVisitor::StartObject()
    {
        Visitor::Result result = ValueBegin();
        if ((m_supportToPolyfill & VisitorFlags::SupportsObjects) == VisitorFlags::Null)
        {
            ResultCombine(result, m_proxiedVisitor->StartObject());
            return result;
        }
        m_nodeStack.push({});
        m_proxiedVisitor->StartNode(s_objectNodeName);
        return result;
    }

    Visitor::Result DomPolyfillVisitor::EndObject(AZ::u64 attributeCount)
    {
        if ((m_supportToPolyfill & VisitorFlags::SupportsObjects) == VisitorFlags::Null)
        {
            Visitor::Result result = m_proxiedVisitor->EndObject(attributeCount);
            ResultCombine(result, ValueEnd());
            return result;
        }
        Visitor::Result result = m_proxiedVisitor->EndNode(m_nodeStack.top().m_attributeSize, m_nodeStack.top().m_elementSize);
        m_nodeStack.pop();
        ResultCombine(result, ValueEnd());
        return result;
    }

    Visitor::Result DomPolyfillVisitor::Key(AZ::Name key)
    {
        if (!m_entryStack.empty())
        {
            EntryStackEntry& stack = m_entryStack.top();
            if (stack.m_node == s_arrayNodeName || stack.m_node == s_objectNodeName && key == s_objectNodeKeyAttributeName)
            {
                stack.m_nextStringIsKey = true;
            }
        }
        if (!m_nodeStack.empty())
        {
            m_nodeStack.top().m_key = key;
            return VisitorSuccess();
        }
        return m_proxiedVisitor->Key(key);
    }

    Visitor::Result DomPolyfillVisitor::RawKey(AZStd::string_view key, Lifetime lifetime)
    {
        if (!m_nodeStack.empty())
        {
            m_nodeStack.top().m_key = key;
            return VisitorSuccess();
        }
        if ((m_supportToPolyfill & VisitorFlags::SupportsRawKeys) == VisitorFlags::Null)
        {
            if (!m_entryStack.empty())
            {
                EntryStackEntry& stack = m_entryStack.top();
                if (stack.m_node == s_arrayNodeName || stack.m_node == s_objectNodeName && key == s_objectNodeKeyAttributeName.GetStringView())
                {
                    stack.m_nextStringIsKey = true;
                }
            }
            return m_proxiedVisitor->RawKey(key, lifetime);
        }
        if (AZStd::regex_match(key.begin(), key.end(), m_quoteRegex))
        {
            key = key.substr(1, key.length() - 2);
        }
        return Key(AZ::Name(key));
    }

    Visitor::Result DomPolyfillVisitor::StartArray()
    {
        Visitor::Result result = ValueBegin();
        if ((m_supportToPolyfill & VisitorFlags::SupportsArrays) == VisitorFlags::Null)
        {
            return m_proxiedVisitor->StartArray();
        }
        m_nodeStack.push({});
        m_proxiedVisitor->StartNode(s_arrayNodeName);
        return result;
    }

    Visitor::Result DomPolyfillVisitor::EndArray(AZ::u64 elementCount)
    {
        if ((m_supportToPolyfill & VisitorFlags::SupportsArrays) == VisitorFlags::Null)
        {
            Visitor::Result result = m_proxiedVisitor->EndArray(elementCount);
            ResultCombine(result, ValueEnd());
            return result;
        }
        Visitor::Result result = m_proxiedVisitor->EndNode(m_nodeStack.top().m_attributeSize, m_nodeStack.top().m_elementSize);
        m_nodeStack.pop();
        ResultCombine(result, ValueEnd());
        return result;
    }

    Visitor::Result DomPolyfillVisitor::StartNode(AZ::Name name)
    {
        m_entryStack.push({ name });
        Visitor::Result result = ValueBegin();
        if (name == s_arrayNodeName)
        {
            m_entryStack.top().m_type = SyntheticNodeType::Array;
            ResultCombine(result, m_proxiedVisitor->StartArray());
        }
        else if (name == s_objectNodeName)
        {
            m_entryStack.top().m_type = SyntheticNodeType::Object;
            ResultCombine(result, m_proxiedVisitor->StartObject());
        }
        else if (name == s_entryNodeName)
        {
            // no-op, just forward the value of the entry
        }
        else if ((m_supportToPolyfill & VisitorFlags::SupportsNodes) == VisitorFlags::Null)
        {
            ResultCombine(result, m_proxiedVisitor->StartNode(name));
        }
        return result;
    }

    Visitor::Result DomPolyfillVisitor::RawStartNode(AZStd::string_view name, [[maybe_unused]] Lifetime lifetime)
    {
        return StartNode(AZ::Name(name));
    }

    Visitor::Result DomPolyfillVisitor::EndNode(AZ::u64 attributeCount, AZ::u64 elementCount)
    {
        
        m_entryStack.pop();
        if ((m_supportToPolyfill & VisitorFlags::SupportsNodes) == VisitorFlags::Null)
        {
            Visitor::Result result = m_proxiedVisitor->EndNode(attributeCount, elementCount);
            ResultCombine(result, ValueEnd());
            return result;
        }
        return VisitorSuccess();
    }

    void DomPolyfillVisitor::Initialize()
    {
        m_quoteRegex = AZStd::regex("^(['\"]).*\\1$", AZStd::regex_constants::optimize);
        m_trueRegex = AZStd::regex("^((yes)|(true))$", AZStd::regex_constants::icase | AZStd::regex_constants::optimize);
        m_falseRegex = AZStd::regex("^((no)|(false))$", AZStd::regex_constants::icase | AZStd::regex_constants::optimize);
        m_nullRegex = AZStd::regex("^((null)|(~))$", AZStd::regex_constants::icase | AZStd::regex_constants::optimize);
        m_numberRegex = AZStd::regex("^(-)?(\\d*)(\\.?\\d*)$", AZStd::regex_constants::icase | AZStd::regex_constants::optimize);

        const VisitorFlags proxiedFlags = m_proxiedVisitor->GetVisitorFlags();

        // If we support Objects & Arrays but not Nodes (e.g. JSON), polyfill faux Node support
        if ((proxiedFlags & VisitorFlags::SupportsNodes) == VisitorFlags::Null &&
            (proxiedFlags & VisitorFlags::SupportsObjects) != VisitorFlags::Null &&
            (proxiedFlags & VisitorFlags::SupportsArrays) != VisitorFlags::Null)
        {
            m_supportToPolyfill |= VisitorFlags::SupportsNodes;
        }
        // If we support Nodes but not Objects or Arrays (e.g. XML), polyfill faux Object & Array support
        else if ((proxiedFlags & VisitorFlags::SupportsNodes) != VisitorFlags::Null)
        {
            if ((proxiedFlags & VisitorFlags::SupportsArrays) == VisitorFlags::Null)
            {
                m_supportToPolyfill |= VisitorFlags::SupportsArrays;
            }
            if ((proxiedFlags & VisitorFlags::SupportsObjects) == VisitorFlags::Null)
            {
                m_supportToPolyfill |= VisitorFlags::SupportsObjects;
            }
        }

        if ((proxiedFlags & VisitorFlags::SupportsRawKeys) == VisitorFlags::Null)
        {
            m_supportToPolyfill |= VisitorFlags::SupportsRawKeys;
        }
        if ((proxiedFlags & VisitorFlags::SupportsRawValues) == VisitorFlags::Null)
        {
            m_supportToPolyfill |= VisitorFlags::SupportsRawValues;
        }
    }

    Visitor::Result DomPolyfillVisitor::ValueBegin()
    {
        if (!m_nodeStack.empty())
        {
            NodeStackEntry& stack = m_nodeStack.top();
            m_proxiedVisitor->StartNode(s_entryNodeName);
            if (stack.m_key.IsEmpty())
            {
                ++stack.m_elementSize;
            }
            else
            {
                ++stack.m_attributeSize;
            }
        }
        return VisitorSuccess();
    }

    Visitor::Result DomPolyfillVisitor::ValueEnd()
    {
        Visitor::Result result = VisitorSuccess();
        if (!m_nodeStack.empty())
        {
            NodeStackEntry& stack = m_nodeStack.top();
            if (stack.m_key.IsEmpty())
            {
                ResultCombine(result, m_proxiedVisitor->Key(s_objectNodeKeyAttributeName));
                ResultCombine(result, m_proxiedVisitor->String(stack.m_key.GetStringView(), Lifetime::Temporary));
                ResultCombine(result, m_proxiedVisitor->EndNode(1, 1));
                stack.m_key = AZ::Name();
            }
            else
            {
                ResultCombine(result, m_proxiedVisitor->EndNode(0, 1));
            }
        }
        return result;
    }

    Visitor::Result DomPolyfillVisitor::HandleValue(AZStd::function<Visitor::Result()> valueHandler)
    {
        Visitor::Result result = ValueBegin();
        ResultCombine(result, valueHandler());
        ResultCombine(result, ValueEnd());
        return result;
    }
} // namespace AZ::Dom
