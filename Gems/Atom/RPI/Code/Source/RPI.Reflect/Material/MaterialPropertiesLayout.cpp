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

        MaterialPropertyIndex MaterialPropertiesLayout::FindPropertyIndex(const Name& propertyName) const
        {
            return m_materialPropertyIndexes.Find(propertyName);
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
