/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Core.h"
#include "Attributes.h"
#include "Contract.h"

#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/ObjectStream.h>

namespace AZ
{
    class SerializeContext;
}

namespace ScriptCanvas
{
    class Node;
    class Graph;
    class BehaviorContextObject;

    namespace Execution
    {
        class PerformanceTracker;
    }

    struct SystemComponentConfiguration
    {
        //! Script Canvas offers infinite loop protection, this allows to specify the max number of iterations to attempt before deciding execution is likely an infinite loop
        int m_maxIterationsForInfiniteLoopDetection; 
        int m_maxHandlerStackDepth;
    };

    ////////////////////////////////////////////////////////////////
    // SystemRequests 
    ////////////////////////////////////////////////////////////////
    class SystemRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using MutexType = AZStd::recursive_mutex;
        static const bool LocklessDispatch = true;

        virtual bool IsScriptUnitTestingInProgress() = 0;
        virtual void MarkScriptUnitTestBegin() = 0;
        virtual void MarkScriptUnitTestEnd() = 0;

        //! Create all the components that entity requires to execute the Script Canvas engine
        virtual void CreateEngineComponentsOnEntity(AZ::Entity* entity) = 0;
        //! Create a graph and attach it to the supplied Entity
        virtual Graph* CreateGraphOnEntity(AZ::Entity*) = 0;

        //! Create a graph, a pointer to the graph/
        //! The Init() function is not called on the graph to remapping of Entity Id's to still work
        virtual ScriptCanvas::Graph* MakeGraph() = 0;
        virtual ScriptCanvasId FindScriptCanvasId(AZ::Entity* /*graphEntity*/)
        {
            return ScriptCanvasId();
        }

        virtual ScriptCanvas::Node* GetNode(const AZ::EntityId&, const AZ::Uuid&) = 0;
     
        //! Given the ClassData for a type create a Script Canvas Node Component on the supplied entity
        virtual Node* CreateNodeOnEntity(const AZ::EntityId& entityId, ScriptCanvasId scriptCanvasId, const AZ::Uuid& nodeType) = 0;

        template <typename NodeType>
        NodeType* GetNode(const AZ::EntityId& nodeId)
        {
            AZ::Entity* nodeEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(nodeEntity, &AZ::ComponentApplicationRequests::FindEntity, nodeId);
            if (nodeEntity)
            {
                return nodeEntity->FindComponent<NodeType>();
            }

            return nullptr;
        }

        //! Adds a mapping of the raw address to an object created by the behavior context to the ScriptCanvas::BehaviorContextObject node that owns that object
        virtual void AddOwnedObjectReference(const void* object, BehaviorContextObject* behaviorContextObject) = 0;
        //! Looks up the supplied address returns the BehaviorContextObject if it is owned by one
        virtual BehaviorContextObject* FindOwnedObjectReference(const void* object) = 0;
        //! Removes a mapping of the raw address of an object created by the behavior context to a BehaviorContextObject node
        virtual void RemoveOwnedObjectReference(const void* object) = 0;

        virtual void SetInterpretedBuildConfiguration(BuildConfiguration config) = 0;
        
        virtual SystemComponentConfiguration GetSystemComponentConfiguration() = 0;
    };

    using SystemRequestBus = AZ::EBus<SystemRequests>;

    //! Sends out event for when a batch operation happens on the ScriptCanvas side
    class BatchOperationNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual void OnCommandStarted([[maybe_unused]] AZ::Crc32 batchCommandTag) {}
        virtual void OnCommandFinished([[maybe_unused]] AZ::Crc32 batchCommandTag) {}
    };

    using BatchOperationNotificationBus = AZ::EBus<BatchOperationNotifications>;

    class ScopedBatchOperation
    {
    public:
        ScopedBatchOperation(AZ::Crc32 commandTag)
            : m_batchCommandTag(commandTag)
        {
            BatchOperationNotificationBus::Broadcast(&BatchOperationNotifications::OnCommandStarted, m_batchCommandTag);
        }

        ~ScopedBatchOperation()
        {
            BatchOperationNotificationBus::Broadcast(&BatchOperationNotifications::OnCommandFinished, m_batchCommandTag);
        }

    private:
        ScopedBatchOperation(const ScopedBatchOperation&) = delete;
        ScopedBatchOperation& operator=(const ScopedBatchOperation&) = delete;

        AZ::Crc32 m_batchCommandTag;
    };
}
