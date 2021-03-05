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
#include "PropertyRowPointer.h"
#include "QPropertyTree.h"
#include "PropertyTreeModel.h"
#include "PropertyDrawContext.h"
#include "Serialization.h"
#include "Unicode.h"
#include <QMenu>


// ---------------------------------------------------------------------------

void ClassMenuItemAdder::generateMenu(QMenu& createItem, const StringList& comboStrings)
{
    StringList::const_iterator it;
    int index = 0;
    for (it = comboStrings.begin(); it != comboStrings.end(); ++it)
    {
        StringList path;
        splitStringList(&path, it->c_str(), '\\');
        int level = 0;
        QMenu* item = &createItem;
        //createItem.addMenu(
        for (int level2 = 0; level2 < int(path.size()); ++level2)
        {
            const char* leaf = path[level2].c_str();
            if (level2 == path.size() - 1)
            {
                addAction(*item, leaf, index++);
            }
            else
            {
                if (QMenu* menu = item->findChild<QMenu*>(leaf))
                {
                    item = menu;
                }
                else
                {
                    item = addMenu(*item, leaf); //&item->add(leaf);
                }
            }
        }
    }
}

void ClassMenuItemAdder::addAction(QMenu& menu, const char* text, [[maybe_unused]] int index)
{
    menu.addAction(text)->setEnabled(false);
}

QMenu* ClassMenuItemAdder::addMenu(QMenu& menu, const char* text)
{
    QMenu* result = menu.addMenu(text);
    result->setObjectName(text);
    return result;
}


// ---------------------------------------------------------------------------

SERIALIZATION_CLASS_NAME(PropertyRow, PropertyRowPointer, "PropertyRowPointer", "SharedPtr");

PropertyRowPointer::PropertyRowPointer()
    : factory_(0)
    , searchHandle_(0)
    , colorOverride_(0, 0, 0, 0)
{
}

void PropertyRowPointer::setDerivedType(const char* typeName, Serialization::IClassFactory* factory)
{
    if (!factory)
    {
        derivedTypeName_.clear();
        return;
    }
    derivedTypeName_ = typeName;
}

bool PropertyRowPointer::assignTo(Serialization::IPointer& ptr)
{
    if (derivedTypeName_ != ptr.registeredTypeName())
    {
        ptr.create(derivedTypeName_.c_str());
    }

    return true;
}


void CreatePointerMenuHandler::onMenuCreateByIndex()
{
    tree->model()->rowAboutToBeChanged(row);
    if (index < 0) // NULL value
    {
        row->setDerivedType("", 0);
        row->clear();
    }
    else
    {
        const PropertyDefaultDerivedTypeValue* defaultValue = tree->model()->defaultType(row->baseType(), index);
        SharedPtr<PropertyRow> clonedDefault = defaultValue->root->clone(tree->model()->constStrings());
        if (defaultValue && defaultValue->root)
        {
            YASLI_ASSERT(defaultValue->root->refCount() == 1);
            if (useDefaultValue)
            {
                row->clear();
                row->swapChildren(clonedDefault, 0);
            }
            row->setDerivedType(defaultValue->registeredName.c_str(), row->factory());
            row->setLabelChanged();
            row->setLabelChangedToChildren();
            tree->expandRow(row);
        }
        else
        {
            row->setDerivedType("", 0);
            row->clear();
        }
    }
    tree->model()->rowChanged(row);
}


string PropertyRowPointer::valueAsString() const
{
    string result;
    const Serialization::TypeDescription* desc = 0;
    if (factory_)
    {
        desc = factory_->descriptionByRegisteredName(derivedTypeName_.c_str());
    }
    if (desc)
    {
        result = desc->label();
    }
    else
    {
        result = derivedTypeName_;
    }

    return result;
}

wstring PropertyRowPointer::generateLabel() const
{
    if (multiValue())
    {
        return L"...";
    }

    wstring str;
    if (!derivedTypeName_.empty())
    {
        const char* textStart = derivedTypeName_.c_str();
        if (factory_)
        {
            const Serialization::TypeDescription* desc = factory_->descriptionByRegisteredName(derivedTypeName_.c_str());

            if (desc)
            {
                textStart = desc->label();
            }
        }
        const char* p = textStart + strlen(textStart);
        while (p > textStart)
        {
            if (*(p - 1) == '\\')
            {
                break;
            }
            --p;
        }
        str = toWideChar(p);
        if (p != textStart)
        {
            str += L" (";
            str += toWideChar(string(textStart, p - 1).c_str());
            str += L")";
        }
    }
    else
    {
        if (factory_)
        {
            str = toWideChar(factory_->nullLabel() ? factory_->nullLabel() : "[ null ]");
        }
        else
        {
            str = L"[ null ]";
        }
    }
    return str;
}

void PropertyRowPointer::redraw(const PropertyDrawContext& context)
{
    QRect widgetRect = context.widgetRect;
    QRect rt = widgetRect;
    rt.adjust(-1, 0, 0, 1);
    wstring str = generateLabel();
    const QFont* font = derivedTypeName_.empty() ? &context.tree->font() : &context.tree->_boldFont();
    int buttonFlags = BUTTON_POPUP_ARROW;
    if (userReadOnly())
    {
        buttonFlags |= BUTTON_DISABLED;
    }
    if (context.m_pressed)
    {
        buttonFlags |= BUTTON_PRESSED;
    }
    context.drawButton(rt, str.c_str(), buttonFlags, font, colorOverride_.a != 0 ? &colorOverride_ : 0);
}

struct ClassMenuItemAdderRowPointer
    : ClassMenuItemAdder
{
    ClassMenuItemAdderRowPointer(PropertyRowPointer* row, QPropertyTree* tree)
        : row_(row)
        , tree_(tree) {}
    void addAction(QMenu& menu, const char* text, int index)
    {
        CreatePointerMenuHandler* handler = new CreatePointerMenuHandler;
        tree_->addMenuHandler(handler);
        handler->row = row_;
        handler->tree = tree_;
        handler->index = index;
        handler->useDefaultValue = !tree_->immediateUpdate();

        QAction* action = menu.addAction(text);

        QObject::connect(action, SIGNAL(triggered()), handler, SLOT(onMenuCreateByIndex()));
    }
protected:
    PropertyRowPointer* row_;
    QPropertyTree* tree_;
};


bool PropertyRowPointer::onActivate(QPropertyTree* tree, [[maybe_unused]] bool force)
{
    if (userReadOnly())
    {
        return false;
    }
    QMenu menu;
    ClassMenuItemAdderRowPointer(this, tree).generateMenu(menu, tree->model()->typeStringList(baseType()));
    tree->_setPressedRow(this);
    menu.exec(tree->_toScreen(QPoint(widgetPos_, pos_.y() + tree->_defaultRowHeight())));
    tree->_setPressedRow(0);
    return true;
}

bool PropertyRowPointer::onMouseDown(QPropertyTree* tree, QPoint point, bool& changed)
{
    if (widgetRect(tree).contains(point))
    {
        if (onActivate(tree, false))
        {
            changed = true;
        }
    }
    return false;
}

bool PropertyRowPointer::onContextMenu(QMenu& menu, QPropertyTree* tree)
{
    if (!menu.isEmpty())
    {
        menu.addSeparator();
    }
    if (!userReadOnly())
    {
        QMenu* createItem = menu.addMenu("Set");
        ClassMenuItemAdderRowPointer(this, tree).generateMenu(*createItem, tree->model()->typeStringList(baseType()));
    }
    return PropertyRow::onContextMenu(menu, tree);
}

void PropertyRowPointer::serializeValue(IArchive& ar)
{
    ar(derivedTypeName_, "derivedTypeName", "Derived Type Name");
}

int PropertyRowPointer::widgetSizeMin(const QPropertyTree* tree) const
{
    QFontMetrics fm(tree->_boldFont());
    QString str(fromWideChar(generateLabel().c_str()).c_str());
    return fm.horizontalAdvance(str) + 24;
}

static Color parseColorString(const char* str)
{
    unsigned int color = 0;
    if (azsscanf(str, "%x", &color) != 1)
    {
        return Color(0, 0, 0, 0);
    }
    Color result((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff, 255);
    return result;
}

void PropertyRowPointer::setValueAndContext(const Serialization::IPointer& ptr, [[maybe_unused]] Serialization::IArchive& ar)
{
    baseType_ = ptr.baseType();
    factory_ = ptr.factory();
    serializer_ = ptr.serializer();
    pointerType_ = ptr.pointerType();
    searchHandle_ = ptr.handle();

    const char* colorString = factory_->findAnnotation(ptr.registeredTypeName(), "color");
    if (colorString[0] != '\0')
    {
        colorOverride_ = parseColorString(colorString);
    }
    else
    {
        colorOverride_ = Color(0, 0, 0, 0);
    }

    const Serialization::TypeDescription* desc = factory_->descriptionByRegisteredName(ptr.registeredTypeName());
    if (desc)
    {
        derivedTypeName_ = desc->name();
    }
    else
    {
        derivedTypeName_.clear();
    }
}

#include <QPropertyTree/moc_PropertyRowPointer.cpp>
// vim:ts=4 sw=4:
