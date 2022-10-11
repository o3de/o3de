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

    template<class T>
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
    void DomPrefixTree<T>::VisitPath(const Path& path, const VisitorFunction& visitor, PrefixTreeTraversalFlags flags)
    {
        const bool processOurPath = (flags & PrefixTreeTraversalFlags::ExcludeExactPath) == PrefixTreeTraversalFlags::None;
        const bool processOurParents = (flags & PrefixTreeTraversalFlags::ExcludeParentPaths) == PrefixTreeTraversalFlags::None;
        const bool processOurChildren = (flags & PrefixTreeTraversalFlags::ExcludeChildPaths) == PrefixTreeTraversalFlags::None;

        Node* rootNode = processOurParents ? &m_rootNode : GetNodeForPath(path);
        if (rootNode == nullptr)
        {
            return;
        }

        Path currentPath = processOurParents ? Path() : path;
        struct PopPathEntry
        {
        };
        using StackEntry = AZStd::variant<Node*, PathEntry, PopPathEntry>;

        const bool depthFirst = (flags & PrefixTreeTraversalFlags::TraverseMostToLeastSpecific) != PrefixTreeTraversalFlags::None;
        AZStd::stack<AZStd::pair<AZ::Dom::Path, Node*>> depthStack;

        auto maybeVisitEntry = [&](Node* node) -> bool
        {
            if (!node->m_data.has_value())
            {
                return true;
            }
            if ((processOurParents && currentPath.Size() < path.Size()) || (processOurPath && currentPath.Size() == path.Size()) ||
                (processOurChildren && currentPath.Size() > path.Size()))
            {
                if (depthFirst)
                {
                    depthStack.emplace(currentPath, node);
                    return true;
                }
                return visitor(currentPath, node->m_data.value());
            }
            return true;
        };

        AZStd::stack<StackEntry> stack({ rootNode });

        auto pushNode = [&](AZStd::pair<PathEntry, Node>& entry)
        {
            // The stack runs this in reverse order, so we'll:
            // 1) Push the current path entry to currentPath
            // 2a) Process the value at the path (if any)
            // 2b) Process the value's descendants at the path (if any)
            // 3) Pop the path entry from the stack
            stack.push(PopPathEntry{});
            stack.push(&entry.second);
            stack.push(entry.first);
        };

        // Returns false if processing should halt
        auto processStackEntry = [&](auto&& value) -> bool
        {
            using CurrentType = AZStd::decay_t<decltype(value)>;
            if constexpr (AZStd::is_same_v<CurrentType, Node*>)
            {
                if (!maybeVisitEntry(value))
                {
                    return false;
                }

                if (processOurChildren && currentPath.Size() >= path.Size())
                {
                    for (auto& entry : value->m_values)
                    {
                        pushNode(entry);
                    }
                }
                else if (currentPath.Size() < path.Size())
                {
                    auto entryIt = value->m_values.find(path[currentPath.Size()]);
                    if (entryIt != value->m_values.end())
                    {
                        pushNode(*entryIt);
                    }
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
            return true;
        };

        while (!stack.empty())
        {
            StackEntry entry = AZStd::move(stack.top());
            stack.pop();

            if (!AZStd::visit(processStackEntry, entry))
            {
                return;
            }
        }

        if (depthFirst)
        {
            while (depthStack.size())
            {
                const auto& entry = depthStack.top();
                if (entry.second->m_data.has_value())
                {
                    if (!visitor(entry.first, entry.second->m_data.value()))
                    {
                        return;
                    }
                }
                depthStack.pop();
            }
        }
    }

    template<class T>
    void DomPrefixTree<T>::VisitPath(const Path& path, const ConstVisitorFunction& visitor, PrefixTreeTraversalFlags flags) const
    {
        // Break const to reuse the non-const implementation with our const visitor function
        const_cast<DomPrefixTree<T>*>(this)->VisitPath(
            path,
            [&visitor](const Path& path, T& value)
            {
                return visitor(path, value);
            },
            flags);
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
        const size_t lengthToIterate = match == PrefixTreeMatch::ParentsOnly ? path.Size() - 1 : path.Size();
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
    T& DomPrefixTree<T>::operator[](const Path& path)
    {
        Node* node = &m_rootNode;
        for (const PathEntry& entry : path)
        {
            // Get or create an entry in this node
            node = &node->m_values[entry];
        }
        if constexpr (AZStd::is_default_constructible_v<T>)
        {
            if (!node->m_data.has_value())
            {
                node->m_data = T{};
            }
        }
        return node->m_data.value();
    }

    template<class T>
    const T& DomPrefixTree<T>::operator[](const Path& path) const
    {
        return ValueAtPath(path, PrefixTreeMatch::ExactPath).value();
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
