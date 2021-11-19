/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzFramework/Viewport/ScreenGeometry.h>

namespace AzFramework
{
    class DebugDisplayRequests;
    struct ViewportInfo;
} // namespace AzFramework

namespace AzToolsFramework
{
    namespace ViewportInteraction
    {
        struct MouseInteractionEvent;
    }

    //! An interface to provide invalid click feedback in the editor viewport.
    class InvalidClick
    {
    public:
        virtual ~InvalidClick() = default;

        //! Begin the feedback.
        //! @param screenPoint The position of the click in screen coordinates.
        virtual void Begin(const AzFramework::ScreenPoint& screenPoint) = 0;
        //! Update the invalid click feedback
        virtual void Update(float deltaTime) = 0;
        //! Report if the click feedback is running or not (returning false will signal the TickBus can be disconnected from).
        virtual bool Updating() = 0;
        //! Display the click feedback in the viewport.
        virtual void Display(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) = 0;
    };

    //! Display expanding fading circles for every click of the mouse that is invalid.
    class ExpandingFadingCircles : public InvalidClick
    {
    public:
        void Begin(const AzFramework::ScreenPoint& screenPoint) override;
        void Update(float deltaTime) override;
        bool Updating() override;
        void Display(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

    private:
        //! Stores a circle representation with a lifetime to grow and fade out over time.
        struct FadingCircle
        {
            AzFramework::ScreenPoint m_position;
            float m_radius;
            float m_opacity;
        };

        using FadingCircles = AZStd::vector<FadingCircle>;
        FadingCircles m_fadingCircles; //!< Collection of fading circles to draw for clicks that have no effect.
    };

    //! Display fading text where an invalid click happened.
    //! @note There is only one fading text, each click will update its position.
    class FadingText : public InvalidClick
    {
    public:
        explicit FadingText(AZStd::string message)
            : m_message(AZStd::move(message))
        {
        }

        void Begin(const AzFramework::ScreenPoint& screenPoint) override;
        void Update(float deltaTime) override;
        bool Updating() override;
        void Display(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

    private:
        AZStd::string m_message; //!< Message to display for fading text.
        float m_opacity = 0.0f; //!< The opacity of the invalid click message.
        //! The position to display the invalid click message.
        AzFramework::ScreenPoint m_invalidClickPosition = AzFramework::ScreenPoint(0, 0);
    };

    //! Interface to begin invalid click feedback (will run all added InvalidClick behaviors).
    class InvalidClicks : private AZ::TickBus::Handler
    {
    public:
        explicit InvalidClicks(AZStd::vector<AZStd::unique_ptr<InvalidClick>> invalidClickBehaviors)
            : m_invalidClickBehaviors(AZStd::move(invalidClickBehaviors))
        {
        }

        //! Add an invalid click and activate one or more of the added invalid click behaviors.
        void AddInvalidClick(const AzFramework::ScreenPoint& screenPoint);

        //! Handle 2d drawing for EditorHelper functionality.
        void Display2d(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay);

    private:
        //! AZ::TickBus overrides ...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        AZStd::vector<AZStd::unique_ptr<InvalidClick>> m_invalidClickBehaviors; //!< Invalid click behaviors to run.
    };
} // namespace AzToolsFramework
