/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/Outliner/EntityOutlinerFlatSheet.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/ItemViewElement.h>

#include <QBrush>
#include <QColor>
#include <QSize>
#include <QString>
#include <QToolButton>
#include <QTreeView>

namespace AzToolsFramework::EntityOutliner
{
    using AzQtComponents::BoxElement;
    using AzQtComponents::ItemViewElement;
    using AzQtComponents::Selectors::named;
    using AzQtComponents::Selectors::withinClass;

    //! Everything private to this file. A named namespace, not an anonymous one: a unity build joins files into one
    //! translation unit, where anonymous namespaces merge.
    namespace EntityOutlinerFlatSheetInternal
    {
        // ------------------------------------------------------------------------------
        // Elements. Each value cites the declaration that grants it, as file:line of EntityOutliner.qss or of the sheet beneath it
        // (AzQtComponents Components/Widgets/).
        // ------------------------------------------------------------------------------

        //! The button of the outliner's search row that opens the display options (EntityOutlinerWidget.cpp:238-239), a QToolButton:
        //! the icon the sheet put on it and the size of that icon (`#m_display_options`, EntityOutliner.qss:9-13), over the
        //! transparent background of '*' (BaseStyleSheet.qss:17). The properties are set when the button is polished, as the sheet
        //! set them, and need nothing of the style.
        const BoxElement DisplayOptionsButton{
            .m_name = "Editor.Box.OutlinerDisplayOptions",
            .m_idle = {
                .m_fill = QBrush(Qt::transparent),                                  // BaseStyleSheet.qss:17
            },
            .m_properties = {
                { "icon", QString(":/stylesheet/img/UI20/menu-centered.svg") },     // qproperty-icon, EntityOutliner.qss:11
                { "iconSize", QSize(16, 16) },                                      // qproperty-iconSize, EntityOutliner.qss:12
            },
        };

        //! The tree of the outliner (`AzToolsFramework--EntityOutlinerWidget QTreeView`, EntityOutliner.qss:15-32): the outliner draws the hover
        //! and the selection of its items itself (the delegate, and EntityOutlinerTreeView::drawBranches for the branch cells), so the rules make
        //! the role of the selection and the fills of an item and a branch transparent in every state that has one (:17, :24-31), over the
        //! fills of TableView.qss (:42-60) that would otherwise show under what the delegate draws. The caret, the indentation and the disabled
        //! text stay the Global tree's. n2 Editor 01_main_window: the rows of the outliner have no fill of their own.
        const ItemViewElement OutlinerTree{
            .m_name = "Editor.ItemView.OutlinerTree",
            .m_selection = QColor(0, 0, 0, 0),                                      // selection-background-color: transparent, EntityOutliner.qss:17
            .m_faces = {
                // ::item:selected, ::branch:selected, with :active: transparent, EntityOutliner.qss:26-31
                { ItemViewElement::Selected, { QColor(0, 0, 0, 0), QColor(), QColor() } },
                // ::item:hover, ::branch:hover: transparent, EntityOutliner.qss:24-25, :31
                { ItemViewElement::Hover, { QColor(0, 0, 0, 0), QColor(), QColor() } },
            },
        };

        // ------------------------------------------------------------------------------
        // Matchers: the selectors that gave each rule its widgets
        // ------------------------------------------------------------------------------

        //! `AzToolsFramework--EntityOutlinerWidget #m_display_options` (EntityOutliner.qss:9): that name, below the outliner widget
        bool isDisplayOptionsButton(const QWidget* widget)
        {
            return named(widget, "m_display_options") && withinClass(widget, "AzToolsFramework::EntityOutlinerWidget");
        }

        //! `AzToolsFramework--EntityOutlinerWidget QTreeView` (EntityOutliner.qss:15): any tree below the outliner widget
        bool isOutlinerTree(const QWidget* widget)
        {
            return withinClass(widget, "AzToolsFramework::EntityOutlinerWidget");
        }
    } // namespace EntityOutlinerFlatSheetInternal

    // ----------------------------------------------------------------------------------
    // Registration
    // ----------------------------------------------------------------------------------

    void RegisterFlatSheetElements()
    {
        // The button is the only widget of that name: the lookup is by the class it is
        BoxElement::registerElement(
            &EntityOutlinerFlatSheetInternal::DisplayOptionsButton, &QToolButton::staticMetaObject,
            &EntityOutlinerFlatSheetInternal::isDisplayOptionsButton);

        // The roles and the item and branch fills of its tree are an item view element's
        ItemViewElement::registerElement(
            &EntityOutlinerFlatSheetInternal::OutlinerTree, &QTreeView::staticMetaObject, &EntityOutlinerFlatSheetInternal::isOutlinerTree);
    }
} // namespace AzToolsFramework::EntityOutliner
