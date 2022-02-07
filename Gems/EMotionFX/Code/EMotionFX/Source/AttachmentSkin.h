/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Attachment.h"
#include "EMotionFXConfig.h"
#include <AzCore/std/containers/vector.h>

namespace EMotionFX
{
    class ActorInstance;

    /**
     * The skin attachment class.
     * This represents an attachment that is influenced by multiple joints, skinned to the main skeleton of the actor it gets attached to.
     * An example could be if you want to put on some pair of pants on the character. This can be used to customize your characters.
     * So this attachment will basically copy the tranformations of the main character to the joints inside the actor instance that represents this attachment.
     */
    class EMFX_API AttachmentSkin
        : public Attachment
    {
        AZ_CLASS_ALLOCATOR_DECL
    public:
        enum
        {
            TYPE_ID = 0x00000002
        };

        /**
         * The joint map entry, which contains a link to a joint inside the actor instance you attach to.
         */
        struct EMFX_API JointMapping
        {
            size_t m_sourceJoint; /**< The source joint in the actor where this is attached to. */
            size_t m_targetJoint; /**< The target joint in the attachment actor instance. */
        };

        struct EMFX_API MorphMapping
        {
            size_t m_sourceMorphIndex; /**< The source morph target index. The source is the actor instance we are attaching to. */
            size_t m_targetMorphIndex; /**< The target morph target index. The target is the attachment actor instance. */
        };

        /**
         * Create the attachment that is influenced by multiple joints.
         * @param attachToActorInstance The actor instance to attach to, for example your main character.
         * @param attachment The actor instance that you want to attach to this actor instance, for example an actor instance that represents some new pants.
         */
        static AttachmentSkin* Create(ActorInstance* attachToActorInstance, ActorInstance* attachment);

        /**
         * Get the attachment type ID.
         * Every class inherited from this base class should have some TYPE ID.
         * @return The type ID of this attachment class.
         */
        uint32 GetType() const override { return TYPE_ID; }

        /**
         * Get the attachment type string.
         * Every class inherited from this base class should have some type ID string, which should be equal to the class name really.
         * @return The type string of this attachment class, which should be the class name.
         */
        const char* GetTypeString() const override { return "AttachmentSkin"; }

        /**
         * Check if this attachment is being influenced by multiple joints or not.
         * This is the case for attachments such as clothing items which get influenced by multiple joints inside the actor instance they are attached to.
         * @result Returns true if it is influenced by multiple joints, otherwise false is returned.
         */
        bool GetIsInfluencedByMultipleJoints() const override final { return true; }

        /**
         * Update the joint transforms of the attachment.
         * This can be implemented for say skin attachments, which copy over joint transforms from the actor instance they are attached to.
         * @param outPose The pose that will be modified.
         */
        virtual void UpdateJointTransforms(Pose& outPose) override;

        /**
         * Update the attachment. This can update the parent world space transform stored inside the actor instance.
         */
        virtual void Update() override;

        /**
         * Get the mapping for a given joint.
         * @param nodeIndex The joint index inside the actor instance that represents the attachment.
         * @result A reference to the mapping information for this joint.
         */
        MCORE_INLINE JointMapping& GetJointMapping(size_t nodeIndex) { return m_jointMap[nodeIndex]; }

        /**
         * Get the mapping for a given joint.
         * @param nodeIndex The joint index inside the actor instance that represents the attachment.
         * @result A reference to the mapping information for this joint.
         */
        MCORE_INLINE const JointMapping& GetJointMapping(size_t nodeIndex) const { return m_jointMap[nodeIndex]; }

    protected:
        AZStd::vector<JointMapping> m_jointMap; /**< Specifies which joints we need to copy transforms from and to. */
        AZStd::vector<MorphMapping> m_morphMap; /**< Maps morph targets of the actor instance we attach to with morphs in the attachment actor instance. */

        /**
         * The constructor for a skin attachment.
         * @param attachToActorInstance The actor instance to attach to (for example a cowboy).
         * @param attachment The actor instance that you want to attach to this joint (for example a gun).
         */
        AttachmentSkin(ActorInstance* attachToActorInstance, ActorInstance* attachment);

        /**
         * The destructor.
         * This does NOT delete the actor instance used by the attachment.
         */
        virtual ~AttachmentSkin();

        /**
         * Initialize the joint map, which links the joints inside the attachment with the actor where we attach to.
         * It is used to copy over the transformations from the main parent actor, to the actor instance representing the attachment object.
         */
        void InitJointMap();

        void InitMorphMap();
    };
} // namespace EMotionFX
