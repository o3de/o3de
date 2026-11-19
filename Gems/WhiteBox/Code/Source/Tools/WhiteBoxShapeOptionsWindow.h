/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include "EditorWhiteBoxDefaultShapeTypes.h"
#include "Tools/WhiteBoxModelingWindow.h"

class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QSpinBox;
class QWidget;

namespace WhiteBox
{
    class EditorWhiteBoxComponent;

    //! Settings for the selected Draw Shape primitive, written straight to the component. Only the rows
    //! the primitive uses are shown.
    class WhiteBoxShapeOptionsWindow : public WhiteBoxModelingWindow
    {
    public:
        WhiteBoxShapeOptionsWindow(const AZ::EntityComponentIdPair& pair, QWidget* parent);
        //! Re-read the component after the switcher (or the pane) changes the primitive.
        void RefreshValues();

    private:
        EditorWhiteBoxComponent* CurrentComponent() const;
        void ApplyValues();
        //! Show only the rows @p shape uses, then shrink to fit - the window is a floating palette, so
        //! leftover empty space reads as broken.
        void ApplyVisibility(DrawShapeType shape, bool cubeStamp);
        //! One labelled control row, hidden and shown as a unit.
        struct Row
        {
            QLabel* m_label = nullptr;
            QWidget* m_field = nullptr;
            void SetVisible(bool visible) const;
        };

        AZ::EntityComponentIdPair m_pair;
        QLabel* m_title = nullptr;
        Row m_sidesRow;
        Row m_tubeSidesRow;
        Row m_holeRatioRow;
        Row m_stepsRow;
        Row m_stepHeightRow;
        Row m_cubeSizeRow;
        QSpinBox* m_sides = nullptr;
        QSpinBox* m_tubeSides = nullptr;
        QDoubleSpinBox* m_holeRatio = nullptr;
        QSpinBox* m_steps = nullptr;
        QDoubleSpinBox* m_stepHeight = nullptr;
        QCheckBox* m_stepsByHeight = nullptr;
        QDoubleSpinBox* m_cubeSize = nullptr;
        QCheckBox* m_cubeShowGrid = nullptr;
        QCheckBox* m_carve = nullptr;
        QCheckBox* m_mergeUnion = nullptr;
        QCheckBox* m_polygonExtrude = nullptr;
        QCheckBox* m_parametric = nullptr;
        bool m_updating = false;
    };
} // namespace WhiteBox
