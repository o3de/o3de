/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiMaskInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiMaskInterface() {}

    //! Get whether masking is enabled
    virtual bool GetIsMaskingEnabled() = 0;

    //! Set whether masking is enabled
    virtual void SetIsMaskingEnabled(bool enableMasking) = 0;

    //! Get whether interaction masking is enabled
    virtual bool GetIsInteractionMaskingEnabled() = 0;

    //! Set whether interaction masking is enabled
    virtual void SetIsInteractionMaskingEnabled(bool enableInteractionMasking) = 0;

    //! Get whether mask visual is drawn to color buffer behind child elements
    virtual bool GetDrawBehind() = 0;

    //! Set whether mask visual is drawn to color buffer behind child elements
    virtual void SetDrawBehind(bool drawMaskVisualBehindChildren) = 0;

    //! Get whether mask visual is drawn to color buffer in front of child elements
    virtual bool GetDrawInFront() = 0;

    //! Set whether mask visual is drawn to color buffer in front of child elements
    virtual void SetDrawInFront(bool drawMaskVisualInFrontOfChildren) = 0;

    //! Get whether to use alpha test when drawing mask visual to stencil
    virtual bool GetUseAlphaTest() = 0;

    //! Set whether to use alpha test when drawing mask visual to stencil
    virtual void SetUseAlphaTest(bool useAlphaTest) = 0;

    //! Get the flag that indicates whether the mask should use render to texture
    virtual bool GetUseRenderToTexture() = 0;

    //! Set the flag that indicates whether the mask should use render to texture
    virtual void SetUseRenderToTexture(bool useRenderToTexture) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiMaskInterface> UiMaskBus;
