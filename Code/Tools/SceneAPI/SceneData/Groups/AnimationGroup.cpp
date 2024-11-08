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
#include <SceneAPI/SceneData/Groups/AnimationGroup.h>
#include <SceneAPI/SceneData/GraphData/RootBoneData.h>

namespace AZ
{
    namespace SceneAPI
    {
        void DataTypes::IAnimationGroup::PerBoneCompression::Reflect(ReflectContext* context)
        {
            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);

            serializeContext->Class<IAnimationGroup::PerBoneCompression>()->Version(1)
                ->Field("boneNamePattern", &IAnimationGroup::PerBoneCompression::m_boneNamePattern)
                ->Field("compressionStrength", &IAnimationGroup::PerBoneCompression::m_compressionStrength)
                ;

            EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<IAnimationGroup::PerBoneCompression>("Compression Override", "Compression settings for an individual bone.")
                    ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute("AutoExpand", true)
                    ->DataElement("NodeListSelection", &IAnimationGroup::PerBoneCompression::m_boneNamePattern, "Bone name/pattern", "Bone name or pattern with wildcards, e.g. \"*arm*\".")
                        ->Attribute("ClassTypeIdFilter", AZ::SceneAPI::DataTypes::IBoneData::TYPEINFO_Uuid())
                        ->Attribute(AZ::Edit::Attributes::ComboBoxEditable, true)
                    ->DataElement(Edit::UIHandlers::Slider, &IAnimationGroup::PerBoneCompression::m_compressionStrength, "Strength", "Compression strength to use for the specified bone.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                        ;
            }
        }

        namespace SceneData
        {            
            AZ_CLASS_ALLOCATOR_IMPL(AnimationGroup, SystemAllocator);

            AnimationGroup::AnimationGroup()
                : m_id(Uuid::CreateRandom())
                , m_startFrame(0)
                , m_endFrame(0)
                , m_defaultCompressionStrength(0.1f)
            {
            }

            const AZStd::string& AnimationGroup::GetName() const
            {
                return m_name;
            }

            void AnimationGroup::SetName(const AZStd::string& name)
            {
                m_name = name;
            }

            void AnimationGroup::SetName(AZStd::string&& name)
            {
                m_name = AZStd::move(name);
            }

            const Uuid& AnimationGroup::GetId() const
            {
                return m_id;
            }

            void AnimationGroup::OverrideId(const Uuid& id)
            {
                m_id = id;
            }

            Containers::RuleContainer& AnimationGroup::GetRuleContainer()
            {
                return m_rules;
            }

            const Containers::RuleContainer& AnimationGroup::GetRuleContainerConst() const
            {
                return m_rules;
            }

            const AZStd::string& AnimationGroup::GetSelectedRootBone() const
            {
                return m_selectedRootBone;
            }

            uint32_t AnimationGroup::GetStartFrame() const
            {
                return m_startFrame;
            }

            uint32_t AnimationGroup::GetEndFrame() const
            {
                return m_endFrame;
            }

            const float AnimationGroup::GetDefaultCompressionStrength() const
            {
                return m_defaultCompressionStrength;
            }

            const DataTypes::IAnimationGroup::PerBoneCompressionList& AnimationGroup::GetPerBoneCompression() const
            {
                return m_perBoneCompression;
            }

            void AnimationGroup::SetSelectedRootBone(const AZStd::string& selectedRootBone)
            {
                m_selectedRootBone = selectedRootBone;
            }

            void AnimationGroup::SetStartFrame(uint32_t frame)
            {
                m_startFrame = frame;
            }

            void AnimationGroup::SetEndFrame(uint32_t frame)
            {
                m_endFrame = frame;
            }

            void AnimationGroup::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                DataTypes::IAnimationGroup::PerBoneCompression::Reflect(context);

                serializeContext->Class<AnimationGroup, DataTypes::IAnimationGroup>()->Version(3, VersionConverter)
                    ->Field("name", &AnimationGroup::m_name)
                    ->Field("id", &AnimationGroup::m_id)
                    ->Field("selectedRootBone", &AnimationGroup::m_selectedRootBone)
                    ->Field("startFrame", &AnimationGroup::m_startFrame)
                    ->Field("endFrame", &AnimationGroup::m_endFrame)
                    ->Field("defaultCompressionStrength", &AnimationGroup::m_defaultCompressionStrength)
                    ->Field("perBoneCompression", &AnimationGroup::m_perBoneCompression)
                    ->Field("rules", &AnimationGroup::m_rules)
                    ;

                EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<AnimationGroup>("Animation group", "Configure animation data exporting.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute("AutoExpand", true)
                            ->Attribute(Edit::Attributes::NameLabelOverride, "")
                            ->Attribute(AZ::Edit::Attributes::CategoryStyle, "display divider")
                            ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://www.o3de.org/docs/user-guide/assets/scene-settings/motions-tab/")
                        ->DataElement(AZ_CRC_CE("ManifestName"), &AnimationGroup::m_name, "Group name",
                            "Name for the group. This name will also be used as the name for the generated file.")
                            ->Attribute("FilterType", DataTypes::IAnimationGroup::TYPEINFO_Uuid())
                        ->DataElement("NodeListSelection", &AnimationGroup::m_selectedRootBone, "Select root bone", "The root bone of the animation that will be exported.")
                            ->Attribute("ClassTypeIdFilter", AZ::SceneAPI::DataTypes::IBoneData::TYPEINFO_Uuid())
                        ->DataElement(Edit::UIHandlers::Default, &AnimationGroup::m_startFrame, "Start frame", "The start frame of the animation that will be exported.")
                        ->DataElement(Edit::UIHandlers::Default, &AnimationGroup::m_endFrame, "End frame", "The end frame of the animation that will be exported.")
                        ->DataElement(Edit::UIHandlers::Default, &AnimationGroup::m_rules, "", "Add or remove rules to fine-tune the export process.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC_CE("PropertyVisibility_ShowChildrenOnly"))
                        ->ClassElement(Edit::ClassElements::Group, "Compression")
                            ->DataElement(Edit::UIHandlers::Slider, &AnimationGroup::m_defaultCompressionStrength, "Default strength", "Default compression strength to use by default for all bones.")
                                ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                                ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                            ->DataElement(Edit::UIHandlers::Default, &AnimationGroup::m_perBoneCompression, "Bone/group overrides", "Compression strength overrides for specific bones, or bone groups (using wildcards).")
                            ;
                }
            }


            bool AnimationGroup::VersionConverter(SerializeContext& context, SerializeContext::DataElementNode& classElement)
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
