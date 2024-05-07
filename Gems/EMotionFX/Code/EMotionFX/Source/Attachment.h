/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include MCore related files
#include "EMotionFXConfig.h"
#include <MCore/Source/RefCounted.h>
#include "Pose.h"


namespace EMotionFX
{
    // forward declarations
    class ActorInstance;

    /**
     * The attachment base class.
     * An attachment can be a simple weapon attached to a hand node, but also a mesh or set of meshes and bones that deform with the main skeleton.
     * This last example is useful for clothing items or character customization.
     */
    class EMFX_API Attachment
        : public MCore::RefCounted
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        /**
         * Get the attachment type ID.
         * Every class inherited from this base class should have some TYPE ID.
         * @return The type ID of this attachment class.
         */
        virtual uint32 GetType() const = 0;

        /**
         * Get the attachment type string.
         * Every class inherited from this base class should have some type ID string, which should be equal to the class name really.
         * @return The type string of this attachment class, which should be the class name.
         */
        virtual const char* GetTypeString() const = 0;

        /**
         * Check if this attachment is being influenced by multiple joints or not.
         * This is the case for attachments such as clothing items which get influenced by multiple joints inside the actor instance they are attached to.
         * @result Returns true if it is influenced by multiple joints, otherwise false is returned.
         */
        virtual bool GetIsInfluencedByMultipleJoints() const = 0;

        /**
         * Update the attachment.
         * This can internally update node matrices for example, or other things.
         * This depends on the attachment type.
         */
        virtual void Update() {}

        /**
         * Update the joint transforms of the attachment.
         * This can be implemented for say skin attachments, which copy over joint transforms from the actor instance they are attached to.
         * @param outPose The pose that will be modified.
         */
        virtual void UpdateJointTransforms(Pose& outPose) { AZ_UNUSED(outPose); };

        /**
         * Get the actor instance object of the attachment.
         * This would for example return the actor instance that represents the gun when you attached a gun to a cowboy.
         * @result The actor instance representing the object you attach to something.
         */
        ActorInstance* GetAttachmentActorInstance() const;

        /**
         * Get the actor instance where we attach this attachment to.
         * This would for example return the cowboy, if we attach a gun to a cowboy.
         * @result The actor instance we are attaching something to.
         */
        ActorInstance* GetAttachToActorInstance() const;

    protected:
        ActorInstance*  m_attachment;        /**< The actor instance that represents the attachment. */
        ActorInstance*  m_actorInstance;     /**< The actor instance where this attachment is added to. */

        /**
         * The constructor.
         * @param attachToActorInstance The actor instance to attach to (for example a cowboy).
         * @param attachment The actor instance that you want to attach to this node (for example a gun).
         */
        Attachment(ActorInstance* attachToActorInstance, ActorInstance* attachment);

        /**
         * The destructor.
         * This does NOT delete the actor instance used by the attachment.
         */
        virtual ~Attachment();
    };
}   // namespace EMotionFX
