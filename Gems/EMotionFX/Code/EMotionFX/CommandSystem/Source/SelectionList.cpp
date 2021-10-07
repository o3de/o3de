/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SelectionList.h"
#include <MCore/Source/LogManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/ActorManager.h>

namespace CommandSystem
{
    SelectionList::SelectionList()
    {
        EMotionFX::ActorNotificationBus::Handler::BusConnect();
        EMotionFX::ActorInstanceNotificationBus::Handler::BusConnect();
    }

    SelectionList::~SelectionList()
    {
        EMotionFX::ActorInstanceNotificationBus::Handler::BusDisconnect();
        EMotionFX::ActorNotificationBus::Handler::BusDisconnect();
    }

    size_t SelectionList::GetNumTotalItems() const
    {
        return m_selectedNodes.size() +
            m_selectedActors.size() +
            m_selectedActorInstances.size() +
            m_selectedMotions.size() +
            m_selectedMotionInstances.size() +
            m_selectedAnimGraphs.size();
    }

    bool SelectionList::GetIsEmpty() const
    {
        return (m_selectedNodes.empty() &&
            m_selectedActors.empty() &&
            m_selectedActorInstances.empty() &&
            m_selectedMotions.empty() &&
            m_selectedMotionInstances.empty() &&
            m_selectedAnimGraphs.empty());
    }

    void SelectionList::Clear()
    {
        m_selectedNodes.clear();
        m_selectedActors.clear();
        m_selectedActorInstances.clear();
        m_selectedMotions.clear();
        m_selectedMotionInstances.clear();
        m_selectedAnimGraphs.clear();
    }

    void SelectionList::AddNode(EMotionFX::Node* node)
    {
        if (!CheckIfHasNode(node))
        {
            m_selectedNodes.emplace_back(node);
        }
    }

    void SelectionList::AddActor(EMotionFX::Actor* actor)
    {
        if (!CheckIfHasActor(actor))
        {
            m_selectedActors.emplace_back(actor);
        }
    }

    // add an actor instance to the selection list
    void SelectionList::AddActorInstance(EMotionFX::ActorInstance* actorInstance)
    {
        if (!CheckIfHasActorInstance(actorInstance))
        {
            m_selectedActorInstances.emplace_back(actorInstance);
        }
    }


    // add a motion to the selection list
    void SelectionList::AddMotion(EMotionFX::Motion* motion)
    {
        if (!CheckIfHasMotion(motion))
        {
            m_selectedMotions.emplace_back(motion);
        }
    }


    // add a motion instance to the selection list
    void SelectionList::AddMotionInstance(EMotionFX::MotionInstance* motionInstance)
    {
        if (!CheckIfHasMotionInstance(motionInstance))
        {
            m_selectedMotionInstances.emplace_back(motionInstance);
        }
    }


    // add a anim graph to the selection list
    void SelectionList::AddAnimGraph(EMotionFX::AnimGraph* animGraph)
    {
        if (!CheckIfHasAnimGraph(animGraph))
        {
            m_selectedAnimGraphs.emplace_back(animGraph);
        }
    }

    // add a complete selection list to this one
    void SelectionList::Add(SelectionList& selection)
    {
        // get the number of selected objects
        const size_t numSelectedNodes           = selection.GetNumSelectedNodes();
        const size_t numSelectedActors          = selection.GetNumSelectedActors();
        const size_t numSelectedActorInstances  = selection.GetNumSelectedActorInstances();
        const size_t numSelectedMotions         = selection.GetNumSelectedMotions();
        const size_t numSelectedMotionInstances = selection.GetNumSelectedMotionInstances();
        const size_t numSelectedAnimGraphs      = selection.GetNumSelectedAnimGraphs();

        // iterate through all nodes and select them
        for (size_t i = 0; i < numSelectedNodes; ++i)
        {
            AddNode(selection.GetNode(i));
        }

        // iterate through all actors and select them
        for (size_t i = 0; i < numSelectedActors; ++i)
        {
            AddActor(selection.GetActor(i));
        }

        // iterate through all actor instances and select them
        for (size_t i = 0; i < numSelectedActorInstances; ++i)
        {
            AddActorInstance(selection.GetActorInstance(i));
        }

        // iterate through all motions and select them
        for (size_t i = 0; i < numSelectedMotions; ++i)
        {
            AddMotion(selection.GetMotion(i));
        }

        // iterate through all motion instances and select them
        for (size_t i = 0; i < numSelectedMotionInstances; ++i)
        {
            AddMotionInstance(selection.GetMotionInstance(i));
        }

        // iterate through all anim graphs and select them
        for (size_t i = 0; i < numSelectedAnimGraphs; ++i)
        {
            AddAnimGraph(selection.GetAnimGraph(i));
        }
    }


    // log the current selection
    void SelectionList::Log()
    {
        // get the number of selected objects
        const size_t numSelectedNodes           = GetNumSelectedNodes();
        const size_t numSelectedActorInstances  = GetNumSelectedActorInstances();
        const size_t numSelectedActors          = GetNumSelectedActors();
        const size_t numSelectedMotions         = GetNumSelectedMotions();
        const size_t numSelectedAnimGraphs      = GetNumSelectedAnimGraphs();

        MCore::LogInfo("SelectionList:");

        // iterate through all nodes and select them
        MCore::LogInfo(" - Nodes (%i)", numSelectedNodes);
        for (size_t i = 0; i < numSelectedNodes; ++i)
        {
            MCore::LogInfo("    + Node #%.3d: name='%s'", i, GetNode(i)->GetName());
        }

        // iterate through all actors and select them
        MCore::LogInfo(" - Actors (%i)", numSelectedActors);
        for (size_t i = 0; i < numSelectedActors; ++i)
        {
            MCore::LogInfo("    + Actor #%.3d: name='%s'", i, GetActor(i)->GetName());
        }

        // iterate through all actor instances and select them
        MCore::LogInfo(" - Actor instances (%i)", numSelectedActorInstances);
        for (size_t i = 0; i < numSelectedActorInstances; ++i)
        {
            MCore::LogInfo("    + Actor instance #%.3d: name='%s'", i, GetActorInstance(i)->GetActor()->GetName());
        }

        // iterate through all motions and select them
        MCore::LogInfo(" - Motions (%i)", numSelectedMotions);
        for (size_t i = 0; i < numSelectedMotions; ++i)
        {
            MCore::LogInfo("    + Motion #%.3d: name='%s'", i, GetMotion(i)->GetName());
        }

        // iterate through all motions and select them
        MCore::LogInfo(" - AnimGraphs (%i)", numSelectedAnimGraphs);
        for (size_t i = 0; i < numSelectedAnimGraphs; ++i)
        {
            MCore::LogInfo("    + AnimGraph #%.3d: %s", i, GetAnimGraph(i)->GetFileName());
        }
    }

    void SelectionList::RemoveNode(EMotionFX::Node* node)
    {
        m_selectedNodes.erase(AZStd::remove(m_selectedNodes.begin(), m_selectedNodes.end(), node), m_selectedNodes.end());
    }

    void SelectionList::RemoveActor(EMotionFX::Actor* actor)
    {
        m_selectedActors.erase(AZStd::remove(m_selectedActors.begin(), m_selectedActors.end(), actor), m_selectedActors.end());
    }

    // remove the actor from the selection list
    void SelectionList::RemoveActorInstance(EMotionFX::ActorInstance* actorInstance)
    {
        m_selectedActorInstances.erase(AZStd::remove(m_selectedActorInstances.begin(), m_selectedActorInstances.end(), actorInstance), m_selectedActorInstances.end());
    }


    // remove the motion from the selection list
    void SelectionList::RemoveMotion(EMotionFX::Motion* motion)
    {
        m_selectedMotions.erase(AZStd::remove(m_selectedMotions.begin(), m_selectedMotions.end(), motion), m_selectedMotions.end());
    }


    // remove the motion instance from the selection list
    void SelectionList::RemoveMotionInstance(EMotionFX::MotionInstance* motionInstance)
    {
        m_selectedMotionInstances.erase(AZStd::remove(m_selectedMotionInstances.begin(), m_selectedMotionInstances.end(), motionInstance), m_selectedMotionInstances.end());
    }


    // remove the anim graph
    void SelectionList::RemoveAnimGraph(EMotionFX::AnimGraph* animGraph)
    {
        m_selectedAnimGraphs.erase(AZStd::remove(m_selectedAnimGraphs.begin(), m_selectedAnimGraphs.end(), animGraph), m_selectedAnimGraphs.end());
    }

    // make the selection valid
    void SelectionList::MakeValid()
    {
        // iterate through all actor instances and remove the invalid ones
        for (size_t i = 0; i < m_selectedActorInstances.size();)
        {
            EMotionFX::ActorInstance* actorInstance = m_selectedActorInstances[i];

            if (EMotionFX::GetActorManager().CheckIfIsActorInstanceRegistered(actorInstance) == false)
            {
                m_selectedActorInstances.erase(m_selectedActorInstances.begin() + i);
            }
            else
            {
                i++;
            }
        }

        // iterate through all anim graphs and remove all valid ones
        for (size_t i = 0; i < m_selectedAnimGraphs.size();)
        {
            EMotionFX::AnimGraph* animGraph = m_selectedAnimGraphs[i];

            if (EMotionFX::GetAnimGraphManager().FindAnimGraphIndex(animGraph) == MCORE_INVALIDINDEX32)
            {
                m_selectedAnimGraphs.erase(m_selectedAnimGraphs.begin() + i);
            }
            else
            {
                i++;
            }
        }
    }



    // get a single selected actor
    EMotionFX::Actor* SelectionList::GetSingleActor() const
    {
        if (GetNumSelectedActors() == 0)
        {
            return nullptr;
        }

        if (GetNumSelectedActors() > 1)
        {
            //LogDebug("Cannot get single selected actor. Multiple actors selected.");
            return nullptr;
        }

        return m_selectedActors[0];
    }


    // get a single selected actor instance
    EMotionFX::ActorInstance* SelectionList::GetSingleActorInstance() const
    {
        if (GetNumSelectedActorInstances() == 0)
        {
            //LogDebug("Cannot get single selected actor instance. No actor instance selected.");
            return nullptr;
        }

        if (GetNumSelectedActorInstances() > 1)
        {
            //LogDebug("Cannot get single selected actor instance. Multiple actor instances selected.");
            return nullptr;
        }

        EMotionFX::ActorInstance* actorInstance = m_selectedActorInstances[0];
        if (!actorInstance)
        {
            return nullptr;
        }

        if (actorInstance->GetIsOwnedByRuntime())
        {
            return nullptr;
        }

        return m_selectedActorInstances[0];
    }


    // get a single selected motion
    EMotionFX::Motion* SelectionList::GetSingleMotion() const
    {
        if (GetNumSelectedMotions() == 0)
        {
            return nullptr;
        }

        if (GetNumSelectedMotions() > 1)
        {
            return nullptr;
        }

        EMotionFX::Motion* motion = m_selectedMotions[0];
        if (!motion)
        {
            return nullptr;
        }

        if (motion->GetIsOwnedByRuntime())
        {
            return nullptr;
        }

        return motion;
    }

    void SelectionList::OnActorDestroyed(EMotionFX::Actor* actor)
    {
        const EMotionFX::Skeleton* skeleton = actor->GetSkeleton();
        const size_t numJoints = skeleton->GetNumNodes();
        for (size_t i = 0; i < numJoints; ++i)
        {
            EMotionFX::Node* joint = skeleton->GetNode(i);
            RemoveNode(joint);
        }

        RemoveActor(actor);
    }

    void SelectionList::OnActorInstanceDestroyed(EMotionFX::ActorInstance* actorInstance)
    {
        RemoveActorInstance(actorInstance);
    }
} // namespace CommandSystem
