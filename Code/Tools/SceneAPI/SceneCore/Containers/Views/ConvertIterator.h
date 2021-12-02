#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/iterator.h>
#include <AzCore/std/typetraits/is_pointer.h>
#include <AzCore/std/typetraits/is_reference.h>
#include <AzCore/std/typetraits/remove_reference.h>
#include <AzCore/std/utils.h>
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
                // Transform allows the value of the given iterator to be converted to the assigned TransformType when dereferencing.
                // A typical use case would be to change a "const shared_ptr<type>" to "const shared<const type>" for read-only
                // iteration of a vector<shared_ptr<type>>.
                // template argument Iterator: the base iterator which will be used as the given iterator.
                // template argument TransformType: the target type the value of base iterator will be converted to.
                //                                  This requires the type stored in iterator to have conversion to this type.
                // template argument Category: the iterator classification. ConvertIterator will automatically derive this
                //                             from the given iterator and match its behavior.
                // Example:
                //      ConvertIterator<Vector<shared_ptr<Object>>::iterator, shared_ptr<const Object>>;
                //
                // WARNING
                //      Types used for conversion that are not convertible through a reference and pointer will return their result
                //      by-value instead of by-reference. This may cause some unexpected behavior the user should be aware of.

                template<typename Iterator>
                struct ConvertIteratorArgumentHelper
                {
                    using ArgumentType = typename AZStd::conditional<AZStd::is_pointer<
                                typename AZStd::iterator_traits<Iterator>::value_type>::value,
                                typename AZStd::iterator_traits<Iterator>::value_type,
                                typename AZStd::iterator_traits<Iterator>::reference>::type;
                };

                template<typename Iterator, typename Function>
                struct ConvertIteratorFunctionHelper
                {
                    using ArgumentType = typename ConvertIteratorArgumentHelper<Iterator>::ArgumentType;
                    using ReturnType = AZStd::invoke_result_t<Function, ArgumentType>;
                };

                template<typename Iterator, typename ReturnType>
                struct ConvertIteratorTypeHelper
                {
                    using ArgumentType = typename ConvertIteratorArgumentHelper<Iterator>::ArgumentType;
                    using Function = ReturnType(*)(ArgumentType);

                    using PointerConditionType = typename AZStd::is_reference<ReturnType>::type;

                    using reference = ReturnType;
                    using pointer = typename AZStd::conditional<AZStd::is_reference<ReturnType>::value,
                                typename AZStd::remove_reference<ReturnType>::type*, ProxyPointer<ReturnType> >::type;
                    using value_type = typename AZStd::remove_reference<ReturnType>::type;
                };


                template<typename Iterator, typename ReturnType, typename Category = typename AZStd::iterator_traits<Iterator>::iterator_category>
                class ConvertIterator
                {
                };

                //
                // Base iterator
                //
                template<typename Iterator, typename ReturnType>
                class ConvertIterator<Iterator, ReturnType, void>
                {
                public:
                    using value_type = typename ConvertIteratorTypeHelper<Iterator, ReturnType>::value_type;
                    using difference_type = typename AZStd::iterator_traits<Iterator>::difference_type;
                    using pointer = typename ConvertIteratorTypeHelper<Iterator, ReturnType>::pointer;
                    using reference = typename ConvertIteratorTypeHelper<Iterator, ReturnType>::reference;
                    using iterator_category = typename AZStd::iterator_traits<Iterator>::iterator_category;

                    using Function = typename ConvertIteratorTypeHelper<Iterator, ReturnType>::Function;

                    using RootIterator = ConvertIterator<Iterator, ReturnType, iterator_category>;

                    ConvertIterator(Iterator iterator, Function converter);
                    ConvertIterator(const ConvertIterator&) = default;

                    ConvertIterator& operator=(const ConvertIterator&) = default;

                    RootIterator& operator++();
                    RootIterator operator++(int);

                    const Iterator& GetBaseIterator() const;
                protected:
                    Iterator m_iterator;
                    Function m_converter;
                };

                //
                // input_iterator_tag
                //
                template<typename Iterator, typename ReturnType>
                class ConvertIterator<Iterator, ReturnType, AZStd::input_iterator_tag>
                    : public ConvertIterator<Iterator, ReturnType, void>
                {
                public:
                    using super = ConvertIterator<Iterator, ReturnType, void>;
                    
                    ConvertIterator(Iterator iterator, typename super::Function converter);
                    ConvertIterator(const ConvertIterator&) = default;

                    typename super::reference operator*() const;
                    typename super::pointer operator->() const;

                    bool operator==(const typename super::RootIterator& rhs) const;
                    bool operator!=(const typename super::RootIterator& rhs) const;

                protected:
                    // Used for pointer casts that don't require an intermediate value.
                    typename super::pointer GetPointer(AZStd::true_type castablePointer) const;
                    // Used for pointer casts that require an intermediate proxy object.
                    typename super::pointer GetPointer(AZStd::false_type intermediateValue) const;
                };

                //
                // forward_iterator_tag
                //
                template<typename Iterator, typename ReturnType>
                class ConvertIterator<Iterator, ReturnType, AZStd::forward_iterator_tag>
                    : public ConvertIterator<Iterator, ReturnType, AZStd::input_iterator_tag>
                {
                public:
                    using super = ConvertIterator<Iterator, ReturnType, AZStd::input_iterator_tag>;
                    
                    ConvertIterator(Iterator iterator, typename super::Function converter);
                    ConvertIterator(const ConvertIterator&) = default;
                    ConvertIterator();
                };

                //
                // bidirectional_iterator_tag
                //
                template<typename Iterator, typename ReturnType>
                class ConvertIterator<Iterator, ReturnType, AZStd::bidirectional_iterator_tag>
                    : public ConvertIterator<Iterator, ReturnType, AZStd::forward_iterator_tag>
                {
                public:
                    using super = ConvertIterator<Iterator, ReturnType, AZStd::forward_iterator_tag>;
                    
                    ConvertIterator(Iterator iterator, typename super::Function converter);
                    ConvertIterator(const ConvertIterator&) = default;
                    ConvertIterator();

                    typename super::RootIterator& operator--();
                    typename super::RootIterator operator--(int);
                };

                //
                // random_access_iterator_tag
                //
                template<typename Iterator, typename ReturnType>
                class ConvertIterator<Iterator, ReturnType, AZStd::random_access_iterator_tag>
                    : public ConvertIterator<Iterator, ReturnType, AZStd::bidirectional_iterator_tag>
                {
                public:
                    using super = ConvertIterator<Iterator, ReturnType, AZStd::bidirectional_iterator_tag>;
                    
                    ConvertIterator(Iterator iterator, typename super::Function converter);
                    ConvertIterator(const ConvertIterator&) = default;
                    ConvertIterator();

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

                // contiguous_iterator_tag
                template<typename Iterator, typename ReturnType>
                class ConvertIterator<Iterator, ReturnType, AZStd::contiguous_iterator_tag>
                    : public ConvertIterator<Iterator, ReturnType, AZStd::random_access_iterator_tag>
                {
                public:
                    using super = ConvertIterator<Iterator, ReturnType, AZStd::random_access_iterator_tag>;
                    
                    ConvertIterator(Iterator iterator, typename super::Function converter);
                    ConvertIterator(const ConvertIterator&) = default;
                    ConvertIterator();
                };

                // Utility functions
                template<typename Iterator, typename Function>
                ConvertIterator<Iterator, typename ConvertIteratorFunctionHelper<Iterator, Function>::ReturnType>
                MakeConvertIterator(Iterator iterator, Function converter);

                template<typename Iterator, typename Function>
                View<ConvertIterator<Iterator, typename ConvertIteratorFunctionHelper<Iterator, Function>::ReturnType>>
                MakeConvertView(Iterator begin, Iterator end, Function converter);

                template<typename ViewType, typename Function>
                View<ConvertIterator<typename ViewType::iterator, typename ConvertIteratorFunctionHelper<typename ViewType::iterator, Function>::ReturnType>>
                MakeConvertView(ViewType& view, Function converter);

                template<typename ViewType, typename Function>
                View<ConvertIterator<typename ViewType::const_iterator, typename ConvertIteratorFunctionHelper<typename ViewType::const_iterator, Function>::ReturnType>>
                MakeConvertView(const ViewType& view, Function converter);
            } // Views
        } // Containers
    } // SceneAPI
} // AZ

#include <SceneAPI/SceneCore/Containers/Views/ConvertIterator.inl>
