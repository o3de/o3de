/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
