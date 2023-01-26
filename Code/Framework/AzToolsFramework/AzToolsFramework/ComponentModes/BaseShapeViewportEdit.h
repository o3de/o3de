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
    //! Base class for objects used in sub component modes which use viewport editing.
    class BaseShapeViewportEdit
    {
    public:
        virtual ~BaseShapeViewportEdit() = default;

        virtual void Setup() = 0;
        virtual void Teardown() = 0;
        virtual void UpdateManipulators() = 0;
        virtual void ResetValues() = 0;
        virtual void AddEntityComponentIdPair(const AZ::EntityComponentIdPair& entityComponentIdPair) = 0;

        void InstallGetManipulatorSpaceFunction(const AZStd::function<AZ::Transform()>& getManipulatorSpaceFunction);
        void InstallGetNonUniformScaleFunction(const AZStd::function<AZ::Vector3()>& getNonUniformScaleFunction);
        void InstallGetTranslationOffsetFunction(const AZStd::function<AZ::Vector3()>& getTranslationOffsetFunction);
        void InstallSetTranslationOffsetFunction(const AZStd::function<void(const AZ::Vector3)>& setTranslationOffsetFunction);

    protected:
        AZ::Transform GetManipulatorSpace() const;
        AZ::Vector3 GetNonUniformScale() const;
        AZ::Vector3 GetTranslationOffset() const;
        void SetTranslationOffset(const AZ::Vector3& translationOffset);

        AZStd::function<AZ::Transform()> m_getManipulatorSpaceFunction;
        AZStd::function<AZ::Vector3()> m_getNonUniformScaleFunction;
        AZStd::function<AZ::Vector3()> m_getTranslationOffsetFunction;
        AZStd::function<void(const AZ::Vector3)> m_setTranslationOffsetFunction;
    };
} // namespace AzToolsFramework
