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

#include <Source/NetworkEntity/NetworkEntityHandle.h>

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

        //! Returns the networkId for the entity that owns this controller.
        //! @return the networkId for the entity that owns this controller
        NetEntityId GetNetEntityId() const;

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

        const NetBindComponent* GetNetBindComponent() const;
        NetBindComponent* GetNetBindComponent();

        //! Returns true if the owning entity is currently inside ProcessInput scope.
        bool IsProcessingInput() const;

        //! Returns the input priority ordering for determining the order of ProcessInput or CreateInput functions.
        virtual InputPriorityOrder GetInputOrder() const = 0;

        //! Hints the rewind system how close an entity should be in order to rewind, this can be very important for performance at scale.
        //! @param networkInput input structure to process
        //! @param deltaTime    amount of time the provided input would be integrated over
        //! @return the maximum distance an entity should be for this component to interact with it given the provided input
        virtual float GetRewindDistanceForInput(const NetworkInput& networkInput, float deltaTime) const = 0;

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
