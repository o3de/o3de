#pragma once

/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzTest/AzTest.h>

// These data structures are sufficient to test all current iterator flags
// e.g. forward, bidirectional, random_access
#include <AzCore/std/iterator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/forward_list.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            namespace Views
            {
                template <typename Collection>
                class IteratorTypedTestsBase
                    : public ::testing::Test
                {
                public:
                    IteratorTypedTestsBase() = default;
                    virtual ~IteratorTypedTestsBase() = default;

                    using CollectionType = Collection;
                    using BaseIterator = typename Collection::iterator;
                    using ConstBaseIterator = typename Collection::const_iterator;
                    using ValueType = typename Collection::value_type;
                    using BaseIteratorReference = typename AZStd::iterator_traits<BaseIterator>::reference;

                    void SetUp()
                    {
                    }

                    void TearDown()
                    {
                    }

                    ConstBaseIterator GetBaseConstIterator(size_t pos)
                    {
                        if (pos <= 0)
                        {
                            return m_testCollection.cbegin();
                        }
                        else
                        {
                            ConstBaseIterator returnIterator = m_testCollection.cbegin();
                            AZStd::advance(returnIterator, pos);
                            return returnIterator;
                        }
                    }

                    BaseIterator GetBaseIterator(size_t pos)
                    {
                        if (pos <= 0)
                        {
                            return m_testCollection.begin();
                        }
                        else
                        {
                            BaseIterator returnIterator = m_testCollection.begin();
                            AZStd::advance(returnIterator, pos);
                            return returnIterator;
                        }
                    }

                    void SetCollectionForTesting(Collection newTestCollection)
                    {
                        m_testCollection = newTestCollection;
                    }

                    Collection m_testCollection;
                };

                // Utility functions for building collections.
                template<typename T>
                void AddElement(AZStd::vector<T>& collection, T newValue)
                {
                    collection.push_back(newValue);
                }

                template<typename T>
                void AddElement(AZStd::list<T>& collection, T newValue)
                {
                    collection.push_back(newValue);
                }

                template<typename T>
                void AddElement(AZStd::forward_list<T>& collection, T newValue)
                {
                    collection.push_front(newValue);
                }

                // Using key and value of same type for simplicity
                template<typename T>
                void AddElement(AZStd::map<T, T>& collection, T newValue)
                {
                    collection.emplace(newValue, newValue);
                }

                template<typename T>
                void AddElement(AZStd::unordered_map<T, T>& collection, T newValue)
                {
                    collection[newValue] = newValue;
                }

                // Utility functions to provide consistency between the order elements are added
                //      and the order in which they are iterated.
                template<typename T>
                void ReorderToMatchIterationWithAddition([[maybe_unused]] AZStd::vector<T>& collection)
                {
                    // Already doing iteration in the correct direction.
                }

                template<typename T>
                void ReorderToMatchIterationWithAddition([[maybe_unused]] AZStd::list<T>& collection)
                {
                    // Already doing iteration in the correct direction.
                }

                template<typename T>
                void ReorderToMatchIterationWithAddition(AZStd::forward_list<T>& collection)
                {
                    collection.reverse();
                }

                // Collection types for common groupings of tests (since different iterator tags support different APIs)
                // Generic type list, for functionality supported by all iterator flags
                typedef ::testing::Types<AZStd::vector<int>, AZStd::list<int>, AZStd::forward_list<int>,
                    AZStd::map<int, int>, AZStd::unordered_map<int, int> > GenericCollectionTypes;
                typedef ::testing::Types<AZStd::vector<int>, AZStd::list<int>, AZStd::forward_list<int> > BasicCollectionTypes;
                typedef ::testing::Types<AZStd::map<int, int>, AZStd::unordered_map<int, int> > MapCollectionTypes;
            } // namespace Views
        } // namespace Containers
    } // namespace SceneAPI
} // namespace AZ
