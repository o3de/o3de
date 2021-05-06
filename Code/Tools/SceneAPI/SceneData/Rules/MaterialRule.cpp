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

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <SceneAPI/SceneData/Rules/MaterialRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            AZ_CLASS_ALLOCATOR_IMPL(MaterialRule, SystemAllocator, 0)

            MaterialRule::MaterialRule()
                : m_removeMaterials(false)
                , m_updateMaterials(false)
            {
            }

            bool MaterialRule::RemoveUnusedMaterials() const
            {
                return m_removeMaterials;
            }

            bool MaterialRule::UpdateMaterials() const
            {
                return m_updateMaterials;
            }

            void MaterialRule::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<MaterialRule, DataTypes::IMaterialRule>()->Version(2)
                    ->Field("updateMaterials", &MaterialRule::m_updateMaterials)
                    ->Field("removeMaterials", &MaterialRule::m_removeMaterials);

                EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<MaterialRule>("Material", "Determine whether to accept material updates from the source files.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute("AutoExpand", true)
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->DataElement(Edit::UIHandlers::Default, &MaterialRule::m_updateMaterials, "Update materials", "Checking this box will accept changes made in the source file into the Open 3D Engine asset.")
                        ->DataElement(Edit::UIHandlers::Default, &MaterialRule::m_removeMaterials, "Remove unused materials","Detects and removes material files from the game project that are not present in the source file.");
                }
            }
        } // namespace SceneData
    } // namespace SceneAPI
} // namespace AZ
