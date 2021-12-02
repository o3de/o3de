#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/typetraits/is_base_of.h>
#include <AzCore/std/typetraits/conditional.h>
#include <AzCore/std/utils.h>
#include <SceneAPI/SceneCore/DataTypes/IManifestObject.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/ConvertIterator.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            // Utility classes to construct common filters for scenes. These can be used in 
            //      algorithms such as find_if or the FilterIterator.
            // Example:
            //      auto result = AZStd::find_if(view.begin(), view.end(), DerivedTypeFilter<IMeshData>());

            namespace Internal
            {
                template<typename T, typename ObjectType, bool ExactType>
                struct TypeFilter
                {
                    bool operator()(const ObjectType& object) const;
                    bool operator()(const ObjectType* const object) const;
                    
                    bool operator()(const AZStd::shared_ptr<const ObjectType>& object) const;
                    bool operator()(const AZStd::shared_ptr<ObjectType>& object) const;

                    template<typename T1>
                    bool operator()(const AZStd::pair<T1, AZStd::shared_ptr<const ObjectType>>& object) const;
                    template<typename T1>
                    bool operator()(const AZStd::pair<T1, AZStd::shared_ptr<ObjectType>>& object) const;
                    template<typename T1>
                    bool operator()(const AZStd::pair<T1, const AZStd::shared_ptr<const ObjectType>&>& object) const;
                    template<typename T1>
                    bool operator()(const AZStd::pair<T1, const AZStd::shared_ptr<ObjectType>&>& object) const;
                    
                    template<typename T2>
                    bool operator()(const AZStd::pair<AZStd::shared_ptr<const ObjectType>, T2>& object) const;
                    template<typename T2>
                    bool operator()(const AZStd::pair<AZStd::shared_ptr<ObjectType>, T2>& object) const;
                    template<typename T2>
                    bool operator()(const AZStd::pair<const AZStd::shared_ptr<const ObjectType>&, T2>& object) const;
                    template<typename T2>
                    bool operator()(const AZStd::pair<const AZStd::shared_ptr<ObjectType>&, T2>& object) const;
                };

                template<typename T>
                struct TypeFilterBaseType
                {
                    static const bool s_isManifestObject = AZStd::is_base_of<DataTypes::IManifestObject, T>::value;
                    static const bool s_isGraphObject = AZStd::is_base_of<DataTypes::IGraphObject, T>::value;
                    
                    static_assert(s_isManifestObject || s_isGraphObject, "Target type is not derived from IManifestObject or IGraphObject.");

                    using type = typename AZStd::conditional<s_isManifestObject, DataTypes::IManifestObject, DataTypes::IGraphObject>::type;
                };

                template<typename T> struct IsConstPayload { static const bool value = AZStd::is_const<T>::value; };
                template<typename T> struct IsConstPayload<AZStd::shared_ptr<T>> { static const bool value = false; };
                template<typename T> struct IsConstPayload<AZStd::unique_ptr<T>> { static const bool value = false; };
                template<typename T> struct IsConstPayload<AZStd::shared_ptr<const T>> { static const bool value = true; };
                template<typename T> struct IsConstPayload<AZStd::unique_ptr<const T>> { static const bool value = true; };

                template<typename T, typename ViewType>
                struct ConversionType
                {
                    using type = typename AZStd::conditional<
                        IsConstPayload<typename AZStd::iterator_traits<typename ViewType::iterator>::value_type>::value,
                        const T, T>::type;
                };
            }

            // Filter object for any type derived from the given type or the type itself. The given type must
            //      itself derive from either IManifestObject or IGraphObject.
            // Example:
            //      auto result = AZStd::find_if(view.begin(), view.end(), DerivedTypeFilter<IMeshData>());
            template<typename T>
            struct DerivedTypeFilter
                : public Internal::TypeFilter<T, typename Internal::TypeFilterBaseType<T>::type, false>
            {
            };

            // Filter object for the given type. The given type must derive from either IManifestObject or 
            //      IGraphObject.
            // Example:
            //      auto view = Views::MakeFilterView(graph.GetContentStorage, ExactTypeFilter<MeshData>());
            template<typename T>
            struct ExactTypeFilter
                : public Internal::TypeFilter<T, typename Internal::TypeFilterBaseType<T>::type, true>
            {
            };

            // Compound view that returns all instances of the requested type and any types deriving from it.
            //      The given type must derive from either IManifestObject or IGraphObject.
            // Example:
            //      auto view = MakeDerivedFilterView<IMeshData>(graph.GetContentStorage());
            //      for (IMeshData& mesh : view)
            //      {
            //          ...
            //      }
            template<typename T, typename ViewType>
            Views::View<Views::ConvertIterator<typename Views::FilterIterator<typename ViewType::iterator>, 
                typename Internal::ConversionType<T, ViewType>::type&>> MakeDerivedFilterView(ViewType& view);
            template<typename T, typename ViewType>
            Views::View<Views::ConvertIterator<typename Views::FilterIterator<typename ViewType::const_iterator>, const T&>> MakeDerivedFilterView(const ViewType& view);

            // Compound view that returns all instances of the requested type.
            //      The given type must derive from either IManifestObject or IGraphObject.
            // Example:
            //      auto view = MakeExactFilterView<MeshData>(graph.GetContentStorage());
            //      for (MeshData& mesh : view)
            //      {
            //          ...
            //      }
            template<typename T, typename ViewType>
            Views::View<Views::ConvertIterator<typename Views::FilterIterator<typename ViewType::iterator>, 
                typename Internal::ConversionType<T, ViewType>::type&>> MakeExactFilterView(ViewType& view);
            template<typename T, typename ViewType>
            Views::View<Views::ConvertIterator<typename Views::FilterIterator<typename ViewType::const_iterator>, const T&>> MakeExactFilterView(const ViewType& view);
        } // Containers
    } // SceneAPI
} // AZ

#include <SceneAPI/SceneCore/Containers/Utilities/Filters.inl>
