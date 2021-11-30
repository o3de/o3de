/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        void MaterialPropertiesLayout::Reflect(ReflectContext* context)
        {            
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<MaterialPropertiesLayout>()
                    ->Version(1)
                    ->Field("Indexes", &MaterialPropertiesLayout::m_materialPropertyIndexes)
                    ->Field("Properties", &MaterialPropertiesLayout::m_materialPropertyDescriptors)
                    ;
            }

            IdReflectionMapForMaterialProperties::Reflect(context);
            MaterialPropertyOutputId::Reflect(context);
            MaterialPropertyDescriptor::Reflect(context);
        }

        size_t MaterialPropertiesLayout::GetPropertyCount() const
        {
            return m_materialPropertyDescriptors.size();
        }

        MaterialPropertyIndex MaterialPropertiesLayout::FindPropertyIndex(const Name& propertyId) const
        {
            return m_materialPropertyIndexes.Find(propertyId);
        }

        const MaterialPropertyDescriptor* MaterialPropertiesLayout::GetPropertyDescriptor(MaterialPropertyIndex index) const
        {
            if (index.IsValid() && index.GetIndex() < m_materialPropertyDescriptors.size())
            {
                return &m_materialPropertyDescriptors[index.GetIndex()];
            }
            else
            {
                return nullptr;
            }
        }

    } // namespace RPI
} // namespace AZ
