/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/PlatformIncl.h>
#include <QMainWindow>
#include <AzCore/Math/Guid.h>
#endif

namespace AzToolsFramework
{
    class SearchCriteriaWidget;
}

class ComponentCategoryList;
class FilteredComponentList;
class ComponentDataModel;

//! ComponentPaletteWindow
//! Provides a window with controls related to the Component Entity system. It provides an intuitive and organized 
//! set of controls to display, sort, filter components. It provides mechanisms for creating entities by dragging
//! and dropping components into the viewport as well as from context menus.
class ComponentPaletteWindow
    : public QMainWindow
{
    Q_OBJECT

public:

    explicit ComponentPaletteWindow(QWidget* parent = 0);

    void Init();

    static const GUID& GetClassID()
    {
        // {4236998F-1138-466D-9DF5-6533BFA1DFCA}
        static const GUID guid =
        {
            0x4236998F, 0x1138, 0x466D, { 0x9D, 0xF5, 0x65, 0x33, 0xBF, 0xA1, 0xDF, 0xCA }
        };
        return guid;
    }

    static void RegisterViewClass();

protected:
    ComponentCategoryList* m_categoryListWidget;
    FilteredComponentList* m_componentListWidget;
    AzToolsFramework::SearchCriteriaWidget* m_filterWidget;

    void keyPressEvent(QKeyEvent* event) override;
};
