/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Trace.h>

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
                template<typename Iterator, typename ReturnType>
                ConvertIterator<Iterator, ReturnType, void>::ConvertIterator(Iterator iterator, Function converter)
                    : m_iterator(iterator)
                    , m_converter(converter)
                {
                }

                template<typename Iterator, typename ReturnType>
                auto ConvertIterator<Iterator, ReturnType, void>::operator++()->RootIterator &
                {
                    ++m_iterator;
                    return *static_cast<RootIterator*>(this);
                }

                template<typename Iterator, typename ReturnType>
                auto ConvertIterator<Iterator, ReturnType, void>::operator++(int)->RootIterator
                {
                    RootIterator result = *static_cast<RootIterator*>(this);
                    ++m_iterator;
                    return result;
                }

                template<typename Iterator, typename ReturnType>
                const Iterator& ConvertIterator<Iterator, ReturnType, void>::GetBaseIterator() const
                {
                    return m_iterator;
                }

                //
                // input_iterator_tag
                //
                template<typename Iterator, typename ReturnType>
                ConvertIterator<Iterator, ReturnType, AZStd::input_iterator_tag>::ConvertIterator(Iterator iterator, typename super::Function converter)
                    : super(iterator, converter)
                {
                }

                template<typename Iterator, typename ReturnType>
                auto ConvertIterator<Iterator, ReturnType, AZStd::input_iterator_tag>::operator*() const->typename super::reference
                {
                    AZ_Assert(super::m_converter, "No valid conversion function set for ConvertIterator.");
                    return super::m_converter(*super::m_iterator);
                }

                template<typename Iterator, typename ReturnType>
                auto ConvertIterator<Iterator, ReturnType, AZStd::input_iterator_tag>::GetPointer(
                    [[maybe_unused]] AZStd::true_type castablePointer) const->typename super::pointer
                {
                    AZ_Assert(super::m_converter, "No valid conversion function set for ConvertIterator.");
                    return &(super::m_converter(*super::m_iterator));
                }

                template<typename Iterator, typename ReturnType>
                auto ConvertIterator<Iterator, ReturnType, AZStd::input_iterator_tag>::GetPointer(
                    [[maybe_unused]] AZStd::false_type intermediateValue) const->typename super::pointer
                {
                    AZ_Assert(super::m_converter, "No valid conversion function set for ConvertIterator.");
                    return typename super::pointer(super::m_converter(*super::m_iterator));
                }

                template<typename Iterator, typename ReturnType>
                auto ConvertIterator<Iterator, ReturnType, AZStd::input_iterator_tag>::operator->() const->typename super::pointer
                {
                    return GetPointer(typename ConvertIteratorTypeHelper<Iterator, ReturnType>::PointerConditionType());
                }

                template<typename Iterator, typename ReturnType>
                bool ConvertIterator<Iterator, ReturnType, AZStd::input_iterator_tag>::operator==(const typename super::RootIterator& rhs) const
                {
                    return super::m_iterator == rhs.m_iterator;
                }
                template<typename Iterator, typename ReturnType>
                bool ConvertIterator<Iterator, ReturnType, AZStd::input_iterator_tag>::operator!=(const typename super::RootIterator& rhs) const
                {
                    return super::m_iterator != rhs.m_iterator;
                }

                //
                // forward_iterator_tag
                //
                template<typename Iterator, typename ReturnType>
                ConvertIterator<Iterator, ReturnType, AZStd::forward_iterator_tag>::ConvertIterator(Iterator iterator, typename super::Function converter)
                    : super(iterator, converter)
                {
                }

                template<typename Iterator, typename ReturnType>
                ConvertIterator<Iterator, ReturnType, AZStd::forward_iterator_tag>::ConvertIterator()
                    : super(Iterator(), nullptr)
                {
                }

                //
                // bidirectional_iterator_tag
                //
                template<typename Iterator, typename ReturnType>
                ConvertIterator<Iterator, ReturnType, AZStd::bidirectional_iterator_tag>::ConvertIterator(Iterator iterator, typename super::Function converter)
                    : super(iterator, converter)
                {
                }

                template<typename Iterator, typename ReturnType>
                ConvertIterator<Iterator, ReturnType, AZStd::bidirectional_iterator_tag>::ConvertIterator()
                    : super()
                {
                }

                template<typename Iterator, typename ReturnType>
                auto ConvertIterator<Iterator, ReturnType, AZStd::bidirectional_iterator_tag>::operator--()->typename super::RootIterator &
                {
                    --super::m_iterator;
                    return *static_cast<typename super::RootIterator*>(this);
                }

                template<typename Iterator, typename ReturnType>
                auto ConvertIterator<Iterator, ReturnType, AZStd::bidirectional_iterator_tag>::operator--(int)->typename super::RootIterator
                {
                    typename super::RootIterator result = *static_cast<typename super::RootIterator*>(this);
                    --super::m_iterator;
                    return result;
                }

                //
                // random_access_iterator_tag
                //
                template<typename Iterator, typename ReturnType>
                ConvertIterator<Iterator, ReturnType, AZStd::random_access_iterator_tag>::ConvertIterator(Iterator iterator, typename super::Function converter)
                    : super(iterator, converter)
                {
                }

                template<typename Iterator, typename ReturnType>
                ConvertIterator<Iterator, ReturnType, AZStd::random_access_iterator_tag>::ConvertIterator()
                    : super()
                {
                }

                template<typename Iterator, typename ReturnType>
                auto ConvertIterator<Iterator, ReturnType, AZStd::random_access_iterator_tag>::operator[](size_t index)->typename super::reference
                {
                    AZ_Assert(super::m_converter, "No valid conversion function set for ConvertIterator.");
                    return super::m_converter(super::m_iterator[index]);
                }

                template<typename Iterator, typename ReturnType>
                bool ConvertIterator<Iterator, ReturnType, AZStd::random_access_iterator_tag>::operator<(const typename super::RootIterator& rhs) const
                {
                    return super::m_iterator < rhs.m_iterator;
                }
                template<typename Iterator, typename ReturnType>
                bool ConvertIterator<Iterator, ReturnType, AZStd::random_access_iterator_tag>::operator>(const typename super::RootIterator& rhs) const
                {
                    return super::m_iterator > rhs.m_iterator;
                }
                template<typename Iterator, typename ReturnType>
                bool ConvertIterator<Iterator, ReturnType, AZStd::random_access_iterator_tag>::operator<=(const typename super::RootIterator& rhs) const
                {
                    return super::m_iterator <= rhs.m_iterator;
                }
                template<typename Iterator, typename ReturnType>
                bool ConvertIterator<Iterator, ReturnType, AZStd::random_access_iterator_tag>::operator>=(const typename super::RootIterator& rhs) const
                {
                    return super::m_iterator >= rhs.m_iterator;
                }

                template<typename Iterator, typename ReturnType>
                auto ConvertIterator<Iterator, ReturnType, AZStd::random_access_iterator_tag>::operator+(size_t n) const->typename super::RootIterator
                {
                    typename super::RootIterator result = *static_cast<const typename super::RootIterator*>(this);
                    result.m_iterator += n;
                    return result;
                }

                template<typename Iterator, typename ReturnType>
                auto ConvertIterator<Iterator, ReturnType, AZStd::random_access_iterator_tag>::operator-(size_t n) const->typename super::RootIterator
                {
                    typename super::RootIterator result = *static_cast<const typename super::RootIterator*>(this);
                    result.m_iterator -= n;
                    return result;
                }

                template<typename Iterator, typename ReturnType>
                auto ConvertIterator<Iterator, ReturnType, AZStd::random_access_iterator_tag>::operator-(const typename super::RootIterator& rhs) const->typename super::difference_type
                {
                    return super::m_iterator - rhs.m_iterator;
                }

                template<typename Iterator, typename ReturnType>
                auto ConvertIterator<Iterator, ReturnType, AZStd::random_access_iterator_tag>::operator+=(size_t n)->typename super::RootIterator &
                {
                    super::m_iterator += n;
                    return *static_cast<typename super::RootIterator*>(this);
                }

                template<typename Iterator, typename ReturnType>
                auto ConvertIterator<Iterator, ReturnType, AZStd::random_access_iterator_tag>::operator-=(size_t n)->typename super::RootIterator &
                {
                    super::m_iterator -= n;
                    return *static_cast<typename super::RootIterator*>(this);
                }

                //
                // contiguous_iterator_tag
                //
                template<typename Iterator, typename ReturnType>
                ConvertIterator<Iterator, ReturnType, AZStd::contiguous_iterator_tag>::ConvertIterator(Iterator iterator, typename super::Function converter)
                    : super(iterator, converter)
                {
                }

                template<typename Iterator, typename ReturnType>
                ConvertIterator<Iterator, ReturnType, AZStd::contiguous_iterator_tag>::ConvertIterator()
                    : super()
                {
                }

                // Utility functions
                template<typename Iterator, typename Function>
                ConvertIterator<Iterator, typename ConvertIteratorFunctionHelper<Iterator, Function>::ReturnType>
                MakeConvertIterator(Iterator iterator, Function converter)
                {
                    using ReturnType = typename ConvertIteratorFunctionHelper<Iterator, Function>::ReturnType;
                    return ConvertIterator<Iterator, ReturnType>(iterator, converter);
                }

                template<typename Iterator, typename Function>
                View<ConvertIterator<Iterator, typename ConvertIteratorFunctionHelper<Iterator, Function>::ReturnType>>
                MakeConvertView(Iterator begin, Iterator end, Function converter)
                {
                    using ReturnType = typename ConvertIteratorFunctionHelper<Iterator, Function>::ReturnType;
                    return View<ConvertIterator<Iterator, ReturnType>>(
                        MakeConvertIterator(begin, converter),
                        MakeConvertIterator(end, converter));
                }

                template<typename ViewType, typename Function>
                View<ConvertIterator<typename ViewType::iterator, typename ConvertIteratorFunctionHelper<typename ViewType::iterator, Function>::ReturnType>>
                MakeConvertView(ViewType& view, Function converter)
                {
                    return MakeConvertView(view.begin(), view.end(), converter);
                }

                template<typename ViewType, typename Function>
                View<ConvertIterator<typename ViewType::const_iterator, typename ConvertIteratorFunctionHelper<typename ViewType::const_iterator, Function>::ReturnType>>
                MakeConvertView(const ViewType& view, Function converter)
                {
                    return MakeConvertView(view.begin(), view.end(), converter);
                }
            } // Views
        } // Containers
    } // SceneAPI
} // AZ
