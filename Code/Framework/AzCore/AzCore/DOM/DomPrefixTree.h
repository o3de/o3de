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
        //! The path, and any of its parent paths, will match.
        //! For the path "/foo/bar" both "/foo/bar" and any parent paths like
        //! "/foo" will match, while "/foo/bar/0" and orthogonal paths like
        //! "/bar" will not
        PathAndParents,
        //! Any of the path's parents will match, excepting the path itself.
        //! For the path "/foo/bar", "/foo/bar" will not match but "/foo"
        //! will.
        ParentsOnly,
    };

    //! Defines traversal behavior when calling DomPrefixTree<T>::VisitPath.
    enum class PrefixTreeTraversalFlags
    {
        //! No flags, will default to visiting all path-relative nodes in least to most specific order.
        None = 0x0,
        //! If set, any entries at the exact path specified to VisitPath will be ignored.
        //! For the path "/foo" an entry at "/foo" will not match, while "/" or "/foo/0" may match, depending on the
        //! other traversal flags.
        ExcludeExactPath = 0x01,
        //! If set, any parent entries to the path specified to VisitPath will be ignored.
        //! For the path "/foo/0", "/" and "/foo" will not match, while "/foo/0" and "/foo/0/1" may match, depending
        //! on the other traversal flags.
        ExcludeParentPaths = 0x02,
        //! If set, any child entries to the patch specified to VisitPath will be ignored.
        //! For the path "/foo", "/foo/1" and "/foo/2" will not match, while "/foo" and "/" may match, depending
        //! on the other traversal flags.
        ExcludeChildPaths = 0x04,
        //! If set, this visit operation will visit paths in a breadth-first manner.
        //! This is the default order of operations, and operates faster than TraverseMostToLeastSpecific, but it is unsafe
        //! to call EraseValue on a path visited in this order when specifying removeChildren.
        TraverseLeastToMostSpecific = 0x08,
        //! If set, this visit operation will visit paths in a depth first manner.
        //! This is slower than TraverseLeastToMostSpecific, but it is safe to call EraseValue on a path visited in this manner,
        //! even when specifying removeChildren.
        TraverseMostToLeastSpecific = 0x10,
    };
    AZ_DEFINE_ENUM_BITWISE_OPERATORS(PrefixTreeTraversalFlags);

    constexpr PrefixTreeTraversalFlags DefaultTraversalFlags =
        PrefixTreeTraversalFlags::ExcludeChildPaths | PrefixTreeTraversalFlags::TraverseLeastToMostSpecific;

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

        //! VisitorFunctions are callbacks invoked by VisitPath.
        //! VisitorFunction is provided with the path to a given value and a reference to the associated value.
        //! The callback returns a boolean value indicating whether or not the visit operation should continue running.
        using VisitorFunction = AZStd::function<bool(const Path&, T&)>;
        using ConstVisitorFunction = AZStd::function<bool(const Path&, const T&)>;

        //! Visits a path and calls a visitor for each matching path and value.
        void VisitPath(const Path& path, const VisitorFunction& visitor, PrefixTreeTraversalFlags flags = DefaultTraversalFlags);
        void VisitPath(const Path& path, const ConstVisitorFunction& visitor, PrefixTreeTraversalFlags flags = DefaultTraversalFlags) const;
        //! Visits a path and returns the most specific matching value, or null if none was found.
        T* ValueAtPath(const Path& path, PrefixTreeMatch match);
        //! \see ValueAtPath
        const T* ValueAtPath(const Path& path, PrefixTreeMatch match) const;
        //! Visits a path and returns the most specific matching value or some default value.
        template<class Deduced>
        T ValueAtPathOrDefault(const Path& path, Deduced&& defaultValue, PrefixTreeMatch match) const;
        //! Get or create a value at the given exact path.
        T& operator[](const Path& path);
        //! Retrieves a value for a given exact path.
        //! \warning This is an unsafe operation for entries that are not already present in the tree.
        //! Prefer ValueAtPath or ValueAtPathOrDefault in those cases.
        const T& operator[](const Path& path) const;

        //! Sets the value stored at path.
        template<class Deduced>
        void SetValue(const Path& path, Deduced&& value);
        //! Removes the value stored at path. If removeChildren is true, also removes any values stored at subpaths.
        void EraseValue(const Path& path, bool removeChildren = false);

        //! Detaches a sub tree whose root node matches the provided path.
        //! The detach operation removes the node and its children from the existing tree.
        //! @param path The path which corresponds to the root node of the subtree to be detached.
        //! @return The DomPrefixTree that is detached.
        DomPrefixTree<T> DetachSubTree(const Path& path);

        //! Attaches a subtree provided at the node that matches the provided path. Attaching will overwrite the node at the path.
        //! @param path The path which corresponds to the node at which the subtree should be attached.
        //! @param subTree The subtree to attach at the provided path.
        //! @return True if the subTree was attached successfully. Return false otherwise.
        bool AttachSubTree(const Path& path, DomPrefixTree&& subTree);
        //! Removes all entries from this tree.
        void Clear();

        //! Returns true if the root node is empty.
        bool IsEmpty() const;

    private:
        struct Node
        {
            AZStd::unordered_map<PathEntry, Node> m_values;
            AZStd::optional<T> m_data;

            bool IsEmpty() const;
        };

        //! Since Node is an internal private struct in this class, this constructor that uses Node is also private.
        explicit DomPrefixTree(Node&& node);

        Node* GetNodeForPath(const Path& path);
        const Node* GetNodeForPath(const Path& path) const;

        //! Detaches the node that matches the provided path.
        //! The detach operation removes the node and its children from the existing tree.
        //! @param path The path at which the node should be detached.
        //! @return The Node that is detached.
        Node DetachNodeAtPath(const Path& path);

        //! Attaches a node that matches the provided path. Attaching will overwrite the node at the path.
        //! @param path The path which corresponds to the node at which the subtree should be attached.
        //! @param node The node to be attached.
        //! @return True if the node was attached successfully. Return false otherwise.
        bool AttachNodeAtPath(const Path& path, Node&& node);

        Node m_rootNode;
    };
} // namespace AZ::Dom

#include <AzCore/DOM/DomPrefixTree.inl>
