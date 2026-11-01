/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Elements/ItemViewElement.h>
#include <AzQtComponents/Components/Elements/ComboPopupElement.h>
#include <AzQtComponents/Components/Elements/Foundations.h>
#include <AzQtComponents/Components/Widgets/TableView.h>
#include <AzQtComponents/Utilities/IconUtilities.h>

#include <QAbstractItemView>
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
    //! Private to this file: named, not anonymous, as a unity build merges the anonymous namespaces of its files
    namespace ItemViewElementInternal
    {
        // ------------------------------------------------------------------------------
        // Values
        // ------------------------------------------------------------------------------

        // ---- The faces of every view
        const QColor SelectedItemFill(0x65, 0x65, 0x65);            // TableView.qss:49 $fill.item.selected.Alt1
        const QColor SelectedBranchFill(0x65, 0x65, 0x65);          // TableView.qss:49 $fill.tree-branch.selected.Alt1
        const QColor HoveredItemFill(0x5A, 0x5A, 0x5A);             // TableView.qss:59 $fill.item.hover.Alt1
        const QColor HoveredBranchFill(0x5A, 0x5A, 0x5A);           // TableView.qss:59 $fill.tree-branch.hover.Alt1

        // ---- A tree: its check box, its carets, its disabled text and its indentation
        // TableView.qss:84, :81 $space.check-indicator.idle.margin-left $space.check-indicator.idle.margin.Alt2
        constexpr QMargins TreeIndicatorMargin(-3, 0, 0, 0);
        // TableView.qss:89 $asset.tree-branch.closed+branch $asset.tree-branch.branch+closed (one rule for two selectors)
        constexpr const char* ClosedBranchImage = ":/TreeView/closed.svg";
        // TableView.qss:90 $space.tree-branch.closed+branch $space.tree-branch.branch+closed (one rule for two selectors)
        constexpr QMargins ClosedBranchPadding(6, 8, 6, 8);
        constexpr const char* OpenBranchImage = ":/TreeView/open.svg";  // TableView.qss:95 $asset.tree-branch.open+branch
        constexpr QMargins OpenBranchPadding(4, 10, 4, 10);         // TableView.qss:96 $space.tree-branch.open+branch
        const QColor DisabledItemText(0x77, 0x77, 0x77);            // TableView.qss:101 $text.item.disabled.Alt1
        constexpr int TreeIndentation = 20;                         // TreeView::IndentationWidth $size.view.idle.indentation

        // ---- A header whose sections are declared transparent (TableViewPage::TableViewPage, the Gallery)
        const QColor TransparentSectionFill = Foundations::Clear;       // TableViewPage::TableViewPage $fill.header.idle.Clear

        // ------------------------------------------------------------------------------
        // Elements
        // ------------------------------------------------------------------------------

        //! A plain QTreeView (the outliner, the asset browser, every tree of the engine): TableView.qss's item, header, colours, disabled
        //! text, check box margin and carets, and the indentation of TreeView::polish
        const ItemViewElement TreeLook{
            .m_name = "Global.ItemView.Tree",
            .m_indicatorMargin = TreeIndicatorMargin,
            .m_closed = { ClosedBranchImage, ClosedBranchPadding },
            .m_open = { OpenBranchImage, OpenBranchPadding },
            .m_indentation = TreeIndentation,
            .m_faces = {
                { .m_states = ItemViewElement::Selected, .m_face = { .m_fill = SelectedItemFill }, .m_scope = ItemViewElement::OnItem },
                { .m_states = ItemViewElement::Selected, .m_face = { .m_fill = SelectedBranchFill }, .m_scope = ItemViewElement::OnBranch },
                { .m_states = ItemViewElement::Hover, .m_face = { .m_fill = HoveredItemFill }, .m_scope = ItemViewElement::OnItem },
                { .m_states = ItemViewElement::Hover, .m_face = { .m_fill = HoveredBranchFill }, .m_scope = ItemViewElement::OnBranch },
                { .m_states = ItemViewElement::Disabled, .m_face = { .m_text = DisabledItemText } },
            },
        };

        //! An AzQtComponents::TableView (and the AssetFolderListView, a TableView): a tree with no indentation of its own. Its sheet's
        //! `image: none` for the branches never applies (the carets' rules above outrank it), so a row with children shows a caret.
        const ItemViewElement TableViewLook{
            .m_name = "Global.ItemView.TableView",
            .m_indicatorMargin = TreeIndicatorMargin,
            .m_closed = { ClosedBranchImage, ClosedBranchPadding },
            .m_open = { OpenBranchImage, OpenBranchPadding },
            .m_faces = {
                { .m_states = ItemViewElement::Selected, .m_face = { .m_fill = SelectedItemFill }, .m_scope = ItemViewElement::OnItem },
                { .m_states = ItemViewElement::Selected, .m_face = { .m_fill = SelectedBranchFill }, .m_scope = ItemViewElement::OnBranch },
                { .m_states = ItemViewElement::Hover, .m_face = { .m_fill = HoveredItemFill }, .m_scope = ItemViewElement::OnItem },
                { .m_states = ItemViewElement::Hover, .m_face = { .m_fill = HoveredBranchFill }, .m_scope = ItemViewElement::OnBranch },
                { .m_states = ItemViewElement::Disabled, .m_face = { .m_text = DisabledItemText } },
            },
        };

        //! A QTableView: TableView.qss's item and header, CheckBox.qss's check box, no caret
        const ItemViewElement TableLook{
            .m_name = "Global.ItemView.Table",
            .m_faces = {
                { .m_states = ItemViewElement::Selected, .m_face = { .m_fill = SelectedItemFill }, .m_scope = ItemViewElement::OnItem },
                { .m_states = ItemViewElement::Selected, .m_face = { .m_fill = SelectedBranchFill }, .m_scope = ItemViewElement::OnBranch },
                { .m_states = ItemViewElement::Hover, .m_face = { .m_fill = HoveredItemFill }, .m_scope = ItemViewElement::OnItem },
                { .m_states = ItemViewElement::Hover, .m_face = { .m_fill = HoveredBranchFill }, .m_scope = ItemViewElement::OnBranch },
            },
        };

        //! A QListView (not a combo box popup's list, which ComboPopupElement owns): the same as a table
        const ItemViewElement ListLook{
            .m_name = "Global.ItemView.List",
            .m_faces = {
                { .m_states = ItemViewElement::Selected, .m_face = { .m_fill = SelectedItemFill }, .m_scope = ItemViewElement::OnItem },
                { .m_states = ItemViewElement::Selected, .m_face = { .m_fill = SelectedBranchFill }, .m_scope = ItemViewElement::OnBranch },
                { .m_states = ItemViewElement::Hover, .m_face = { .m_fill = HoveredItemFill }, .m_scope = ItemViewElement::OnItem },
                { .m_states = ItemViewElement::Hover, .m_face = { .m_fill = HoveredBranchFill }, .m_scope = ItemViewElement::OnBranch },
            },
        };

        // ------------------------------------------------------------------------------
        // Helpers
        // ------------------------------------------------------------------------------

        //! The dynamic property that says a tree has been shown
        constexpr const char* ShownProperty = "_o3de_itemViewShown";

        ElementRegistry<ItemViewElement>& registry()
        {
            static ElementRegistry<ItemViewElement> rows("_o3de_itemViewElement");
            return rows;
        }

        //! The Global element of a view's kind; none for a header, a combo box popup's list (ComboPopupElement's) and a QAbstractItemView
        //! of a kind no rule covered
        const ItemViewElement* globalElement(const QWidget* widget)
        {
            if (!qobject_cast<const QAbstractItemView*>(widget) || qobject_cast<const QHeaderView*>(widget)
                || ComboPopupElement::coversView(widget))
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
            return entry.m_states != 0 || entry.m_face.m_fill.isValid() || entry.m_face.m_text.isValid()
                || entry.m_face.m_selectedText.isValid();
        }

        //! A module's element over the Global one of its view's kind: the module's members where they differ from the defaults, the
        //! Global element's otherwise, and the faces of both, the module's last. One per pair, kept for the process's life.
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
            const ItemViewElement* global = globalElement(widget);
            if (!global)
            {
                return nullptr;
            }
            const ItemViewElement* module = registry().find(widget);
            return module ? layered(module, global) : global;
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

        //! The header of a view: its sections in the section rule's colour (the Button role, as the base style paints it) and nothing
        //! beside them: the legacy header and its viewport were transparent (BaseStyleSheet.qss:15 `*`; QStyleSheetStyle::polish took the
        //! viewport's own fill away), so what shows beyond the last section is what is behind the header
        void applyHeader(QHeaderView* header)
        {
            QPalette palette = header->palette();
            palette.setColor(QPalette::Button, ItemViewElement::headerFillOf(header));
            header->setPalette(palette);
            header->viewport()->setAutoFillBackground(false);
        }
    } // namespace ItemViewElementInternal

    // ----------------------------------------------------------------------------------
    // Geometry and drawing
    // ----------------------------------------------------------------------------------

    unsigned ItemViewElement::statesOf(const QStyleOption* option)
    {
        // The pseudo-classes (pseudoClass, qstylesheetstyle.cpp): a disabled item is not hovered, and a window is active or is not
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
        // (border: none has no width)
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
        // The rule's size is the image's plus the margin around it (QRenderRule::size)
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
        // text role, its selection colour the selected text
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

    bool ItemViewElement::drawBranch(
        const QStyle* style, const QStyleOptionViewItem* option, QPainter* painter, const QWidget* widget) const
    {
        // A branch's rule is drawn when it has a background or an image (QRenderRule::hasDrawable); otherwise the base style draws the
        // branch (QStyleSheetStyle::drawPrimitive, PE_IndicatorBranch)
        const QColor fill = fillOf(statesOf(option), OnBranch);
        const bool children = option->state.testFlag(QStyle::State_Children);
        const Branch& branch = children ? (option->state.testFlag(QStyle::State_Open) ? m_open : m_closed) : Branch{};
        const bool hasImage = branch.m_image && *branch.m_image;
        if (!fill.isValid() && !hasImage)
        {
            return false;
        }

        // The panel of the row first (the sheet asked its proxy, which is the style), then the rule: its background over the whole cell
        // and its image (a QIcon of the url, as QStyleSheetImageData holds it) in the cell less the padding (QRenderRule::drawRule),
        // where QIcon::paint fits and centres it
        style->drawPrimitive(QStyle::PE_PanelItemViewRow, option, painter, widget);
        if (fill.isValid() && fill.alpha() > 0)
        {
            painter->fillRect(option->rect, fill);
        }
        if (hasImage)
        {
            CachedIcon(QString::fromUtf8(branch.m_image)).paint(painter, option->rect.marginsRemoved(branch.m_padding), Qt::AlignCenter);
        }
        return true;
    }

    // ----------------------------------------------------------------------------------
    // Resolution
    // ----------------------------------------------------------------------------------

    void ItemViewElement::registerElement(
        const ItemViewElement* element, const QMetaObject* widgetClass, ElementMatcher matcher, ElementScope scope)
    {
        ItemViewElementInternal::registry().add(element, widgetClass, matcher, scope);
        resolvePolished(widgetClass, [](QWidget* widget)
            {
                resolve(widget);
                widget->update();
            });
    }

    void ItemViewElement::resolve(QWidget* widget)
    {
        auto view = qobject_cast<QAbstractItemView*>(widget);
        if (!view || qobject_cast<QHeaderView*>(widget))
        {
            return;
        }

        // Kept on the view: a draw reads it back without resolving again (a null element is kept too)
        const ItemViewElement* element = ItemViewElementInternal::findElement(view);
        ItemViewElementInternal::registry().keep(view, element);
        if (!element)
        {
            return;
        }

        // The view's roles: alternate-background-color, selection-background-color and selection-color were the view's palette in
        // every colour group (QRenderRule::configurePalette)
        QPalette palette = view->palette();
        palette.setColor(QPalette::AlternateBase, element->m_alternate);
        palette.setColor(QPalette::Highlight, element->m_selection);
        palette.setColor(QPalette::HighlightedText, element->m_selectionText);
        view->setPalette(palette);

        // The sheet tracked the pointer of a view whose rules have a :hover (QStyleSheetStyle::polish): every view's item rules do
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
        return ItemViewElementInternal::registry().kept(widget).value_or(nullptr);
    }

    const ItemViewElement* ItemViewElement::forHeader(const QWidget* header)
    {
        const QWidget* view = header ? header->parentWidget() : nullptr;
        if (!view)
        {
            return nullptr;
        }
        // The header can be polished before its view: a view not resolved yet is looked up now, and nothing is kept
        const std::optional<const ItemViewElement*> kept = ItemViewElementInternal::registry().kept(view);
        return kept ? *kept : ItemViewElementInternal::findElement(view);
    }

    QColor ItemViewElement::headerFillOf(const QWidget* header)
    {
        // The Gallery's log table sets its sections transparent, where the legacy page had an inline sheet
        // `QHeaderView::section { background: transparent }`
        if (header && Selectors::hasProperty(header, "SectionBackground", "transparent"))
        {
            return ItemViewElementInternal::TransparentSectionFill;
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
