/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzToolsFramework/Picking/BoundInterface.h>

namespace AzToolsFramework
{
    class ManipulatorManager;
    class BaseManipulator;

    using ManipulatorId = IdType<struct ManipulatorType>;
    static const ManipulatorId InvalidManipulatorId = ManipulatorId(0);

    using ManipulatorManagerId = IdType<struct ManipulatorManagerType>;
    static const ManipulatorManagerId InvalidManipulatorManagerId = ManipulatorManagerId(0);

    //! EBus interface used to send requests to ManipulatorManager.
    class ManipulatorManagerRequests : public AZ::EBusTraits
    {
    public:
        //! We can have multiple manipulator managers.
        //! In the case where there are multiple viewports, each displaying
        //! a different set of entities, a different manipulator manager is required
        //! to provide a different collision space for each viewport so that mouse
        //! hit detection can be handled properly.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ManipulatorManagerId;

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual ~ManipulatorManagerRequests() = default;

        //! Register a manipulator with the Manipulator Manager.
        //! @param manipulator The manipulator parameter is passed as a shared_ptr so
        //! that the system responsible for managing manipulators can maintain ownership
        //! of the manipulator even if is destroyed while in use.
        virtual void RegisterManipulator(AZStd::shared_ptr<BaseManipulator> manipulator) = 0;

        //! Unregister a manipulator from the Manipulator Manager.
        //! After unregistering the manipulator, it will be excluded from mouse hit detection
        //! and will not receive any mouse action events. The Manipulator Manager will also
        //! relinquish ownership of the manipulator.
        virtual void UnregisterManipulator(BaseManipulator* manipulator) = 0;

        //! Delete a manipulator bound.
        virtual void DeleteManipulatorBound(Picking::RegisteredBoundId boundId) = 0;

        //! Mark the bound of a manipulator dirty so it's excluded from mouse hit detection.
        //! This should be called whenever a manipulator is moved.
        virtual void SetBoundDirty(Picking::RegisteredBoundId boundId) = 0;

        //! Returns true if the manipulator manager is currently interacting, otherwise false.
        virtual bool Interacting() const = 0;

        //! Update the bound for a manipulator.
        //! If \ref boundId hasn't been registered before or it's invalid, a new bound is created and set using \ref boundShapeData.
        //! @param manipulatorId The id of the manipulator whose bound needs to update.
        //! @param boundId The id of the bound that needs to update.
        //! @param boundShapeData The pointer to the new bound shape data.
        //! @return If \ref boundId has been registered return the same id, otherwise create a new bound and return its id.
        virtual Picking::RegisteredBoundId UpdateBound(
            ManipulatorId manipulatorId, Picking::RegisteredBoundId boundId, const Picking::BoundRequestShapeBase& boundShapeData) = 0;
    };

    //! Type to inherit to implement ManipulatorManagerRequests.
    using ManipulatorManagerRequestBus = AZ::EBus<ManipulatorManagerRequests>;

} // namespace AzToolsFramework
