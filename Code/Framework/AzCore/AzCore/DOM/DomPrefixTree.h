/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomPath.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/ranges/ranges.h>
#include <AzCore/std/tuple.h>

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

    template<class Range, class T, class = void>
    constexpr bool RangeConvertibleToPrefixTree = false;

    template<class Range, class T>
    constexpr bool RangeConvertibleToPrefixTree<
        Range,
        T,
        AZStd::enable_if_t<
            AZStd::ranges::input_range<Range> && AZStd::tuple_size<AZStd::ranges::range_value_t<Range>>::value == 2 &&
            AZStd::convertible_to<AZStd::tuple_element_t<0, AZStd::ranges::range_value_t<Range>>, Path> &&
            AZStd::convertible_to<AZStd::tuple_element_t<1, AZStd::ranges::range_value_t<Range>>, T>>> = true;

    //! A prefix tree that maps DOM paths to some arbitrary value.
    template<class T>
    class DomPrefixTree
    {
    public:
        DomPrefixTree() = default;
        DomPrefixTree(const DomPrefixTree&) = default;
        DomPrefixTree(DomPrefixTree&&) = default;
        explicit DomPrefixTree(AZStd::initializer_list<AZStd::pair<Path, T>> init);

        template<class Range, class = AZStd::enable_if_t<RangeConvertibleToPrefixTree<Range, T>>>
        explicit DomPrefixTree(Range&& range);

        DomPrefixTree& operator=(const DomPrefixTree&) = default;
        DomPrefixTree& operator=(DomPrefixTree&&) = default;

        using VisitorFunction = AZStd::function<void(const Path&, const T&)>;

        //! Visits a path and calls a visitor for each matching path and value.
        void VisitPath(const Path& path, PrefixTreeMatch match, const VisitorFunction& visitor) const;
        //! Visits a path and returns the most specific matching value, or null if none was found.
        T* ValueAtPath(const Path& path, PrefixTreeMatch match);
        //! \see ValueAtPath
        const T* ValueAtPath(const Path& path, PrefixTreeMatch match) const;
        //! Visits a path and returns the most specific matching value or some default value.
        template<class Deduced>
        T ValueAtPathOrDefault(const Path& path, Deduced&& defaultValue, PrefixTreeMatch match) const;

        //! Sets the value stored at path.
        template<class Deduced>
        void SetValue(const Path& path, Deduced&& value);
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
