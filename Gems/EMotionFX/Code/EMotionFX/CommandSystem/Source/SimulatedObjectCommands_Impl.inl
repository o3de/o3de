/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(CommandAddSimulatedObject, EMotionFX::CommandAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(CommandRemoveSimulatedObject, EMotionFX::CommandAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(CommandAdjustSimulatedObject, EMotionFX::CommandAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(CommandAddSimulatedJoints, EMotionFX::CommandAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(CommandRemoveSimulatedJoints, EMotionFX::CommandAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(CommandAdjustSimulatedJoint, EMotionFX::CommandAllocator)

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CommandSimulatedObjectHelpers
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void CommandSimulatedObjectHelpers::JointIndicesToString(const AZStd::vector<size_t>& jointIndices, AZStd::string& outJointIndicesString)
    {
        outJointIndicesString.clear();
        for (size_t jointIndex : jointIndices)
        {
            if (!outJointIndicesString.empty())
            {
                outJointIndicesString += ';';
            }
            outJointIndicesString += AZStd::string::format("%zu", jointIndex);
        }
    }

    void CommandSimulatedObjectHelpers::StringToJointIndices(const AZStd::string& jointIndicesString, AZStd::vector<size_t>& outJointIndices)
    {
        outJointIndices.clear();
        AZStd::vector<AZStd::string> jointIndicesStrings;
        AzFramework::StringFunc::Tokenize(jointIndicesString.c_str(), jointIndicesStrings, ';', false, true);
        for (const AZStd::string& jointIndexStr : jointIndicesStrings)
        {
            outJointIndices.emplace_back(AzFramework::StringFunc::ToInt(jointIndexStr.c_str()));
        }
    }


    bool CommandSimulatedObjectHelpers::AddSimulatedObject(AZ::u32 actorId, AZStd::optional<AZStd::string> name,
        MCore::CommandGroup* commandGroup, bool executeInsideCommand)
    {
        AZStd::string command = AZStd::string::format("%s -%s %d",
            CommandAddSimulatedObject::s_commandName,
            CommandAddSimulatedObject::s_actorIdParameterName,
            actorId);

        if (name.has_value())
        {
            command += AZStd::string::format(" -%s %s", CommandAddSimulatedObject::s_nameParameterName, name.value().c_str());
        }

        return CommandSystem::GetCommandManager()->ExecuteCommandOrAddToGroup(command, commandGroup, executeInsideCommand);
    }

    bool CommandSimulatedObjectHelpers::RemoveSimulatedObject(AZ::u32 actorId, size_t objectIndex,
        MCore::CommandGroup* commandGroup, bool executeInsideCommand)
    {
        AZStd::string command = AZStd::string::format("%s -%s %u -%s %zu",
            CommandRemoveSimulatedObject::s_commandName,
            CommandRemoveSimulatedObject::s_actorIdParameterName, actorId,
            CommandRemoveSimulatedObject::s_objectIndexParameterName, objectIndex);

        return CommandSystem::GetCommandManager()->ExecuteCommandOrAddToGroup(command, commandGroup, executeInsideCommand);
    }

    bool CommandSimulatedObjectHelpers::AddSimulatedJoints(AZ::u32 actorId, const AZStd::vector<size_t>& jointIndices, size_t objectIndex, bool addChildren,
        MCore::CommandGroup* commandGroup, bool executeInsideCommand)
    {
        AZStd::string jointIndicesStr;
        JointIndicesToString(jointIndices, jointIndicesStr);

        AZStd::string command = AZStd::string::format("%s -%s %d -%s %s -%s %zu -%s %s",
            CommandAddSimulatedJoints::s_commandName,
            CommandAddSimulatedJoints::s_actorIdParameterName, actorId,
            CommandAddSimulatedJoints::s_jointIndicesParameterName, jointIndicesStr.c_str(),
            CommandAddSimulatedJoints::s_objectIndexParameterName, objectIndex,
            CommandAddSimulatedJoints::s_addChildrenParameterName, addChildren ? "true" : "false");

        return CommandSystem::GetCommandManager()->ExecuteCommandOrAddToGroup(command, commandGroup, executeInsideCommand);
    }

    bool CommandSimulatedObjectHelpers::RemoveSimulatedJoints(AZ::u32 actorId, const AZStd::vector<size_t>& jointIndices, size_t objectIndex, bool removeChildren,
        MCore::CommandGroup* commandGroup, bool executeInsideCommand)
    {
        AZStd::string jointIndicesStr;
        JointIndicesToString(jointIndices, jointIndicesStr);

        AZStd::string command = AZStd::string::format("%s -%s %d -%s %s -%s %zu -%s %s",
            CommandRemoveSimulatedJoints::s_commandName,
            CommandRemoveSimulatedJoints::s_actorIdParameterName, actorId,
            CommandRemoveSimulatedJoints::s_jointIndicesParameterName, jointIndicesStr.c_str(),
            CommandRemoveSimulatedJoints::s_objectIndexParameterName, objectIndex,
            CommandRemoveSimulatedJoints::s_removeChildrenParameterName, removeChildren ? "true" : "false");

        return CommandSystem::GetCommandManager()->ExecuteCommandOrAddToGroup(command, commandGroup, executeInsideCommand);
    }

    bool CommandSimulatedObjectHelpers::ReplaceTag(const AZStd::string& oldTag, const AZStd::string& newTag, AZStd::vector<AZStd::string>& outTags)
    {
        bool changed = false;
        for (AZStd::string& tag : outTags)
        {
            if (tag == oldTag)
            {
                tag = newTag;
                changed = true;
            }
        }
        return changed;
    }

    void CommandSimulatedObjectHelpers::ReplaceTag(const Actor* actor, const PhysicsSetup::ColliderConfigType colliderType, const AZStd::string& oldTag, const AZStd::string& newTag, MCore::CommandGroup& outCommandGroup)
    {
        if (colliderType != PhysicsSetup::ColliderConfigType::SimulatedObjectCollider || !actor)
        {
            return;
        }

        SimulatedObjectSetup* simulatedObjectSetup = actor->GetSimulatedObjectSetup().get();
        if (!simulatedObjectSetup)
        {
            return;
        }

        const AZ::u32 actorId = actor->GetID();
        AZStd::vector<AZStd::string> tags;
        const size_t numSimulatedObjects = simulatedObjectSetup->GetNumSimulatedObjects();
        for (size_t objectIndex = 0; objectIndex < numSimulatedObjects; ++objectIndex)
        {
            const EMotionFX::SimulatedObject* simulatedObject = simulatedObjectSetup->GetSimulatedObject(objectIndex);
            tags = simulatedObject->GetColliderTags();
            const bool objectChanged = ReplaceTag(oldTag, newTag, tags);
            if (objectChanged)
            {
                CommandAdjustSimulatedObject* command = aznew CommandAdjustSimulatedObject(actorId, objectIndex);
                command->SetOldColliderTags(simulatedObject->GetColliderTags());
                command->SetColliderTags(tags);
                outCommandGroup.AddCommand(command);
            }

            const size_t numSimulatedJoints = simulatedObject->GetNumSimulatedJoints();
            for (size_t jointIndex = 0; jointIndex < numSimulatedJoints; ++jointIndex)
            {
                const SimulatedJoint* simulatedJoint = simulatedObject->GetSimulatedJoint(jointIndex);
                tags = simulatedJoint->GetColliderExclusionTags();
                const bool jointChanged = ReplaceTag(oldTag, newTag, tags);
                if (jointChanged)
                {
                    CommandAdjustSimulatedJoint* command = aznew CommandAdjustSimulatedJoint(actorId, objectIndex, jointIndex);
                    command->SetOldColliderExclusionTags(simulatedJoint->GetColliderExclusionTags());
                    command->SetColliderExclusionTags(tags);
                    outCommandGroup.AddCommand(command);
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CommandAddSimulatedObject
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    const char* CommandAddSimulatedObject::s_commandName = "AddSimulatedObject";
    const char* CommandAddSimulatedObject::s_objectIndexParameterName = "objectIndex";
    const char* CommandAddSimulatedObject::s_nameParameterName = "name";
    const char* CommandAddSimulatedObject::s_contentParameterName = "contents";

    CommandAddSimulatedObject::CommandAddSimulatedObject(MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
    {
    }

    CommandAddSimulatedObject::CommandAddSimulatedObject(AZ::u32 actorId, AZStd::optional<AZStd::string> name, MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
        , ParameterMixinActorId(actorId)
        , m_name(AZStd::move(name))
    {
    }

    void CommandAddSimulatedObject::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<CommandAddSimulatedObject, MCore::Command, ParameterMixinActorId>()
            ->Version(1)
            ;
    }

    bool CommandAddSimulatedObject::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        const AZStd::shared_ptr<SimulatedObjectSetup>& simulatedObjectSetup = actor->GetSimulatedObjectSetup();
        if(!m_contents.empty())
        {
            // Handle remove simulated object undo cases.
            SimulatedObject* newObject = simulatedObjectSetup->InsertSimulatedObjectAt(m_objectIndex);
            MCore::ReflectionSerializer::Deserialize(newObject, m_contents);
            newObject->InitAfterLoading(simulatedObjectSetup.get());
        }
        else
        {
            if (m_name.has_value())
            {
                simulatedObjectSetup->AddSimulatedObject(m_name.value());
            }
            else
            {
                simulatedObjectSetup->AddSimulatedObject();
            }
            m_objectIndex = simulatedObjectSetup->GetNumSimulatedObjects() - 1;
        }

        SimulatedObjectNotificationBus::Broadcast(&SimulatedObjectNotificationBus::Events::OnSimulatedObjectChanged);

        m_oldDirtyFlag = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);
        return true;
    }

    bool CommandAddSimulatedObject::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        const AZStd::shared_ptr<SimulatedObjectSetup>& simulatedObjectSetup = actor->GetSimulatedObjectSetup();
        simulatedObjectSetup->RemoveSimulatedObject(m_objectIndex);

        SimulatedObjectNotificationBus::Broadcast(&SimulatedObjectNotificationBus::Events::OnSimulatedObjectChanged);

        actor->SetDirtyFlag(m_oldDirtyFlag);
        return true;
    }

    void CommandAddSimulatedObject::InitSyntax()
    {
        MCore::CommandSyntax& syntax = GetSyntax();
        syntax.ReserveParameters(4);
        ParameterMixinActorId::InitSyntax(syntax);
        syntax.AddParameter(s_objectIndexParameterName, "The simulated object index we want to insert at", MCore::CommandSyntax::PARAMTYPE_INT, "");
        syntax.AddParameter(s_nameParameterName, "The name to assign to the new simulated object", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        syntax.AddParameter(s_contentParameterName, "The contents of the simulated object index we want to add", MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }

    bool CommandAddSimulatedObject::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinActorId::SetCommandParameters(parameters);

        if (parameters.CheckIfHasParameter(s_objectIndexParameterName))
        {
            m_objectIndex = static_cast<size_t>(parameters.GetValueAsInt(s_objectIndexParameterName, this));
        }

        if (parameters.CheckIfHasParameter(s_nameParameterName))
        {
            m_name = parameters.GetValue(s_nameParameterName, this);
        }

        if (parameters.CheckIfHasParameter(s_contentParameterName))
        {
            parameters.GetValue(s_contentParameterName, this, &m_contents);
        }

        return true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CommandRemoveSimulatedObject
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    const char* CommandRemoveSimulatedObject::s_commandName = "RemoveSimulatedObject";
    const char* CommandRemoveSimulatedObject::s_objectIndexParameterName = "objectIndex";

    CommandRemoveSimulatedObject::CommandRemoveSimulatedObject(MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
    {
    }

    CommandRemoveSimulatedObject::CommandRemoveSimulatedObject(AZ::u32 actorId, MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
        , ParameterMixinActorId(actorId)
    {
    }

    void CommandRemoveSimulatedObject::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<CommandRemoveSimulatedObject, MCore::Command, ParameterMixinActorId>()
            ->Version(1)
            ;
    }

    bool CommandRemoveSimulatedObject::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        const AZStd::shared_ptr<SimulatedObjectSetup>& simulatedObjectSetup = actor->GetSimulatedObjectSetup();
        m_oldContents = MCore::ReflectionSerializer::Serialize(simulatedObjectSetup->GetSimulatedObject(m_objectIndex)).GetValue();
        simulatedObjectSetup->RemoveSimulatedObject(m_objectIndex);

        SimulatedObjectNotificationBus::Broadcast(&SimulatedObjectNotificationBus::Events::OnSimulatedObjectChanged);

        m_oldDirtyFlag = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);
        return true;
    }

    bool CommandRemoveSimulatedObject::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        AZStd::string command = AZStd::string::format("%s -%s %d -%s %zu -%s %s",
            CommandAddSimulatedObject::s_commandName,
            CommandAddSimulatedObject::s_actorIdParameterName, actor->GetID(),
            CommandAddSimulatedObject::s_objectIndexParameterName, m_objectIndex,
            CommandAddSimulatedObject::s_contentParameterName, m_oldContents.c_str() );

        if (CommandSystem::GetCommandManager()->ExecuteCommandInsideCommand(command, outResult) == false)
        {
            AZ_Error("EMotionFX", false, outResult.c_str());
            return false;
        }

        actor->SetDirtyFlag(m_oldDirtyFlag);
        return true;
    }

    void CommandRemoveSimulatedObject::InitSyntax()
    {
        MCore::CommandSyntax& syntax = GetSyntax();
        syntax.ReserveParameters(2);
        ParameterMixinActorId::InitSyntax(syntax);
        syntax.AddRequiredParameter(s_objectIndexParameterName, "The simulated object index we want to remove", MCore::CommandSyntax::PARAMTYPE_INT);
    }

    bool CommandRemoveSimulatedObject::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinActorId::SetCommandParameters(parameters);

        m_objectIndex = static_cast<size_t>(parameters.GetValueAsInt("objectIndex", this));

        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    // CommandAdjustSimulatedObject
    ///////////////////////////////////////////////////////////////////////////

    const char* const CommandAdjustSimulatedObject::s_commandName = "AdjustSimulatedObject";
    const char* const CommandAdjustSimulatedObject::s_objectNameParameterName = "objectName";
    const char* const CommandAdjustSimulatedObject::s_gravityFactorParameterName = "gravityFactor";
    const char* const CommandAdjustSimulatedObject::s_stiffnessFactorParameterName = "stiffnessFactor";
    const char* const CommandAdjustSimulatedObject::s_dampingFactorParameterName = "dampingFactor";
    const char* const CommandAdjustSimulatedObject::s_colliderTagsParameterName = "colliderTags";

    CommandAdjustSimulatedObject::CommandAdjustSimulatedObject(MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
    {
    }

    CommandAdjustSimulatedObject::CommandAdjustSimulatedObject(AZ::u32 actorId, size_t objectIndex, MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
        , ParameterMixinActorId(actorId)
        , m_objectIndex(objectIndex)
    {
    }

    void CommandAdjustSimulatedObject::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<CommandAdjustSimulatedObject, MCore::Command, ParameterMixinActorId>()
            ->Version(1)
            ;
    }

    bool CommandAdjustSimulatedObject::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        SimulatedObject* object = GetSimulatedObject(outResult);
        if (!object)
        {
            return false;
        }

        if (m_objectName.has_value())
        {
            // Check if the name is unique - if not, generate a new unique name.
            AZStd::string uniqueName = m_objectName.value();
            size_t num = 1;
            while (!actor->GetSimulatedObjectSetup()->IsSimulatedObjectNameUnique(uniqueName, object))
            {
                uniqueName = AZStd::string::format("%s %zu", m_objectName.value().c_str(), num++);
            }

            if (!m_oldObjectName.has_value())
            {
                m_oldObjectName = object->GetName();
            }
            object->SetName(uniqueName);
            m_objectName = uniqueName;
        }
        if (m_gravityFactor.has_value())
        {
            if (!m_oldGravityFactor.has_value())
            {
                m_oldGravityFactor = object->GetGravityFactor();
            }
            object->SetGravityFactor(m_gravityFactor.value());
        }
        if (m_stiffnessFactor.has_value())
        {
            if (!m_oldStiffnessFactor.has_value())
            {
                m_oldStiffnessFactor = object->GetStiffnessFactor();
            }
            object->SetStiffnessFactor(m_stiffnessFactor.value());
        }
        if (m_dampingFactor.has_value())
        {
            if (!m_oldDampingFactor.has_value())
            {
                m_oldDampingFactor = object->GetDampingFactor();
            }
            object->SetDampingFactor(m_dampingFactor.value());
        }
        if (m_colliderTags.has_value())
        {
            if (!m_oldColliderTags.has_value())
            {
                m_oldColliderTags = object->GetColliderTags();
            }
            object->SetColliderTags(m_colliderTags.value());

            SimulatedObjectNotificationBus::Broadcast(&SimulatedObjectNotificationBus::Events::OnSimulatedObjectChanged);
        }

        m_oldDirtyFlag = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);
        return true;
    }

    bool CommandAdjustSimulatedObject::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        SimulatedObject* object = GetSimulatedObject(outResult);
        if (!object)
        {
            return false;
        }

        if (m_oldObjectName.has_value())
        {
            object->SetName(m_oldObjectName.value());
        }
        if (m_oldGravityFactor.has_value())
        {
            object->SetGravityFactor(m_oldGravityFactor.value());
        }
        if (m_oldStiffnessFactor.has_value())
        {
            object->SetStiffnessFactor(m_oldStiffnessFactor.value());
        }
        if (m_oldDampingFactor.has_value())
        {
            object->SetDampingFactor(m_oldDampingFactor.value());
        }
        if (m_oldColliderTags.has_value())
        {
            object->SetColliderTags(m_oldColliderTags.value());
            SimulatedObjectNotificationBus::Broadcast(&SimulatedObjectNotificationBus::Events::OnSimulatedObjectChanged);
        }

        actor->SetDirtyFlag(m_oldDirtyFlag);
        return true;
    }

    void CommandAdjustSimulatedObject::InitSyntax()
    {
        MCore::CommandSyntax& syntax = GetSyntax();
        syntax.ReserveParameters(7);
        ParameterMixinActorId::InitSyntax(syntax);
        syntax.AddRequiredParameter("objectIndex", "The simulated object index to adjust.", MCore::CommandSyntax::PARAMTYPE_INT);
        syntax.AddParameter(s_objectNameParameterName, "The new name for this object to have.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        syntax.AddParameter(s_gravityFactorParameterName, "The new gravity factor for this object to use.", MCore::CommandSyntax::PARAMTYPE_FLOAT, "1.0");
        syntax.AddParameter(s_stiffnessFactorParameterName, "The new stiffness factor for this object to use.", MCore::CommandSyntax::PARAMTYPE_FLOAT, "1.0");
        syntax.AddParameter(s_dampingFactorParameterName, "The new damping factor for this object to use.", MCore::CommandSyntax::PARAMTYPE_FLOAT, "1.0");
        syntax.AddParameter(s_colliderTagsParameterName, "The new list of tags whose colliders should affect the joints in this object.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }

    bool CommandAdjustSimulatedObject::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinActorId::SetCommandParameters(parameters);
        m_objectIndex = static_cast<size_t>(parameters.GetValueAsInt("objectIndex", this));

        if (parameters.CheckIfHasParameter(s_objectNameParameterName))
        {
            m_objectName = parameters.GetValue(s_objectNameParameterName, this);
        }
        if (parameters.CheckIfHasParameter(s_gravityFactorParameterName))
        {
            m_gravityFactor = parameters.GetValueAsFloat(s_gravityFactorParameterName, this);
        }
        if (parameters.CheckIfHasParameter(s_stiffnessFactorParameterName))
        {
            m_stiffnessFactor = parameters.GetValueAsFloat(s_stiffnessFactorParameterName, this);
        }
        if (parameters.CheckIfHasParameter(s_dampingFactorParameterName))
        {
            m_dampingFactor = parameters.GetValueAsFloat(s_dampingFactorParameterName, this);
        }
        if (parameters.CheckIfHasParameter(s_colliderTagsParameterName))
        {
            m_colliderTags = AZStd::vector<AZStd::string>();
            AzFramework::StringFunc::Tokenize(
                parameters.GetValue(s_colliderTagsParameterName, this).data(),
                *m_colliderTags,
                ';'
            );
        }
        return true;
    }

    size_t CommandAdjustSimulatedObject::GetObjectIndex() const
    {
        return m_objectIndex;
    }

    void CommandAdjustSimulatedObject::SetObjectName(AZStd::optional<AZStd::string> newObjectName)
    {
        m_objectName = AZStd::move(newObjectName);
    }

    void CommandAdjustSimulatedObject::SetGravityFactor(AZStd::optional<float> newGravityFactor)
    {
        m_gravityFactor = AZStd::move(newGravityFactor);
    }

    void CommandAdjustSimulatedObject::SetStiffnessFactor(AZStd::optional<float> newStiffnessFactor)
    {
        m_stiffnessFactor = AZStd::move(newStiffnessFactor);
    }

    void CommandAdjustSimulatedObject::SetDampingFactor(AZStd::optional<float> newDampingFactor)
    {
        m_dampingFactor = AZStd::move(newDampingFactor);
    }

    void CommandAdjustSimulatedObject::SetOldObjectName(AZStd::optional<AZStd::string> newObjectName)
    {
        m_oldObjectName = AZStd::move(newObjectName);
    }

    void CommandAdjustSimulatedObject::SetOldGravityFactor(AZStd::optional<float> newGravityFactor)
    {
        m_oldGravityFactor = AZStd::move(newGravityFactor);
    }

    void CommandAdjustSimulatedObject::SetOldStiffnessFactor(AZStd::optional<float> newStiffnessFactor)
    {
        m_oldStiffnessFactor = AZStd::move(newStiffnessFactor);
    }

    void CommandAdjustSimulatedObject::SetOldDampingFactor(AZStd::optional<float> newDampingFactor)
    {
        m_oldDampingFactor = AZStd::move(newDampingFactor);
    }

    SimulatedObject* CommandAdjustSimulatedObject::GetSimulatedObject(AZStd::string& outResult) const
    {
        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return nullptr;
        }

        const AZStd::shared_ptr<SimulatedObjectSetup>& simulatedObjectSetup = actor->GetSimulatedObjectSetup();
        SimulatedObjectSetup* setup = simulatedObjectSetup.get();

        if (!setup)
        {
            outResult = "Can't find any simulated object.";
            return nullptr;
        }

        SimulatedObject* object = setup->GetSimulatedObject(m_objectIndex);
        if (!object)
        {
            outResult = "Can't find simulated object with index " + AZStd::to_string(m_objectIndex) + ".";
            return nullptr;
        }

        return object;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CommandAddSimulatedJoints
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    const char* CommandAddSimulatedJoints::s_commandName = "AddSimulatedJoints";
    const char* CommandAddSimulatedJoints::s_jointIndicesParameterName = "jointIndices";
    const char* CommandAddSimulatedJoints::s_objectIndexParameterName = "objectIndex";
    const char* CommandAddSimulatedJoints::s_addChildrenParameterName = "addChildren";
    const char* CommandAddSimulatedJoints::s_contentsParameterName = "contents";

    CommandAddSimulatedJoints::CommandAddSimulatedJoints(MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
    {
    }

    CommandAddSimulatedJoints::CommandAddSimulatedJoints(AZ::u32 actorId, MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
        , ParameterMixinActorId(actorId)
    {
    }

    void CommandAddSimulatedJoints::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<CommandAddSimulatedJoints, MCore::Command, ParameterMixinActorId>()
            ->Version(1)
            ;
    }

    bool CommandAddSimulatedJoints::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        const AZStd::shared_ptr<SimulatedObjectSetup>& simulatedObjectSetup = actor->GetSimulatedObjectSetup();
        SimulatedObjectSetup* setup = simulatedObjectSetup.get();
        if (!setup || setup->GetNumSimulatedObjects() == 0)
        {
            outResult = "Can't find any simulated object.";
            return false;
        }

        SimulatedObject* object = setup->GetSimulatedObject(m_objectIndex);
        if (!object)
        {
            outResult = AZStd::string::format("Can't find simulated object with index %zu.", m_objectIndex);
            return false;
        }

        if (m_contents.has_value())
        {
            object->Clear();
            MCore::ReflectionSerializer::Deserialize(object, m_contents.value());
            object->InitAfterLoading(setup);
        }
        else if(!m_addChildren)
        {
            // Simulated object already handles duplication.
            object->AddSimulatedJoints(m_jointIndices);
        }
        else
        {
            for (size_t jointIndex: m_jointIndices)
            {
                object->AddSimulatedJointAndChildren(jointIndex);
            }
        }

        SimulatedObjectNotificationBus::Broadcast(&SimulatedObjectNotificationBus::Events::OnSimulatedObjectChanged);

        m_oldDirtyFlag = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);
        return true;
    }

    bool CommandAddSimulatedJoints::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        const AZStd::shared_ptr<SimulatedObjectSetup>& simulatedObjectSetup = actor->GetSimulatedObjectSetup();
        SimulatedObjectSetup* setup = simulatedObjectSetup.get();
        if (!setup || setup->GetNumSimulatedObjects() == 0)
        {
            outResult = "Can't find any simulated object.";
            return false;
        }

        AZStd::string jointIndicesStr;
        CommandSimulatedObjectHelpers::JointIndicesToString(m_jointIndices, jointIndicesStr);

        AZStd::string command = AZStd::string::format("%s -%s %d -%s %s -%s %zu -%s %s",
            CommandRemoveSimulatedJoints::s_commandName,
            CommandRemoveSimulatedJoints::s_actorIdParameterName, actor->GetID(),
            CommandRemoveSimulatedJoints::s_jointIndicesParameterName, jointIndicesStr.c_str(),
            CommandRemoveSimulatedJoints::s_objectIndexParameterName, m_objectIndex,
            CommandRemoveSimulatedJoints::s_removeChildrenParameterName, m_addChildren ? "true" : "false");

        actor->SetDirtyFlag(m_oldDirtyFlag);
        return CommandSystem::GetCommandManager()->ExecuteCommandInsideCommand(command, outResult);
    }

    void CommandAddSimulatedJoints::InitSyntax()
    {
        MCore::CommandSyntax& syntax = GetSyntax();
        syntax.ReserveParameters(5);
        ParameterMixinActorId::InitSyntax(syntax);
        syntax.AddRequiredParameter(s_jointIndicesParameterName, "The joint indices to add in the simulated object.", MCore::CommandSyntax::PARAMTYPE_STRING);
        syntax.AddRequiredParameter(s_objectIndexParameterName, "The simulated object index of which the joints are going to add to.", MCore::CommandSyntax::PARAMTYPE_INT);
        syntax.AddParameter(s_addChildrenParameterName, "If we want to add the joints and all its children", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "");
        syntax.AddParameter(s_contentsParameterName, "The contents of the simulated object we are adding joints to.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }

    bool CommandAddSimulatedJoints::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinActorId::SetCommandParameters(parameters);
        m_objectIndex = static_cast<size_t>(parameters.GetValueAsInt(s_objectIndexParameterName, this));

        AZStd::string jointIndicesStr = parameters.GetValue(s_jointIndicesParameterName, this);
        CommandSimulatedObjectHelpers::StringToJointIndices(jointIndicesStr, m_jointIndices);

        if (parameters.CheckIfHasParameter(s_addChildrenParameterName))
        {
            m_addChildren = parameters.GetValueAsBool("addChildren", this);
        }

        if (parameters.CheckIfHasParameter(s_contentsParameterName))
        {
            m_contents = parameters.GetValue(s_contentsParameterName, this);
        }

        return true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CommandRemoveSimulatedJoints
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    const char* CommandRemoveSimulatedJoints::s_commandName = "RemoveSimulatedJoints";
    const char* CommandRemoveSimulatedJoints::s_jointIndicesParameterName = "jointIndices";
    const char* CommandRemoveSimulatedJoints::s_objectIndexParameterName = "objectIndex";
    const char* CommandRemoveSimulatedJoints::s_removeChildrenParameterName = "removeChildren";

    CommandRemoveSimulatedJoints::CommandRemoveSimulatedJoints(MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
    {
    }

    CommandRemoveSimulatedJoints::CommandRemoveSimulatedJoints(AZ::u32 actorId, MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
        , ParameterMixinActorId(actorId)
    {
    }

    void CommandRemoveSimulatedJoints::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<CommandRemoveSimulatedJoints, MCore::Command, ParameterMixinActorId>()
            ->Version(1)
            ;
    }

    bool CommandRemoveSimulatedJoints::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        const AZStd::shared_ptr<SimulatedObjectSetup>& simulatedObjectSetup = actor->GetSimulatedObjectSetup();
        SimulatedObjectSetup* setup = simulatedObjectSetup.get();

        if (!setup || setup->GetNumSimulatedObjects() == 0)
        {
            AZ_Warning("EMotionFX", false, "Can't find any simulated object.");
            return false;
        }

        SimulatedObject* object = setup->GetSimulatedObject(m_objectIndex);
        if (!object)
        {
            AZ_Warning("EMotionFX", false, "Can't find simulated object with index %d.", m_objectIndex);
            return false;
        }

        // Serialize the entire object for supporting undo.
        // The other option is to create another object with the removed joints. The trade-off is we have to deal with mem allocation, building the root joint list for the new object that only used for undo
        // and having to deal with merging two object. Since we are rebuilding the simulated object model when removing joints anyway, it's more convenient to serialize the whole object.
        m_oldContents = MCore::ReflectionSerializer::Serialize(object).GetValue();

        for (size_t jointIndex : m_jointIndices)
        {
            if (!object->FindSimulatedJointBySkeletonJointIndex(jointIndex))
            {
                AZ_Warning("EMotionFX", false, "Joint %d not in the object.", jointIndex);
                return false;
            }
            object->RemoveSimulatedJoint(jointIndex, m_removeChildren);
        }

        // After removing joints, the object could contain sparse chains - therefore we should do another loading to determine if we need to build the root list again.
        object->InitAfterLoading(setup);

        SimulatedObjectNotificationBus::Broadcast(&SimulatedObjectNotificationBus::Events::OnSimulatedObjectChanged);

        m_oldDirtyFlag = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);
        return true;
    }

    bool CommandRemoveSimulatedJoints::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        const AZStd::shared_ptr<SimulatedObjectSetup>& simulatedObjectSetup = actor->GetSimulatedObjectSetup();
        SimulatedObjectSetup* setup = simulatedObjectSetup.get();
        if (!setup || setup->GetNumSimulatedObjects() == 0)
        {
            AZ_Warning("EMotionFX", false, "Can't find any simulated object.");
            return false;
        }

        AZStd::string jointIndicesString;
        CommandSimulatedObjectHelpers::JointIndicesToString(m_jointIndices, jointIndicesString);

        AZStd::string command = AZStd::string::format("%s -%s %d -%s %s -%s %zu -%s %s",
            CommandAddSimulatedJoints::s_commandName,
            CommandAddSimulatedJoints::s_actorIdParameterName, actor->GetID(),
            CommandAddSimulatedJoints::s_jointIndicesParameterName, jointIndicesString.c_str(),
            CommandAddSimulatedJoints::s_objectIndexParameterName, m_objectIndex,
            CommandAddSimulatedJoints::s_addChildrenParameterName, m_removeChildren ? "true" : "false");

        if (m_oldContents.has_value())
        {
            command += AZStd::string::format(" -%s {%s}", CommandAddSimulatedJoints::s_contentsParameterName, m_oldContents.value().c_str());
        }

        if (!CommandSystem::GetCommandManager()->ExecuteCommandInsideCommand(command, outResult))
        {
            AZ_Error("EMotionFX", false, outResult.c_str());
            return false;
        }

        actor->SetDirtyFlag(m_oldDirtyFlag);
        return true;
    }

    void CommandRemoveSimulatedJoints::InitSyntax()
    {
        MCore::CommandSyntax& syntax = GetSyntax();
        syntax.ReserveParameters(3);
        ParameterMixinActorId::InitSyntax(syntax);
        syntax.AddRequiredParameter(s_jointIndicesParameterName, "The joint indices to remove in the simulated object.", MCore::CommandSyntax::PARAMTYPE_STRING);
        syntax.AddRequiredParameter(s_objectIndexParameterName, "The simulated object index of which the joint are going to be removed from.", MCore::CommandSyntax::PARAMTYPE_INT);
        syntax.AddParameter(s_removeChildrenParameterName, "If we want to remove the joints and all its children", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "");
    }

    bool CommandRemoveSimulatedJoints::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinActorId::SetCommandParameters(parameters);
        m_objectIndex = static_cast<size_t>(parameters.GetValueAsInt(s_objectIndexParameterName, this));
        AZStd::string jointIndicesString = parameters.GetValue(s_jointIndicesParameterName, this);
        CommandSimulatedObjectHelpers::StringToJointIndices(jointIndicesString, m_jointIndices);

        if (parameters.CheckIfHasParameter(s_removeChildrenParameterName))
        {
            m_removeChildren = parameters.GetValueAsBool("removeChildren", this);
        }

        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    // CommandAdjustSimulatedJoint
    ///////////////////////////////////////////////////////////////////////////

    const char* const CommandAdjustSimulatedJoint::s_commandName = "AdjustSimulatedJoint";
    const char* const CommandAdjustSimulatedJoint::s_objectIndexParameterName = "objectIndex";
    const char* const CommandAdjustSimulatedJoint::s_jointIndexParameterName = "jointIndex";
    const char* const CommandAdjustSimulatedJoint::s_coneAngleLimitParameterName = "coneAngleLimit";
    const char* const CommandAdjustSimulatedJoint::s_massParameterName = "mass";
    const char* const CommandAdjustSimulatedJoint::s_stiffnessParameterName = "stiffness";
    const char* const CommandAdjustSimulatedJoint::s_dampingParameterName = "damping";
    const char* const CommandAdjustSimulatedJoint::s_gravityFactorParameterName = "gravityFactor";
    const char* const CommandAdjustSimulatedJoint::s_frictionParameterName = "friction";
    const char* const CommandAdjustSimulatedJoint::s_pinnedParameterName = "pinned";
    const char* const CommandAdjustSimulatedJoint::s_colliderExclusionTagsParameterName = "colliderExclusionTags";
    const char* const CommandAdjustSimulatedJoint::s_autoExcludeModeParameterName = "autoExcludeMode";
    const char* const CommandAdjustSimulatedJoint::s_geometricAutoExclusionParameterName = "geometricAutoExclusion";

    CommandAdjustSimulatedJoint::CommandAdjustSimulatedJoint(MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
    {
    }

    CommandAdjustSimulatedJoint::CommandAdjustSimulatedJoint(AZ::u32 actorId, size_t objectIndex, size_t jointIndex, MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
        , ParameterMixinActorId(actorId)
        , m_objectIndex(objectIndex)
        , m_jointIndex(jointIndex)
    {
    }

    void CommandAdjustSimulatedJoint::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<CommandAdjustSimulatedJoint, MCore::Command, ParameterMixinActorId>()
            ->Version(1)
            ;
    }

    bool CommandAdjustSimulatedJoint::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);
        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        const AZStd::shared_ptr<SimulatedObjectSetup>& simulatedObjectSetup = actor->GetSimulatedObjectSetup();
        SimulatedObjectSetup* setup = simulatedObjectSetup.get();

        if (!setup)
        {
            outResult = "Can't find any simulated object.";
            return false;
        }

        SimulatedObject* object = setup->GetSimulatedObject(m_objectIndex);
        if (!object)
        {
            outResult = "Can't find simulated object with index " + AZStd::to_string(m_objectIndex) + ".";
            return false;
        }

        SimulatedJoint* joint = object->GetSimulatedJoint(m_jointIndex);
        if (!joint)
        {
            outResult = "Can't find simulated joint with index " + AZStd::to_string(m_jointIndex) + ".";
            return false;
        }

        if (m_coneAngleLimit.has_value())
        {
            if (!m_oldConeAngleLimit.has_value())
            {
                m_oldConeAngleLimit = joint->GetConeAngleLimit();
            }
            joint->SetConeAngleLimit(m_coneAngleLimit.value());
        }
        if (m_mass.has_value())
        {
            if (!m_oldMass.has_value())
            {
                m_oldMass = joint->GetMass();
            }
            joint->SetMass(m_mass.value());
        }
        if (m_stiffness.has_value())
        {
            if (!m_oldStiffness.has_value())
            {
                m_oldStiffness = joint->GetStiffness();
            }
            joint->SetStiffness(m_stiffness.value());
        }
        if (m_damping.has_value())
        {
            if (!m_oldDamping.has_value())
            {
                m_oldDamping = joint->GetDamping();
            }
            joint->SetDamping(m_damping.value());
        }
        if (m_gravityFactor.has_value())
        {
            if (!m_oldGravityFactor.has_value())
            {
                m_oldGravityFactor = joint->GetGravityFactor();
            }
            joint->SetGravityFactor(m_gravityFactor.value());
        }
        if (m_friction.has_value())
        {
            if (!m_oldFriction.has_value())
            {
                m_oldFriction = joint->GetFriction();
            }
            joint->SetFriction(m_friction.value());
        }
        if (m_pinned.has_value())
        {
            if (!m_oldPinned.has_value())
            {
                m_oldPinned = joint->IsPinned();
            }
            joint->SetPinned(m_pinned.value());
        }
        if (m_colliderExclusionTags.has_value())
        {
            if (!m_oldColliderExclusionTags.has_value())
            {
                m_oldColliderExclusionTags = joint->GetColliderExclusionTags();
            }
            joint->SetColliderExclusionTags(m_colliderExclusionTags.value());
        }
        if (m_autoExcludeMode.has_value())
        {
            if (!m_oldAutoExcludeMode.has_value())
            {
                m_oldAutoExcludeMode = joint->GetAutoExcludeMode();
            }
            joint->SetAutoExcludeMode(m_autoExcludeMode.value());
        }
        if (m_geometricAutoExclusion.has_value())
        {
            if (!m_oldGeometricAutoExclusion.has_value())
            {
                m_oldGeometricAutoExclusion = joint->IsGeometricAutoExclusion();
            }
            joint->SetGeometricAutoExclusion(m_geometricAutoExclusion.value());
        }

        if (m_colliderExclusionTags.has_value() || m_autoExcludeMode.has_value() || m_geometricAutoExclusion.has_value())
        {
            SimulatedObjectNotificationBus::Broadcast(&SimulatedObjectNotificationBus::Events::OnSimulatedObjectChanged);
        }

        m_oldDirtyFlag = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);
        return true;
    }

    bool CommandAdjustSimulatedJoint::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);
        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        const AZStd::shared_ptr<SimulatedObjectSetup>& simulatedObjectSetup = actor->GetSimulatedObjectSetup();
        SimulatedObjectSetup* setup = simulatedObjectSetup.get();

        if (!setup)
        {
            outResult = "Can't find any simulated object.";
            return false;
        }

        SimulatedObject* object = setup->GetSimulatedObject(m_objectIndex);
        if (!object)
        {
            outResult = "Can't find simulated object with index " + AZStd::to_string(m_objectIndex) + ".";
            return false;
        }

        SimulatedJoint* joint = object->GetSimulatedJoint(m_jointIndex);
        if (!joint)
        {
            outResult = "Can't find simulated joint with index " + AZStd::to_string(m_jointIndex) + ".";
            return false;
        }

        if (m_oldConeAngleLimit.has_value())
        {
            joint->SetConeAngleLimit(m_oldConeAngleLimit.value());
        }
        if (m_oldMass.has_value())
        {
            joint->SetMass(m_oldMass.value());
        }
        if (m_oldStiffness.has_value())
        {
            joint->SetStiffness(m_oldStiffness.value());
        }
        if (m_oldDamping.has_value())
        {
            joint->SetDamping(m_oldDamping.value());
        }
        if (m_oldGravityFactor.has_value())
        {
            joint->SetGravityFactor(m_oldGravityFactor.value());
        }
        if (m_oldFriction.has_value())
        {
            joint->SetFriction(m_oldFriction.value());
        }
        if (m_oldPinned.has_value())
        {
            joint->SetPinned(m_oldPinned.value());
        }
        if (m_oldColliderExclusionTags.has_value())
        {
            joint->SetColliderExclusionTags(m_oldColliderExclusionTags.value());
        }
        if (m_oldAutoExcludeMode.has_value())
        {
            joint->SetAutoExcludeMode(m_oldAutoExcludeMode.value());
        }
        if (m_oldGeometricAutoExclusion.has_value())
        {
            joint->SetGeometricAutoExclusion(m_oldGeometricAutoExclusion.value());
        }

        if (m_oldColliderExclusionTags.has_value() || m_oldAutoExcludeMode.has_value() || m_oldGeometricAutoExclusion.has_value())
        {
            SimulatedObjectNotificationBus::Broadcast(&SimulatedObjectNotificationBus::Events::OnSimulatedObjectChanged);
        }

        actor->SetDirtyFlag(m_oldDirtyFlag);
        return true;
    }

    void CommandAdjustSimulatedJoint::InitSyntax()
    {
        MCore::CommandSyntax& syntax = GetSyntax();
        syntax.ReserveParameters(11);
        ParameterMixinActorId::InitSyntax(syntax);
        syntax.AddRequiredParameter(s_objectIndexParameterName, "The simulated object index to adjust.", MCore::CommandSyntax::PARAMTYPE_INT);
        syntax.AddRequiredParameter(s_jointIndexParameterName, "The index of the joint inside the simulated object to adjust.", MCore::CommandSyntax::PARAMTYPE_INT);
        syntax.AddParameter(s_coneAngleLimitParameterName, "The new cone angle limit for this joint.", MCore::CommandSyntax::PARAMTYPE_FLOAT, "1.0");
        syntax.AddParameter(s_massParameterName, "The new mass for this joint.", MCore::CommandSyntax::PARAMTYPE_FLOAT, "1.0");
        syntax.AddParameter(s_stiffnessParameterName, "The new stiffness for this joint.", MCore::CommandSyntax::PARAMTYPE_FLOAT, "1.0");
        syntax.AddParameter(s_dampingParameterName, "The new damping for this joint.", MCore::CommandSyntax::PARAMTYPE_FLOAT, "1.0");
        syntax.AddParameter(s_gravityFactorParameterName, "The new gravity factor for this joint.", MCore::CommandSyntax::PARAMTYPE_FLOAT, "1.0");
        syntax.AddParameter(s_frictionParameterName, "The new friction for this joint.", MCore::CommandSyntax::PARAMTYPE_FLOAT, "1.0");
        syntax.AddParameter(s_pinnedParameterName, "The new pinned state for this joint.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "false");
        syntax.AddParameter(s_colliderExclusionTagsParameterName, "Ignore collision detection with the colliders inside this list.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }

    bool CommandAdjustSimulatedJoint::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinActorId::SetCommandParameters(parameters);
        m_objectIndex = parameters.GetValueAsInt(s_objectIndexParameterName, this);
        m_jointIndex = parameters.GetValueAsInt(s_jointIndexParameterName, this);

        if (parameters.CheckIfHasParameter(s_coneAngleLimitParameterName))
        {
            m_coneAngleLimit = parameters.GetValueAsFloat(s_coneAngleLimitParameterName, this);
        }
        if (parameters.CheckIfHasParameter(s_massParameterName))
        {
            m_mass = parameters.GetValueAsFloat(s_massParameterName, this);
        }
        if (parameters.CheckIfHasParameter(s_stiffnessParameterName))
        {
            m_stiffness = parameters.GetValueAsFloat(s_stiffnessParameterName, this);
        }
        if (parameters.CheckIfHasParameter(s_dampingParameterName))
        {
            m_damping = parameters.GetValueAsFloat(s_dampingParameterName, this);
        }
        if (parameters.CheckIfHasParameter(s_gravityFactorParameterName))
        {
            m_gravityFactor = parameters.GetValueAsFloat(s_gravityFactorParameterName, this);
        }
        if (parameters.CheckIfHasParameter(s_frictionParameterName))
        {
            m_friction = parameters.GetValueAsFloat(s_frictionParameterName, this);
        }
        if (parameters.CheckIfHasParameter(s_pinnedParameterName))
        {
            m_pinned = parameters.GetValueAsBool(s_pinnedParameterName, this);
        }
        if (parameters.CheckIfHasParameter(s_colliderExclusionTagsParameterName))
        {
            m_colliderExclusionTags = AZStd::vector<AZStd::string>();
            AzFramework::StringFunc::Tokenize(
                parameters.GetValue(s_colliderExclusionTagsParameterName, this).data(),
                *m_colliderExclusionTags,
                ';'
            );
        }
        if (parameters.CheckIfHasParameter(s_autoExcludeModeParameterName))
        {
            const AZStd::string& modeName = parameters.GetValue(s_autoExcludeModeParameterName, this);
            if (AzFramework::StringFunc::Equal(modeName.c_str(), "All", false))
            {
                m_autoExcludeMode = SimulatedJoint::AutoExcludeMode::All;
            }
            else if (AzFramework::StringFunc::Equal(modeName.c_str(), "None", false))
            {
                m_autoExcludeMode = SimulatedJoint::AutoExcludeMode::None;
            }
            else if (AzFramework::StringFunc::Equal(modeName.c_str(), "Self", false))
            {
                m_autoExcludeMode = SimulatedJoint::AutoExcludeMode::Self;
            }
            else if (AzFramework::StringFunc::Equal(modeName.c_str(), "SelfAndNeighbors", false))
            {
                m_autoExcludeMode = SimulatedJoint::AutoExcludeMode::SelfAndNeighbors;
            }
        }
        if (parameters.CheckIfHasParameter(s_geometricAutoExclusionParameterName))
        {
            m_geometricAutoExclusion = parameters.GetValueAsBool(s_geometricAutoExclusionParameterName, this);
        }
        return true;
    }

    SimulatedJoint* CommandAdjustSimulatedJoint::GetSimulatedJoint() const
    {
        Actor* actor = GetEMotionFX().GetActorManager()->FindActorByID(m_actorId);
        if (!actor)
        {
            return nullptr;
        }

        const AZStd::shared_ptr<SimulatedObjectSetup>& setup = actor->GetSimulatedObjectSetup();
        if (!setup)
        {
            return nullptr;
        }

        SimulatedObject* object = setup->GetSimulatedObject(m_objectIndex);
        if (!object)
        {
            return nullptr;
        }

        SimulatedJoint* joint = object->GetSimulatedJoint(m_jointIndex);
        if (!joint)
        {
            return nullptr;
        }
        return joint;
    }

    void CommandAdjustSimulatedJoint::SetConeAngleLimit(float newConeAngleLimit)
    {
        m_coneAngleLimit = newConeAngleLimit;
    }

    void CommandAdjustSimulatedJoint::SetMass(float newMass)
    {
        m_mass = newMass;
    }

    void CommandAdjustSimulatedJoint::SetStiffness(float newStiffness)
    {
        m_stiffness = newStiffness;
    }

    void CommandAdjustSimulatedJoint::SetDamping(float newDamping)
    {
        m_damping = newDamping;
    }

    void CommandAdjustSimulatedJoint::SetGravityFactor(float newGravityFactor)
    {
        m_gravityFactor = newGravityFactor;
    }

    void CommandAdjustSimulatedJoint::SetFriction(float newFriction)
    {
        m_friction = newFriction;
    }

    void CommandAdjustSimulatedJoint::SetPinned(bool newPinned)
    {
        m_pinned = newPinned;
    }

    void CommandAdjustSimulatedJoint::SetAutoExcludeMode(SimulatedJoint::AutoExcludeMode newMode)
    {
        m_autoExcludeMode = newMode;
    }

    void CommandAdjustSimulatedJoint::SetGeometricAutoExclusion(bool newEnabled)
    {
        m_geometricAutoExclusion = newEnabled;
    }

    void CommandAdjustSimulatedJoint::SetOldConeAngleLimit(float oldConeAngleLimit)
    {
        m_oldConeAngleLimit = oldConeAngleLimit;
    }

    void CommandAdjustSimulatedJoint::SetOldMass(float oldMass)
    {
        m_oldMass = oldMass;
    }

    void CommandAdjustSimulatedJoint::SetOldStiffness(float oldStiffness)
    {
        m_oldStiffness = oldStiffness;
    }

    void CommandAdjustSimulatedJoint::SetOldDamping(float oldDamping)
    {
        m_oldDamping = oldDamping;
    }

    void CommandAdjustSimulatedJoint::SetOldGravityFactor(float oldGravityFactor)
    {
        m_oldGravityFactor = oldGravityFactor;
    }

    void CommandAdjustSimulatedJoint::SetOldFriction(float oldFriction)
    {
        m_oldFriction = oldFriction;
    }

    void CommandAdjustSimulatedJoint::SetOldPinned(bool oldPinned)
    {
        m_oldPinned = oldPinned;
    }

    void CommandAdjustSimulatedJoint::SetOldAutoExcludeMode(SimulatedJoint::AutoExcludeMode oldMode)
    {
        m_oldAutoExcludeMode = oldMode;
    }

    void CommandAdjustSimulatedJoint::SetOldGeometricAutoExclusion(bool oldEnabled)
    {
        m_oldGeometricAutoExclusion = oldEnabled;
    }
} // namespace EMotionFX
