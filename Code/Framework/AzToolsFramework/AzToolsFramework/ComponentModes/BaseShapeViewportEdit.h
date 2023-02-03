/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/functional.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>

namespace AZ
{
    class EntityComponentIdPair;
    class Vector3;
    class Transform;
} // namespace AZ

namespace AzToolsFramework
{
    //! Base class for viewport editing of shapes.
    //! This class and its descendants are intended to be viewport independent, so avoid using buses addressed by EntityId or
    //! EntityComponentIdPair.
    //! To facilitate uses in the main viewport, an EntityComponentIdPair can optionally be specified using AddEntityComponentIdPair, in
    //! order to hook manipulators up to undo/redo behavior and UI refreshing.
    class BaseShapeViewportEdit
    {
    public:
        virtual ~BaseShapeViewportEdit() = default;

        //! Install the functions required for the manipulators to interact with the shape and the viewport.
        //! @{
        void InstallGetManipulatorSpace(AZStd::function<AZ::Transform()> getManipulatorSpace);
        void InstallGetNonUniformScale(AZStd::function<AZ::Vector3()> getNonUniformScale);
        void InstallGetTranslationOffset(AZStd::function<AZ::Vector3()> getTranslationOffset);
        void InstallSetTranslationOffset(AZStd::function<void(const AZ::Vector3&)> setTranslationOffset);
        //! @}

        //! Install optional functions to provide hooks for connecting to undo/redo systems, refreshing UI etc.
        //! These are not expected to be necessary in the main editor viewport, because the manipulators handle those things automatically,
        //! but they may be useful in other viewports.
        //! @{
        void InstallBeginEditing(AZStd::function<void()> beginEditing);
        void InstallFinishEditing(AZStd::function<void()> finishEditing);
        //! @}

        //! Create manipulators for the shape properties to be edited.
        //! Make sure to install all the required functions before calling Setup.
        virtual void Setup(const ManipulatorManagerId manipulatorManagerId = g_mainManipulatorManagerId) = 0;
        //! Destroy the manipulators for the shape properties being edited.
        virtual void Teardown() = 0;
        //! Call after modifying the shape to ensure that the space the manipulators operate in is updated, along with other properties.
        virtual void UpdateManipulators() = 0;
        //! Reset the shape properties being edited to their default values.
        virtual void ResetValues() = 0;
        //! Optionally used to associate an EntityComponentIdPair with the shape manipulators.
        //! This is useful in the main editor viewport for hooking up undo/redo behavior and UI refreshing.
        //! This should be called after Setup. Otherwise, the manipulators will not have been created yet.
        virtual void AddEntityComponentIdPair(const AZ::EntityComponentIdPair& entityComponentIdPair) = 0;

    protected:
        AZ::Transform GetManipulatorSpace() const;
        AZ::Vector3 GetNonUniformScale() const;
        AZ::Vector3 GetTranslationOffset() const;
        void SetTranslationOffset(const AZ::Vector3& translationOffset);
        void BeginEditing();
        void FinishEditing();

        AZStd::function<AZ::Transform()> m_getManipulatorSpace;
        AZStd::function<AZ::Vector3()> m_getNonUniformScale;
        AZStd::function<AZ::Vector3()> m_getTranslationOffset;
        AZStd::function<void(const AZ::Vector3&)> m_setTranslationOffset;

        AZStd::function<void()> m_beginEditing;
        AZStd::function<void()> m_finishEditing;
    };
} // namespace AzToolsFramework
