#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <utility>
#include <AzCore/std/utils.h>
#include <AzCore/std/typetraits/is_same.h>
#include <AzCore/std/typetraits/is_base_of.h>
#include <AzCore/std/iterator.h>
#include <SceneAPI/SceneCore/Containers/Views/View.h>
#include <SceneAPI/SceneCore/Containers/Utilities/ProxyPointer.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            namespace Views
            {
                // Merges two iterators together that have a pair relation.
                //      Both iterators must point to the same relative entry and must contain
                //      the same amount of remaining increments (and decrements if appropriate).
                //      If the categories of the given iterators differ the PairIterator
                //      will only support functionality shared between them. The categories
                //      must be implementations of or be related to the categories defined by
                //      the C++ standard.
                // Example:
                //      vector<string> names;
                //      vector<int> values;
                //      auto view = MakePairView(names.begin(), names.end(), values.begin(), values.end());
                //      for(auto it : view)
                //      {
                //          printf("%s has value %i\n", it.first.c_str(), it.second);
                //      }

                namespace Internal
                {
                    template<typename FirstIterator, typename SecondIterator>
                    struct PairIteratorCategory
                    {
                        static constexpr bool s_sameCategory = AZStd::is_same<
                            typename AZStd::iterator_traits<FirstIterator>::iterator_category,
                            typename AZStd::iterator_traits<SecondIterator>::iterator_category>::value;

                        // True if both categories are the same.
                        // True if FirstIterator has the lower category in the hierarchy
                        // False if ValueItator has the lower category or is unrelated.
                        static constexpr bool s_firstIteratorCategoryIsBaseOfSecondIterator = AZStd::is_base_of<
                            typename AZStd::iterator_traits<FirstIterator>::iterator_category,
                            typename AZStd::iterator_traits<SecondIterator>::iterator_category>::value;

                        // True if both categories are the same.
                        // True if SecondIterator has the lower category in the hierarchy
                        // False if FirstItator has the lower category or is unrelated.
                        static constexpr bool s_SecondIteratorCategoryIsBaseOfFirstIterator = AZStd::is_base_of<
                            typename AZStd::iterator_traits<SecondIterator>::iterator_category,
                            typename AZStd::iterator_traits<FirstIterator>::iterator_category>::value;

                        static_assert(s_sameCategory || (s_firstIteratorCategoryIsBaseOfSecondIterator != s_SecondIteratorCategoryIsBaseOfFirstIterator),
                            "The iterator categories for the first and second in the PairIterator are unrelated categories.");

                        using Category = typename AZStd::conditional<s_firstIteratorCategoryIsBaseOfSecondIterator,
                                    typename AZStd::iterator_traits<FirstIterator>::iterator_category,
                                    typename AZStd::iterator_traits<SecondIterator>::iterator_category>::type;
                    };
                }

                template<typename FirstIterator, typename SecondIterator,
                    typename Category = typename Internal::PairIteratorCategory<FirstIterator, SecondIterator>::Category>
                class PairIterator
                {
                };

                //
                // Base iterator
                //
                template<typename FirstIterator, typename SecondIterator>
                class PairIterator<FirstIterator, SecondIterator, void>
                {
                public:
                    using value_type = AZStd::pair<AZStd::iter_value_t<FirstIterator>, AZStd::iter_value_t<SecondIterator>>;
                    using difference_type = AZStd::ptrdiff_t;
                    using reference = AZStd::pair<AZStd::iter_reference_t<FirstIterator>, AZStd::iter_reference_t<SecondIterator>>;
                    using pointer = ProxyPointer<reference>;
                    using iterator_category = typename Internal::PairIteratorCategory<FirstIterator, SecondIterator>::Category;

                    using RootIterator = PairIterator<FirstIterator, SecondIterator, iterator_category>;

                    PairIterator(FirstIterator First, SecondIterator second);
                    PairIterator(const PairIterator&) = default;

                    PairIterator& operator=(const PairIterator&) = default;

                    RootIterator& operator++();
                    RootIterator operator++(int);

                    const FirstIterator& GetFirstIterator();
                    const SecondIterator& GetSecondIterator();

                protected:
                    FirstIterator m_first;
                    SecondIterator m_second;
                };

                //
                // Input iterator
                //
                template<typename FirstIterator, typename SecondIterator>
                class PairIterator<FirstIterator, SecondIterator, AZStd::input_iterator_tag>
                    : public PairIterator<FirstIterator, SecondIterator, void>
                {
                public:
                    using super = PairIterator<FirstIterator, SecondIterator, void>;

                    PairIterator(FirstIterator first, SecondIterator second);
                    PairIterator(const PairIterator& rhs) = default;

                    bool operator==(const typename super::RootIterator& rhs) const;
                    bool operator!=(const typename super::RootIterator& rhs) const;

                    typename super::reference operator*() const;
                    typename super::pointer operator->() const;
                };

                //
                // Forward iterator
                //
                template<typename FirstIterator, typename SecondIterator>
                class PairIterator<FirstIterator, SecondIterator, AZStd::forward_iterator_tag>
                    : public PairIterator<FirstIterator, SecondIterator, AZStd::input_iterator_tag>
                {
                public:
                    using super = PairIterator<FirstIterator, SecondIterator, AZStd::input_iterator_tag>;

                    PairIterator(FirstIterator first, SecondIterator second);
                    PairIterator(const PairIterator& rhs) = default;
                    PairIterator();
                };

                //
                // Bidirectional iterator
                //
                template<typename FirstIterator, typename SecondIterator>
                class PairIterator<FirstIterator, SecondIterator, AZStd::bidirectional_iterator_tag>
                    : public PairIterator<FirstIterator, SecondIterator, AZStd::forward_iterator_tag>
                {
                public:
                    using super = PairIterator<FirstIterator, SecondIterator, AZStd::forward_iterator_tag>;

                    PairIterator(FirstIterator first, SecondIterator second);
                    PairIterator(const PairIterator& rhs) = default;
                    PairIterator();

                    typename super::RootIterator& operator--();
                    typename super::RootIterator operator--(int);
                };

                //
                // Random Access iterator
                //
                template<typename FirstIterator, typename SecondIterator>
                class PairIterator<FirstIterator, SecondIterator, AZStd::random_access_iterator_tag>
                    : public PairIterator<FirstIterator, SecondIterator, AZStd::bidirectional_iterator_tag>
                {
                public:
                    using super = PairIterator<FirstIterator, SecondIterator, AZStd::bidirectional_iterator_tag>;

                    PairIterator(FirstIterator first, SecondIterator second);
                    PairIterator(const PairIterator& rhs) = default;
                    PairIterator();

                    typename super::reference operator[](size_t index);

                    bool operator<(const typename super::RootIterator& rhs) const;
                    bool operator>(const typename super::RootIterator& rhs) const;
                    bool operator<=(const typename super::RootIterator& rhs) const;
                    bool operator>=(const typename super::RootIterator& rhs) const;

                    typename super::RootIterator operator+(size_t n) const;
                    typename super::RootIterator operator-(size_t n) const;
                    typename super::difference_type operator-(const typename super::RootIterator& rhs) const;
                    typename super::RootIterator& operator+=(size_t n);
                    typename super::RootIterator& operator-=(size_t n);
                };

                //
                // Contiguous Random Access iterator
                //
                template<typename FirstIterator, typename SecondIterator>
                class PairIterator<FirstIterator, SecondIterator, AZStd::contiguous_iterator_tag>
                    : public PairIterator<FirstIterator, SecondIterator, AZStd::random_access_iterator_tag>
                {
                public:
                    using super = PairIterator<FirstIterator, SecondIterator, AZStd::random_access_iterator_tag>;

                    PairIterator(FirstIterator first, SecondIterator second);
                    PairIterator(const PairIterator& rhs) = default;
                    PairIterator();
                };

                //
                // Utility functions
                //
                template<typename FirstIterator, typename SecondIterator>
                PairIterator<FirstIterator, SecondIterator> MakePairIterator(FirstIterator first, SecondIterator second);

                template<typename FirstIterator, typename SecondIterator>
                View<PairIterator<FirstIterator, SecondIterator>>
                MakePairView(FirstIterator firstBegin, FirstIterator firstEnd, SecondIterator secondBegin, SecondIterator secondEnd);

                template<typename FirstView, typename SecondView>
                View<PairIterator<typename FirstView::iterator, typename SecondView::iterator>>
                MakePairView(FirstView& firstView, SecondView& secondView);

                template<typename FirstView, typename SecondView>
                View<PairIterator<typename FirstView::const_iterator, typename SecondView::iterator>>
                MakePairView(const FirstView& firstView, SecondView& secondView);

                template<typename FirstView, typename SecondView>
                View<PairIterator<typename FirstView::iterator, typename SecondView::const_iterator>>
                MakePairView(FirstView& firstView, const SecondView& secondView);

                template<typename FirstView, typename SecondView>
                View<PairIterator<typename FirstView::const_iterator, typename SecondView::const_iterator>>
                MakePairView(const FirstView& firstView, const SecondView& secondView);

                template<typename First, typename Second, typename Category>
                void iter_swap(const PairIterator<First, Second, Category>& lhs, const PairIterator<First, Second, Category>& rhs);
            } // Views
        } // Containers
    } // SceneAPI
} // AZ

#include <SceneAPI/SceneCore/Containers/Views/PairIterator.inl>
