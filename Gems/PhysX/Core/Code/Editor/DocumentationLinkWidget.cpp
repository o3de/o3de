/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/DocumentationLinkWidget.h>

#include <AzQtComponents/Components/Elements/BoxElement.h>
#include <AzQtComponents/Components/Elements/Foundations.h>
#include <AzQtComponents/Components/Elements/MenuElement.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <QMenu>

namespace PhysX
{
    namespace Editor
    {
        // ----------------------------------------------------------------------------------
        // Flattened GUI: the documentation link's fill
        //
        // The link gave itself a sheet of one fill. The legacy GUI sets it; the flattened GUI sets none, and these elements draw what
        // it drew: the fill, in the box of every label (the label rule of the application sheet, which the legacy cascade laid under
        // the link's own sheet), and the same fill under the menu the link pops up, which the sheet reached as a descendant.
        // ----------------------------------------------------------------------------------

        //! Everything private to this file. A named namespace, not an anonymous one: a unity build joins files into one translation
        //! unit, where anonymous namespaces merge.
        namespace DocumentationLinkWidgetFlatInternal
        {
            //! background-color: DocumentationLinkWidget.cpp:24, the form before the flattening. TOKENIZE: DocumentationLinkFill
            const QColor LinkFill(0x33, 0x33, 0x33);

            //! The `background-color` of the link, over the box of every label (`QLabel { margin-top: 1px; margin-bottom: 1px }`,
            //! Text.qss:93-94): an element is the whole definition of its frame, so it carries that margin, which Style gives a label
            //! without one (Style::subElementRect, SE_ShapedFrameContents)
            const AzQtComponents::BoxElement Link{
                .m_name = "PhysX.Box.DocumentationLink",
                .m_margin = QMargins(0, AzQtComponents::Foundations::Label::MarginV, 0, AzQtComponents::Foundations::Label::MarginV),
                .m_idle = {
                    .m_fill = QBrush(LinkFill),
                },
            };

            //! The widgets of the class the element is registered for: the link itself (the menu it pops up is isLinkMenu's)
            bool isLink(const QWidget*)
            {
                return true;
            }

            //! The standard context menu of a text label (the right-click menu: Copy, Copy Link Location, Select All) is a QMenu
            //! the label parents (QWidgetTextControl::createStandardContextMenu), so the link's selector-less sheet, `* { ... }`,
            //! reached it as a descendant, and a sheet of the widget or an ancestor outranks the application sheet: the legacy menu's
            //! surface was the link's fill, not the `background-color: #222222` of `QMenu` (Menu.qss:12). Everything else of the menu
            //! is the application sheet's, which the Global popup element already carries, so this element is a copy of it with the
            //! surface (a copy made when first needed, as the Global element is private to MenuElement.cpp)
            const AzQtComponents::MenuElement& linkMenu()
            {
                static const AzQtComponents::MenuElement menu = []
                {
                    AzQtComponents::MenuElement element = AzQtComponents::MenuElement::forWidget(nullptr);
                    element.m_name = "PhysX.Menu.DocumentationLink";
                    element.m_surface = LinkFill;
                    return element;
                }();
                return menu;
            }

            //! The menus inside a link, as the sheet reached its descendants (`Class W`)
            bool isLinkMenu(const QWidget* menu)
            {
                return AzQtComponents::Selectors::withinClass(menu, DocumentationLinkWidget::staticMetaObject.className());
            }
        } // namespace DocumentationLinkWidgetFlatInternal

        void DocumentationLinkWidget::RegisterFlatElements()
        {
            static bool registered = false;
            if (!registered)
            {
                registered = true;
                namespace Flat = DocumentationLinkWidgetFlatInternal;
                AzQtComponents::BoxElement::registerElement(&Flat::Link, &DocumentationLinkWidget::staticMetaObject, &Flat::isLink);
                AzQtComponents::MenuElement::registerElement(&Flat::linkMenu(), &QMenu::staticMetaObject, &Flat::isLinkMenu);
            }
        }

        DocumentationLinkWidget::DocumentationLinkWidget(const QString& format, const QString& address)
            : QLabel()
        {
            setText(format.arg(address));
            setTextInteractionFlags(Qt::TextBrowserInteraction);
            setOpenExternalLinks(true);
            setAlignment(Qt::AlignCenter);
            setContentsMargins(60, 7, 60, 7);
            setWordWrap(true);

            // The legacy GUI sets the link's sheet, the fill both rungs read (LinkFill), written as the rgb() text the sheet always
            // had. The flattened GUI sets none, and its element draws the fill (RegisterFlatElements).
            const QColor& fill = DocumentationLinkWidgetFlatInternal::LinkFill;
            AzQtComponents::StyleManager::setLegacyStyleSheet(
                this, QString("background-color: rgb(%1, %2, %3);").arg(fill.red()).arg(fill.green()).arg(fill.blue()));
            if (AzQtComponents::StyleManager::stylesheetsDisabled())
            {
                RegisterFlatElements();
            }
        }
    }
}
