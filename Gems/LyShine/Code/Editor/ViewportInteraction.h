/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include "EditorCommon.h"

#include <QObject>

#include <AzCore/Math/Vector2.h>
#include <AzCore/std/optional.h>
#endif

class EditorWindow;
class Draw2dHelper;
class QMouseEvent;
class QKeyEvent;
class QWheelEvent;

class ViewportDragInteraction;
class ViewportInteractionExpanderWatcher;

class ViewportInteraction
    : public QObject
{
    Q_OBJECT

public: // types

    enum class NudgeDirection
    {
        Up,
        Down,
        Left,
        Right
    };

    enum class NudgeSpeed
    {
        Slow,
        Fast
    };

    struct TranslationAndScale
    {
        TranslationAndScale()
            : translation(0.0)
            , scale(1.0f) { }

        bool operator == (const TranslationAndScale& other) const
        {
            return translation == other.translation && scale == other.scale;
        }

        AZ::Vector3 translation;
        float scale;
    };

public: // member functions

    ViewportInteraction(EditorWindow* editorWindow);
    virtual ~ViewportInteraction();

    bool GetLeftButtonIsActive();
    bool GetSpaceBarIsActive();
    void ActivateSpaceBar();

    void Draw(Draw2dHelper& draw2d,
        const QTreeWidgetItemRawPtrQList& selectedItems);

    void MousePressEvent(QMouseEvent* ev);
    void MouseMoveEvent(QMouseEvent* ev,
        const QTreeWidgetItemRawPtrQList& selectedItems);
    void MouseReleaseEvent(QMouseEvent* ev,
        const QTreeWidgetItemRawPtrQList& selectedItems);
    bool MouseWheelEvent(QWheelEvent* ev);

    bool KeyPressEvent(QKeyEvent* ev);
    bool KeyReleaseEvent(QKeyEvent* ev);

    //! Mode of interaction in the viewport
    //! This is driven by a toolbar.
    enum class InteractionMode
    {
        SELECTION,
        MOVE,
        ROTATE,
        RESIZE,
        ANCHOR
    };

    //! Type of coordinate system in the viewport
    enum class CoordinateSystem
    {
        LOCAL,
        VIEW
    };

    //! Type of interaction in the viewport
    //! This is driven by hovering and the left mouse button.
    enum class InteractionType
    {
        DIRECT, //!< The bounding box.
        TRANSFORM_GIZMO, //!< The base axes or circular manipulator.
        ANCHORS,
        PIVOT, //!< The dot.
        GUIDE,
        NONE
    };

    void SetMode(InteractionMode mode);
    InteractionMode GetMode() const;

    InteractionType GetInteractionType() const;

    AZ::Entity* GetActiveElement() const;
    const AZ::EntityId& GetActiveElementId() const;

    ViewportHelpers::SelectedAnchors GetGrabbedAnchors() const;

    void SetCoordinateSystem(CoordinateSystem s);
    CoordinateSystem GetCoordinateSystem() const;

    void InitializeToolbars();

    float GetCanvasToViewportScale() const { return m_canvasViewportMatrixProps.scale; }
    AZ::Vector3 GetCanvasToViewportTranslation() const { return m_canvasViewportMatrixProps.translation; }
    
    const TranslationAndScale& GetCanvasViewportMatrixProps();
    void SetCanvasViewportMatrixProps(const TranslationAndScale& canvasViewportMatrixProps);

    //! Centers the entirety of the canvas so that it's viewable within the viewport.
    //
    //! The scale of the canvas-to-viewport matrix is decreased (zoomed out) for
    //! canvases that are bigger than the viewport, and increased (zoomed in) for
    //! canvases that are smaller than the viewport. This scaled view of the canvas
    //! is then use to center the canvas within the viewport.
    //
    //! \param newCanvasSize Because of a one-frame delay in canvas size, if the canvas
    //! size was recently changed, and the caller knows the new canvas size, the size
    //! can be passed to this function to be immediately applied.
    void CenterCanvasInViewport(const AZ::Vector2* newCanvasSize = nullptr);

    //! "Zooms out" the view of the canvas in the viewport by an incremental amount
    void DecreaseCanvasToViewportScale();

    //! "Zooms in" the view of the canvas in the viewport by an incremental amount
    void IncreaseCanvasToViewportScale();

    //! Assigns a scale of 1.0 to the canvas-to-viewport matrix.
    void ResetCanvasToViewportScale();

    //! Sets the scale of the canvas-to-viewport matrix.
    void SetCanvasZoomPercent(float scale);

    //! Return whether the canvas should be scaled to fit when the viewport is resized
    bool ShouldScaleToFitOnViewportResize() const { return m_shouldScaleToFitOnViewportResize; }

    void UpdateZoomFactorLabel();

    //! Reset all transform interaction variables except the interaction mode
    void ClearInteraction(bool clearSpaceBarIsActive = true);

    //! Move the selected elements a certain number of pixels at a time
    void Nudge(NudgeDirection direction, NudgeSpeed speed);

    //! Start/stop object pick mode for assigning an entityId property
    void StartObjectPickMode();
    void StopObjectPickMode();

private: // types

private: // member functions

    //! Update the interaction type based on where the cursor is right now
    void UpdateInteractionType(const AZ::Vector2& mousePosition,
        const QTreeWidgetItemRawPtrQList& selectedItems);

    //! Update the cursor based on the current interaction
    void UpdateCursor();

    //! Update which element is being hovered over
    void UpdateHoverElement(const AZ::Vector2 mousePosition);

    //! Clear the hover element
    void InvalidateHoverElement();

    //! Set the string that is to be displayed near the cursor
    void SetCursorStr(const AZStd::string& cursorStr);

    //! Should be called when our translation and scale properties change for the canvas-to-viewport matrix.
    void UpdateCanvasToViewportMatrix();

    //! Assigns the given scale to the canvas-to-viewport matrix, clamped between 0.1 and 10.0.
    //
    //! Note that this method is private since ViewportInteraction matrix exclusively manages
    //! the viewport-to-canvas matrix.
    void SetCanvasToViewportScale(float newScale, const AZStd::optional<AZ::Vector2>& pivotPoint = AZStd::nullopt);

    //! Given a zoom scale quantize it to be a multiple of the zoom step
    float QuantizeZoomScale(float newScale);

    void GetScaleToFitTransformProps(const AZ::Vector2* newCanvasSize, TranslationAndScale& propsOut);

    //! Called when a pan or a zoom is performed.
    //! Updates flag that determines whether the canvas will scale to fit when the viewport resizes.
    void UpdateShouldScaleToFitOnResize();

    //! Process click and drag interaction
    void ProcessInteraction(const AZ::Vector2& mousePosition,
        Qt::KeyboardModifiers modifiers,
        const QTreeWidgetItemRawPtrQList& selectedItems);

    //! Draw a transform gizmo on the element
    void DrawAxisGizmo(Draw2dHelper& draw2d, const AZ::Entity* element, CoordinateSystem coordinateSystem, const ViewportIcon* lineTextureX, const ViewportIcon* lineTextureY);
    void DrawCircleGizmo(Draw2dHelper& draw2d, const AZ::Entity* element);

    // The coordinate system toolbar updates based on the interaction mode and coordinate system setting
    void UpdateCoordinateSystemToolbarSection();

    bool AreaSelectionIsActive();

    void BeginReversibleAction(const QTreeWidgetItemRawPtrQList& selectedItems);
    void EndReversibleAction();

    void PanOnMouseMoveEvent(const AZ::Vector2& mousePosition);

    const AZ::Uuid& InitAndGetTransformComponentType();

private: // data

    EditorWindow* m_editorWindow;

    // The element that is being interacted with
    AZ::EntityId m_activeElementId;

    // Used for anchor picking
    std::unique_ptr< ViewportIcon > m_anchorWhole;

    // Used for pivot picking
    std::unique_ptr< ViewportIcon > m_pivotIcon;

    // Used for transform interaction
    InteractionMode m_interactionMode;
    InteractionType m_interactionType;
    CoordinateSystem m_coordinateSystem;
    bool m_spaceBarIsActive;                                    //!< True when the spacebar is held down, false otherwise.
    bool m_leftButtonIsActive;                                  //!< True when the left mouse button is down, false otherwise.
    bool m_middleButtonIsActive;                                //!< True when the middle mouse button is down, false otherwise.
    bool m_reversibleActionStarted;
    AZ::Vector2 m_startMouseDragPos;
    AZ::Vector2 m_lastMouseDragPos;
    LyShine::EntityArray m_selectedElementsAtSelectionStart;
    TranslationAndScale m_canvasViewportMatrixProps;            //!< Stores translation and scale properties for canvasToViewport matrix.
                                                                //!< Used for zoom and pan functionality.

    AZStd::string m_cursorStr;
    QCursor m_cursorRotate;
    
    ViewportInteraction::InteractionMode m_interactionModeBeforePickMode;
    AZ::EntityId m_hoverElement;
    bool m_entityPickedOnMousePress; // used to ignore mouse move/release events if element was picked on the mouse press

    bool m_shouldScaleToFitOnViewportResize;

    // Used to refresh the properties panel
    AZ::Uuid m_transformComponentType;

    ViewportHelpers::ElementEdges m_grabbedEdges;
    UiTransform2dInterface::Anchors m_startAnchors;
    ViewportHelpers::SelectedAnchors m_grabbedAnchors;
    ViewportHelpers::GizmoParts m_grabbedGizmoParts;

    // Used for drawing the transform gizmo
    std::unique_ptr<ViewportIcon> m_lineTriangleX;
    std::unique_ptr<ViewportIcon> m_lineTriangleY;
    std::unique_ptr<ViewportIcon> m_circle;
    std::unique_ptr<ViewportIcon> m_lineSquareX;
    std::unique_ptr<ViewportIcon> m_lineSquareY;
    std::unique_ptr<ViewportIcon> m_centerSquare;

    // Used for rubber band selection
    std::unique_ptr< ViewportIcon > m_dottedLine;

    ViewportDragInteraction* m_dragInteraction;
    ViewportInteractionExpanderWatcher* m_expanderWatcher;
    bool m_isAreaSelectionActive = false; //!< True while left mouse is held down for a drag select

    // Variables set when InteractionType is GUIDE
    bool m_activeGuideIsVertical = false;
    int m_activeGuideIndex = 0;

    SerializeHelpers::SerializedEntryList m_selectedEntitiesUndoState;  //! This can be eliminated once the dragInteractions all use ViewportDragInteraction
};

ADD_ENUM_CLASS_ITERATION_OPERATORS(ViewportInteraction::InteractionMode,
    ViewportInteraction::InteractionMode::SELECTION,
    ViewportInteraction::InteractionMode::ANCHOR);

ADD_ENUM_CLASS_ITERATION_OPERATORS(ViewportInteraction::CoordinateSystem,
    ViewportInteraction::CoordinateSystem::LOCAL,
    ViewportInteraction::CoordinateSystem::VIEW);
