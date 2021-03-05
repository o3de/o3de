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
#include <QMainWindow>
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