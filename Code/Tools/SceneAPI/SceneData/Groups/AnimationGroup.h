/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <SceneAPI/SceneCore/Containers/RuleContainer.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IAnimationGroup.h>


namespace AZ
{
    class ReflectContext;

    namespace SceneAPI
    {
        namespace SceneData
        {
            class AnimationGroup
                : public DataTypes::IAnimationGroup
            {
            public:
                AZ_RTTI(AnimationGroup, "{982E0030-8131-43E9-BA8C-23775A3B7219}", DataTypes::IAnimationGroup);
                AZ_CLASS_ALLOCATOR_DECL
                
                AnimationGroup();
                ~AnimationGroup() override = default;

                const AZStd::string& GetName() const override;
                void SetName(const AZStd::string& name);
                void SetName(AZStd::string&& name);
                const Uuid& GetId() const override;
                void OverrideId(const Uuid& id);

                Containers::RuleContainer& GetRuleContainer() override;
                const Containers::RuleContainer& GetRuleContainerConst() const override;

                const AZStd::string& GetSelectedRootBone() const override;
                uint32_t GetStartFrame() const override;
                uint32_t GetEndFrame() const override;
                const float GetDefaultCompressionStrength() const override;
                const DataTypes::IAnimationGroup::PerBoneCompressionList& GetPerBoneCompression() const override;
                void SetSelectedRootBone(const AZStd::string& selectedRootBone) override;
                void SetStartFrame(uint32_t frame) override;
                void SetEndFrame(uint32_t frame) override;

                static void Reflect(ReflectContext* context);
                static bool VersionConverter(SerializeContext& context, SerializeContext::DataElementNode& classElement);

            protected:
                DataTypes::IAnimationGroup::PerBoneCompressionList m_perBoneCompression;
                Containers::RuleContainer  m_rules;
                AZStd::string m_selectedRootBone;
                AZStd::string m_name;
                Uuid m_id;
                uint32_t m_startFrame;
                uint32_t m_endFrame;
                float m_defaultCompressionStrength;
            };
        }
    }
}
