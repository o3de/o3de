/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentBus.h>
#include <AzToolsFramework/ComponentModes/BaseViewportEdit.h>
#include <AzToolsFramework/Manipulators/TranslationManipulators.h>

namespace AzToolsFramework
{
    //! Wraps translation manipulators for editing shape translation offsets.
    class ShapeTranslationOffsetViewportEdit : public BaseViewportEdit
    {
    public:
        ShapeTranslationOffsetViewportEdit() = default;

        // BaseViewportEdit overrides ...
        void Setup() override;
        void Teardown() override;
        void UpdateManipulators() override;
        void ResetValues() override;
        void AddEntityComponentIdPair(const AZ::EntityComponentIdPair& entityComponentIdPair) override;

        void InstallGetManipulatorSpaceFunction(const AZStd::function<AZ::Transform()>& getManipulatorSpaceFunction);
        void InstallGetNonUniformScaleFunction(const AZStd::function<AZ::Vector3()>& getNonUniformScaleFunction);
        void InstallGetTranslationOffsetFunction(const AZStd::function<AZ::Vector3()>& getTranslationOffsetFunction);
        void InstallSetTranslationOffsetFunction(const AZStd::function<void(const AZ::Vector3)>& setTranslationOffsetFunction);
    private:
        AZ::Transform GetManipulatorSpace() const;
        AZ::Vector3 GetNonUniformScale() const;
        AZ::Vector3 GetTranslationOffset() const;
        void SetTranslationOffset(const AZ::Vector3& translationOffset);

        AZStd::shared_ptr<TranslationManipulators> m_translationManipulators; //!< Manipulators for editing shape offset.

        AZStd::function<AZ::Transform()> m_getManipulatorSpaceFunction;
        AZStd::function<AZ::Vector3()> m_getNonUniformScaleFunction;
        AZStd::function<AZ::Vector3()> m_getTranslationOffsetFunction;
        AZStd::function<void(const AZ::Vector3)> m_setTranslationOffsetFunction;
    };
} // namespace AzToolsFramework
