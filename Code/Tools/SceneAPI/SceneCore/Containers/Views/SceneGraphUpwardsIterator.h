#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <stack>
#include <iterator>
#include <type_traits>
#include <AzCore/std/iterator.h>
#include <AzCore/std/typetraits/is_base_of.h>
#include <SceneAPI/SceneCore/Containers/Views/View.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            namespace Views
            {
                // Iterator to traverse a SceneGraph from a given node upwards to the root.
                //      The given node will be included as the first returned value.
                // Example:
                //      auto view = MakeSceneGraphUpwardsView(graph, graph.ConvertToHierarchyIterator(nodeIndex), graph.GetNameStorage().begin(), true);
                //      for (auto it : view)
                //      {
                //          printf("Node: %s\n", it.c_str());
                //      }
                template<typename Iterator>
                class SceneGraphUpwardsIterator
                {
                    static_assert(AZStd::is_base_of<AZStd::bidirectional_iterator_tag, typename AZStd::iterator_traits<Iterator>::iterator_category>::value,
                        "SceneGraphUpwardsIterator needs at least a bidrectional iterator for its data iterator.");
                public:
                    using value_type = typename AZStd::iterator_traits<Iterator>::value_type;
                    using difference_type = AZStd::ptrdiff_t;
                    using pointer = typename AZStd::iterator_traits<Iterator>::pointer;
                    using reference = typename AZStd::iterator_traits<Iterator>::reference;
                    using iterator_category = AZStd::forward_iterator_tag;

                    // Creates an iterator at a specified node in the hierarchy.
                    // graph: SceneGraph that will traversed.
                    // graphIterator: The node to start traversing from.
                    // iterator: The data iterator to be dereferenced from.
                    // rootIterator: If true the data iterator will be the first iterator from the data and
                    //      this iterator will be moved forward to match the graph iterator. If false
                    //      the graph iterator and the data iterator should be pointing to the same relative
                    //      element.
                    SceneGraphUpwardsIterator(const SceneGraph& graph, SceneGraph::HierarchyStorageConstIterator graphIterator, Iterator iterator, bool rootIterator);
                    SceneGraphUpwardsIterator(const SceneGraphUpwardsIterator&) = default;
                    SceneGraphUpwardsIterator();

                    SceneGraphUpwardsIterator& operator=(const SceneGraphUpwardsIterator&) = default;

                    SceneGraphUpwardsIterator& operator++();
                    SceneGraphUpwardsIterator operator++(int);

                    bool operator==(const SceneGraphUpwardsIterator& rhs) const;
                    bool operator!=(const SceneGraphUpwardsIterator& rhs) const;

                    reference operator*() const;
                    pointer operator->() const;

                    SceneGraph::HierarchyStorageConstIterator GetHierarchyIterator() const;

                private:
                    // For iterators that are raw pointers.
                    pointer GetPointer(AZStd::true_type) const;
                    // For all other types of iterator.
                    pointer GetPointer(AZStd::false_type) const;

                    void MoveToNext();

                    const SceneGraph* m_graph;
                    Iterator m_iterator;
                    int32_t m_index;
                };

                template<typename Iterator>
                SceneGraphUpwardsIterator<Iterator> MakeSceneGraphUpwardsIterator(
                    const SceneGraph& graph, SceneGraph::HierarchyStorageConstIterator graphIterator, Iterator iterator, bool rootIterator);

                template<typename Iterator>
                SceneGraphUpwardsIterator<Iterator> MakeSceneGraphUpwardsIterator(
                    const SceneGraph& graph, SceneGraph::NodeIndex node, Iterator iterator, bool rootIterator);

                template<typename Iterator>
                View<SceneGraphUpwardsIterator<Iterator> > MakeSceneGraphUpwardsView(
                    const SceneGraph& graph, SceneGraph::HierarchyStorageConstIterator graphIterator, Iterator iterator, bool rootIterator);

                template<typename Iterator>
                View<SceneGraphUpwardsIterator<Iterator> > MakeSceneGraphUpwardsView(
                    const SceneGraph& graph, SceneGraph::NodeIndex node, Iterator iterator, bool rootIterator);
            } // Views
        } // Containers
    } // SceneAPI
} // AZ

#include <SceneAPI/SceneCore/Containers/Views/SceneGraphUpwardsIterator.inl>
