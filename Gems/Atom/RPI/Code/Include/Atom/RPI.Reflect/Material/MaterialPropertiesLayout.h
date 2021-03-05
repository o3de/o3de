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
#pragma once

#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>
#include <Atom/RHI.Reflect/NameIdReflectionMap.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <Atom/RHI.Reflect/Base.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>

namespace AZ
{
    namespace RPI
    {
        //! Provides a set of MaterialPropertyDescriptors which define the topology for a material.
        class MaterialPropertiesLayout
            : public AZStd::intrusive_base
        {
            friend class MaterialTypeAssetCreator;
        public:
            AZ_TYPE_INFO(MaterialPropertiesLayout, "{0CBBC21F-700A-45AD-84FF-67B0210E79CA}");
            AZ_CLASS_ALLOCATOR(MaterialPropertiesLayout, SystemAllocator, 0);

            using PropertyList = AZStd::vector<MaterialPropertyDescriptor>;

            static void Reflect(ReflectContext* context);

            MaterialPropertiesLayout() = default;
            AZ_DISABLE_COPY_MOVE(MaterialPropertiesLayout);

            size_t GetPropertyCount() const;
            MaterialPropertyIndex FindPropertyIndex(const Name& propertyName) const;
            const MaterialPropertyDescriptor* GetPropertyDescriptor(MaterialPropertyIndex index) const;

        private:
            using IdReflectionMapForMaterialProperties = RHI::NameIdReflectionMap<MaterialPropertyIndex>;
            IdReflectionMapForMaterialProperties m_materialPropertyIndexes;
            PropertyList m_materialPropertyDescriptors;
        };

    } // namespace RPI
} // namespace AZ
