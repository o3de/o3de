/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ::Dom
{
    template<class T>
    DomPrefixTree<T>::DomPrefixTree(AZStd::initializer_list<AZStd::pair<Path, T>> init)
    {
        for (const auto& [path, value] : init)
        {
            SetValue(path, value);
        }
    }

    template <class T>
    template<class Range, class>
    DomPrefixTree<T>::DomPrefixTree(Range&& range)
    {
        for (auto&& [path, value] : AZStd::forward<Range>(range))
        {
            SetValue(path, value);
        }
    }

    template<class T>
    auto DomPrefixTree<T>::GetNodeForPath(const Path& path) -> Node*
    {
        Node* node = &m_rootNode;
        for (const auto& entry : path)
        {
            auto entryIt = node->m_values.find(entry);
            if (entryIt == node->m_values.end())
            {
                return nullptr;
            }
            node = &entryIt->second;
        }
        return node;
    }

    template<class T>
    auto DomPrefixTree<T>::GetNodeForPath(const Path& path) const -> const Node*
    {
        const Node* node = &m_rootNode;
        for (const auto& entry : path)
        {
            auto entryIt = node->m_values.find(entry);
            if (entryIt == node->m_values.end())
            {
                return nullptr;
            }
            node = &entryIt->second;
        }
        return node;
    }

    template<class T>
    void DomPrefixTree<T>::VisitPath(const Path& path, PrefixTreeMatch match, const VisitorFunction& visitor) const
    {
        const Node* rootNode = GetNodeForPath(path);
        if (rootNode == nullptr)
        {
            return;
        }

        if ((match == PrefixTreeMatch::ExactPath || match == PrefixTreeMatch::PathAndSubpaths) && rootNode->m_data.has_value())
        {
            visitor(path, rootNode->m_data.value());
        }

        if (match == PrefixTreeMatch::ExactPath)
        {
            return;
        }

        Path currentPath = path;
        struct PopPathEntry
        {
        };
        using StackEntry = AZStd::variant<const Node*, PathEntry, PopPathEntry>;
        AZStd::stack<StackEntry> stack({ rootNode });
        while (!stack.empty())
        {
            StackEntry entry = AZStd::move(stack.top());
            stack.pop();
            AZStd::visit(
                [&](auto&& value)
                {
                    using CurrentType = AZStd::decay_t<decltype(value)>;
                    if constexpr (AZStd::is_same_v<CurrentType, const Node*>)
                    {
                        if (value != rootNode && value->m_data.has_value())
                        {
                            visitor(currentPath, value->m_data.value());
                        }

                        for (const auto& entry : value->m_values)
                        {
                            // The stack runs this in reverse order, so we'll:
                            // 1) Push the current path entry to currentPath
                            // 2a) Process the value at the path (if any)
                            // 2b) Process the value's descendants at the path (if any)
                            // 3) Pop the path entry from the stack
                            stack.push(PopPathEntry{});
                            stack.push(&entry.second);
                            stack.push(entry.first);
                        }
                    }
                    else if constexpr (AZStd::is_same_v<CurrentType, PathEntry>)
                    {
                        currentPath.Push(value);
                    }
                    else if constexpr (AZStd::is_same_v<CurrentType, PopPathEntry>)
                    {
                        currentPath.Pop();
                    }
                },
                entry);
        }
    }

    template<class T>
    T* DomPrefixTree<T>::ValueAtPath(const Path& path, PrefixTreeMatch match)
    {
        // Just look up the node if we're looking for an exact path
        if (match == PrefixTreeMatch::ExactPath)
        {
            if (Node* node = GetNodeForPath(path); node != nullptr && node->m_data.has_value())
            {
                return &node->m_data.value();
            }
            return {};
        }

        // Otherwise, walk to find the closest anscestor with a value
        Node* node = &m_rootNode;
        T* result = nullptr;
        const size_t lengthToIterate = match == PrefixTreeMatch::SubpathsOnly ? path.Size() - 1 : path.Size();
        for (size_t i = 0; i < lengthToIterate; ++i)
        {
            if (node->m_data.has_value())
            {
                result = &node->m_data.value();
            }

            const PathEntry& entry = path[i];
            auto entryIt = node->m_values.find(entry);
            if (entryIt == node->m_values.end())
            {
                break;
            }

            node = &entryIt->second;
        }

        if (node->m_data.has_value())
        {
            result = &node->m_data.value();
        }

        return result;
    }

    template<class T>
    const T* DomPrefixTree<T>::ValueAtPath(const Path& path, PrefixTreeMatch match) const
    {
        // Const coerce the ValueAtPath result, which doesn't mutate but returns a mutable pointer
        return const_cast<DomPrefixTree<T>*>(this)->ValueAtPath(path, match);
    }

    template<class T>
    template<class Deduced>
    T DomPrefixTree<T>::ValueAtPathOrDefault(const Path& path, Deduced&& defaultValue, PrefixTreeMatch match) const
    {
        const T* value = ValueAtPath(path, match);
        return value == nullptr ? AZStd::forward<Deduced>(defaultValue) : *value;
    }

    template<class T>
    template<class Deduced>
    void DomPrefixTree<T>::SetValue(const Path& path, Deduced&& value)
    {
        Node* node = &m_rootNode;
        for (const PathEntry& entry : path)
        {
            // Get or create an entry in this node
            node = &node->m_values[entry];
        }
        node->m_data = AZStd::forward<Deduced>(value);
    }

    template<class T>
    void DomPrefixTree<T>::EraseValue(const Path& path, bool removeChildren)
    {
        Node* node = &m_rootNode;
        const size_t entriesToIterate = path.Size() - 1;
        for (size_t i = 0; i < entriesToIterate; ++i)
        {
            const PathEntry& entry = path[i];
            auto nodeIt = node->m_values.find(entry);
            if (nodeIt == node->m_values.end())
            {
                return;
            }
            node = &nodeIt->second;
        }

        auto nodeIt = node->m_values.find(path[path.Size() - 1]);
        if (nodeIt != node->m_values.end())
        {
            if (removeChildren)
            {
                node->m_values.erase(nodeIt);
            }
            else
            {
                nodeIt->second.m_data = {};
            }
        }
    }

    template<class T>
    void DomPrefixTree<T>::Clear()
    {
        m_rootNode = Node();
    }
} // namespace AZ::Dom
