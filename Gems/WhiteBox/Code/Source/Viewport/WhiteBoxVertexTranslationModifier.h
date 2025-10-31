/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Viewport/WhiteBoxViewportConstants.h"

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace AzToolsFramework
{
    class ManipulatorViewSphere;
    class MultiLinearManipulator;

    namespace ViewportInteraction
    {
        struct MouseInteraction;
    }
} // namespace AzToolsFramework

namespace WhiteBox
{
    //! VertexTranslationModifier provides the ability to translate a single vertex in the viewport.
    class VertexTranslationModifier
        : private AzFramework::ViewportDebugDisplayEventBus::Handler
        , private AZ::TickBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        using HandleType = Api::VertexHandle;

        VertexTranslationModifier(
            const AZ::EntityComponentIdPair& entityComponentIdPair, Api::VertexHandle vertexHandle,
            const AZ::Vector3& intersectionPoint);
        ~VertexTranslationModifier();

        bool MouseOver() const;
        void ForwardMouseOverEvent(const AzToolsFramework::ViewportInteraction::MouseInteraction& interaction);

        Api::VertexHandle GetHandle() const; // Generic context version
        Api::VertexHandle GetVertexHandle() const;
        void SetVertexHandle(Api::VertexHandle vertexHandle);
        void SetColor(const AZ::Color& color);

        void Refresh();
        void CreateView();
        bool PerformingAction() const;

        static const int InvalidAxisIndex = -1;

    private:
        void CreateManipulator();
        void DestroyManipulator();

        // ViewportDebugDisplayEventBus ...
        void DisplayViewport(
            const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

        // TickBis ...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        Api::VertexHandle m_vertexHandle; //!< The vertex handle this modifier is currently associated with.
        AZ::EntityComponentIdPair
            m_entityComponentIdPair; //!< The entity and component id this modifier is associated with.
        AZStd::shared_ptr<AzToolsFramework::MultiLinearManipulator>
            m_translationManipulator; //!< Manipulator for performing vertex translation.
        AZStd::shared_ptr<AzToolsFramework::ManipulatorViewSphere>
            m_vertexView; //!< Manipulator view used to represent a mesh vertex for selection.
        AZ::Color m_color = ed_whiteBoxVertexHover; //!< The current color of the vertex.
        AZ::Vector3
            m_localPositionAtMouseDown; //!< The position of the modifier in local space at the time of mouse down.
        int m_actionIndex = InvalidAxisIndex; //!< Which action (axis) are we moving along for the given vertex.
        float m_pressTime = 0.0f; //!< Duration of press and hold of modifier.
    };

    inline Api::VertexHandle VertexTranslationModifier::GetHandle() const
    {
        return GetVertexHandle();
    }

    inline Api::VertexHandle VertexTranslationModifier::GetVertexHandle() const
    {
        return m_vertexHandle;
    }

    inline void VertexTranslationModifier::SetVertexHandle(const Api::VertexHandle vertexHandle)
    {
        m_vertexHandle = vertexHandle;
    }

    inline void VertexTranslationModifier::SetColor(const AZ::Color& color)
    {
        m_color = color;
    }
} // namespace WhiteBox
