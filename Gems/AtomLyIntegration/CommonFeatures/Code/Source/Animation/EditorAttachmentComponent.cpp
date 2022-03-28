/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorAttachmentComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <LmbrCentral/Animation/SkeletalHierarchyRequestBus.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>

namespace AZ
{
    namespace Render
    {
        bool EditorAttachmentComponentVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() < 2)
            {
                float uniformScaleOffset = 1.0f;

                int scaleElementIndex = classElement.FindElement(AZ_CRC_CE("Scale Offset"));
                if (scaleElementIndex != -1)
                {
                    AZ::Vector3 oldScaleValue = AZ::Vector3::CreateOne();
                    AZ::SerializeContext::DataElementNode& dataElementNode = classElement.GetSubElement(scaleElementIndex);
                    if (dataElementNode.GetData<AZ::Vector3>(oldScaleValue))
                    {
                        uniformScaleOffset = oldScaleValue.GetMaxElement();
                    }
                    classElement.RemoveElement(scaleElementIndex);
                }

                classElement.AddElementWithData(context, "Uniform Scale Offset", uniformScaleOffset);
            }

            return true;
        }

        void EditorAttachmentComponent::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<EditorAttachmentComponent, EditorComponentBase>()
                    ->Version(2, &EditorAttachmentComponentVersionConverter)
                    ->Field("Target ID", &EditorAttachmentComponent::m_targetId)
                    ->Field("Target Bone Name", &EditorAttachmentComponent::m_targetBoneName)
                    ->Field("Position Offset", &EditorAttachmentComponent::m_positionOffset)
                    ->Field("Rotation Offset", &EditorAttachmentComponent::m_rotationOffset)
                    ->Field("Uniform Scale Offset", &EditorAttachmentComponent::m_uniformScaleOffset)
                    ->Field("Attached Initially", &EditorAttachmentComponent::m_attachedInitially)
                    ->Field("Scale Source", &EditorAttachmentComponent::m_scaleSource);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext
                        ->Class<EditorAttachmentComponent>(
                            "Attachment", "The Attachment component lets an entity attach to a bone on the skeleton of another entity")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Animation")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Attachment.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Attachment.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(
                            AZ::Edit::Attributes::HelpPageURL,
                            "https://o3de.org/docs/user-guide/components/reference/animation/attachment/")
                        ->DataElement(0, &EditorAttachmentComponent::m_targetId, "Target entity", "Attach to this entity.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorAttachmentComponent::OnTargetIdChanged)
                        ->DataElement(
                            AZ::Edit::UIHandlers::ComboBox, &EditorAttachmentComponent::m_targetBoneName, "Joint name",
                            "Attach to this joint on target entity.")
                        ->Attribute(AZ::Edit::Attributes::StringList, &EditorAttachmentComponent::GetTargetBoneOptions)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorAttachmentComponent::OnTargetBoneChanged)
                        ->DataElement(
                            0, &EditorAttachmentComponent::m_positionOffset, "Position offset", "Local position offset from target bone")
                        ->Attribute(AZ::Edit::Attributes::Suffix, "m")
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorAttachmentComponent::OnTargetOffsetChanged)
                        ->DataElement(
                            0, &EditorAttachmentComponent::m_rotationOffset, "Rotation offset", "Local rotation offset from target bone")
                        ->Attribute(AZ::Edit::Attributes::Suffix, "deg")
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                        ->Attribute(AZ::Edit::Attributes::Min, -AZ::RadToDeg(AZ::Constants::TwoPi))
                        ->Attribute(AZ::Edit::Attributes::Max, AZ::RadToDeg(AZ::Constants::TwoPi))
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorAttachmentComponent::OnTargetOffsetChanged)
                        ->DataElement(0, &EditorAttachmentComponent::m_uniformScaleOffset, "Scale offset", "Local scale offset from target entity")
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.001f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorAttachmentComponent::OnTargetOffsetChanged)
                        ->DataElement(
                            0, &EditorAttachmentComponent::m_attachedInitially, "Attached initially",
                            "Whether to attach to target upon activation.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorAttachmentComponent::OnAttachedInitiallyChanged)
                        ->DataElement(
                            AZ::Edit::UIHandlers::ComboBox, &EditorAttachmentComponent::m_scaleSource, "Scaling",
                            "How object scale should be determined. "
                            "Use world scale = Attached object is scaled in world space, Use target entity scale = Attached object adopts "
                            "scale of target entity., Use target bone scale = Attached object adopts scale of target entity/joint.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorAttachmentComponent::OnScaleSourceChanged)
                        ->EnumAttribute(AttachmentConfiguration::ScaleSource::WorldScale, "Use world scale")
                        ->EnumAttribute(AttachmentConfiguration::ScaleSource::TargetEntityScale, "Use target entity scale")
                        ->EnumAttribute(AttachmentConfiguration::ScaleSource::TargetBoneScale, "Use target bone scale");
                }
            }
        }

        void EditorAttachmentComponent::Activate()
        {
            Base::Activate();
            m_boneFollower.Activate(GetEntity(), CreateAttachmentConfiguration(), /*targetCanAnimate=*/true);
        }

        void EditorAttachmentComponent::Deactivate()
        {
            m_boneFollower.Deactivate();
            Base::Deactivate();
        }

        void EditorAttachmentComponent::BuildGameEntity(AZ::Entity* gameEntity)
        {
            AttachmentComponent* component = gameEntity->CreateComponent<AttachmentComponent>();
            if (component)
            {
                component->m_initialConfiguration = CreateAttachmentConfiguration();
            }
        }

        AttachmentConfiguration EditorAttachmentComponent::CreateAttachmentConfiguration() const
        {
            AttachmentConfiguration configuration;
            configuration.m_targetId = m_targetId;
            configuration.m_targetBoneName = m_targetBoneName;
            configuration.m_targetOffset = GetTargetOffset();
            configuration.m_attachedInitially = m_attachedInitially;
            configuration.m_scaleSource = m_scaleSource;
            return configuration;
        }

        AZ::Transform EditorAttachmentComponent::GetTargetOffset() const
        {
            AZ::Transform offset = AZ::ConvertEulerDegreesToTransform(m_rotationOffset);
            offset.SetTranslation(m_positionOffset);
            offset.MultiplyByUniformScale(m_uniformScaleOffset);
            return offset;
        }

        AZStd::vector<AZStd::string> EditorAttachmentComponent::GetTargetBoneOptions() const
        {
            AZStd::vector<AZStd::string> names;

            // insert blank entry, so user may choose to bind to NO bone.
            names.push_back("");

            // track whether currently-set bone is found
            bool currentTargetBoneFound = false;

            // Get character and iterate over bones
            AZ::u32 jointCount = 0;
            LmbrCentral::SkeletalHierarchyRequestBus::EventResult(jointCount, m_targetId, &LmbrCentral::SkeletalHierarchyRequests::GetJointCount);
            for (AZ::u32 jointIndex = 0; jointIndex < jointCount; ++jointIndex)
            {
                const char* name = nullptr;
                LmbrCentral::SkeletalHierarchyRequestBus::EventResult(name, m_targetId, &LmbrCentral::SkeletalHierarchyRequests::GetJointNameByIndex, jointIndex);
                if (name)
                {
                    names.push_back(name);

                    if (!currentTargetBoneFound)
                    {
                        currentTargetBoneFound = (m_targetBoneName == names.back());
                    }
                }
            }

            // If we never found currently-set bone name,
            // stick it at top of list, just in case user wants to keep it anyway
            if (!currentTargetBoneFound && !m_targetBoneName.empty())
            {
                names.insert(names.begin(), m_targetBoneName);
            }

            return names;
        }

        AZ::u32 EditorAttachmentComponent::OnTargetIdChanged()
        {
            // Warn about bad setups (it won't crash, but it's nice to handle this early)
            if (m_targetId == GetEntityId())
            {
                AZ_Warning(GetEntity()->GetName().c_str(), false, "AttachmentComponent cannot target self.") m_targetId.SetInvalid();
            }

            // Warn about children attaching to a parent
            AZ::EntityId parentOfTarget;
            AZ::TransformBus::EventResult(parentOfTarget, m_targetId, &AZ::TransformBus::Events::GetParentId);
            while (parentOfTarget.IsValid())
            {
                if (parentOfTarget == GetEntityId())
                {
                    AZ_Warning(
                        GetEntity()->GetName().c_str(), parentOfTarget != GetEntityId(), "AttachmentComponent cannot target child entity");
                    m_targetId.SetInvalid();
                    break;
                }

                AZ::EntityId currentParentId = parentOfTarget;
                parentOfTarget.SetInvalid();
                AZ::TransformBus::EventResult(parentOfTarget, currentParentId, &AZ::TransformBus::Events::GetParentId);
            }

            AttachOrDetachAsNecessary();

            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues; // refresh bone options
        }

        AZ::u32 EditorAttachmentComponent::OnTargetBoneChanged()
        {
            AttachOrDetachAsNecessary();
            return AZ::Edit::PropertyRefreshLevels::None;
        }

        AZ::u32 EditorAttachmentComponent::OnTargetOffsetChanged()
        {
            EBUS_EVENT_ID(GetEntityId(), LmbrCentral::AttachmentComponentRequestBus, SetAttachmentOffset, GetTargetOffset());
            return AZ::Edit::PropertyRefreshLevels::None;
        }

        AZ::u32 EditorAttachmentComponent::OnAttachedInitiallyChanged()
        {
            AttachOrDetachAsNecessary();
            return AZ::Edit::PropertyRefreshLevels::None;
        }

        AZ::u32 EditorAttachmentComponent::OnScaleSourceChanged()
        {
            m_boneFollower.Deactivate();
            m_boneFollower.Activate(GetEntity(), CreateAttachmentConfiguration(), false);
            return AZ::Edit::PropertyRefreshLevels::None;
        }

        void EditorAttachmentComponent::AttachOrDetachAsNecessary()
        {
            if (m_attachedInitially && m_targetId.IsValid())
            {
                EBUS_EVENT_ID(
                    GetEntityId(), LmbrCentral::AttachmentComponentRequestBus, Attach, m_targetId, m_targetBoneName.c_str(), GetTargetOffset());
            }
            else
            {
                EBUS_EVENT_ID(GetEntityId(), LmbrCentral::AttachmentComponentRequestBus, Detach);
            }
        }
    } // namespace Render
 } // namespace AZ
