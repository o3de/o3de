/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/Bus/UiLayoutFitterBus.h>
#include <LyShine/Bus/UiLayoutControllerBus.h>
#include <LyShine/UiComponentTypes.h>

#include <AzCore/Component/Component.h>

#include <UiLayoutHelpers.h>

namespace AZ
{
    class ReflectContext;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//! This component resizes its element to fit its content. It uses cell sizing information given to
//! it by other Layout components, Text component, or Image component (fixed type).
class UiLayoutFitterComponent
    : public AZ::Component
    , public UiLayoutControllerBus::Handler
    , public UiLayoutFitterBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiLayoutFitterComponent, LyShine::UiLayoutFitterComponentUuid, AZ::Component);

    UiLayoutFitterComponent();
    ~UiLayoutFitterComponent() override;

    // UiLayoutControllerInterface
    void ApplyLayoutWidth() override;
    void ApplyLayoutHeight() override;
    // ~UiLayoutControllerInterface

    // UiFitToComponentInterface
    bool GetHorizontalFit() override;
    void SetHorizontalFit(bool horizontalFit) override;
    bool GetVerticalFit() override;
    void SetVerticalFit(bool verticalFit) override;
    FitType GetFitType() override;
    // ~UiFitToComponentInterface

public:  // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("UiFitToContentService"));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("UiFitToContentService"));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("UiElementService"));
        required.push_back(AZ_CRC_CE("UiTransformService"));
    }

    static void Reflect(AZ::ReflectContext* context);

protected: // member functions

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    // UiLayoutControllerInterface
    unsigned int GetPriority() const override;
    // ~UiLayoutControllerInterface

    AZ_DISABLE_COPY_MOVE(UiLayoutFitterComponent);

    //! Called on a property change that has caused this element's layout to be invalid
    void CheckFitterAndInvalidateLayout();

    //! Called on a property change that has caused properties on Transform2d to get modified
    void RefreshEditorTransformProperties();

protected: // data

    bool m_horizontalFit;
    bool m_verticalFit;
};
