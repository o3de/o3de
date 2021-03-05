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

// include the required headers
#include "AttachmentCommands.h"
#include "CommandManager.h"
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/AttachmentNode.h>
#include <EMotionFX/Source/AttachmentSkin.h>
#include <EMotionFX/Source/ActorManager.h>


namespace CommandSystem
{
    //--------------------------------------------------------------------------------
    // CommandAddAttachment
    //--------------------------------------------------------------------------------

    // constructor
    CommandAddAttachment::CommandAddAttachment(MCore::Command* orgCommand)
        : MCore::Command("AddAttachment", orgCommand)
    {
    }


    // destructor
    CommandAddAttachment::~CommandAddAttachment()
    {
    }


    // add an attachment
    bool CommandAddAttachment::AddAttachment(MCore::Command* command, const MCore::CommandLine& parameters, AZStd::string& outResult, bool remove)
    {
        AZStd::string attachToNode;
        parameters.GetValue("attachToNode", command, &attachToNode);

        uint32 attachToActorID      = parameters.GetValueAsInt("attachToID", command);
        uint32 attachToActorIndex   = parameters.GetValueAsInt("attachToIndex", command);

        // in case we only specified an attach to index, get the id from that
        if (attachToActorIndex != MCORE_INVALIDINDEX32 && attachToActorID == MCORE_INVALIDINDEX32)
        {
            if (attachToActorIndex >= EMotionFX::GetActorManager().GetNumActorInstances())
            {
                outResult = "Cannot add attachment. Attach to actor index is out of range.";
                return false;
            }

            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(attachToActorIndex);
            attachToActorID = actorInstance->GetID();
        }

        uint32 attachmentID     = parameters.GetValueAsInt("attachmentID", command);
        uint32 attachmentIndex  = parameters.GetValueAsInt("attachmentIndex", command);
        if (attachmentID == MCORE_INVALIDINDEX32)
        {
            // in case we only specified an attachment index, get the id from that
            if (attachmentIndex != MCORE_INVALIDINDEX32 && attachmentIndex < EMotionFX::GetActorManager().GetNumActorInstances())
            {
                EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(attachmentIndex);
                attachmentID = actorInstance->GetID();
            }
            else
            {
                // check if there is a single actor instance selected and use that as attachment in case it is valid
                EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
                if (actorInstance == nullptr)
                {
                    outResult = "Cannot add attachment. No or multiple actor instance selected.";
                    return false;
                }

                // use the actor instance as attachment id
                attachmentID = actorInstance->GetID();
            }
        }

        MCORE_ASSERT(attachmentID != MCORE_INVALIDINDEX32 && attachToActorID != MCORE_INVALIDINDEX32);

        // check if we want to attach itself as attachment
        if (attachmentID == attachToActorID)
        {
            outResult = "Cannot add/remove attachment to the same actor instance. The attachToID and attachmentID are equal.";
            return false;
        }

        // get the corresponding actor instances
        EMotionFX::ActorInstance* attachment = EMotionFX::GetActorManager().FindActorInstanceByID(attachmentID);
        if (attachment == nullptr)
        {
            outResult = AZStd::string::format("Cannot add/remove attachment with ID %i. Attachment actor instance ID not valid.", attachmentID);
            return false;
        }

        EMotionFX::ActorInstance* attachToActorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(attachToActorID);
        if (attachToActorInstance == nullptr)
        {
            outResult = AZStd::string::format("Cannot add/remove attachment to the given actor instance with ID %i. Actor instance ID not valid.", attachToActorID);
            return false;
        }

        // get the actor instance the toAttachTo actor might be attached to
        EMotionFX::ActorInstance* attachToActorInstanceAttachment = attachToActorInstance->GetAttachedTo();
        uint32 attachToActorInstanceAttachmentID = MCORE_INVALIDINDEX32;
        if (attachToActorInstanceAttachment)
        {
            attachToActorInstanceAttachmentID = attachToActorInstanceAttachment->GetID();
        }

        // check if we want to attach the actor instance that has this attachment already as attachment
        if (attachmentID == attachToActorInstanceAttachmentID)
        {
            outResult = "Cannot add/remove attachment to the specified actor instance. The actor instance with attachmentID is already attached to the actor instance attachToID.";
            return false;
        }

        // get the node from the given name
        EMotionFX::Node* node = nullptr;
        if (remove == false)
        {
            EMotionFX::Actor* attachToActor = attachToActorInstance->GetActor();
            node = attachToActor->GetSkeleton()->FindNodeByName(attachToNode.c_str());
            if (node == nullptr)
            {
                outResult = AZStd::string::format("Cannot add attachment to node '%s'. The given node cannot be found.", attachToNode.c_str());
                return false;
            }
        }

        // lastly add/remove the attachment to the actor instance
        if (remove == false)
        {
            EMotionFX::AttachmentNode* newAttachment = EMotionFX::AttachmentNode::Create(attachToActorInstance, node->GetNodeIndex(), attachment);
            attachToActorInstance->AddAttachment(newAttachment);
            //attachToActorInstance->AddAttachment( node->GetNodeIndex(), attachment );
        }
        else
        {
            attachToActorInstance->RemoveAttachment(attachment, true);
        }
        //      attachToActorInstance->RemoveAttachment( attachment, false );

        return true;
    }


    // execute
    bool CommandAddAttachment::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        return AddAttachment(this, parameters, outResult, false);
    }


    // undo the command
    bool CommandAddAttachment::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        return AddAttachment(this, parameters, outResult, true);
    }


    // init the syntax of the command
    void CommandAddAttachment::InitSyntax()
    {
        GetSyntax().ReserveParameters(5);
        GetSyntax().AddRequiredParameter("attachToNode",    "The node name to which the attachment will get attached to.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("attachmentID",            "The ID of the attachment actor instance.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("attachmentIndex",         "The index inside the actor manager of the attachment actor instance.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("attachToID",              "The ID of the actor instance that will get the attachment.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("attachToIndex",           "The index inside the actor manager of the actor instance that will get the attachment.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
    }


    // get the description
    const char* CommandAddAttachment::GetDescription() const
    {
        return "This command can be used to .";
    }


    //--------------------------------------------------------------------------------
    // CommandRemoveAttachment
    //--------------------------------------------------------------------------------

    // constructor
    CommandRemoveAttachment::CommandRemoveAttachment(MCore::Command* orgCommand)
        : MCore::Command("RemoveAttachment", orgCommand)
    {
    }


    // destructor
    CommandRemoveAttachment::~CommandRemoveAttachment()
    {
    }


    // execute
    bool CommandRemoveAttachment::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        return CommandAddAttachment::AddAttachment(this, parameters, outResult, true);
    }


    // undo the command
    bool CommandRemoveAttachment::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        return CommandAddAttachment::AddAttachment(this, parameters, outResult, false);
    }


    // init the syntax of the command
    void CommandRemoveAttachment::InitSyntax()
    {
        GetSyntax().ReserveParameters(3);
        GetSyntax().AddRequiredParameter("attachToID",      "The ID of the actor instance that will get the attachment.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("attachmentID",    "The ID of the attachment actor instance.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("attachToNode",    "The node name to which the attachment will get attached to.", MCore::CommandSyntax::PARAMTYPE_STRING);
    }


    // get the description
    const char* CommandRemoveAttachment::GetDescription() const
    {
        return "This command can be used to .";
    }


    //--------------------------------------------------------------------------------
    // CommandClearAttachments
    //--------------------------------------------------------------------------------

    // execute
    bool CommandClearAttachments::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        uint32 actorInstanceID = parameters.GetValueAsInt("actorInstanceID", MCORE_INVALIDINDEX32);
        MCORE_ASSERT(actorInstanceID != MCORE_INVALIDINDEX32);

        // get the corresponding actor instances
        EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
        if (actorInstance == nullptr)
        {
            outResult = AZStd::string::format("Cannot remove attachments from actor instance with ID %i. Actor instance ID not valid.", actorInstanceID);
            return false;
        }

        // remove all attachments
        actorInstance->RemoveAllAttachments(false);
        return true;
    }


    // undo the command
    bool CommandClearAttachments::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    // init the syntax of the command
    void CommandClearAttachments::InitSyntax()
    {
        GetSyntax().ReserveParameters(1);
        GetSyntax().AddRequiredParameter("actorInstanceID", "The ID of the actor instance from which all attachments will get removed.", MCore::CommandSyntax::PARAMTYPE_INT);
    }


    // get the description
    const char* CommandClearAttachments::GetDescription() const
    {
        return "This command can be used to .";
    }


    //--------------------------------------------------------------------------------
    // CommandAddDeformableAttachment
    //--------------------------------------------------------------------------------

    // constructor
    CommandAddDeformableAttachment::CommandAddDeformableAttachment(MCore::Command* orgCommand)
        : MCore::Command("AddDeformableAttachment", orgCommand)
    {
    }


    // destructor
    CommandAddDeformableAttachment::~CommandAddDeformableAttachment()
    {
    }


    bool CommandAddDeformableAttachment::AddAttachment(MCore::Command* command, const MCore::CommandLine& parameters, AZStd::string& outResult, bool remove)
    {
        uint32 attachToActorID      = parameters.GetValueAsInt("attachToID", command);
        uint32 attachToActorIndex   = parameters.GetValueAsInt("attachToIndex", command);

        // in case we only specified an attach to index, get the id from that
        if (attachToActorIndex != MCORE_INVALIDINDEX32 && attachToActorID == MCORE_INVALIDINDEX32)
        {
            if (EMotionFX::GetActorManager().GetNumActorInstances() <= attachToActorIndex)
            {
                return false;
            }

            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(attachToActorIndex);
            attachToActorID = actorInstance->GetID();
        }

        uint32 attachmentID     = parameters.GetValueAsInt("attachmentID", command);
        uint32 attachmentIndex  = parameters.GetValueAsInt("attachmentIndex", command);
        if (attachmentID == MCORE_INVALIDINDEX32)
        {
            // in case we only specified an attachment index, get the id from that
            if (attachmentIndex != MCORE_INVALIDINDEX32)
            {
                EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(attachmentIndex);
                attachmentID = actorInstance->GetID();
            }
            else
            {
                // check if there is a single actor instance selected and use that as attachment in case it is valid
                EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
                if (actorInstance == nullptr)
                {
                    outResult = "Cannot add attachment. No or multiple actor instance selected.";
                    return false;
                }

                // use the actor instance as attachment id
                attachmentID = actorInstance->GetID();
            }
        }


        MCORE_ASSERT(attachmentID != MCORE_INVALIDINDEX32 && attachToActorID != MCORE_INVALIDINDEX32);

        // check if we want to attach itself as attachment
        if (attachmentID == attachToActorID)
        {
            outResult = "Cannot add attachment to the same actor instance. The attachToID and attachmentID are equal.";
            return false;
        }

        // get the corresponding actor instances
        EMotionFX::ActorInstance* attachment = EMotionFX::GetActorManager().FindActorInstanceByID(attachmentID);
        if (attachment == nullptr)
        {
            outResult = AZStd::string::format("Cannot add skin attachment with ID %i. Attachment actor instance ID not valid.", attachmentID);
            return false;
        }

        EMotionFX::ActorInstance* attachToActorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(attachToActorID);
        if (attachToActorInstance == nullptr)
        {
            outResult = AZStd::string::format("Cannot add skin attachment to the given actor instance with ID %i. Actor instance ID not valid.", attachToActorID);
            return false;
        }

        // lastly add/remove the attachment to the actor instance
        if (remove == false)
        {
            EMotionFX::AttachmentSkin* newAttachment = EMotionFX::AttachmentSkin::Create(attachToActorInstance, attachment);
            attachToActorInstance->AddAttachment(newAttachment);
            //attachToActorInstance->AddDeformableAttachment( attachment );
        }
        else
        {
            attachToActorInstance->RemoveAttachment(attachment, false);
        }

        return true;
    }


    // execute
    bool CommandAddDeformableAttachment::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        return AddAttachment(this, parameters, outResult, false);
    }


    // undo the command
    bool CommandAddDeformableAttachment::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        return AddAttachment(this, parameters, outResult, true);
    }


    // init the syntax of the command
    void CommandAddDeformableAttachment::InitSyntax()
    {
        GetSyntax().ReserveParameters(4);
        GetSyntax().AddParameter("attachToID",      "The ID of the actor instance that will get the skin attachment.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("attachToIndex",   "The index inside the actor manager of the actor instance that will get the attachment.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("attachmentID",    "The ID of the skin attachment actor instance.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("attachmentIndex", "The index inside the actor manager of the attachment actor instance.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
    }


    // get the description
    const char* CommandAddDeformableAttachment::GetDescription() const
    {
        return "This command can be used to .";
    }
} // namespace CommandSystem
