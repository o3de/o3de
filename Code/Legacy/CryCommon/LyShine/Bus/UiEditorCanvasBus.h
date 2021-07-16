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
class UiEditorCanvasInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiEditorCanvasInterface() {}

    //! Get the snap state.
    virtual bool GetIsSnapEnabled() = 0;

    //! Set the snap state.
    virtual void SetIsSnapEnabled(bool enabled) = 0;

    //! Get the translation distance to snap to
    virtual float GetSnapDistance() = 0;

    //! Set the translation distance to snap to
    virtual void SetSnapDistance(float distance) = 0;

    //! Get the degrees of rotation to snap to
    virtual float GetSnapRotationDegrees() = 0;

    //! Set the degrees of rotation to snap to
    virtual void SetSnapRotationDegrees(float degrees) = 0;

    //! Get the positions of the horizontal guide lines (along y-axis in canvas pixels)
    virtual AZStd::vector<float> GetHorizontalGuidePositions() = 0;

    //! Add a horizontal guide line
    virtual void AddHorizontalGuide(float position) = 0;

    //! Remove the horizontal guide line at the given index
    virtual void RemoveHorizontalGuide(int index) = 0;

    //! Set the position of the horizontal guide line at the given index
    virtual void SetHorizontalGuidePosition(int index, float position) = 0;

    //! Get the positions of the vertical guide lines (along x-axis in canvas pixels)
    virtual AZStd::vector<float> GetVerticalGuidePositions() = 0;

    //! Add a vertical guide line
    virtual void AddVerticalGuide(float position) = 0;

    //! Remove the vertical guide line at the given index
    virtual void RemoveVerticalGuide(int index) = 0;

    //! Set the position of the vertical guide line at the given index
    virtual void SetVerticalGuidePosition(int index, float position) = 0;

    //! Remove all of the guides
    virtual void RemoveAllGuides() = 0;

    //! Get the color to draw the guide lines on this canvas
    virtual AZ::Color GetGuideColor() = 0;

    //! Set the color to draw the guide lines on this canvas
    virtual void SetGuideColor(const AZ::Color& color) = 0;

    //! Get whether the guides on this canvas are locked
    virtual bool GetGuidesAreLocked() = 0;

    //! Set whether the guides on this canvas are locked
    virtual void SetGuidesAreLocked(bool areLocked) = 0;

    //! Check the canvas for any orphaned elements. These are elements not referenced as a child by the canvas or any of its descendant elements.
    virtual bool CheckForOrphanedElements() = 0;

    //! Recover any orphaned elements in the canvas by placing them under a special top-level element.
    virtual void RecoverOrphanedElements() = 0;

    //! Remove any orphaned elements in the canvas.
    virtual void RemoveOrphanedElements() = 0;

    //! Update the canvas from the UI Editor
    //! \param deltaTime the amount of time in seconds since the last call to this function
    //! \param isInGame, true if canvas being updated in preview mode, false if being updated in edit mode
    virtual void UpdateCanvasInEditorViewport(float deltaTime, bool isInGame) = 0;

    //! Render the canvas in the UI Editor
    //! \param isInGame, true if canvas being rendered in preview mode, false if being rendered in edit mode
    //! \param viewportSize, this is the size of the viewport that the canvas is being rendered to
    virtual void RenderCanvasInEditorViewport(bool isInGame, AZ::Vector2 viewportSize) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiEditorCanvasInterface> UiEditorCanvasBus;
