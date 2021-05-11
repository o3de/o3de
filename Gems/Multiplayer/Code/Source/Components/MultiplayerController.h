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

#pragma once

#include <Include/NetworkEntityHandle.h>
#include <AzCore/Math/Aabb.h>

namespace Multiplayer
{
    class NetworkInput;
    class NetBindComponent;
    class MultiplayerComponent;

    //! @class MultiplayerController
    //! @brief A base class for all multiplayer component controllers responsible for running local prediction logic.
    class MultiplayerController
    {
    public:
        enum class InputPriorityOrder
        {
            First = 0,
            Default = 1000,
            SubEntities = 90000,
            Last = 100000
        };

        MultiplayerController(MultiplayerComponent& owner);
        virtual ~MultiplayerController() = default;

        //! Activates the controller.
        virtual void Activate(EntityIsMigrating entityIsMigrating) = 0;

        //! Deactivates the controller.
        virtual void Deactivate(EntityIsMigrating entityIsMigrating) = 0;

        //! Returns the networkId for the entity that owns this controller.
        //! @return the networkId for the entity that owns this controller
        NetEntityId GetNetEntityId() const;

        //! Returns the networkRole for the entity that owns this controller.
        //! @return the networkRole for the entity that owns this controller
        NetEntityRole GetNetEntityRole() const;

        //! Returns the raw AZ::Entity pointer for the entity that owns this controller.
        //! @return the raw AZ::Entity pointer for the entity that owns this controller
        AZ::Entity* GetEntity() const;

        //! Returns the network entity handle for the entity that owns this controller.
        //! @return the network entity handle for the entity that owns this controller
        ConstNetworkEntityHandle GetEntityHandle() const;

        //! Returns the network entity handle for the entity that owns this controller.
        //! @return the network entity handle for the entity that owns this controller
        NetworkEntityHandle GetEntityHandle();

    protected:

        //! Returns the NetBindComponent responsible for net binding for this controller
        //! @{
        const NetBindComponent* GetNetBindComponent() const;
        NetBindComponent* GetNetBindComponent();
        //! @}

        //! Returns the MultiplayerComponent that owns this controller instance.
        //! @return the MultiplayerComponent that owns this controller instance
        //! @{
        const MultiplayerComponent& GetOwner() const;
        MultiplayerComponent& GetOwner();
        //! @}

        //! Returns true if the owning entity is currently inside ProcessInput scope.
        bool IsProcessingInput() const;

        //! Returns the input priority ordering for determining the order of ProcessInput or CreateInput functions.
        virtual InputPriorityOrder GetInputOrder() const = 0;

        //! Queries the rewind system to determine what volume is relevent for a given input, this is very important for performance at scale.
        //! @param networkInput input structure to process
        //! @param deltaTime    amount of time the provided input would be integrated over
        //! @return a world-space aabb representing the volume relevent to the provided input
        virtual AZ::Aabb GetRewindBoundsForInput(const NetworkInput& networkInput, float deltaTime) const = 0;

        //! Base execution for ProcessInput packet, do not call directly.
        //! @param networkInput input structure to process
        //! @param deltaTime    amount of time to integrate the provided inputs over
        virtual void ProcessInput(NetworkInput& networkInput, float deltaTime) = 0;

        //! Only valid on a client, should never be invoked on the server.
        //! @param networkInput input structure to process
        //! @param deltaTime    amount of time to integrate the provided inputs over
        virtual void CreateInput(NetworkInput& networkInput, float deltaTime) = 0;

        template <typename ComponentType>
        const ComponentType* FindComponent() const;

        template <typename ComponentType>
        ComponentType* FindComponent();

        template <typename ControllerType>
        ControllerType* FindController(const NetworkEntityHandle& entityHandle) const;

        MultiplayerController* FindController(const AZ::Uuid& typeId, const NetworkEntityHandle& entityHandle) const;

    private:

        MultiplayerComponent& m_owner;
        friend class NetBindComponent; // For access to create and process input methods
    };

    template <typename ComponentType>
    inline const ComponentType* MultiplayerController::FindComponent() const
    {
        return GetEntity()->FindComponent<ComponentType>();
    }

    template <typename ComponentType>
    inline ComponentType* MultiplayerController::FindComponent()
    {
        return GetEntity()->FindComponent<ComponentType>();
    }

    template <typename ControllerType>
    inline ControllerType* MultiplayerController::FindController(const NetworkEntityHandle& entityHandle) const
    {
        return static_cast<ControllerType*>(FindController(typename ControllerType::ComponentType::RTTI_Type(), entityHandle));
    }
}
