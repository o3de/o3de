/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Trace.h>
#include <AzCore/std/typetraits/remove_pointer.h>
#include <AzCore/std/typetraits/remove_reference.h>

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
                template<typename FirstIterator, typename SecondIterator>
                PairIterator<FirstIterator, SecondIterator, void>::PairIterator(FirstIterator first, SecondIterator second)
                    : m_first(first)
                    , m_second(second)
                {
                }

                template<typename FirstIterator, typename SecondIterator>
                auto PairIterator<FirstIterator, SecondIterator, void>::operator++()->RootIterator &
                {
                    ++m_first;
                    ++m_second;
                    return *static_cast<RootIterator*>(this);
                }

                template<typename FirstIterator, typename SecondIterator>
                auto PairIterator<FirstIterator, SecondIterator, void>::operator++(int)->RootIterator
                {
                    RootIterator result = *static_cast<RootIterator*>(this);
                    ++m_first;
                    ++m_second;
                    return result;
                }

                template<typename FirstIterator, typename SecondIterator>
                auto PairIterator<FirstIterator, SecondIterator, void>::GetFirstIterator()->const FirstIterator&
                {
                    return m_first;
                }

                template<typename FirstIterator, typename SecondIterator>
                auto PairIterator<FirstIterator, SecondIterator, void>::GetSecondIterator()->const SecondIterator&
                {
                    return m_second;
                }

                //
                // Input iterator
                //
                template<typename FirstIterator, typename SecondIterator>
                PairIterator<FirstIterator, SecondIterator, AZStd::input_iterator_tag>::PairIterator(FirstIterator first, SecondIterator second)
                    : super(first, second)
                {
                }

                template<typename FirstIterator, typename SecondIterator>
                bool PairIterator<FirstIterator, SecondIterator, AZStd::input_iterator_tag>::operator==(const typename super::RootIterator& rhs) const
                {
                    return super::m_first == rhs.m_first && super::m_second == rhs.m_second;
                }

                template<typename FirstIterator, typename SecondIterator>
                bool PairIterator<FirstIterator, SecondIterator, AZStd::input_iterator_tag>::operator!=(const typename super::RootIterator& rhs) const
                {
                    return super::m_first != rhs.m_first || super::m_second != rhs.m_second;
                }

                template<typename FirstIterator, typename SecondIterator>
                auto PairIterator<FirstIterator, SecondIterator, AZStd::input_iterator_tag>::operator*() const ->typename super::reference
                {
                    return typename super::reference(*super::m_first, *super::m_second);
                }

                template<typename FirstIterator, typename SecondIterator>
                auto PairIterator<FirstIterator, SecondIterator, AZStd::input_iterator_tag>::operator->() const ->typename super::pointer
                {
                    return typename super::pointer(operator*());
                }

                //
                // Forward iterator 
                //
                template<typename FirstIterator, typename SecondIterator>
                PairIterator<FirstIterator, SecondIterator, AZStd::forward_iterator_tag>::PairIterator(FirstIterator first, SecondIterator second)
                    : super(first, second)
                {
                }

                template<typename FirstIterator, typename SecondIterator>
                PairIterator<FirstIterator, SecondIterator, AZStd::forward_iterator_tag>::PairIterator()
                    : super(FirstIterator(), SecondIterator())
                {
                }

                //
                // Bidirectional iterator
                //
                template<typename FirstIterator, typename SecondIterator>
                PairIterator<FirstIterator, SecondIterator, AZStd::bidirectional_iterator_tag>::PairIterator(FirstIterator first, SecondIterator second)
                    : super(first, second)
                {
                }

                template<typename FirstIterator, typename SecondIterator>
                PairIterator<FirstIterator, SecondIterator, AZStd::bidirectional_iterator_tag>::PairIterator()
                    : super()
                {
                }

                template<typename FirstIterator, typename SecondIterator>
                auto PairIterator<FirstIterator, SecondIterator, AZStd::bidirectional_iterator_tag>::operator--()->typename super::RootIterator &
                {
                    --super::m_first;
                    --super::m_second;
                    return *static_cast<typename super::RootIterator*>(this);
                }

                template<typename FirstIterator, typename SecondIterator>
                auto PairIterator<FirstIterator, SecondIterator, AZStd::bidirectional_iterator_tag>::operator--(int)->typename super::RootIterator
                {
                    typename super::RootIterator result = *static_cast<typename super::RootIterator*>(this);
                    --super::m_first;
                    --super::m_second;
                    return result;
                }

                //
                // Random Access iterator
                //
                template<typename FirstIterator, typename SecondIterator>
                PairIterator<FirstIterator, SecondIterator, AZStd::random_access_iterator_tag>::PairIterator(FirstIterator first, SecondIterator second)
                    : super(first, second)
                {
                }

                template<typename FirstIterator, typename SecondIterator>
                PairIterator<FirstIterator, SecondIterator, AZStd::random_access_iterator_tag>::PairIterator()
                    : super()
                {
                }

                template<typename FirstIterator, typename SecondIterator>
                auto PairIterator<FirstIterator, SecondIterator, AZStd::random_access_iterator_tag>::operator[](size_t index)->typename super::reference
                {
                    return typename super::reference(super::m_first[index], super::m_second[index]);
                }

                template<typename FirstIterator, typename SecondIterator>
                bool IsSmaller(const FirstIterator& lhsFirst, const FirstIterator& lhsSecond, const SecondIterator& rhsFirst, const SecondIterator& rhsSecond)
                {
                    return (lhsFirst < rhsFirst || (!(rhsFirst < lhsFirst) && lhsSecond < rhsSecond));
                }

                template<typename FirstIterator, typename SecondIterator>
                bool PairIterator<FirstIterator, SecondIterator, AZStd::random_access_iterator_tag>::operator<(const typename super::RootIterator& rhs) const
                {
                    return IsSmaller(super::m_first, super::m_second, rhs.m_first, rhs.m_second);
                }

                template<typename FirstIterator, typename SecondIterator>
                bool PairIterator<FirstIterator, SecondIterator, AZStd::random_access_iterator_tag>::operator>(const typename super::RootIterator& rhs) const
                {
                    return IsSmaller(rhs.m_first, rhs.m_second, super::m_first, super::m_second);
                }

                template<typename FirstIterator, typename SecondIterator>
                bool PairIterator<FirstIterator, SecondIterator, AZStd::random_access_iterator_tag>::operator<=(const typename super::RootIterator& rhs) const
                {
                    return !IsSmaller(rhs.m_first, rhs.m_second, super::m_first, super::m_second);
                }

                template<typename FirstIterator, typename SecondIterator>
                bool PairIterator<FirstIterator, SecondIterator, AZStd::random_access_iterator_tag>::operator>=(const typename super::RootIterator& rhs) const
                {
                    return !IsSmaller(super::m_first, super::m_second, rhs.m_first, rhs.m_second);
                }

                template<typename FirstIterator, typename SecondIterator>
                auto PairIterator<FirstIterator, SecondIterator, AZStd::random_access_iterator_tag>::operator+(size_t n) const->typename super::RootIterator
                {
                    typename super::RootIterator result = *static_cast<const typename super::RootIterator*>(this);
                    result.m_first += n;
                    result.m_second += n;
                    return result;
                }

                template<typename FirstIterator, typename SecondIterator>
                auto PairIterator<FirstIterator, SecondIterator, AZStd::random_access_iterator_tag>::operator-(size_t n) const->typename super::RootIterator
                {
                    typename super::RootIterator result = *static_cast<const typename super::RootIterator*>(this);
                    result.m_first -= n;
                    result.m_second -= n;
                    return result;
                }

                template<typename FirstIterator, typename SecondIterator>
                auto PairIterator<FirstIterator, SecondIterator, AZStd::random_access_iterator_tag>::operator-(const typename super::RootIterator& rhs) const->typename super::difference_type
                {
                    typename super::difference_type result = super::m_first - rhs.m_first;
                    AZ_Assert((super::m_second - rhs.m_second) == result, "First and second in PairIterator have gone out of lockstep.");
                    return result;
                }

                template<typename FirstIterator, typename SecondIterator>
                auto PairIterator<FirstIterator, SecondIterator, AZStd::random_access_iterator_tag>::operator+=(size_t n)->typename super::RootIterator &
                {
                    super::m_first += n;
                    super::m_second += n;
                    return *static_cast<typename super::RootIterator*>(this);
                }

                template<typename FirstIterator, typename SecondIterator>
                auto PairIterator<FirstIterator, SecondIterator, AZStd::random_access_iterator_tag>::operator-=(size_t n)->typename super::RootIterator &
                {
                    super::m_first -= n;
                    super::m_second -= n;
                    return *static_cast<typename super::RootIterator*>(this);
                }

                //
                // Contiguous Random Access iterator
                //
                template<typename FirstIterator, typename SecondIterator>
                PairIterator<FirstIterator, SecondIterator, AZStd::contiguous_iterator_tag>::PairIterator(
                    FirstIterator first, SecondIterator second)
                    : super(first, second)
                {
                }

                template<typename FirstIterator, typename SecondIterator>
                PairIterator<FirstIterator, SecondIterator, AZStd::contiguous_iterator_tag>::PairIterator()
                    : super()
                {
                }

                //
                // Utility functions
                //
                template<typename FirstIterator, typename SecondIterator>
                PairIterator<FirstIterator, SecondIterator> MakePairIterator(FirstIterator first, SecondIterator second)
                {
                    return PairIterator<FirstIterator, SecondIterator>(first, second);
                }

                template<typename FirstIterator, typename SecondIterator>
                View<PairIterator<FirstIterator, SecondIterator>>
                MakePairView(FirstIterator firstBegin, FirstIterator firstEnd, SecondIterator secondBegin, SecondIterator secondEnd)
                {
                    return View<PairIterator<FirstIterator, SecondIterator>>(
                        PairIterator<FirstIterator, SecondIterator>(firstBegin, secondBegin),
                        PairIterator<FirstIterator, SecondIterator>(firstEnd, secondEnd));
                }

                template<typename FirstView, typename SecondView>
                View<PairIterator<typename FirstView::iterator, typename SecondView::iterator>>
                MakePairView(FirstView& firstView, SecondView& secondView)
                {
                    return MakePairView(firstView.begin(), firstView.end(), secondView.begin(), secondView.end());
                }

                template<typename FirstView, typename SecondView>
                View<PairIterator<typename FirstView::const_iterator, typename SecondView::iterator>>
                MakePairView(const FirstView& firstView, SecondView& secondView)
                {
                    return MakePairView(firstView.begin(), firstView.end(), secondView.begin(), secondView.end());
                }

                template<typename FirstView, typename SecondView>
                View<PairIterator<typename FirstView::iterator, typename SecondView::const_iterator>>
                MakePairView(FirstView& firstView, const SecondView& secondView)
                {
                    return MakePairView(firstView.begin(), firstView.end(), secondView.begin(), secondView.end());
                }

                template<typename FirstView, typename SecondView>
                View<PairIterator<typename FirstView::const_iterator, typename SecondView::const_iterator>>
                MakePairView(const FirstView& firstView, const SecondView& secondView)
                {
                    return MakePairView(firstView.begin(), firstView.end(), secondView.begin(), secondView.end());
                }

                // iterator swap
                template<typename First, typename Second, typename Category>
                void iter_swap(const PairIterator<First, Second, Category>& lhs, const PairIterator<First, Second, Category>& rhs)
                {
                    auto tmpFirst = AZStd::move(lhs->first);
                    auto tmpSecond = AZStd::move(lhs->second);

                    lhs->first = AZStd::move(rhs->first);
                    lhs->second = AZStd::move(rhs->second);

                    rhs->first = AZStd::move(tmpFirst);
                    rhs->second = AZStd::move(tmpSecond);
                }
            } // Views
        } // Containers
    } // SceneAPI
} // AZ
