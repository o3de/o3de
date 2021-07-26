/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            namespace Internal
            {
                template<typename T, typename ObjectType, bool ExactType>
                bool TypeFilter<T, ObjectType, ExactType>::operator()(const ObjectType& object) const
                {
                    if (ExactType)
                    {
                        return object.RTTI_GetType() == T::TYPEINFO_Uuid();
                    }
                    else
                    {
                        return object.RTTI_IsTypeOf(T::TYPEINFO_Uuid());
                    }
                }

                template<typename T, typename ObjectType, bool ExactType>
                bool TypeFilter<T, ObjectType, ExactType>::operator()(const ObjectType* const object) const
                {
                    if (ExactType)
                    {
                        return object && object->RTTI_GetType() == T::TYPEINFO_Uuid();
                    }
                    else
                    {
                        return object && object->RTTI_IsTypeOf(T::TYPEINFO_Uuid());
                    }
                }

                template<typename T, typename ObjectType, bool ExactType>
                bool TypeFilter<T, ObjectType, ExactType>::operator()(const AZStd::shared_ptr<const ObjectType>& object) const
                {
                    if (ExactType)
                    {
                        return object && object->RTTI_GetType() == T::TYPEINFO_Uuid();
                    }
                    else
                    {
                        return object && object->RTTI_IsTypeOf(T::TYPEINFO_Uuid());
                    }
                }
                
                template<typename T, typename ObjectType, bool ExactType>
                bool TypeFilter<T, ObjectType, ExactType>::operator()(const AZStd::shared_ptr<ObjectType>& object) const
                {
                    if (ExactType)
                    {
                        return object && object->RTTI_GetType() == T::TYPEINFO_Uuid();
                    }
                    else
                    {
                        return object && object->RTTI_IsTypeOf(T::TYPEINFO_Uuid());
                    }
                }

                template<typename T, typename ObjectType, bool ExactType> template<typename T1>
                bool TypeFilter<T, ObjectType, ExactType>::operator()(const AZStd::pair<T1, AZStd::shared_ptr<const ObjectType>>& object) const
                {
                    if (ExactType)
                    {
                        return object.second && object.second->RTTI_GetType() == T::TYPEINFO_Uuid();
                    }
                    else
                    {
                        return object.second && object.second->RTTI_IsTypeOf(T::TYPEINFO_Uuid());
                    }
                }

                template<typename T, typename ObjectType, bool ExactType> template<typename T1>
                bool TypeFilter<T, ObjectType, ExactType>::operator()(const AZStd::pair<T1, AZStd::shared_ptr<ObjectType>>& object) const
                {
                    if (ExactType)
                    {
                        return object.second && object.second->RTTI_GetType() == T::TYPEINFO_Uuid();
                    }
                    else
                    {
                        return object.second && object.second->RTTI_IsTypeOf(T::TYPEINFO_Uuid());
                    }
                }

                template<typename T, typename ObjectType, bool ExactType> template<typename T1>
                bool TypeFilter<T, ObjectType, ExactType>::operator()(const AZStd::pair<T1, const AZStd::shared_ptr<const ObjectType>&>& object) const
                {
                    if (ExactType)
                    {
                        return object.second && object.second->RTTI_GetType() == T::TYPEINFO_Uuid();
                    }
                    else
                    {
                        return object.second && object.second->RTTI_IsTypeOf(T::TYPEINFO_Uuid());
                    }
                }

                template<typename T, typename ObjectType, bool ExactType> template<typename T1>
                bool TypeFilter<T, ObjectType, ExactType>::operator()(const AZStd::pair<T1, const AZStd::shared_ptr<ObjectType>&>& object) const
                {
                    if (ExactType)
                    {
                        return object.second && object.second->RTTI_GetType() == T::TYPEINFO_Uuid();
                    }
                    else
                    {
                        return object.second && object.second->RTTI_IsTypeOf(T::TYPEINFO_Uuid());
                    }
                }

                template<typename T, typename ObjectType, bool ExactType> template<typename T2>
                bool TypeFilter<T, ObjectType, ExactType>::operator()(const AZStd::pair<AZStd::shared_ptr<const ObjectType>, T2>& object) const
                {
                    if (ExactType)
                    {
                        return object.first && object.first->RTTI_GetType() == T::TYPEINFO_Uuid();
                    }
                    else
                    {
                        return object.first && object.first->RTTI_IsTypeOf(T::TYPEINFO_Uuid());
                    }
                }

                template<typename T, typename ObjectType, bool ExactType> template<typename T2>
                bool TypeFilter<T, ObjectType, ExactType>::operator()(const AZStd::pair<AZStd::shared_ptr<ObjectType>, T2>& object) const
                {
                    if (ExactType)
                    {
                        return object.first && object.first->RTTI_GetType() == T::TYPEINFO_Uuid();
                    }
                    else
                    {
                        return object.first && object.first->RTTI_IsTypeOf(T::TYPEINFO_Uuid());
                    }
                }

                template<typename T, typename ObjectType, bool ExactType> template<typename T2>
                bool TypeFilter<T, ObjectType, ExactType>::operator()(const AZStd::pair<const AZStd::shared_ptr<const ObjectType>&, T2>& object) const
                {
                    if (ExactType)
                    {
                        return object.first && object.first->RTTI_GetType() == T::TYPEINFO_Uuid();
                    }
                    else
                    {
                        return object.first && object.first->RTTI_IsTypeOf(T::TYPEINFO_Uuid());
                    }
                }

                template<typename T, typename ObjectType, bool ExactType> template<typename T2>
                bool TypeFilter<T, ObjectType, ExactType>::operator()(const AZStd::pair<const AZStd::shared_ptr<ObjectType>&, T2>& object) const
                {
                    if (ExactType)
                    {
                        return object.first && object.first->RTTI_GetType() == T::TYPEINFO_Uuid();
                    }
                    else
                    {
                        return object.first && object.first->RTTI_IsTypeOf(T::TYPEINFO_Uuid());
                    }
                }
            } // Internal

            template<typename T, typename ViewType>
            Views::View<Views::ConvertIterator<typename Views::FilterIterator<typename ViewType::iterator>, 
                typename Internal::ConversionType<T, ViewType>::type&>> MakeDerivedFilterView(ViewType& view)
            {
                auto filterView = Views::MakeFilterView(view, DerivedTypeFilter<T>());
                auto convertView = Views::MakeConvertView(filterView,
                    // Note that by default begin() returns a reference to the value, so the argument will
                    //      already be by-reference so an extra reference doesn't need to be added and this
                    //      function isn't being called by value.
                    [](decltype(*filterView.begin()) instance) -> typename Internal::ConversionType<T, ViewType>::type&
                    {
                        AZ_Assert(instance.get(), "Null pointer encountered.");
                        typename Internal::ConversionType<T, ViewType>::type* result = 
                            azrtti_cast<typename Internal::ConversionType<T, ViewType>::type*>(instance.get());
                        AZ_Assert(result, "Unable to cast to target type.");
                        return *result;
                    }
                );
                return convertView;
            }

            template<typename T, typename ViewType>
            Views::View<Views::ConvertIterator<typename Views::FilterIterator<typename ViewType::const_iterator>, const T&>> MakeDerivedFilterView(const ViewType& view)
            {
                auto filterView = Views::MakeFilterView(view, DerivedTypeFilter<T>());
                auto convertView = Views::MakeConvertView(filterView,
                    // Note that by default begin() returns a reference to the value, so the argument will
                    //      already be by-reference so an extra reference doesn't need to be added and this
                    //      function isn't being called by value.
                    [](decltype(*filterView.begin()) instance) -> const T&
                    {
                        AZ_Assert(instance.get(), "Null pointer encountered.");
                        const T* result = azrtti_cast<const T*>(instance.get());
                        AZ_Assert(result, "Unable to cast to target type.");
                        return *result;
                    }
                );
                return convertView;
            }

            template<typename T, typename ViewType>
            Views::View<Views::ConvertIterator<typename Views::FilterIterator<typename ViewType::iterator>, 
                typename Internal::ConversionType<T, ViewType>::type&>> MakeExactFilterView(ViewType& view)
            {
                auto filterView = Views::MakeFilterView(view, ExactTypeFilter<T>());
                auto convertView = Views::MakeConvertView(filterView,
                    [](decltype(*filterView.begin()) instance) -> typename Internal::ConversionType<T, ViewType>::type&
                    {
                        AZ_Assert(instance.get(), "Null pointer encountered.");
                        typename Internal::ConversionType<T, ViewType>::type* result = 
                            azrtti_cast<typename Internal::ConversionType<T, ViewType>::type*>(instance.get());
                        AZ_Assert(result, "Unable to cast to target type.");
                        return *result;
                    }
                );
                return convertView;
            }

            template<typename T, typename ViewType>
            Views::View<Views::ConvertIterator<typename Views::FilterIterator<typename ViewType::const_iterator>, const T&>> MakeExactFilterView(const ViewType& view)
            {
                auto filterView = Views::MakeFilterView(view, ExactTypeFilter<T>());
                auto convertView = Views::MakeConvertView(filterView,
                    [](decltype(*filterView.begin())& instance) -> const T&
                    {
                        AZ_Assert(instance.get(), "Null pointer encountered.");
                        const T* result = azrtti_cast<const T*>(instance.get());
                        AZ_Assert(result, "Unable to cast to target type.");
                        return *result;
                    }
                );
                return convertView;
            }
        } // Containers
    } // SceneAPI
} // AZ
