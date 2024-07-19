/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/functional.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/Attachment.h>
#include <EMotionFX/Source/BlendTreeSimulatedObjectNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/PhysicsSetup.h>
#include <EMotionFX/Source/Pose.h>
#include <EMotionFX/Source/SimulatedObjectSetup.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeSimulatedObjectNode, AnimGraphAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeSimulatedObjectNode::Simulation, AnimGraphAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeSimulatedObjectNode::UniqueData, AnimGraphObjectUniqueDataAllocator)

    BlendTreeSimulatedObjectNode::UniqueData::UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
        : AnimGraphNodeData(node, animGraphInstance)
    {
    }

    BlendTreeSimulatedObjectNode::UniqueData::~UniqueData()
    {
        for (Simulation* simulation : m_simulations)
        {
            delete simulation;
        }
    }

    void BlendTreeSimulatedObjectNode::UniqueData::Update()
    {
        BlendTreeSimulatedObjectNode* simulatedObjectNode = azdynamic_cast<BlendTreeSimulatedObjectNode*>(m_object);
        AZ_Assert(simulatedObjectNode, "Unique data linked to incorrect node type.");

        const bool solverInitResult = simulatedObjectNode->InitSolvers(GetAnimGraphInstance(), this);
        SetHasError(!solverInitResult);
    }

    BlendTreeSimulatedObjectNode::BlendTreeSimulatedObjectNode()
        : AnimGraphNode()
    {
        // Setup the input ports.
        InitInputPorts(5);
        SetupInputPort("Pose", INPUTPORT_POSE, AttributePose::TYPE_ID, PORTID_INPUT_POSE);
        SetupInputPortAsNumber("Stiffness factor", INPUTPORT_STIFFNESSFACTOR, PORTID_INPUT_STIFFNESSFACTOR);
        SetupInputPortAsNumber("Gravity factor", INPUTPORT_GRAVITYFACTOR, PORTID_INPUT_GRAVITYFACTOR);
        SetupInputPortAsNumber("Damping factor", INPUTPORT_DAMPINGFACTOR, PORTID_INPUT_DAMPINGFACTOR);
        SetupInputPortAsBool("Active", INPUTPORT_ACTIVE, PORTID_INPUT_ACTIVE);

        // Setup the output ports.
        InitOutputPorts(1);
        SetupOutputPortAsPose("Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
    }

    BlendTreeSimulatedObjectNode::~BlendTreeSimulatedObjectNode()
    {
        SimulatedObjectNotificationBus::Handler::BusDisconnect();
    }

    void BlendTreeSimulatedObjectNode::Reinit()
    {
        if (!m_animGraph)
        {
            return;
        }

        SimulatedObjectNotificationBus::Handler::BusConnect();
        AnimGraphNode::Reinit();
    }

    bool BlendTreeSimulatedObjectNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();
        Reinit();
        return true;
    }

    const char* BlendTreeSimulatedObjectNode::GetPaletteName() const
    {
        return "Simulated Object";
    }

    AnimGraphObject::ECategory BlendTreeSimulatedObjectNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_PHYSICS;
    }

    float BlendTreeSimulatedObjectNode::GetStiffnessFactor(AnimGraphInstance* animGraphInstance) const
    {
        const MCore::AttributeFloat* input = GetInputFloat(animGraphInstance, INPUTPORT_STIFFNESSFACTOR);
        return input ? input->GetValue() : m_stiffnessFactor;
    }

    float BlendTreeSimulatedObjectNode::GetGravityFactor(AnimGraphInstance* animGraphInstance) const
    {
        const MCore::AttributeFloat* input = GetInputFloat(animGraphInstance, INPUTPORT_GRAVITYFACTOR);
        return input ? input->GetValue() : m_gravityFactor;
    }

    float BlendTreeSimulatedObjectNode::GetDampingFactor(AnimGraphInstance* animGraphInstance) const
    {
        const MCore::AttributeFloat* input = GetInputFloat(animGraphInstance, INPUTPORT_DAMPINGFACTOR);
        return input ? input->GetValue() : m_dampingFactor;
    }

    void BlendTreeSimulatedObjectNode::Rewind(AnimGraphInstance* animGraphInstance)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        for (Simulation* sim : uniqueData->m_simulations)
        {
            sim->m_solver.Stabilize();
        }
    }

    bool BlendTreeSimulatedObjectNode::InitSolvers(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData)
    {
        // Delete existing solvers
        for (Simulation* sim : uniqueData->m_simulations)
        {
            delete sim;
        }
        uniqueData->m_simulations.clear();

        if (GetEMotionFX().GetEnableServerOptimization())
        {
            // Doesn't need to init solvers when server optimization is enabled.
            return false;
        }

        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        const Actor* actor = actorInstance->GetActor();
        const SimulatedObjectSetup* simObjectSetup = actor->GetSimulatedObjectSetup().get();
        if (!simObjectSetup)
        {
            //AZ_Warning("EMotionFX", false, "The actor has no simulated object setup, so the simulated object anim graph node '%s' cannot do anything.", GetName());
            return false;
        }

        // Give a warning for simulated object names that have been setup in this node, but that do not exist in the simulated object setup of this actor.
        for (const AZStd::string& simObjectName : m_simulatedObjectNames)
        {
            if (!simObjectSetup->FindSimulatedObjectByName(simObjectName.c_str()))
            {
                //AZ_Warning("EMotionFX", false, "Anim graph simulated object node '%s' references a simulated object with the name '%s', which does not exist in the simulated object setup for this actor.", GetName(), simObjectName.c_str());
            }
        }

        // Create and init a solver for each simulated object.
        auto& simulations = uniqueData->m_simulations;
        simulations.reserve(simObjectSetup->GetNumSimulatedObjects());
        for (const SimulatedObject* simObject : simObjectSetup->GetSimulatedObjects())
        {
            // Check if this simulated object is in our list of simulated objects that the user picked.
            // If not, then we can skip adding this simulated object.
            if (!m_simulatedObjectNames.empty())
            {
                if (AZStd::find(m_simulatedObjectNames.begin(), m_simulatedObjectNames.end(), simObject->GetName()) == m_simulatedObjectNames.end())
                {
                    continue;
                }
            }

            // Create the simulation, which holds the solver.
            Simulation* sim = aznew Simulation();
            SpringSolver& solver = sim->m_solver;
            sim->m_simulatedObject = simObject;

            // Initialize the solver inside this sim object.
            SpringSolver::InitSettings initSettings;
            initSettings.m_actorInstance = actorInstance;
            initSettings.m_simulatedObject = simObject;
            initSettings.m_colliderTags = simObject->GetColliderTags();
            initSettings.m_name = GetName(); // The name is the anim graph node's name, used when printing some warning/error messages.
            if (!solver.Init(initSettings))
            {
                delete sim;
                continue;
            }
            solver.SetNumIterations(m_numIterations);
            solver.SetCollisionEnabled(m_collisionDetection);

            simulations.emplace_back(sim);
        }

        return true;
    }

    void BlendTreeSimulatedObjectNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        AnimGraphNode::Update(animGraphInstance, timePassedInSeconds);

        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        uniqueData->m_timePassedInSeconds = timePassedInSeconds;
    }

    void BlendTreeSimulatedObjectNode::Output(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphPose* outputPose;

        // If nothing is connected to the input pose, output a bind pose.
        if (!GetInputPort(INPUTPORT_POSE).m_connection)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
            outputPose->InitFromBindPose(actorInstance);
            return;
        }

        // Check whether we are active or not.
        bool isActive = true;
        if (GetInputPort(INPUTPORT_ACTIVE).m_connection)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_ACTIVE));
            isActive = GetInputNumberAsBool(animGraphInstance, INPUTPORT_ACTIVE);
        }

        // If we're not active or if this node is disabled or it is optimized for server, we can skip all calculations and just output the input pose.
        if (!isActive || m_disabled || GetEMotionFX().GetEnableServerOptimization())
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE));
            const AnimGraphPose* inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            *outputPose = *inputPose;
            return;
        }

        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_STIFFNESSFACTOR));
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_GRAVITYFACTOR));
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_DAMPINGFACTOR));

        // Get the input pose and copy it over to the output pose
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE));
        AnimGraphPose* inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
        RequestPoses(animGraphInstance);
        outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
        *outputPose = *inputPose;

        // Check if we have a valid configuration.
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        if (uniqueData->GetHasError())
        {
            if (GetEMotionFX().GetIsInEditorMode())
            {
                SetHasError(uniqueData, true);
            }
            return;
        }

        if (GetEMotionFX().GetIsInEditorMode())
        {
            SetHasError(uniqueData, false);
        }

        // If we are an attachment, update the transforms in the output pose.
        // It is possible that we are a skin attachment and we copy transforms from the main skeleton.
        Attachment* attachment = animGraphInstance->GetActorInstance()->GetSelfAttachment();
        if (attachment)
        {
            attachment->UpdateJointTransforms(outputPose->GetPose());
        }

        // Perform the solver update, and modify the output pose.
        for (Simulation* sim : uniqueData->m_simulations)
        {
            SpringSolver& solver = sim->m_solver;
            solver.SetStiffnessFactor(GetStiffnessFactor(animGraphInstance));
            solver.SetGravityFactor(GetGravityFactor(animGraphInstance));
            solver.SetDampingFactor(GetDampingFactor(animGraphInstance));
            solver.SetCollisionEnabled(m_collisionDetection);
            solver.Update(inputPose->GetPose(), outputPose->GetPose(), uniqueData->m_timePassedInSeconds);
        }

        // Debug draw.
        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            for (const Simulation* sim : uniqueData->m_simulations)
            {
                sim->m_solver.DebugRender(outputPose->GetPose(), m_collisionDetection, true, m_visualizeColor);
            }
        }
    }

    void BlendTreeSimulatedObjectNode::OnSimulatedObjectChanged()
    {
        InvalidateUniqueDatas();
    }

    void BlendTreeSimulatedObjectNode::SetSimulatedObjectNames(const AZStd::vector<AZStd::string>& simObjectNames)
    {
        m_simulatedObjectNames = simObjectNames;
    }

    void BlendTreeSimulatedObjectNode::AdjustParticles(const SpringSolver::ParticleAdjustFunction& func)
    {
        if (!m_animGraph)
        {
            return;
        }

        const size_t numInstances = m_animGraph->GetNumAnimGraphInstances();
        for (size_t i = 0; i < numInstances; ++i)
        {
            AnimGraphInstance* animGraphInstance = m_animGraph->GetAnimGraphInstance(i);
            UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueNodeData(this));
            if (!uniqueData)
            {
                continue;
            }

            for (Simulation* sim : uniqueData->m_simulations)
            {
                sim->m_solver.AdjustParticles(func);
            }
        }
    }

    void BlendTreeSimulatedObjectNode::OnPropertyChanged(const PropertyChangeFunction& func)
    {
        if (!m_animGraph)
        {
            return;
        }

        const size_t numInstances = m_animGraph->GetNumAnimGraphInstances();
        for (size_t i = 0; i < numInstances; ++i)
        {
            AnimGraphInstance* animGraphInstance = m_animGraph->GetAnimGraphInstance(i);
            UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueNodeData(this));
            if (!uniqueData)
            {
                continue;
            }

            func(uniqueData);
        }
    }

    void BlendTreeSimulatedObjectNode::OnNumIterationsChanged()
    {
        OnPropertyChanged([this](UniqueData* uniqueData) {
            for (Simulation* sim : uniqueData->m_simulations)
            {
                sim->m_solver.SetNumIterations(m_numIterations);
            }
        });
    }


    bool BlendTreeSimulatedObjectNode::VersionConverter([[maybe_unused]] AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElementNode)
    {
        if (rootElementNode.GetVersion() == 1)
        {
            rootElementNode.RemoveElementByName(AZ_CRC_CE("simulationRate"));
        }

        return true;
    }

    void BlendTreeSimulatedObjectNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeSimulatedObjectNode, AnimGraphNode>()
            ->Version(2, VersionConverter)
            ->Field("simulatedObjectNames", &BlendTreeSimulatedObjectNode::m_simulatedObjectNames)
            ->Field("stiffnessFactor", &BlendTreeSimulatedObjectNode::m_stiffnessFactor)
            ->Field("gravityFactor", &BlendTreeSimulatedObjectNode::m_gravityFactor)
            ->Field("dampingFactor", &BlendTreeSimulatedObjectNode::m_dampingFactor)
            ->Field("numIterations", &BlendTreeSimulatedObjectNode::m_numIterations)
            ->Field("collisionDetection", &BlendTreeSimulatedObjectNode::m_collisionDetection);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeSimulatedObjectNode>("Simulated objects", "Simulated objects settings")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ_CRC_CE("SimulatedObjectSelection"), &BlendTreeSimulatedObjectNode::m_simulatedObjectNames, "Simulated object names", "The simulated objects we want to pick from this actor.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeSimulatedObjectNode::Reinit)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->ElementAttribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &BlendTreeSimulatedObjectNode::m_gravityFactor, "Gravity factor", "The gravity multiplier, which is a multiplier over the individual joint gravity values.")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, 20.0f)
                ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &BlendTreeSimulatedObjectNode::m_stiffnessFactor, "Stiffness factor", "The stiffness multiplier, which is a multiplier over the individual joint stiffness values.")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &BlendTreeSimulatedObjectNode::m_dampingFactor, "Damping factor", "The damping multiplier, which is a multiplier over the individual joint damping values.")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &BlendTreeSimulatedObjectNode::m_numIterations, "Number of iterations", "The number of iterations in the simulation. Higher values can be more stable. Lower numbers give faster performance.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeSimulatedObjectNode::OnNumIterationsChanged)
                ->Attribute(AZ::Edit::Attributes::Min, 1)
                ->Attribute(AZ::Edit::Attributes::Max, 10)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeSimulatedObjectNode::m_collisionDetection, "Enable collisions", "Enable collision detection with its colliders?");
    }
} // namespace EMotionFX
