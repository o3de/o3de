/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QToolBar>
#include <AzQtComponents/Components/Widgets/SpinBox.h>
#endif

class EditorWindow;
class NewElementToolbarSection;
class CoordinateSystemToolbarSection;
class CanvasSizeToolbarSection;
class QLabel;

class MainToolbar
    : public QToolBar
{
    Q_OBJECT

public:

    explicit MainToolbar(EditorWindow* parent);

    NewElementToolbarSection* GetNewElementToolbarSection()
    {
        return m_newElementToolbarSection.get();
    }

    CoordinateSystemToolbarSection* GetCoordinateSystemToolbarSection()
    {
        return m_coordinateSystemToolbarSection.get();
    }

    CanvasSizeToolbarSection* GetCanvasSizeToolbarSection()
    {
        return m_canvasSizeToolbarSection.get();
    }

    void SetZoomPercent(float zoomPercent);

private:

    std::unique_ptr<NewElementToolbarSection> m_newElementToolbarSection;
    std::unique_ptr<CoordinateSystemToolbarSection> m_coordinateSystemToolbarSection;
    std::unique_ptr<CanvasSizeToolbarSection> m_canvasSizeToolbarSection;

    AzQtComponents::DoubleSpinBox* m_zoomFactorSpinBox;
};
