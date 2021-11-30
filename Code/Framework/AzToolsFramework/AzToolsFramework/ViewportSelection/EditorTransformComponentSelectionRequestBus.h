/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Transform.h>
#include <AzFramework/Entity/EntityContextBus.h>

namespace AzToolsFramework
{
    //! @name Reverse URLs.
    //! Used to identify common actions and override them when necessary.
    //@{
    constexpr inline AZ::Crc32 LockSelection = AZ_CRC_CE("com.o3de.action.editortransform.lockselect");
    constexpr inline AZ::Crc32 UnlockSelection = AZ_CRC_CE("com.o3de.action.editortransform.unlockselect");
    constexpr inline AZ::Crc32 HideSelection = AZ_CRC_CE("com.o3de.action.editortransform.hideselect");
    constexpr inline AZ::Crc32 ShowSelection = AZ_CRC_CE("com.o3de.action.editortransform.showselect");
    constexpr inline AZ::Crc32 UnlockAll = AZ_CRC_CE("com.o3de.action.editortransform.unlockall");
    constexpr inline AZ::Crc32 ShowAll = AZ_CRC_CE("com.o3de.action.editortransform.unhideall");
    constexpr inline AZ::Crc32 SelectAll = AZ_CRC_CE("com.o3de.action.editortransform.selectall");
    constexpr inline AZ::Crc32 InvertSelect = AZ_CRC_CE("com.o3de.action.editortransform.invertselect");
    constexpr inline AZ::Crc32 DuplicateSelect = AZ_CRC_CE("com.o3de.action.editortransform.duplicateselect");
    constexpr inline AZ::Crc32 DeleteSelect = AZ_CRC_CE("com.o3de.action.editortransform.deleteselect");
    constexpr inline AZ::Crc32 EditEscaspe = AZ_CRC_CE("com.o3de.action.editortransform.editescape");
    constexpr inline AZ::Crc32 EditPivot = AZ_CRC_CE("com.o3de.action.editortransform.editpivot");
    constexpr inline AZ::Crc32 EditReset = AZ_CRC_CE("com.o3de.action.editortransform.editreset");
    constexpr inline AZ::Crc32 EditResetManipulator = AZ_CRC_CE("com.o3de.action.editortransform.editresetmanipulator");
    constexpr inline AZ::Crc32 ViewportUiVisible = AZ_CRC_CE("com.o3de.action.editortransform.viewportuivisible");
    //@}

    //! Provide interface for EditorTransformComponentSelection requests.
    class EditorTransformComponentSelectionRequests : public AZ::EBusTraits
    {
    public:
        using BusIdType = AzFramework::EntityContextId;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        static void Reflect(AZ::ReflectContext* context);

        //! What type of transform editing are we in.
        enum class Mode
        {
            // note: ordering of these is important - do not change.
            // ensures scrolling the mouse wheel cycles between modes in the right order.
            Rotation,
            Translation,
            Scale
        };

        //! Specify the type of refresh (what type of transform modification caused the refresh).
        enum class RefreshType
        {
            Translation,
            Orientation,
            All
        };

        //! How is the pivot aligned (object/authored position or center).
        enum class Pivot
        {
            Object,
            Center
        };

        //! Set what kind of transform the type that implements this bus should use.
        virtual void SetTransformMode(Mode mode) = 0;

        //! Return what transform mode the type that implements this bus is using.
        virtual Mode GetTransformMode() = 0;

        //! Return the current Entity Manipulator transform.
        //! An AZStd::optional<AZ::Transform> is returned as if we do not have a selection
        //! there will be no Manipulator present. In this case we return an empty optional.
        virtual AZStd::optional<AZ::Transform> GetManipulatorTransform() = 0;

        //! Refresh the Manipulator based on the current entity selection.
        //! This may be useful if the Entity transform has been set outside
        //! of the EditorTransformComponentSelection and we want to make sure the
        //! Manipulator stays up to date (in sync) with the current Entity transform.
        virtual void RefreshManipulators(RefreshType refreshType) = 0;

        //! Set an orientation override for the Manipulator.
        //! Useful if we've picked another Entity transform to use as our reference point.
        virtual void OverrideManipulatorOrientation(const AZ::Quaternion& orientation) = 0;

        //! Set a translation override for the Manipulator.
        //! Useful if we've picked another Entity transform to use as our reference point.
        virtual void OverrideManipulatorTranslation(const AZ::Vector3& translation) = 0;

        //! Copy translation to each individual entity so they all appear in the exact same position.
        virtual void CopyTranslationToSelectedEntitiesIndividual(const AZ::Vector3& translation) = 0;

        //! Copy translation to manipulator position with each entity keeping the same relative position as before.
        virtual void CopyTranslationToSelectedEntitiesGroup(const AZ::Vector3& translation) = 0;

        //! Reset the translation of an entity to the same position as its parent.
        //! Note: This is a noop if the entity does not have a parent.
        virtual void ResetTranslationForSelectedEntitiesLocal() = 0;

        //! Copy orientation to each individual entity so they all appear in the exact same orientation.
        virtual void CopyOrientationToSelectedEntitiesIndividual(const AZ::Quaternion& orientation) = 0;

        //! Copy orientation to manipulator with each entity keeping the same relative orientation as before.
        virtual void CopyOrientationToSelectedEntitiesGroup(const AZ::Quaternion& orientation) = 0;

        //! Reset the orientation of an entity to the same orientation as its parent.
        //! Note: This will be the aligned to the world axes (identity) if the entity does not have a parent.
        virtual void ResetOrientationForSelectedEntitiesLocal() = 0;

        //! Copy scale to each individual entity in local space without moving position.
        virtual void CopyScaleToSelectedEntitiesIndividualLocal(float scale) = 0;

        //! Copy scale to to each individual entity in world (absolute) space.
        virtual void CopyScaleToSelectedEntitiesIndividualWorld(float scale) = 0;

        //! Snap selected entities to be aligned with the world space grid.
        virtual void SnapSelectedEntitiesToWorldGrid(float gridSize) = 0;

    protected:
        ~EditorTransformComponentSelectionRequests() = default;
    };

    //! Type to inherit to implement EditorTransformComponentSelectionRequests.
    using EditorTransformComponentSelectionRequestBus = AZ::EBus<EditorTransformComponentSelectionRequests>;

} // namespace AzToolsFramework
