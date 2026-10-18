/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Elements/ItemViewElement.h>
#include <AzQtComponents/Components/Elements/ComboPopupElement.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/TableView.h>

#include <QAbstractItemView>
#include <QApplication>
#include <QCoreApplication>
#include <QHash>
#include <QHeaderView>
#include <QIcon>
#include <QEvent>
#include <QListView>
#include <QPainter>
#include <QPalette>
#include <QPointer>
#include <QPair>
#include <QStyleOption>
#include <QTableView>
#include <QTreeView>
#include <QVariant>
#include <QVector>

#include <memory>
#include <vector>

namespace AzQtComponents
{
    //! Everything private to this file. A named namespace, not an anonymous one: a unity build joins this file
    //! with others into one translation unit, where anonymous namespaces merge (FrameElement.cpp does the same).
    namespace ItemViewElementInternal
    {
        // ------------------------------------------------------------------------------
        // Global elements. Each value cites the declaration that grants it, as file:line of the legacy sheets in Components/Widgets/.
        // What an element leaves out is what TableView.qss gave every view (the defaults of ItemViewElement).
        // ------------------------------------------------------------------------------

        //! A plain QTreeView (the outliner, the asset browser, every tree of the engine): TableView.qss gave it the item and the
        //! header (:31-40, :62-76), the colours (:21-24, :42-60), the disabled text (:99-102), the check box margin (:78-85), the
        //! two carets (:87-97) and the indentation of TreeView::polish (TreeView.h:39, TreeView.cpp:250). TOKENIZE: TreeViewIndentation
        const ItemViewElement TreeLook{
            .m_name = "Global.ItemView.Tree",
            .m_indicatorMargin = QMargins(-3, 0, 0, 0),                         // :80-84 QTreeView::indicator { margin: 0px; margin-left: -3px }
            .m_closed = { ":/TreeView/closed.svg", QMargins(6, 8, 6, 8) },      // :87-91 closed.svg, padding: 8px 6px
            .m_open = { ":/TreeView/open.svg", QMargins(4, 10, 4, 10) },        // :93-97 open.svg, padding: 10px 4px
            .m_indentation = 20,                                                // TreeView.h:39 IndentationWidth
            .m_faces = {
                { ItemViewElement::Selected, { QColor(101, 101, 101), QColor(), QColor() } },   // :42-50 :selected { background: rgb(101,101,101) }. TOKENIZE: ItemViewSelection
                { ItemViewElement::Hover, { QColor(90, 90, 90), QColor(), QColor() } },         // :52-60 :hover { background: rgb(90,90,90) }. TOKENIZE: ItemViewHover. UNVERIFIED: no Native capture holds a hovered item
                { ItemViewElement::Disabled, { QColor(), QColor(0x77, 0x77, 0x77), QColor() } },    // :99-102 QTreeView::item:disabled { color: #777777 }. TOKENIZE: ItemViewDisabledText.
                                                                                                    // UNVERIFIED: no Native capture holds a disabled item with a text
            },
        };

        //! An AzQtComponents::TableView (and the AssetFolderListView, a TableView): the same as a tree, with no indentation of its own
        //! (TableView::polish sets none). Its sheet's `image: none` for its branches (TableView.qss:104-108) never applies: it names two
        //! pseudo-classes where the rules of the carets above name three, which outrank it, so a row with children shows a caret as in a
        //! tree (the legacy twin of the tests has it so), as the PNG carets of AssetFolderListView.qss:14-24 do not apply either: a rule
        //! of equal specificity declared later wins, and TableView.qss comes after AssetFolderListView.qss in the application sheet.
        const ItemViewElement TableViewLook{
            .m_name = "Global.ItemView.TableView",
            .m_indicatorMargin = QMargins(-3, 0, 0, 0),                         // :80-84
            .m_closed = { ":/TreeView/closed.svg", QMargins(6, 8, 6, 8) },      // :87-91
            .m_open = { ":/TreeView/open.svg", QMargins(4, 10, 4, 10) },        // :93-97
            .m_faces = {
                { ItemViewElement::Selected, { QColor(101, 101, 101), QColor(), QColor() } },   // :42-50. TOKENIZE: ItemViewSelection
                { ItemViewElement::Hover, { QColor(90, 90, 90), QColor(), QColor() } },         // :52-60. TOKENIZE: ItemViewHover. UNVERIFIED (see TreeLook)
                { ItemViewElement::Disabled, { QColor(), QColor(0x77, 0x77, 0x77), QColor() } },    // :99-102 (a TableView is a QTreeView). TOKENIZE: ItemViewDisabledText. UNVERIFIED
            },
        };

        //! A QTableView: the item and the header of TableView.qss, the check box of CheckBox.qss:23-28 (margin 2px), no caret
        const ItemViewElement TableLook{
            .m_name = "Global.ItemView.Table",
        };

        //! A QListView (not the list of a combo box popup, which ComboPopupElement owns): the same as a table
        const ItemViewElement ListLook{
            .m_name = "Global.ItemView.List",
        };

        // ------------------------------------------------------------------------------
        // Resolution
        // ------------------------------------------------------------------------------

        //! The name of the dynamic property that keeps the element of a view
        constexpr const char* ElementProperty = "_o3de_itemViewElement";
        //! ... and the one that says a tree has been shown
        constexpr const char* ShownProperty = "_o3de_itemViewShown";

        struct Row
        {
            const ItemViewElement* m_element;
            const QMetaObject* m_widgetClass;
            ItemViewElement::Matcher m_matcher;
        };

        //! The elements modules registered, checked before the Global ones
        QVector<Row>& moduleRows()
        {
            static QVector<Row> rows;
            return rows;
        }

        //! The Global element of the kind of a view, or null: it is none of the kinds an element covers (a header is no view of this
        //! element, the list of a combo box popup is ComboPopupElement's, and a QAbstractItemView of another kind had no rule of the sheets)
        const ItemViewElement* globalFor(const QWidget* widget)
        {
            if (!qobject_cast<const QAbstractItemView*>(widget) || qobject_cast<const QHeaderView*>(widget) || ComboPopupElement::coversView(widget))
            {
                return nullptr;
            }
            if (qobject_cast<const TableView*>(widget))
            {
                return &TableViewLook;
            }
            if (qobject_cast<const QTreeView*>(widget))
            {
                return &TreeLook;
            }
            if (qobject_cast<const QTableView*>(widget))
            {
                return &TableLook;
            }
            if (qobject_cast<const QListView*>(widget))
            {
                return &ListLook;
            }
            return nullptr;
        }

        bool used(const ItemViewElement::StateFace& entry)
        {
            return entry.m_states != 0 || entry.m_face.m_fill.isValid() || entry.m_face.m_text.isValid() || entry.m_face.m_selectedText.isValid();
        }

        //! A module's element over the Global one of the kind of its view (see ItemViewElement::registerElement): the Global element where
        //! the module's member has the value of the defaults, the module's where it changed it, and the faces of both, the module's last.
        //! One per pair, kept for the life of the process, as the elements are.
        const ItemViewElement* layered(const ItemViewElement* module, const ItemViewElement* global)
        {
            static QHash<QPair<const ItemViewElement*, const ItemViewElement*>, const ItemViewElement*> cache;
            static std::vector<std::unique_ptr<ItemViewElement>> owned;
            static const ItemViewElement defaults{};
            const auto key = qMakePair(module, global);
            if (const auto found = cache.constFind(key); found != cache.constEnd())
            {
                return found.value();
            }

            auto merged = std::make_unique<ItemViewElement>(*global);
            merged->m_name = module->m_name;

            // a member that the module changed from the defaults is the module's, every other member is the Global element's
            const auto take = [&](auto ItemViewElement::*member)
            {
                if (module->*member != defaults.*member)
                {
                    (*merged).*member = module->*member;
                }
            };
            take(&ItemViewElement::m_alternate);
            take(&ItemViewElement::m_selection);
            take(&ItemViewElement::m_selectionText);
            take(&ItemViewElement::m_itemHeight);
            take(&ItemViewElement::m_itemMargin);
            take(&ItemViewElement::m_itemPadding);
            take(&ItemViewElement::m_indicatorSize);
            take(&ItemViewElement::m_indicatorMargin);
            take(&ItemViewElement::m_indentation);
            take(&ItemViewElement::m_headerFill);
            const auto takeBranch = [&](ItemViewElement::Branch ItemViewElement::*member)
            {
                const ItemViewElement::Branch& changed = module->*member;
                const ItemViewElement::Branch& unchanged = defaults.*member;
                if (qstrcmp(changed.m_image, unchanged.m_image) != 0 || changed.m_padding != unchanged.m_padding)
                {
                    (*merged).*member = changed;
                }
            };
            takeBranch(&ItemViewElement::m_closed);
            takeBranch(&ItemViewElement::m_open);

            // the faces: the Global entries, then the module's; where they do not all fit the module's come first
            constexpr int Capacity = static_cast<int>(sizeof(merged->m_faces) / sizeof(merged->m_faces[0]));
            std::vector<ItemViewElement::StateFace> faces;
            for (const ItemViewElement* source : { global, module })
            {
                for (const ItemViewElement::StateFace& entry : source->m_faces)
                {
                    if (used(entry))
                    {
                        faces.push_back(entry);
                    }
                }
            }
            if (static_cast<int>(faces.size()) > Capacity)
            {
                qWarning("ItemViewElement %s: %d faces, the first %d of the Global element's and the module's are dropped", module->m_name,
                    static_cast<int>(faces.size()), static_cast<int>(faces.size()) - Capacity);
                faces.erase(faces.begin(), faces.begin() + (faces.size() - Capacity));
            }
            for (int i = 0; i < Capacity; ++i)
            {
                merged->m_faces[i] = (i < static_cast<int>(faces.size())) ? faces[i] : ItemViewElement::StateFace();
            }

            const ItemViewElement* result = merged.get();
            owned.push_back(std::move(merged));
            cache.insert(key, result);
            return result;
        }

        //! The element of a view, or null: a module's over the Global one of its kind, else the Global one
        const ItemViewElement* findElement(const QWidget* widget)
        {
            const ItemViewElement* global = globalFor(widget);
            if (!global)
            {
                return nullptr;
            }
            for (const Row& row : moduleRows())
            {
                if (widget->metaObject()->inherits(row.m_widgetClass) && row.m_matcher(widget))
                {
                    return layered(row.m_element, global);
                }
            }
            return global;
        }

        //! The icon of a resource, as the sheet made it for an `image:` (QStyleSheetImageData holds a QIcon of the url)
        const QIcon& iconOf(const char* resource)
        {
            static QHash<QByteArray, QIcon> icons;
            const QByteArray key(resource);
            auto found = icons.find(key);
            if (found == icons.end())
            {
                found = icons.insert(key, QIcon(QString::fromUtf8(resource)));
            }
            return found.value();
        }

        //! Tells a tree it has been shown (ItemViewElement::showTree). It filters the events of the trees an element covers.
        class ShowWatcher
            : public QObject
        {
        public:
            explicit ShowWatcher(QObject* parent)
                : QObject(parent)
            {
            }

            bool eventFilter(QObject* watched, QEvent* event) override
            {
                if (event->type() == QEvent::Show)
                {
                    ItemViewElement::showTree(qobject_cast<QWidget*>(watched));
                }
                return QObject::eventFilter(watched, event);
            }
        };

        //! One for the application (a child of it, so the next application of a process makes its own)
        QObject* showWatcher()
        {
            static QPointer<ShowWatcher> watcher;
            if (!watcher)
            {
                watcher = new ShowWatcher(QCoreApplication::instance());
            }
            return watcher;
        }

        //! The headers of a view: the one of a tree, the two of a table
        QVector<QHeaderView*> headersOf(QAbstractItemView* view)
        {
            QVector<QHeaderView*> headers;
            if (auto tree = qobject_cast<QTreeView*>(view))
            {
                headers.append(tree->header());
            }
            else if (auto table = qobject_cast<QTableView*>(view))
            {
                headers.append(table->horizontalHeader());
                headers.append(table->verticalHeader());
            }
            return headers;
        }

        //! The header of a view: its sections have the colour of the section rule and the area beside them has none. The legacy header and
        //! its viewport were transparent (BaseStyleSheet.qss:15-21 `*`; qstylesheetstyle.cpp:2952-2959 took the viewport's own fill away):
        //! what shows beyond the last section is what is behind the header, and a section declared transparent shows it too.
        //! The Button role is the section's colour, as the base style would paint it.
        void applyHeader(QHeaderView* header)
        {
            QPalette palette = header->palette();
            palette.setColor(QPalette::Button, ItemViewElement::headerFillOf(header));
            header->setPalette(palette);
            header->viewport()->setAutoFillBackground(false);
        }
    } // namespace ItemViewElementInternal

    // ----------------------------------------------------------------------------------
    // Element
    // ----------------------------------------------------------------------------------

    unsigned ItemViewElement::statesOf(const QStyleOption* option)
    {
        // The pseudo-classes of qstylesheetstyle.cpp:1757-1804: a disabled item is not hovered, and a window is active or is not
        unsigned states = 0;
        if (!option->state.testFlag(QStyle::State_Enabled))
        {
            states |= Disabled;
        }
        else if (option->state.testFlag(QStyle::State_MouseOver))
        {
            states |= Hover;
        }
        if (option->state.testFlag(QStyle::State_Selected))
        {
            states |= Selected;
        }
        states |= option->state.testFlag(QStyle::State_Active) ? Active : Inactive;
        return states;
    }

    QColor ItemViewElement::fillOf(unsigned states, Scope scope) const
    {
        for (int i = static_cast<int>(sizeof(m_faces) / sizeof(m_faces[0])) - 1; i >= 0; --i)
        {
            if ((states & m_faces[i].m_states) == m_faces[i].m_states && m_faces[i].m_face.m_fill.isValid()
                && (m_faces[i].m_scope == Both || m_faces[i].m_scope == scope))
            {
                return m_faces[i].m_face.m_fill;
            }
        }
        return QColor();
    }

    QColor ItemViewElement::textOf(unsigned states) const
    {
        for (int i = static_cast<int>(sizeof(m_faces) / sizeof(m_faces[0])) - 1; i >= 0; --i)
        {
            if ((states & m_faces[i].m_states) == m_faces[i].m_states && m_faces[i].m_face.m_text.isValid()
                && m_faces[i].m_scope != OnBranch)
            {
                return m_faces[i].m_face.m_text;
            }
        }
        return QColor();
    }

    QColor ItemViewElement::selectedTextOf(unsigned states) const
    {
        for (int i = static_cast<int>(sizeof(m_faces) / sizeof(m_faces[0])) - 1; i >= 0; --i)
        {
            if ((states & m_faces[i].m_states) == m_faces[i].m_states && m_faces[i].m_face.m_selectedText.isValid()
                && m_faces[i].m_scope != OnBranch)
            {
                return m_faces[i].m_face.m_selectedText;
            }
        }
        return QColor();
    }

    QSize ItemViewElement::itemSize(const QSize& base) const
    {
        // QRenderRule::adjustSize gives the contents the rule's height, QRenderRule::boxSize the margin and the padding around them
        // (qstylesheetstyle.cpp:560-571, 1096-1122; border: none has no width)
        const int height = (m_itemHeight == Unset) ? base.height() : m_itemHeight;
        return QSize(base.width() + m_itemMargin.left() + m_itemMargin.right() + m_itemPadding.left() + m_itemPadding.right(),
            height + m_itemMargin.top() + m_itemMargin.bottom() + m_itemPadding.top() + m_itemPadding.bottom());
    }

    QRect ItemViewElement::itemContentsRect(const QRect& rect) const
    {
        return rect.marginsRemoved(m_itemMargin).marginsRemoved(m_itemPadding);
    }

    QSize ItemViewElement::indicatorBox() const
    {
        // The rule's size is the image's plus the margin around it (QRenderRule::size, qstylesheetstyle.cpp:555)
        return QSize(m_indicatorSize.width() + m_indicatorMargin.left() + m_indicatorMargin.right(),
            m_indicatorSize.height() + m_indicatorMargin.top() + m_indicatorMargin.bottom());
    }

    QRect ItemViewElement::indicatorImageRect(const QRect& slot) const
    {
        return slot.marginsRemoved(m_indicatorMargin);
    }

    void ItemViewElement::prepareItem(QStyleOptionViewItem* option) const
    {
        // QRenderRule::configurePalette(palette, Text | HighlightedText, ...): the rule's colour is the text, window text and the item's
        // text role, its selection colour the selected text (qstylesheetstyle.cpp:1393-1422)
        const unsigned states = statesOf(option);
        const bool selected = option->state.testFlag(QStyle::State_Selected);
        if (const QColor text = textOf(states); text.isValid())
        {
            option->palette.setColor(QPalette::Text, text);
            option->palette.setColor(QPalette::WindowText, text);
            option->palette.setColor(selected ? QPalette::HighlightedText : QPalette::Text, text);
        }
        if (const QColor selectedText = selectedTextOf(states); selectedText.isValid())
        {
            option->palette.setColor(QPalette::HighlightedText, selectedText);
        }
    }

    void ItemViewElement::drawFill(QPainter* painter, const QStyleOption* option) const
    {
        const QColor fill = fillOf(statesOf(option));
        if (fill.isValid() && fill.alpha() > 0)
        {
            painter->fillRect(option->rect.marginsRemoved(m_itemMargin), fill);
        }
    }

    bool ItemViewElement::drawBranch(const QStyle* style, const QStyleOptionViewItem* option, QPainter* painter, const QWidget* widget) const
    {
        // The rule of a branch is drawn when it has a background or an image (QRenderRule::hasDrawable); otherwise the base style
        // draws the branch, and what it draws is no caret for a view that has a rule for each (qstylesheetstyle.cpp:4777-4790)
        const QColor fill = fillOf(statesOf(option), OnBranch);
        const Branch& branch = option->state.testFlag(QStyle::State_Children) ? (option->state.testFlag(QStyle::State_Open) ? m_open : m_closed) : Branch{};
        const bool hasImage = branch.m_image && *branch.m_image;
        if (!fill.isValid() && !hasImage)
        {
            return false;
        }

        // The panel of the row first (the sheet asked its proxy, which is the style), then the rule: its background over the whole cell
        // and its image in the cell less the padding (QRenderRule::drawRule), where QIcon::paint fits and centres it
        style->drawPrimitive(QStyle::PE_PanelItemViewRow, option, painter, widget);
        if (fill.isValid() && fill.alpha() > 0)
        {
            painter->fillRect(option->rect, fill);
        }
        if (hasImage)
        {
            ItemViewElementInternal::iconOf(branch.m_image).paint(painter, option->rect.marginsRemoved(branch.m_padding), Qt::AlignCenter);
        }
        return true;
    }

    // ----------------------------------------------------------------------------------
    // Resolution
    // ----------------------------------------------------------------------------------

    void ItemViewElement::registerElement(const ItemViewElement* element, const QMetaObject* widgetClass, Matcher matcher)
    {
        ItemViewElementInternal::moduleRows().append({ element, widgetClass, matcher });
        if (!StyleManager::stylesheetsDisabled())
        {
            return;
        }

        // A module builds some windows before it sets up its styles: resolve the views polished before this call
        const QWidgetList widgets = QApplication::allWidgets();
        for (QWidget* widget : widgets)
        {
            if (widget->testAttribute(Qt::WA_WState_Polished) && widget->metaObject()->inherits(widgetClass))
            {
                apply(widget);
                widget->update();
            }
        }
    }

    void ItemViewElement::apply(QWidget* widget)
    {
        auto view = qobject_cast<QAbstractItemView*>(widget);
        if (!view || qobject_cast<QHeaderView*>(widget))
        {
            return;
        }

        // Kept on the view: a draw reads it back without resolving again (a null element is kept too)
        const ItemViewElement* element = ItemViewElementInternal::findElement(view);
        view->setProperty(ItemViewElementInternal::ElementProperty, QVariant::fromValue(reinterpret_cast<quintptr>(element)));
        if (!element)
        {
            return;
        }

        // The roles of the view: the sheet's alternate-background-color, selection-background-color and selection-color were the view's
        // palette (QRenderRule::configurePalette, qstylesheetstyle.cpp:1393-1422), for every colour group
        QPalette palette = view->palette();
        palette.setColor(QPalette::AlternateBase, element->m_alternate);
        palette.setColor(QPalette::Highlight, element->m_selection);
        palette.setColor(QPalette::HighlightedText, element->m_selectionText);
        view->setPalette(palette);

        // The sheet style tracked the pointer of a view whose rules have a :hover (qstylesheetstyle.cpp:2894-2903): every view's item rules do
        view->setAttribute(Qt::WA_Hover);
        view->viewport()->setAttribute(Qt::WA_Hover);
        view->viewport()->setMouseTracking(true);

        if (auto tree = qobject_cast<QTreeView*>(view); tree && element->m_indentation != Unset)
        {
            tree->setIndentation(element->m_indentation);
            tree->installEventFilter(ItemViewElementInternal::showWatcher());
        }

        for (QHeaderView* header : ItemViewElementInternal::headersOf(view))
        {
            ItemViewElementInternal::applyHeader(header);
        }
    }

    const ItemViewElement* ItemViewElement::forWidget(const QWidget* widget)
    {
        if (!widget)
        {
            return nullptr;
        }
        const QVariant stored = widget->property(ItemViewElementInternal::ElementProperty);
        return stored.isValid() ? reinterpret_cast<const ItemViewElement*>(stored.value<quintptr>()) : nullptr;
    }

    const ItemViewElement* ItemViewElement::forHeader(const QWidget* header)
    {
        const QWidget* view = header ? header->parentWidget() : nullptr;
        if (!view)
        {
            return nullptr;
        }
        // The header can be polished before its view: the element of a view not resolved yet is looked up now (and not kept: no property is set here)
        const QVariant stored = view->property(ItemViewElementInternal::ElementProperty);
        return stored.isValid() ? reinterpret_cast<const ItemViewElement*>(stored.value<quintptr>()) : ItemViewElementInternal::findElement(view);
    }

    QColor ItemViewElement::headerFillOf(const QWidget* header)
    {
        // The Gallery's log table sets its sections transparent: Native had an inline sheet `QHeaderView::section { background: transparent }`
        if (header && header->property("SectionBackground").toString() == QLatin1String("transparent"))
        {
            return QColor(0, 0, 0, 0);
        }
        static const QColor tableViewFill = ItemViewElement().m_headerFill;
        const ItemViewElement* element = forHeader(header);
        return element ? element->m_headerFill : tableViewFill;
    }

    void ItemViewElement::showTree(QWidget* tree)
    {
        if (tree->property(ItemViewElementInternal::ShownProperty).toBool())
        {
            return;
        }
        tree->setProperty(ItemViewElementInternal::ShownProperty, true);
        const ItemViewElement* element = forWidget(tree);
        if (auto treeView = qobject_cast<QTreeView*>(tree); treeView && element && element->m_indentation != Unset)
        {
            treeView->setIndentation(element->m_indentation);
        }
    }
} // namespace AzQtComponents
