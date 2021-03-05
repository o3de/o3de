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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "EditorCommon_precompiled.h"
#include "PropertyRowContainer.h"
#include "PropertyRowPointer.h"
#include "QPropertyTree.h"
#include "PropertyTreeModel.h"
#include "PropertyDrawContext.h"
#include "Serialization.h"
#include "PropertyRowPointer.h"

#include <QMenu>
#include <QKeyEvent>

// ---------------------------------------------------------------------------

ContainerMenuHandler::ContainerMenuHandler(QPropertyTree* tree, PropertyRowContainer* container)
    : element()
    , container(container)
    , tree(tree)
    , pointerIndex(-1)
{
}



// ---------------------------------------------------------------------------
SERIALIZATION_CLASS_NAME(PropertyRow, PropertyRowContainer, "PropertyRowContainer", "Container");

PropertyRowContainer::PropertyRowContainer()
    : fixedSize_(false)
    , elementTypeName_("")
    , inlined_(false)
{
    buttonLabel_[0] = '\0';
}

struct ClassMenuItemAdderRowContainer
    : ClassMenuItemAdder
{
    ClassMenuItemAdderRowContainer(PropertyRowContainer* row, QPropertyTree* tree, bool insert = false)
        : row_(row)
        , tree_(tree)
        , insert_(insert) {}

    void addAction(QMenu& menu, const char* text, int index) override
    {
        ContainerMenuHandler* handler = row_->createMenuHandler(tree_, row_);
        tree_->addMenuHandler(handler);
        handler->pointerIndex = index;

        QAction* action = menu.addAction(text);
        QObject::connect(action, SIGNAL(triggered()), handler, SLOT(onMenuAppendPointerByIndex()));
    }
protected:
    PropertyRowContainer* row_;
    QPropertyTree* tree_;
    bool insert_;
};

void PropertyRowContainer::redraw(const PropertyDrawContext& context)
{
    QRect widgetRect = context.widgetRect;
    if (widgetRect.width() == 0 || inlined_)
    {
        return;
    }
    QRect rt = widgetRect;
    rt.adjust(0, 1, -1, -1);
    QColor brushColor = context.tree->palette().button().color();
    QLinearGradient gradient(rt.left(), rt.top(), rt.left(), rt.bottom());
    gradient.setColorAt(0.0f, brushColor);
    gradient.setColorAt(0.6f, brushColor);
    gradient.setColorAt(1.0f, context.tree->palette().color(QPalette::Shadow));
    QBrush brush(gradient);

    const wchar_t* text = multiValue() ? L"..." : buttonLabel_;
    int buttonFlags = BUTTON_CENTER | BUTTON_POPUP_ARROW;
    if (userReadOnly())
    {
        buttonFlags |= BUTTON_DISABLED;
    }
    if (context.m_pressed)
    {
        buttonFlags |= BUTTON_PRESSED;
    }
    context.drawButton(rt, text, buttonFlags, &context.tree->font());
}


bool PropertyRowContainer::onActivate(const PropertyActivationEvent& e)
{
    if (e.reason == e.REASON_RELEASE)
    {
        return false;
    }
    if (userReadOnly())
    {
        return false;
    }
    if (inlined_)
    {
        return false;
    }
    QMenu menu;
    generateMenu(menu, e.tree, true);
    e.tree->_setPressedRow(this);
    menu.exec(e.tree->_toScreen(QPoint(widgetPos_, pos_.y() + e.tree->_defaultRowHeight())));
    e.tree->_setPressedRow(0);
    return true;
}

ContainerMenuHandler* PropertyRowContainer::createMenuHandler(QPropertyTree* tree, PropertyRowContainer* container)
{
    return new ContainerMenuHandler(tree, container);
}

void PropertyRowContainer::generateMenu(QMenu& menu, QPropertyTree* tree, bool addActions)
{
    ContainerMenuHandler* handler = createMenuHandler(tree, this);
    tree->addMenuHandler(handler);

    if (fixedSize_)
    {
        if (!inlined_)
        {
            menu.addAction("[ Fixed Size Container ]")->setEnabled(false);
        }
    }
    else if (userReadOnly())
    {
        menu.addAction("[ Read Only Container ]")->setEnabled(false);
    }
    else
    {
        if (addActions)
        {
            PropertyRow* row = defaultRow(tree->model());
            if (row && row->isPointer())
            {
                QMenu* createItem = menu.addMenu("Add");
                menu.addSeparator();

                PropertyRowPointer* pointerRow = static_cast<PropertyRowPointer*>(row);
                ClassMenuItemAdderRowContainer(this, tree).generateMenu(*createItem, tree->model()->typeStringList(pointerRow->baseType()));
            }
            else
            {
                menu.addAction("Insert", handler, SLOT(onMenuAddElement()));
                menu.addAction("Add", handler, SLOT(onMenuAppendElement()), Qt::Key_Insert);
            }
        }

        if (!menu.isEmpty())
        {
            menu.addSeparator();
        }

        QAction* removeAll = menu.addAction(pulledUp() ? "Remove Children" : "Remove All");
        removeAll->setShortcut(QKeySequence("Shift+Delete"));
        removeAll->setEnabled(!userReadOnly());
        QObject::connect(removeAll, SIGNAL(triggered()), handler, SLOT(onMenuRemoveAll()));
    }
}

bool PropertyRowContainer::onContextMenu(QMenu& menu, QPropertyTree* tree)
{
    if (!menu.isEmpty())
    {
        menu.addSeparator();
    }

    generateMenu(menu, tree, true);

    if (pulledUp())
    {
        return !menu.isEmpty();
    }

    return PropertyRow::onContextMenu(menu, tree);
}


void ContainerMenuHandler::onMenuRemoveAll()
{
    tree->model()->rowAboutToBeChanged(container);
    container->clear();
    tree->model()->rowChanged(container);
}

PropertyRow* PropertyRowContainer::defaultRow(PropertyTreeModel* model)
{
    PropertyRow* defaultType = model->defaultType(elementTypeName_);
    //YASLI_ASSERT(defaultType);
    //YASLI_ASSERT(defaultType->numRef() == 1);
    return defaultType;
}

const PropertyRow* PropertyRowContainer::defaultRow(const PropertyTreeModel* model) const
{
    const PropertyRow* defaultType = model->defaultType(elementTypeName_);
    return defaultType;
}

void ContainerMenuHandler::onMenuAddElement()
{
    container->addElement(tree, false);
}

void ContainerMenuHandler::onMenuAppendElement()
{
    container->addElement(tree, true);
}

PropertyRow* PropertyRowContainer::addElement(QPropertyTree* tree, bool append)
{
    tree->model()->rowAboutToBeChanged(this);
    PropertyRow* defaultType = defaultRow(tree->model());
    YASLI_ESCAPE(defaultType != 0, return 0);
    SharedPtr<PropertyRow> clonedRow = defaultType->clone(tree->model()->constStrings());
    if (count() == 0)
    {
        tree->expandRow(this);
    }
    if (append)
    {
        add(clonedRow);
    }
    else
    {
        addBefore(clonedRow, 0);
    }
    clonedRow->setHideChildren(tree->outlineMode());
    clonedRow->setLabelChanged();
    clonedRow->setLabelChangedToChildren();
    setMultiValue(false);
    if (expanded())
    {
        tree->model()->selectRow(clonedRow, true);
    }
    tree->expandRow(clonedRow);
    TreePath path = tree->model()->pathFromRow(clonedRow);
    tree->model()->rowChanged(clonedRow);
    clonedRow = tree->model()->rowFromPath(path);
    tree->update();
    clonedRow = tree->model()->rowFromPath(path);
    if (clonedRow)
    {
        PropertyTreeModel::Selection sel;
        sel.push_back(path);
        tree->model()->setSelection(sel);
        if (clonedRow->activateOnAdd())
        {
            PropertyActivationEvent e;
            e.tree = tree;
            e.reason = e.REASON_NEW_ELEMENT;
            clonedRow->onActivate(e);
        }
    }
    return clonedRow;
}


void ContainerMenuHandler::onMenuAppendPointerByIndex()
{
    PropertyRow* defaultType = container->defaultRow(tree->model());
    PropertyRowPointer* defaultTypePointer = static_cast<PropertyRowPointer*>(defaultType);
    SharedPtr<PropertyRow> clonedRow = defaultType->clone(tree->model()->constStrings());
    if (container->count() == 0)
    {
        tree->expandRow(container);
    }
    container->add(clonedRow);
    clonedRow->setLabelChanged();
    clonedRow->setLabelChangedToChildren();
    clonedRow->setHideChildren(tree->outlineMode());
    container->setMultiValue(false);
    PropertyRowPointer* clonedRowPointer = static_cast<PropertyRowPointer*>(clonedRow.get());
    clonedRowPointer->setDerivedType(defaultTypePointer->derivedTypeName(), defaultTypePointer->factory());
    clonedRowPointer->setBaseType(defaultTypePointer->baseType());
    clonedRowPointer->setFactory(defaultTypePointer->factory());
    if (container->expanded())
    {
        tree->model()->selectRow(clonedRow, true);
    }
    tree->expandRow(clonedRowPointer);
    PropertyTreeModel::Selection sel = tree->model()->selection();

    CreatePointerMenuHandler handler;
    handler.tree = tree;
    handler.row = clonedRowPointer;
    handler.index = pointerIndex;
    handler.onMenuCreateByIndex();
    tree->model()->setSelection(sel);
    tree->update();
}

void ContainerMenuHandler::onMenuChildInsertBefore()
{
    tree->model()->rowAboutToBeChanged(container);
    PropertyRow* defaultType = tree->model()->defaultType(container->elementTypeName());
    if (!defaultType)
    {
        return;
    }
    SharedPtr<PropertyRow> clonedRow = defaultType->clone(tree->model()->constStrings());
    clonedRow->setHideChildren(tree->outlineMode());
    element->setSelected(false);
    container->addBefore(clonedRow, element);
    container->setMultiValue(false);
    tree->model()->selectRow(clonedRow, true);
    PropertyTreeModel::Selection sel = tree->model()->selection();
    tree->model()->rowChanged(clonedRow);
    tree->model()->setSelection(sel);
    tree->update();
    clonedRow = tree->selectedRow();
    if (clonedRow->activateOnAdd())
    {
        PropertyActivationEvent e;
        e.tree = tree;
        e.reason = PropertyActivationEvent::REASON_NEW_ELEMENT;
        clonedRow->onActivate(e);
    }
}

void ContainerMenuHandler::onMenuChildRemove()
{
    tree->model()->rowAboutToBeChanged(container);
    container->erase(element);
    container->setMultiValue(false);
    tree->model()->rowChanged(container);
}


void PropertyRowContainer::labelChanged()
{
    swprintf(buttonLabel_, sizeof(buttonLabel_) / sizeof(buttonLabel_[0]), L"%zi", count());
}

void PropertyRowContainer::serializeValue(IArchive& ar)
{
    ar(ConstStringWrapper(constStrings_, elementTypeName_), "elementTypeName", "ElementTypeName");
    ar(fixedSize_, "fixedSize", "fixedSize");
}

string PropertyRowContainer::valueAsString() const
{
    char buf[32] = { 0 };
    sprintf_s(buf, "%d", (int)children_.size());
    return string(buf);
}

const char* PropertyRowContainer::typeNameForFilter(QPropertyTree* tree) const
{
    const PropertyRow* defaultType = defaultRow(tree->model());
    if (defaultType)
    {
        return defaultType->typeNameForFilter(tree);
    }
    else
    {
        return elementTypeName_;
    }
}

bool PropertyRowContainer::processesKeyContainer([[maybe_unused]] QPropertyTree* tree, const QKeyEvent* ev)
{
    if (ev->key() == Qt::Key_Delete && ev->modifiers() == Qt::SHIFT)
    {
        return true;
    }

    if (ev->key() == Qt::Key_Insert && ev->modifiers() == Qt::NoModifier)
    {
        return true;
    }

    return false;
}

bool PropertyRowContainer::processesKey(QPropertyTree* tree, const QKeyEvent* ev)
{
    if (processesKeyContainer(tree, ev))
    {
        return true;
    }

    return PropertyRow::processesKey(tree, ev);
}

bool PropertyRowContainer::onKeyDownContainer(QPropertyTree* tree, const QKeyEvent* ev)
{
    if (userReadOnly())
    {
        return false;
    }

    std::unique_ptr<ContainerMenuHandler> handler(createMenuHandler(tree, this));
    if (ev->key() == Qt::Key_Delete && ev->modifiers() == Qt::SHIFT)
    {
        handler->onMenuRemoveAll();
        return true;
    }

    if (ev->key() == Qt::Key_Insert && ev->modifiers() == Qt::NoModifier)
    {
        handler->onMenuAppendElement();
        return true;
    }

    return false;
}

bool PropertyRowContainer::onKeyDown(QPropertyTree* tree, const QKeyEvent* ev)
{
    if (onKeyDownContainer(tree, ev))
    {
        return true;
    }
    return PropertyRow::onKeyDown(tree, ev);
}

int PropertyRowContainer::widgetSizeMin(const QPropertyTree* tree) const
{
    return inlined_ ? 0 : (userWidgetSize() >= 0 ? userWidgetSize() : aznumeric_cast<int>(tree->_defaultRowHeight() * 1.7f));
}

#include <QPropertyTree/moc_PropertyRowContainer.cpp>
