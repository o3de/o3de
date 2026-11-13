/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UiEditorFlat.h"

#include "Windows/EditorCommon.h"

#include <AzQtComponents/Components/Elements/ElementCommon.h>
#include <AzQtComponents/Components/Elements/MenuElement.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <QMenu>
#include <QString>
#include <QWidget>

//////////////////////////////////////////////////////////////////////////
//  THE ELEMENT OF THE UI EDITOR'S MENUS
//
//  Everything private to this file. A named namespace, not an anonymous one: a unity build joins files into one translation unit,
//  where anonymous namespaces merge.
//////////////////////////////////////////////////////////////////////////
namespace UiEditorFlatInternal
{
    //! The popup menu of the Global elements (QMenu, Menu.qss:10-14, with its selected item, separator and padding), whose disabled
    //! items are the grey of the menu's own sheet, UiEditorFlat::DisabledText (`QMenu::item:disabled { color }`, EditorCommon.h:50
    //! before the flattening), in place of Menu.qss:25. The sheet was the menu's own, so it outranked the application sheet's rule
    //! whatever the specificity; it has no box and no other property, so the rest is the popup's. A copy of the Global popup, made
    //! when first needed, as the Global element is private to MenuElement.cpp.
    const AzQtComponents::MenuElement& PopupMenu()
    {
        static const AzQtComponents::MenuElement menu = []
        {
            AzQtComponents::MenuElement element = AzQtComponents::MenuElement::forWidget(nullptr);
            element.m_name = UiEditorFlat::PopupMenuElementName;
            element.m_disabledText = UiEditorFlat::DisabledText;
            return element;
        }();
        return menu;
    }

    //! The menu SetPopupMenuSheet tagged, and the menus below it: the sheet reached the sub-menus a menu keeps, which are its
    //! children (QMenu::addMenu), and theirs. The menu's own tag first; then up through the parents for as long as they are menus,
    //! so that a menu of another feature pays one property read and one cast.
    bool IsPopupMenu(const QWidget* menu)
    {
        for (const QWidget* widget = menu; widget; widget = widget->parentWidget())
        {
            if (AzQtComponents::Selectors::tagged(widget, UiEditorFlat::PopupMenuTag))
            {
                return true;
            }
            if (!qobject_cast<const QMenu*>(widget->parentWidget()))
            {
                return false;
            }
        }
        return false;
    }

    //! A registration lives for the whole run, and the registries do not look for repeats: the menus of the editor call this at
    //! every construction
    void RegisterFlatElements()
    {
        static bool registered = false;
        if (!registered)
        {
            registered = true;
            AzQtComponents::MenuElement::registerElement(&PopupMenu(), &QMenu::staticMetaObject, &IsPopupMenu);
        }
    }
} // namespace UiEditorFlatInternal

//////////////////////////////////////////////////////////////////////////
//  THE MENU SITES' ONE LINE
//////////////////////////////////////////////////////////////////////////
namespace UiEditorFlat
{
    void SetPopupMenuSheet(QMenu* menu)
    {
        // The legacy GUI: the menu's own sheet, as it always was. Built here and not at the call, so that the flattened GUI does not
        // build text it never sets.
        if (!AzQtComponents::StyleManager::stylesheetsDisabled())
        {
            AzQtComponents::StyleManager::setLegacyStyleSheet(menu, UICANVASEDITOR_QMENU_ITEM_DISABLED_STYLESHEET);
            return;
        }

        // The flattened GUI: no sheet, the tag. Before the menu is shown the menus below it resolve their elements when they are
        // polished, and the tag is there by then.
        if (AzQtComponents::StyleManager::flatStylingActive())
        {
            UiEditorFlatInternal::RegisterFlatElements();
            AzQtComponents::Style::addClass(menu, QString::fromLatin1(UiEditorFlat::PopupMenuTag));
        }
    }
} // namespace UiEditorFlat
