/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/typetraits/is_pointer.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            namespace Views
            {
                //
                // Base iterator
                //
                template<typename Iterator>
                FilterIterator<Iterator, void>::FilterIterator(Iterator iterator, Iterator end, const Predicate& predicate)
                    : m_iterator(iterator)
                    , m_end(end)
                    , m_predicate(predicate)
                {
                    while (m_iterator != m_end && !m_predicate(*m_iterator))
                    {
                        AZStd::advance(m_iterator, 1);
                    }
                }

                template<typename Iterator>
                FilterIterator<Iterator, void>::FilterIterator(Iterator defaultIterator)
                    : m_iterator(defaultIterator)
                    , m_end(defaultIterator)
                    , m_predicate()
                {
                }

                template<typename Iterator>
                auto FilterIterator<Iterator, void>::operator++()->RootIterator &
                {
                    MoveToNext();
                    return *static_cast<RootIterator*>(this);
                }

                template<typename Iterator>
                auto FilterIterator<Iterator, void>::operator++(int)->RootIterator
                {
                    RootIterator result = *static_cast<RootIterator*>(this);
                    MoveToNext();
                    return result;
                }

                template<typename Iterator>
                auto FilterIterator<Iterator, void>::GetBaseIterator() const -> const Iterator&
                {
                    return m_iterator;
                }

                template<typename Iterator>
                void FilterIterator<Iterator, void>::MoveToNext()
                {
                    if (m_iterator != m_end)
                    {
                        AZStd::advance(m_iterator, 1);
                    }

                    while (m_iterator != m_end && !m_predicate(*m_iterator))
                    {
                        AZStd::advance(m_iterator, 1);
                    }
                }

                //
                // Input iterator
                //
                template<typename Iterator>
                FilterIterator<Iterator, AZStd::input_iterator_tag>::FilterIterator(Iterator iterator, Iterator end, const typename super::Predicate& predicate)
                    : super(iterator, end, predicate)
                {
                }

                template<typename Iterator>
                FilterIterator<Iterator, AZStd::input_iterator_tag>::FilterIterator(Iterator defaultIterator)
                    : super(defaultIterator)
                {
                }

                template<typename Iterator>
                bool FilterIterator<Iterator, AZStd::input_iterator_tag>::operator==(const typename super::RootIterator& rhs) const
                {
                    return super::m_iterator == rhs.m_iterator;
                }

                template<typename Iterator>
                bool FilterIterator<Iterator, AZStd::input_iterator_tag>::operator!=(const typename super::RootIterator& rhs) const
                {
                    return super::m_iterator != rhs.m_iterator;
                }

                template<typename Iterator>
                auto FilterIterator<Iterator, AZStd::input_iterator_tag>::operator*() const->typename super::reference
                {
                    return static_cast<typename super::reference>(*super::m_iterator);
                }

                // Used when iterator is raw pointer
                template<typename Iterator>
                auto FilterIterator<Iterator, AZStd::input_iterator_tag>::GetPointer(AZStd::true_type) const->typename super::pointer
                {
                    return super::m_iterator;
                }

                // Used when iterator is an object
                template<typename Iterator>
                auto FilterIterator<Iterator, AZStd::input_iterator_tag>::GetPointer(AZStd::false_type) const->typename super::pointer
                {
                    return super::m_iterator.operator->();
                }

                template<typename Iterator>
                auto FilterIterator<Iterator, AZStd::input_iterator_tag>::operator->() const->typename super::pointer
                {
                    return GetPointer(typename AZStd::is_pointer<Iterator>::type());
                }

                //
                // Forward iterator
                //
                template<typename Iterator>
                FilterIterator<Iterator, AZStd::forward_iterator_tag>::FilterIterator(Iterator iterator, Iterator end, const typename super::Predicate& predicate)
                    : super(iterator, end, predicate)
                {
                }

                template<typename Iterator>
                FilterIterator<Iterator, AZStd::forward_iterator_tag>::FilterIterator()
                    : super(Iterator())
                {
                }

                //
                // Bidirectional iterator
                //
                template<typename Iterator>
                FilterIterator<Iterator, AZStd::bidirectional_iterator_tag>::FilterIterator(Iterator iterator, Iterator end, const typename super::Predicate& predicate)
                    : super(iterator, end, predicate)
                {
                    // Matches the iterator that's been moved forward to the first element that passes the predicate.
                    m_begin = super::m_iterator;
                }

                template<typename Iterator>
                FilterIterator<Iterator, AZStd::bidirectional_iterator_tag>::FilterIterator(Iterator iterator, Iterator begin, Iterator end, const typename super::Predicate& predicate)
                    : super(iterator, end, predicate)
                    , m_begin(begin)
                {
                    while (m_begin != super::m_end && !super::m_predicate(*m_begin))
                    {
                        AZStd::advance(m_begin, 1);
                    }
                }

                template<typename Iterator>
                FilterIterator<Iterator, AZStd::bidirectional_iterator_tag>::FilterIterator()
                    : super()
                {
                }

                template<typename Iterator>
                auto FilterIterator<Iterator, AZStd::bidirectional_iterator_tag>::operator--()->typename super::RootIterator &
                {
                    MoveToPrevious();
                    return *static_cast<typename super::RootIterator*>(this);
                }

                template<typename Iterator>
                auto FilterIterator<Iterator, AZStd::bidirectional_iterator_tag>::operator--(int)->typename super::RootIterator
                {
                    typename super::RootIterator result = *static_cast<typename super::RootIterator*>(this);
                    MoveToPrevious();
                    return result;
                }

                template<typename Iterator>
                void FilterIterator<Iterator, AZStd::bidirectional_iterator_tag>::MoveToPrevious()
                {
                    if (super::m_iterator != m_begin)
                    {
                        AZStd::advance(super::m_iterator, -1);
                    }

                    while (super::m_iterator != m_begin && !super::m_predicate(*super::m_iterator))
                    {
                        AZStd::advance(super::m_iterator, -1);
                    }
                }

                //
                // Random Access iterator
                //
                template<typename Iterator>
                FilterIterator<Iterator, AZStd::random_access_iterator_tag>::FilterIterator(Iterator iterator, Iterator end, const typename super::Predicate& predicate)
                    : super(iterator, end, predicate)
                {
                }

                template<typename Iterator>
                FilterIterator<Iterator, AZStd::random_access_iterator_tag>::FilterIterator(Iterator iterator, Iterator begin, Iterator end, const typename super::Predicate& predicate)
                    : super(iterator, begin, end, predicate)
                {
                }

                template<typename Iterator>
                FilterIterator<Iterator, AZStd::random_access_iterator_tag>::FilterIterator()
                    : super()
                {
                }

                //
                // Continuous Random Access iterator
                //
                template<typename Iterator>
                FilterIterator<Iterator, AZStd::contiguous_iterator_tag>::FilterIterator(
                    Iterator iterator, Iterator end, const typename super::Predicate& predicate)
                    : super(iterator, end, predicate)
                {
                }

                template<typename Iterator>
                FilterIterator<Iterator, AZStd::contiguous_iterator_tag>::FilterIterator(
                    Iterator iterator, Iterator begin, Iterator end, const typename super::Predicate& predicate)
                    : super(iterator, begin, end, predicate)
                {
                }

                template<typename Iterator>
                FilterIterator<Iterator, AZStd::contiguous_iterator_tag>::FilterIterator()
                    : super()
                {
                }

                //
                // Utility functions
                //
                namespace Internal
                {
                    template<typename Iterator>
                    View<FilterIterator<Iterator> > MakeFilterView(Iterator current, Iterator end,
                        const typename FilterIterator<Iterator>::Predicate& predicate, AZStd::false_type)
                    {
                        return View<FilterIterator<Iterator> >(
                            FilterIterator<Iterator>(current, end, predicate),
                            FilterIterator<Iterator>(end, end, predicate));
                    }

                    template<typename Iterator>
                    View<FilterIterator<Iterator> > MakeFilterView(Iterator current, Iterator end,
                        const typename FilterIterator<Iterator>::Predicate& predicate, AZStd::true_type)
                    {
                        return View<FilterIterator<Iterator> >(
                            FilterIterator<Iterator>(current, current, end, predicate),
                            FilterIterator<Iterator>(end, current, end, predicate));
                    }
                }

                template<typename Iterator>
                FilterIterator<Iterator> MakeFilterIterator(Iterator current, Iterator end, const typename FilterIterator<Iterator, void>::Predicate& predicate)
                {
                    return FilterIterator<Iterator>(current, end, predicate);
                }

                template<typename Iterator, typename AZStd::enable_if<Internal::FilterIteratorNeedsFullRange<Iterator>::value>::type>
                FilterIterator<Iterator> MakeFilterIterator(Iterator current, Iterator begin, Iterator end, const typename FilterIterator<Iterator, void>::Predicate& predicate)
                {
                    return FilterIterator<Iterator>(current, begin, end, predicate);
                }

                template<typename Iterator>
                View<FilterIterator<Iterator> > MakeFilterView(Iterator current, Iterator end, const typename FilterIterator<Iterator, void>::Predicate& predicate)
                {
                    return Internal::MakeFilterView(current, end, predicate,
                        typename AZStd::is_base_of<AZStd::bidirectional_iterator_tag, typename AZStd::iterator_traits<Iterator>::iterator_category>::type());
                }

                template<typename Iterator, typename AZStd::enable_if<Internal::FilterIteratorNeedsFullRange<Iterator>::value>::type>
                View<FilterIterator<Iterator> > MakeFilterView(Iterator current, Iterator begin, Iterator end, const typename FilterIterator<Iterator>::Predicate& predicate)
                {
                    return View<FilterIterator<Iterator> >(
                        FilterIterator<Iterator>(current, begin, end, predicate),
                        FilterIterator<Iterator>(end, begin, end, predicate));
                }

                template<typename ViewType>
                View<FilterIterator<typename ViewType::iterator> > MakeFilterView(ViewType& view, const typename FilterIterator<typename ViewType::iterator>::Predicate& predicate)
                {
                    return MakeFilterView(view.begin(), view.end(), predicate);
                }

                template<typename ViewType>
                const View<FilterIterator<typename ViewType::const_iterator> > MakeFilterView(const ViewType& view, const typename FilterIterator<typename ViewType::const_iterator>::Predicate& predicate)
                {
                    return MakeFilterView(view.begin(), view.end(), predicate);
                }
               
            } // Views
        } // Containers
    } // SceneAPI
} // AZ
