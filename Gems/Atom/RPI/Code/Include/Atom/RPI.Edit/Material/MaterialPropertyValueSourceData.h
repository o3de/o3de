/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <Atom/RPI.Edit/Configuration.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>

namespace AZ
{
    namespace RPI
    {
        class JsonMaterialPropertyValueSourceDataSerializer;

        //! The class to store a material property value.
        //! If this object was loaded from JSON, Resolve() must be called before the value can be read;
        //! otherwise, SetValue() must be called instead.
        class ATOM_RPI_EDIT_API MaterialPropertyValueSourceData final
        {
            friend class JsonMaterialPropertyValueSourceDataSerializer;
        public:
            AZ_RTTI(AZ::RPI::MaterialPropertyValueSourceData, "{BC6D3B5A-F562-42F2-B806-A356C4FE4BDB}");
            AZ_CLASS_ALLOCATOR(MaterialPropertyValueSourceData, AZ::SystemAllocator);

            static void Reflect(ReflectContext* context);

            //! Resolve the actual property value and type among all candidates. Const qualifier applies here because
            //! this function does not change the true value it represents, it only finalizes the value. See also mutable m_resolvedValue.
            //! @param  propertiesLayout the material property layout that this property value belongs to.
            //! @param  materialPropertyName the name of the material property associated with this property value.
            //! @return if resolving is successful.
            bool Resolve(const MaterialPropertiesLayout& propertiesLayout, const AZ::Name& materialPropertyName) const;

            //! Check whether the value has been resolved.
            bool IsResolved() const;

            //! Get the current value. The value must be resolved before calling GetValue.
            const MaterialPropertyValue& GetValue() const;

            //! Set the current value.
            void SetValue(const MaterialPropertyValue& value);

            //! Compare if two values are potentially equal or similar.
            //! If they are both resolved, compare the resolved values. Otherwise, use matched possible source values.
            static bool AreSimilar(const MaterialPropertyValueSourceData& lhs, const MaterialPropertyValueSourceData& rhs);

        private:
            //! The resolved value with a valid type of a property. It needs to be mutable to allow post-resolving when parent objects are declared as const.
            mutable MaterialPropertyValue m_resolvedValue;
            //! Candidate values from serialization.
            AZStd::map<AZ::TypeId, MaterialPropertyValue> m_possibleValues; 
        };
    } // namespace RPI
} // namespace AZ
