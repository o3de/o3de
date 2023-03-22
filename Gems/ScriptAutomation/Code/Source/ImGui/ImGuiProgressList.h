/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/containers/vector.h>

#include <AzCore/Component/TickBus.h>

namespace AtomSampleViewer
{
    //! A simple modal window that displays a list of items (strings).
    //! Useful for cases when a set of items are pending some kind of processing.
    class ImGuiProgressList final
        : public AZ::TickBus::Handler
    {
    public:
        // TickBus::Handler
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        //! Needs to be called once before being able to call AddItem/RemoveItem/TickPopup.
        //! @itemsList can be empty.
        void OpenPopup(AZStd::string_view title, AZStd::string_view description, const AZStd::vector<AZStd::string>& itemsList,
            AZStd::function<void()> onUserAction, bool automaticallyCloseOnAction = true, AZStd::string_view actionButtonLabel = "Close");
        void ClosePopup();

        void AddItem(AZStd::string_view item);
        void RemoveItem(AZStd::string_view item);

        void TickPopup();

    private:
        enum class State
        {
            Closed,
            Opening,
            Open
        };

        int32_t m_selectedItemIndex = -1;
        State m_state = State::Closed;

        AZStd::string m_title;
        AZStd::string m_description;
        AZStd::string m_actionButtonLabel;
        AZStd::function<void()> m_onUserAction;
        bool m_automaticallyCloseOnAction = true;

        AZStd::vector<AZStd::string> m_itemsList;
    };

} // namespace AtomSampleViewer
