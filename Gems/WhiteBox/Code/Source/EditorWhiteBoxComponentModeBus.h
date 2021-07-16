/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>
#include <QPointer>

class QWidget;

namespace WhiteBox
{
    //! Enum for the current sub-mode of the White Box ComponentMode.
    enum class SubMode
    {
        Default,
        EdgeRestore
    };

    using KeyboardModifierQueryFn = AZStd::function<AzToolsFramework::ViewportInteraction::KeyboardModifiers()>;

    //! Request bus for generic White Box ComponentMode operations (irrespective of the sub-mode).
    class EditorWhiteBoxComponentModeRequests : public AZ::EntityComponentBus
    {
    public:
        //! Signal that the white box has changed and the intersection data needs to be rebuilt.
        virtual void MarkWhiteBoxIntersectionDataDirty() = 0;
        //! Get the current sub-mode that White Box is in (Default Mode or Edge Restore mode).
        virtual SubMode GetCurrentSubMode() const = 0;
        //! Provides the ability to customize how keyboard modifier keys are queried.
        //! @note This could be overridden to return nothing or a fixed modifier value.
        virtual void OverrideKeyboardModifierQuery(const KeyboardModifierQueryFn& keyboardModifierQueryFn) = 0;

    protected:
        ~EditorWhiteBoxComponentModeRequests() = default;
    };

    using EditorWhiteBoxComponentModeRequestBus = AZ::EBus<EditorWhiteBoxComponentModeRequests>;
} // namespace WhiteBox
