/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/functional.h>

namespace AZ
{
    class EntityComponentIdPair;
    class Vector3;
    class Transform;
} // namespace AZ

namespace AzToolsFramework
{
    //! Base class for objects used in shape component mode sub modes which use viewport editing.
    //! This class and its descendants are intended to be viewport independent, so avoid using buses addressed by EntityId or
    //! EntityComponentIdPair.
    //! To facilitate uses in the main viewport, an EntityComponentIdPair can optionally be specified using AddEntityComponentIdPair, in
    //! order to hook manipulators up to undo/redo behavior and UI refreshing.
    class BaseShapeViewportEdit
    {
    public:
        virtual ~BaseShapeViewportEdit() = default;

        virtual void Setup() = 0;
        virtual void Teardown() = 0;
        virtual void UpdateManipulators() = 0;
        virtual void ResetValues() = 0;
        virtual void AddEntityComponentIdPair(const AZ::EntityComponentIdPair& entityComponentIdPair) = 0;

        void InstallGetManipulatorSpace(const AZStd::function<AZ::Transform()>& getManipulatorSpace);
        void InstallGetNonUniformScale(const AZStd::function<AZ::Vector3()>& getNonUniformScale);
        void InstallGetTranslationOffset(const AZStd::function<AZ::Vector3()>& getTranslationOffset);
        void InstallSetTranslationOffset(const AZStd::function<void(const AZ::Vector3)>& setTranslationOffset);

    protected:
        AZ::Transform GetManipulatorSpace() const;
        AZ::Vector3 GetNonUniformScale() const;
        AZ::Vector3 GetTranslationOffset() const;
        void SetTranslationOffset(const AZ::Vector3& translationOffset);

        AZStd::function<AZ::Transform()> m_getManipulatorSpace;
        AZStd::function<AZ::Vector3()> m_getNonUniformScale;
        AZStd::function<AZ::Vector3()> m_getTranslationOffset;
        AZStd::function<void(const AZ::Vector3)> m_setTranslationOffset;
    };
} // namespace AzToolsFramework
