/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    //! Enumerates the different type of transform sub-modes available.
    enum class TransformType
    {
        Translation,
        Rotation,
        Scale
    };

    enum class TransformModelingLatch
    {
        None,
        Extrude,
        Inset
    };

    //! Request bus for White Box ComponentMode operations while in 'transform' mode.
    class EditorWhiteBoxTransformModeRequests : public AZ::EntityComponentBus
    {
    public:
        //! Change the TransformType for the WhiteBox Transform sub-mode.
        virtual void ChangeTransformType(TransformType subModeType) = 0;
        virtual void SetModelingLatch(TransformModelingLatch /*latch*/) {}
        virtual TransformModelingLatch GetModelingLatch() const { return TransformModelingLatch::None; }
        //! True while a latched drag is in flight, so Escape can cancel the drag rather than the window.
        virtual bool HasLatchedDrag() const { return false; }

        //! Polygon selection on the active editable layer (empty for edge/vertex selection).
        virtual Api::PolygonHandles GetSelectedPolygons() const { return {}; }

        virtual Api::EdgeHandles GetSelectedEdges() const { return {}; }
        virtual Api::VertexHandles GetSelectedVertices() const { return {}; }
        //! Drop cached topology handles after an operation replaces the mesh.
        virtual void ClearSelection() {}
        //! Rebuild the manipulators so they pick up a changed editing space, keeping the selection.
        virtual void RefreshManipulatorSpace() {}
        virtual void SetSelectedPolygons(const Api::PolygonHandles& /*polygons*/) {}
        virtual void BeginLoopCut() {}
        //! Expand the current edge selection. False selects loops, true selects rings.
        virtual bool ExpandEdgeSelection(bool /*ring*/) { return false; }

        // ------------------------------------------------------------------ //
        // Blender-style numeric input dispatch methods                       //
        // ------------------------------------------------------------------ //
        virtual void NumericBeginMove()           = 0;
        virtual void NumericBeginRotate()         = 0;
        virtual void NumericBeginScale()          = 0;
        virtual void NumericSetAxisX()            = 0;
        virtual void NumericSetAxisY()            = 0;
        virtual void NumericSetAxisZ()            = 0;
        virtual void NumericConfirm()             = 0;
        virtual void NumericCancel()              = 0;
        virtual void NumericBackspace()           = 0;
        virtual void NumericDecimal()             = 0;
        virtual void NumericNegate()              = 0; //!< '-' operator
        virtual void NumericAppendDigit(char d)   = 0;
        virtual void NumericAppendOperatorPlus()  = 0; //!< '+' binary add
        virtual void NumericAppendOperatorMult()  = 0; //!< '*' multiply
        virtual void NumericAppendOperatorDiv()   = 0; //!< '/' divide

    protected:
        ~EditorWhiteBoxTransformModeRequests() = default;
    };

    using EditorWhiteBoxTransformModeRequestBus = AZ::EBus<EditorWhiteBoxTransformModeRequests>;
} // namespace WhiteBox
