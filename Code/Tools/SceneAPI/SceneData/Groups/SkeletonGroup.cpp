/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneData/Groups/SkeletonGroup.h>
#include <SceneAPI/SceneData/GraphData/RootBoneData.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            AZ_CLASS_ALLOCATOR_IMPL(SkeletonGroup, SystemAllocator);

            SkeletonGroup::SkeletonGroup()
                : m_id(Uuid::CreateRandom())
            {
            }

            const AZStd::string& SkeletonGroup::GetName() const
            {
                return m_name;
            }

            void SkeletonGroup::SetName(const AZStd::string& name)
            {
                m_name = name;
            }

            void SkeletonGroup::SetName(AZStd::string&& name)
            {
                m_name = AZStd::move(name);
            }

            const Uuid& SkeletonGroup::GetId() const
            {
                return m_id;
            }

            void SkeletonGroup::OverrideId(const Uuid& id)
            {
                m_id = id;
            }

            Containers::RuleContainer& SkeletonGroup::GetRuleContainer()
            {
                return m_rules;
            }

            const Containers::RuleContainer& SkeletonGroup::GetRuleContainerConst() const
            {
                return m_rules;
            }
            
            const AZStd::string& SkeletonGroup::GetSelectedRootBone() const
            {
                return m_selectedRootBone;
            }

            void SkeletonGroup::SetSelectedRootBone(const AZStd::string& selectedRootBone)
            {
                m_selectedRootBone = selectedRootBone;
            }

            void SkeletonGroup::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<SkeletonGroup, DataTypes::ISkeletonGroup>()->Version(3, VersionConverter)
                    ->Field("name", &SkeletonGroup::m_name)
                    ->Field("selectedRootBone", &SkeletonGroup::m_selectedRootBone)
                    ->Field("rules", &SkeletonGroup::m_rules)
                    ->Field("id", &SkeletonGroup::m_id);

                EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<SkeletonGroup>("Skeleton group", "Name and configure a skeleton from your source file.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute("AutoExpand", true)
                            ->Attribute(Edit::Attributes::NameLabelOverride, "")
                        ->DataElement(AZ_CRC_CE("ManifestName"), &SkeletonGroup::m_name, "Name skeleton",
                            "Name the skeleton as you want it to appear in the Open 3D Engine Asset Browser.")
                            ->Attribute("FilterType", DataTypes::ISkeletonGroup::TYPEINFO_Uuid())
                        ->DataElement("NodeListSelection", &SkeletonGroup::m_selectedRootBone, "Select root bone", "Select the root bone of the skeleton.")
                            ->Attribute("ClassTypeIdFilter", AZ::SceneData::GraphData::RootBoneData::TYPEINFO_Uuid())
                        ->DataElement(Edit::UIHandlers::Default, &SkeletonGroup::m_rules, "", "Add or remove rules to fine-tune the export process.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC_CE("PropertyVisibility_ShowChildrenOnly"));
                }
            }


            bool SkeletonGroup::VersionConverter(SerializeContext& context, SerializeContext::DataElementNode& classElement)
            {
                const unsigned int version = classElement.GetVersion();

                bool result = true;

                // Replaced vector<IRule> with RuleContainer.
                if (version == 1)
                {
                    result = result && Containers::RuleContainer::VectorToRuleContainerConverter(context, classElement);
                }

                // Added a uuid "id" as the unique identifier to replace the file name.
                // Setting it to null by default and expecting a behavior to patch this when additional information is available.
                if (version <= 2)
                {
                    result = result && classElement.AddElementWithData<AZ::Uuid>(context, "id", AZ::Uuid::CreateNull()) != -1;
                }

                return result;
            }

        } // namespace SceneData
    } // namespace SceneAPI
} // namespace AZ
