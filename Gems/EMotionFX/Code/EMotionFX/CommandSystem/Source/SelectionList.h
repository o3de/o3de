/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "CommandSystemConfig.h"
#include <EMotionFX/Source/ActorBus.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/ActorInstanceBus.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/AnimGraph.h>

namespace CommandSystem
{
    /**
     * A selection list stores links to objects which are selected at a
     * specific time stamp in a scene.
     */
    class COMMANDSYSTEM_API SelectionList
        : private EMotionFX::ActorNotificationBus::Handler
        , private EMotionFX::ActorInstanceNotificationBus::Handler
    {
        MCORE_MEMORYOBJECTCATEGORY(SelectionList, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_COMMANDSYSTEM);

    public:
        /**
         * The default constructor.
         * Creates an empty selection list.
         */
        SelectionList();

        /**
         * The destructor.
         */
        ~SelectionList();

        /**
         * Get the number of selected nodes.
         * @return The number of selected nodes.
         */
        MCORE_INLINE size_t GetNumSelectedNodes() const                             { return m_selectedNodes.size(); }

        /**
         * Get the number of selected actors
         */
        MCORE_INLINE size_t GetNumSelectedActors() const                            { return m_selectedActors.size(); }

        /**
         * Get the number of selected actor instances.
         * @return The number of selected actor instances.
         */
        MCORE_INLINE size_t GetNumSelectedActorInstances() const                    { return m_selectedActorInstances.size(); }

        /**
         * Get the number of selected motion instances.
         * @return The number of selected motion instances.
         */
        MCORE_INLINE size_t GetNumSelectedMotionInstances() const                   { return m_selectedMotionInstances.size(); }

        /**
         * Get the number of selected motions.
         * @return The number of selected motions.
         */
        MCORE_INLINE size_t GetNumSelectedMotions() const                           { return m_selectedMotions.size(); }

        /**
         * Get the number of selected anim graphs.
         * @return The number of selected anim graphs.
         */
        MCORE_INLINE size_t GetNumSelectedAnimGraphs() const                       { return m_selectedAnimGraphs.size(); }

        /**
         * Get the total number of selected objects.
         * @return The number of selected nodes, actors and motions.
         */
        MCORE_INLINE size_t GetNumTotalItems() const;

        /**
         * Check whether or not the selection list contains any objects.
         * @return True if the selection list is empty / if there are no selected objects, false if not.
         */
        bool GetIsEmpty() const;

        /**
         * Clear the selection list. This will unselect all objects.
         */
        void Clear();

        /**
         * Add a node to the selection list. Select node without replacing the old selection.
         * @param node The node which will be added to the selection.
         */
        void AddNode(EMotionFX::Node* node);

        /**
         * Add actor
         */
        void AddActor(EMotionFX::Actor* actor);

        /**
         * Add an actor instance to the selection list. Select actor instance without replacing the old selection.
         * @param actorInstance The actor which will be added to the selection.
         */
        void AddActorInstance(EMotionFX::ActorInstance* actorInstance);

        /**
         * Add a motion to the selection list. Select motion without replacing the old selection.
         * @param motion The motion which will be added to the selection.
         */
        void AddMotion(EMotionFX::Motion* motion);

        /**
         * Add a motion instance to the selection list. Select motion instance without replacing the old selection.
         * @param motion The motion instance which will be added to the selection.
         */
        void AddMotionInstance(EMotionFX::MotionInstance* motionInstance);

        /**
         * Add an anim graph to the selection list. Selects anim graph without replacing the old selection.
         * @param animGraph The anim graph which will be added to the selection.
         */
        void AddAnimGraph(EMotionFX::AnimGraph* animGraph);

        /**
         * Add a complete selection list to this one. Select motionobjects without replacing the old selection.
         * @param selection The selection list which will be added to the selection.
         */
        void Add(SelectionList& selection);

        /**
         * Get the given node from the selection list.
         * @param index The index of the node to get from the selection list.
         * @return A pointer to the given node from the selection list.
         */
        MCORE_INLINE EMotionFX::Node* GetNode(size_t index) const                               { return m_selectedNodes[index]; }

        /**
         * Get the first node from the selection list.
         * @return A pointer to the first node from the selection list. nullptr is there is no node selected.
         */
        MCORE_INLINE EMotionFX::Node* GetFirstNode() const
        {
            if (m_selectedNodes.empty())
            {
                return nullptr;
            }
            return m_selectedNodes[0];
        }

        /**
         * Get the given actor from the selection list.
         * @param index The index of the actor to get from the selection list.
         * @return A pointer to the given actor from the selection list.
         */
        MCORE_INLINE EMotionFX::Actor* GetActor(size_t index) const                             { return m_selectedActors[index]; }

        /**
         * Get the first actor from the selection list.
         * @return A pointer to the first actor from the selection list. nullptr is there is no actor selected.
         */
        MCORE_INLINE EMotionFX::Actor* GetFirstActor() const
        {
            if (m_selectedActors.empty())
            {
                return nullptr;
            }
            return m_selectedActors[0];
        }

        /**
         * Get the given actor instance from the selection list.
         * @param index The index of the actor instance to get from the selection list.
         * @return A pointer to the given actor instance from the selection list.
         */
        MCORE_INLINE EMotionFX::ActorInstance* GetActorInstance(size_t index) const             { return m_selectedActorInstances[index]; }

        /**
         * Get the first actor instance from the selection list.
         * @return A pointer to the first actor instance from the selection list. nullptr if there is no actor instance selected.
         */
        MCORE_INLINE EMotionFX::ActorInstance* GetFirstActorInstance() const
        {
            if (m_selectedActorInstances.empty())
            {
                return nullptr;
            }
            return m_selectedActorInstances[0];
        }

        /**
         * Get the given anim graph from the selection list.
         * @param index The index of the anim graph to get from the selection list.
         * @return A pointer to the given anim graph from the selection list.
         */
        MCORE_INLINE EMotionFX::AnimGraph* GetAnimGraph(size_t index) const                   { return m_selectedAnimGraphs[index]; }

        /**
         * Get the first anim graph from the selection list.
         * @return A pointer to the first anim graph from the selection list. nullptr if there is no anim graph selected.
         */
        MCORE_INLINE EMotionFX::AnimGraph* GetFirstAnimGraph() const
        {
            if (m_selectedAnimGraphs.empty())
            {
                return nullptr;
            }
            return m_selectedAnimGraphs[0];
        }

        /**
         * Get the single selected actor.
         * @return A pointer to the only selected actor. nullptr is there if no actor selected at all or if there are multiple, so more than one, actor instances selected.
         */
        EMotionFX::Actor* GetSingleActor() const;

        /**
         * Get the single selected actor instance.
         * @return A pointer to the only selected actor instance. nullptr is there if no actor instance selected at all or if there are multiple, so more than one, actor instances selected.
         */
        EMotionFX::ActorInstance* GetSingleActorInstance() const;

        /**
         * Get the given motion from the selection list.
         * @param index The index of the motion to get from the selection list.
         * @return A pointer to the given motion from the selection list.
         */
        MCORE_INLINE EMotionFX::Motion* GetMotion(size_t index) const                           { return m_selectedMotions[index]; }

        /**
         * Get the first motion from the selection list.
         * @return A pointer to the first motion from the selection list. nullptr is there is no motion selected.
         */
        MCORE_INLINE EMotionFX::Motion* GetFirstMotion() const
        {
            if (m_selectedMotions.empty())
            {
                return nullptr;
            }
            return m_selectedMotions[0];
        }

        /**
         * Get the single selected motion.
         * @return A pointer to the only selected motion. nullptr is there if no motion selected at all or if there are multiple, so more than one, motions selected.
         */
        EMotionFX::Motion* GetSingleMotion() const;

        /**
         * Get the given motion instance from the selection list.
         * @param index The index of the motion instance to get from the selection list.
         * @return A pointer to the given motion instance from the selection list.
         */
        MCORE_INLINE EMotionFX::MotionInstance* GetMotionInstance(size_t index) const           { return m_selectedMotionInstances[index]; }

        /**
         * Get the first motion instance from the selection list.
         * @return A pointer to the first motion instance from the selection list. nullptr is there is no motion instance selected.
         */
        MCORE_INLINE EMotionFX::MotionInstance* GetFirstMotionInstance() const
        {
            if (m_selectedMotionInstances.empty())
            {
                return nullptr;
            }
            return m_selectedMotionInstances[0];
        }

        /**
         * Remove the given node from the selection list.
         * @param index The index of the node to be removed from the selection list.
         */
        MCORE_INLINE void RemoveNode(size_t index)                                              { m_selectedNodes.erase(m_selectedNodes.begin() + index); }

        /**
         * Remove the given actor instance from the selection list.
         * @param index The index of the actor instance to be removed from the selection list.
         */
        MCORE_INLINE void RemoveActor(size_t index)                                             { m_selectedActors.erase(m_selectedActors.begin() + index); }

        /**
         * Remove the given actor instance from the selection list.
         * @param index The index of the actor instance to be removed from the selection list.
         */
        MCORE_INLINE void RemoveActorInstance(size_t index)                                     { m_selectedActorInstances.erase(m_selectedActorInstances.begin() + index); }

        /**
         * Remove the given motion from the selection list.
         * @param index The index of the motion to be removed from the selection list.
         */
        MCORE_INLINE void RemoveMotion(size_t index)                                            { m_selectedMotions.erase(m_selectedMotions.begin() + index); }

        /**
         * Remove the given motion instance from the selection list.
         * @param index The index of the motion instance to be removed from the selection list.
         */
        MCORE_INLINE void RemoveMotionInstance(size_t index)                                    { m_selectedMotionInstances.erase(m_selectedMotionInstances.begin() + index); }

        /**
         * Remove the given anim graph from the selection list.
         * @param index The index of the anim graph to remove from the selection list.
         */
        MCORE_INLINE void RemoveAnimGraph(size_t index)                                        { m_selectedAnimGraphs.erase(m_selectedAnimGraphs.begin() + index); }

        /**
         * Remove the given node from the selection list.
         * @param node The node to be removed from the selection list.
         */
        void RemoveNode(EMotionFX::Node* node);

        /**
         * Remove the given actor instance from the selection list.
         * @param actorInstance The actor instance to be removed from the selection list.
         */
        void RemoveActor(EMotionFX::Actor* actor);

        /**
         * Remove the given actor instance from the selection list.
         * @param actorInstance The actor instance to be removed from the selection list.
         */
        void RemoveActorInstance(EMotionFX::ActorInstance* actorInstance);

        /**
         * Remove the given motion from the selection list.
         * @param motion The motion to be removed from the selection list.
         */
        void RemoveMotion(EMotionFX::Motion* motion);

        /**
         * Remove the given motion instance from the selection list.
         * @param motionInstance The motion instance to be removed from the selection list.
         */
        void RemoveMotionInstance(EMotionFX::MotionInstance* motionInstance);

        /**
         * Remove the given anim graph from the selection list.
         * @param animGraph The anim graph to be removed from the list.
         */
        void RemoveAnimGraph(EMotionFX::AnimGraph* animGraph);

        /**
         * Check if a given node is selected / is in this selection list.
         * @param node A pointer to the node to be checked.
         * @return True if the node is inside this selection list, false if not.
         */
        MCORE_INLINE bool CheckIfHasNode(EMotionFX::Node* node) const                                   { return(AZStd::find(m_selectedNodes.begin(), m_selectedNodes.end(), node) != m_selectedNodes.end()); }

        /**
         * Has actor
         */
        MCORE_INLINE bool CheckIfHasActor(EMotionFX::Actor* actor) const                                { return (AZStd::find(m_selectedActors.begin(), m_selectedActors.end(), actor) != m_selectedActors.end()); }

        /**
         * Check if a given actor instance is selected / is in this selection list.
         * @param node A pointer to the actor instance to be checked.
         * @return True if the actor instance is inside this selection list, false if not.
         */
        MCORE_INLINE bool CheckIfHasActorInstance(EMotionFX::ActorInstance* actorInstance) const        { return (AZStd::find(m_selectedActorInstances.begin(), m_selectedActorInstances.end(), actorInstance) != m_selectedActorInstances.end()); }

        /**
         * Check if a given motion is selected / is in this selection list.
         * @param node A pointer to the motion to be checked.
         * @return True if the motion is inside this selection list, false if not.
         */
        MCORE_INLINE bool CheckIfHasMotion(EMotionFX::Motion* motion) const                             { return (AZStd::find(m_selectedMotions.begin(), m_selectedMotions.end(), motion) != m_selectedMotions.end()); }

        /**
         * Check if a given anim graph is in this selection list.
         * @param animGraph The anim graph to check for.
         * @return True if the anim graph is selected inside the selection list, otherwise false is returned.
         */
        MCORE_INLINE bool CheckIfHasAnimGraph(EMotionFX::AnimGraph* animGraph) const                 { return (AZStd::find(m_selectedAnimGraphs.begin(), m_selectedAnimGraphs.end(), animGraph) != m_selectedAnimGraphs.end()); }

        /**
         * Check if a given motion instance is selected / is in this selection list.
         * @param node A pointer to the motion instance to be checked.
         * @return True if the motion instance is inside this selection list, false if not.
         */
        MCORE_INLINE bool CheckIfHasMotionInstance(EMotionFX::MotionInstance* motionInstance) const     { return (AZStd::find(m_selectedMotionInstances.begin(), m_selectedMotionInstances.end(), motionInstance) != m_selectedMotionInstances.end()); }

        MCORE_INLINE void ClearActorSelection()                                     { m_selectedActors.clear(); }
        MCORE_INLINE void ClearActorInstanceSelection()                             { m_selectedActorInstances.clear(); }
        MCORE_INLINE void ClearNodeSelection()                                      { m_selectedNodes.clear(); }
        MCORE_INLINE void ClearMotionSelection()                                    { m_selectedMotions.clear(); }
        MCORE_INLINE void ClearMotionInstanceSelection()                            { m_selectedMotionInstances.clear(); }
        MCORE_INLINE void ClearAnimGraphSelection()                                 { m_selectedAnimGraphs.clear(); }

        void Log();
        void MakeValid();

    private:
        // ActorNotificationBus overrides
        void OnActorDestroyed(EMotionFX::Actor* actor) override;

        // ActorInstanceNotificationBus overrides
        void OnActorInstanceDestroyed(EMotionFX::ActorInstance* actorInstance) override;

        AZStd::vector<EMotionFX::Node*>             m_selectedNodes;             /**< Array of selected nodes. */
        AZStd::vector<EMotionFX::Actor*>            m_selectedActors;            /**< The selected actors.  */
        AZStd::vector<EMotionFX::ActorInstance*>    m_selectedActorInstances;    /**< Array of selected actor instances. */
        AZStd::vector<EMotionFX::MotionInstance*>   m_selectedMotionInstances;   /**< Array of selected motion instances. */
        AZStd::vector<EMotionFX::Motion*>           m_selectedMotions;           /**< Array of selected motions. */
        AZStd::vector<EMotionFX::AnimGraph*>        m_selectedAnimGraphs;        /**< Array of selected anim graphs. */
    };
} // namespace CommandSystem
