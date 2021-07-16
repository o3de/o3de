/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include "AttachmentComponent.h"

namespace AZ
{
    namespace Render
    {
        /*!
         * In-editor attachment component.
         * \ref AttachmentComponent
         */
        class EditorAttachmentComponent
            : public AzToolsFramework::Components::EditorComponentBase
        {
        private:
            using Base = AzToolsFramework::Components::EditorComponentBase;
        public:
            AZ_COMPONENT(EditorAttachmentComponent, "{DA6072FD-E696-47D8-81D9-1F77D3464200}", Base);
            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                AttachmentComponent::GetProvidedServices(provided);
            }

            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
            {
                AttachmentComponent::GetIncompatibleServices(incompatible);
            }

            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
            {
                AttachmentComponent::GetRequiredServices(required);
            }

            ~EditorAttachmentComponent() override = default;
            void BuildGameEntity(AZ::Entity* gameEntity) override;

        protected:

            //////////////////////////////////////////////////////////////////////////
            // AZ::Component interface implementation
            void Activate() override;
            void Deactivate() override;
            //////////////////////////////////////////////////////////////////////////

            AZ::u32 OnTargetIdChanged();
            AZ::u32 OnTargetBoneChanged();
            AZ::u32 OnTargetOffsetChanged();
            AZ::u32 OnAttachedInitiallyChanged();
            AZ::u32 OnScaleSourceChanged();

            //! Invoked when an attachment property changes
            void AttachOrDetachAsNecessary();

            //! For populating ComboBox
            AZStd::vector<AZStd::string> GetTargetBoneOptions() const;

            //! Create runtime configuration from editor configuration
            AttachmentConfiguration CreateAttachmentConfiguration() const;

            //! Create AZ::Transform from position and rotation
            AZ::Transform GetTargetOffset() const;

            //! Attach to this entity.
            AZ::EntityId m_targetId;

            //! Attach to this bone on target entity.
            AZStd::string m_targetBoneName;

            //! Offset from target bone's position.
            AZ::Vector3 m_positionOffset = AZ::Vector3::CreateZero();

            //! Offset from target bone's rotation.
            AZ::Vector3 m_rotationOffset = AZ::Vector3::CreateZero();

            //! Offset from target entity's scale.
            float m_uniformScaleOffset = 1.0f;

            //! Observe scale information from the specified source.       
            AttachmentConfiguration::ScaleSource m_scaleSource = AttachmentConfiguration::ScaleSource::WorldScale;

            //! Whether to attach to target upon activation.
            //! If false, the entity remains detached until Attach() is called.
            bool m_attachedInitially = true;

            //! Implements actual attachment functionality
            AZ::Render::BoneFollower m_boneFollower;
        };
    } // namespace Render
} // namespace AZ
