#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <deque>
#include <iterator>
#include <type_traits>
#include <AzCore/std/iterator.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Views/View.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            namespace Views
            {
                // Tag for depth-first traversal
                struct DepthFirst {};
                // Tag for breadth-first traversal
                struct BreadthFirst {};

                // Iterator to traverse a SceneGraph downwards from the root or a given hierarchy iterator either
                //      depth-first or breadth-first. If a hierarchy iterator is specified it will be the first
                //      entry returned, otherwise the root.
                // Example:
                //      auto view = MakeSceneGraphDownwardsView<DepthFirst>(graph, graph.GetNameStorage().begin());
                //      for (auto it : view)
                //      {
                //          printf("Node: %s\n", it.c_str());
                //      }
                // Example:
                //      SceneGraph::NodeIndex search = graph.Find("A.C");
                //      auto view = MakeSceneGraphDownwardsView<BreadthFirst>(graph, graph.ConvertToHierarchyIterator(search), graph.GetNameStorage().begin(), true);
                //      for (auto it : view)
                //      {
                //          printf("Node: %s\n", it.c_str());
                //      }
                template<typename Iterator, typename Traversal>
                class SceneGraphDownwardsIterator
                {
                    static_assert(std::is_base_of<AZStd::bidirectional_iterator_tag, typename AZStd::iterator_traits<Iterator>::iterator_category>::value,
                        "SceneGraphDownwardsIterator needs at least a bidrectional iterator for its data iterator.");
                public:
                    using value_type = typename AZStd::iterator_traits<Iterator>::value_type;
                    using difference_type = std::ptrdiff_t;
                    using pointer = typename AZStd::iterator_traits<Iterator>::pointer;
                    using reference = typename AZStd::iterator_traits<Iterator>::reference;
                    using iterator_category = AZStd::forward_iterator_tag;

                    // Creates an iterator at the root node in the hierarchy.
                    SceneGraphDownwardsIterator(const SceneGraph& graph, Iterator iterator);

                    // Creates an iterator at a specified node in the hierarchy.
                    // graph: SceneGraph that will traversed.
                    // graphIterator: The node to start traversing from.
                    // iterator: The data iterator to be dereferenced from.
                    // rootIterator: If true the data iterator will be the first iterator from the data and
                    //      this iterator will be moved forward to match the graph iterator. If false
                    //      the graph iterator and the data iterator should be pointing to the same relative
                    //      element.
                    SceneGraphDownwardsIterator(const SceneGraph& graph, SceneGraph::HierarchyStorageConstIterator graphIterator, Iterator iterator, bool rootIterator);
                    SceneGraphDownwardsIterator(const SceneGraphDownwardsIterator&) = default;
                    SceneGraphDownwardsIterator();

                    SceneGraphDownwardsIterator& operator=(const SceneGraphDownwardsIterator&) = default;

                    SceneGraphDownwardsIterator& operator++();
                    SceneGraphDownwardsIterator operator++(int);

                    bool operator==(const SceneGraphDownwardsIterator& rhs) const;
                    bool operator!=(const SceneGraphDownwardsIterator& rhs) const;

                    reference operator*() const;
                    pointer operator->() const;

                    SceneGraph::HierarchyStorageConstIterator GetHierarchyIterator() const;

                    // Stops the iterator from descending into the children of the current node. Pending nodes and their children will be processed as normal.
                    //      This call can be made multiple times for different nodes.
                    void IgnoreNodeDescendants();

                private:
                    // For iterators that are raw pointers.
                    pointer GetPointer(AZStd::true_type) const;
                    // For all other types of iterator.
                    pointer GetPointer(AZStd::false_type) const;

                    void MoveToNext(DepthFirst);
                    void MoveToNext(BreadthFirst);
                    void JumpTo(int32_t index);

                    std::deque<int32_t> m_pending;
                    const SceneGraph* m_graph;
                    Iterator m_iterator;
                    int32_t m_index : 30;
                    int32_t m_firstNode : 1;
                    int32_t m_ignoreDescendants : 1;
                };

                template<typename Traversal, typename Iterator>
                SceneGraphDownwardsIterator<Iterator, Traversal> MakeSceneGraphDownwardsIterator(const SceneGraph& graph, Iterator iterator);
                template<typename Traversal, typename Iterator>
                SceneGraphDownwardsIterator<Iterator, Traversal> MakeSceneGraphDownwardsIterator(
                    const SceneGraph& graph, SceneGraph::HierarchyStorageConstIterator graphIterator, Iterator iterator, bool rootIterator);
                template<typename Traversal, typename Iterator>
                SceneGraphDownwardsIterator<Iterator, Traversal> MakeSceneGraphDownwardsIterator(
                    const SceneGraph& graph, SceneGraph::NodeIndex node, Iterator iterator, bool rootIterator);

                template<typename Traversal, typename Iterator>
                View<SceneGraphDownwardsIterator<Iterator, Traversal> > MakeSceneGraphDownwardsView(const SceneGraph& graph, Iterator iterator);
                template<typename Traversal, typename Iterator>
                View<SceneGraphDownwardsIterator<Iterator, Traversal> > MakeSceneGraphDownwardsView(
                    const SceneGraph& graph, SceneGraph::HierarchyStorageConstIterator graphIterator, Iterator iterator, bool rootIterator);
                template<typename Traversal, typename Iterator>
                View<SceneGraphDownwardsIterator<Iterator, Traversal> > MakeSceneGraphDownwardsView(
                    const SceneGraph& graph, SceneGraph::NodeIndex node, Iterator iterator, bool rootIterator);
            } // Views
        } // Containers
    } // SceneAPI
} // AZ

#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.inl>
