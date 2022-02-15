/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomPath.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/optional.h>

namespace AZ::Dom
{
    //! Specifies how a path matches against a DomPrefixTree
    enum class PrefixTreeMatch
    {
        //! Only an exact path will match.
        //! For the path "/foo/bar" only "/foo/bar" will match while
        //! "/foo" and "/foo/bar/baz" will not.
        ExactPath,
        //! The path, and any of its subpaths, will match.
        //! For the path "/foo/bar" both "/foo/bar" and any subpaths like
        //! "/foo/bar/0" will match, while "/foo" and orthogonal paths like
        //! "/bar" will not
        PathAndSubpaths,
        //! Any of the path's subpaths will match, excepting the path itself.
        //! For the path "/foo/bar", "/foo/bar" will not match but "/foo/bar/0"
        //! will.
        SubpathsOnly,
    };

    //! A prefix tree that maps DOM paths to some arbitrary value.
    template<class T>
    class DomPrefixTree
    {
    public:
        DomPrefixTree() = default;
        DomPrefixTree(const DomPrefixTree&) = default;
        DomPrefixTree(DomPrefixTree&&) = default;
        explicit DomPrefixTree(AZStd::initializer_list<AZStd::pair<Path, T>> init);

        //! Visits a path and calls a visitor for each matching path and value.
        void VisitPath(const Path& path, PrefixTreeMatch match, const AZStd::function<void(const Path&, const T&)>& visitor) const;
        //! Visits a path and returns the most specific matching value, or null if none was found.
        T* ValueAtPath(const Path& path, PrefixTreeMatch match);
        //! \see ValueAtPath
        const T* ValueAtPath(const Path& path, PrefixTreeMatch match) const;
        //! Visits a path and returns the most specific matching value or some default value.
        T ValueAtPathOrDefault(const Path& path, T&& defaultValue, PrefixTreeMatch match) const;

        //! Sets the value stored at path.
        void SetValue(const Path& path, T&& value);
        //! Removes the value stored at path. If removeChildren is true, also removes any values stored at subpaths.
        void EraseValue(const Path& path, bool removedChildren = false);
        //! Removes all entries from this tree.
        void Clear();

    private:
        struct Node
        {
            AZStd::unordered_map<PathEntry, Node> m_values;
            AZStd::optional<T> m_data;
        };

        Node* GetNodeForPath(const Path& path);
        const Node* GetNodeForPath(const Path& path) const;

        Node m_rootNode;
    };
} // namespace AZ::Dom

#include <AzCore/DOM/DomPrefixTree.inl>
