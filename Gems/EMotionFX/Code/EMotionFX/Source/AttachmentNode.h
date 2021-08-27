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
#include "Attachment.h"


namespace EMotionFX
{
    // forward declarations
    class ActorInstance;
    class Node;


    /**
     * A regular node attachment.
     * With node we mean that this attachment is only influenced by one given node in the ActorInstance it is attached to.
     * An example of this could be a gun attached to the hand node of a character.
     * Please keep in mind that the actor instance you attach can be a fully animated character as well. It is just being influenced by one single node of
     * the actor instance you attach it to.
     */
    class EMFX_API AttachmentNode
        : public Attachment
    {
        AZ_CLASS_ALLOCATOR_DECL
    public:
        enum
        {
            TYPE_ID = 0x00000001
        };

        /**
         * Create an attachment that is attached to a single node.
         * @param attachToActorInstance The actor instance to attach to, for example the main character in the game.
         * @param attachToNodeIndex The node to attach to. This has to be part of the actor where the attachToActorInstance is instanced from.
         * @param attachment The actor instance that you want to attach to this node (for example a gun).
         * @param managedExternally Specify whether the parent transform (where we are attached to) propagates into the attachment actor instance.
         */
        static AttachmentNode* Create(ActorInstance* attachToActorInstance, size_t attachToNodeIndex, ActorInstance* attachment, bool managedExternally = false);

        /**
         * Get the attachment type ID.
         * Every class inherited from this base class should have some type ID.
         * @return The type ID of this attachment class.
         */
        uint32 GetType() const override                                 { return TYPE_ID; }

        /**
         * Get the attachment type string.
         * Every class inherited from this base class should have some type ID string, which should be equal to the class name really.
         * @return The type string of this attachment class, which should be the class name.
         */
        const char* GetTypeString() const override                      { return "AttachmentNode"; }

        /**
         * Check if this attachment is being influenced by multiple joints or not.
         * This is the case for attachments such as clothing items which get influenced by multiple joints inside the actor instance they are attached to.
         * @result Returns true if it is influenced by multiple joints, otherwise false is returned.
         */
        bool GetIsInfluencedByMultipleJoints() const override final      { return false; }

        /**
         * Get the node where we attach something to.
         * This node is part of the actor from which the actor instance returned by GetAttachToActorInstance() is created.
         * @result The node index where we will attach this attachment to.
         */
        size_t GetAttachToNodeIndex() const;

        /**
         * Check whether the transformations of the attachment are modified by using a parent-child relationship in forward kinematics.
         * When external management is disbled (which it is on default), then the parent actor instance's global transform is forwarded into the attachment's
         * actor instance. When external management is disabled, this will not happen.
         * @result Returns true when this attachments transforms are managed externally.
         */
        bool GetIsManagedExternally() const;

        /**
         * Specify whether the transformations of the attachment are modified by using a parent-child relationship in forward kinematics.
         * When external management is disbled (which it is on default), then the parent actor instance's global transform is forwarded into the attachment's
         * actor instance. When external management is disabled, this will not happen.
         * @param managedExternally When set to true, the parent transformation will not propagate into the attachment's actor instance transformation.
         */
        void SetIsManagedExternally(bool managedExternally);

        /**
         * The main update method.
         */
        void Update() override;


    protected:
        size_t m_attachedToNode;       /**< The node where the attachment is linked to. */
        bool    m_isManagedExternally;  /**< Is this attachment basically managed (transformation wise) by something else? (like an Attachment component). The default is false. */

        /**
         * The constructor for a regular attachment.
         * @param attachToActorInstance The actor instance to attach to (for example a cowboy).
         * @param attachToNodeIndex The node to attach to. This has to be part of the actor where the attachToActorInstance is instanced from.
         * @param attachment The actor instance that you want to attach to this node (for example a gun).
         * @param managedExternally Specify whether the parent transform (where we are attached to) propagates into the attachment actor instance.
         */
        AttachmentNode(ActorInstance* attachToActorInstance, size_t attachToNodeIndex, ActorInstance* attachment, bool managedExternally = false);

        /**
         * The destructor.
         * This does NOT delete the actor instance used by the attachment.
         */
        virtual ~AttachmentNode();
    };
}   // namespace EMotionFX
