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
#include "PropertyRowResourceSelector.h"
#include "Serialization/ClassFactory.h"
#include "Serialization/Decorators/Resources.h"
#include "Serialization/Decorators/INavigationProvider.h"
#include "Serialization/Decorators/IconXPM.h"
#include <Include/EditorCoreAPI.h>
#include "IEditor.h"
#include <QMenu>
#include <QFileDialog>
#include <QIcon>
#include <QKeyEvent>
#include <Util/PathUtil.h>

enum Button
{
    BUTTON_PICK,
    BUTTON_CREATE
};

ResourceSelectorMenuHandler::ResourceSelectorMenuHandler(QPropertyTree* tree, PropertyRowResourceSelector* self)
    : self(self)
    , tree(tree)
{
}


void ResourceSelectorMenuHandler::onMenuClear()
{
    tree->model()->rowAboutToBeChanged(self);
    self->clear();
    tree->model()->rowChanged(self);
}

void ResourceSelectorMenuHandler::onMenuPickResource()
{
    self->pickResource(tree);
}

void ResourceSelectorMenuHandler::onMenuCreateFile()
{
    self->createFile(tree);
}

void ResourceSelectorMenuHandler::onMenuJumpTo()
{
    self->jumpTo(tree);
}

bool PropertyRowResourceSelector::onActivate(const PropertyActivationEvent& e)
{
    if (PropertyRowField::onActivate(e))
    {
        return true;
    }

    bool canSelect = !userReadOnly() && !multiValue() && provider_ && provider_->CanSelect(type_.c_str(), value_.c_str(), id_);

    if (!userReadOnly() && e.reason == e.REASON_DOUBLECLICK && provider_ && provider_->CanPickFile(type_.c_str(), id_))
    {
        return pickResource(e.tree);
    }

    if (canSelect)
    {
        jumpTo(e.tree);
        return true;
    }
    else if (!userReadOnly())
    {
        if (!provider_ && !userReadOnly())
        {
            pickResource(e.tree);
        }
    }
    return false;
}

int PropertyRowResourceSelector::buttonCount() const
{
    if (!provider_)
    {
        return 1;
    }

    int result = 0;
    if (provider_->CanPickFile(type_.c_str(), id_))
    {
        result = 1;
        if (!multiValue() && value_.empty() && provider_->CanCreate(type_.c_str(), id_))
        {
            result = 2;
        }
    }
    return result;
}

bool PropertyRowResourceSelector::onActivateButton(int button, const PropertyActivationEvent& e)
{
    if (userReadOnly())
    {
        return false;
    }
    if (button == BUTTON_PICK)
    {
        return pickResource(e.tree);
    }
    else if (button == BUTTON_CREATE)
    {
        return createFile(e.tree);
    }
    return true;
}

bool PropertyRowResourceSelector::getHoverInfo(PropertyHoverInfo* hover, const QPoint& cursorPos, const QPropertyTree* tree) const
{
    if (fieldRect(tree).contains(cursorPos) && provider_ && provider_->CanSelect(type_.c_str(), value_.c_str(), id_) && !provider_->IsSelected(type_.c_str(), value_.c_str(), id_))
    {
        hover->cursor = QCursor(Qt::PointingHandCursor);
    }
    else
    {
        hover->cursor = QCursor();
    }
    hover->toolTip = QString::fromLocal8Bit(value_.c_str());
    return true;
}

void PropertyRowResourceSelector::jumpTo([[maybe_unused]] QPropertyTree* tree)
{
    if (multiValue())
    {
        return;
    }
    if (provider_)
    {
        provider_->Select(type_.c_str(), value_.c_str(), id_);
    }
    return;
}

bool PropertyRowResourceSelector::pickResource(QPropertyTree* tree)
{
    if (!GetIEditor())
    {
        return false;
    }

    context_.typeName = type_.c_str();
    context_.parentWidget = tree;
    QString filename = GetIEditor()->GetResourceSelectorHost()->SelectResource(context_, value_.c_str());

    tree->model()->rowAboutToBeChanged(this);
    value_ = filename.toUtf8().constData();
    tree->model()->rowChanged(this);

    return true;
}

QString convertMFCToQtFileFilter(QString* defaultSuffix, const char* mfcFilter);

bool PropertyRowResourceSelector::createFile(QPropertyTree* tree)
{
    if (!provider_)
    {
        return false;
    }

    QString title;
    if (labelUndecorated())
    {
        title = QString("Create file for '") + labelUndecorated() + "'";
    }
    else
    {
        title = "Choose file";
    }

    string originalFilter;
    originalFilter = provider_->GetFileSelectorMaskForType(type_.c_str());

    QString gameFolder = QString::fromLocal8Bit(Path::GetEditingGameDataFolder().c_str());
    QDir gameFolderDir(QDir::fromNativeSeparators(gameFolder));
    QString defaultSuffix;
    QString filter = convertMFCToQtFileFilter(&defaultSuffix, originalFilter.c_str());
    QString existingFile = QString::fromLocal8Bit(PathUtil::ReplaceExtension(defaultPath_.empty() ? value_.c_str() : defaultPath_.c_str(), defaultSuffix.toLocal8Bit().data()));

    QString existingFilePath = (existingFile.isEmpty() || QDir::isAbsolutePath(existingFile)) ? existingFile : gameFolderDir.absoluteFilePath(existingFile);

    // Not using QFileDialog().exec() as it implements custom file dialog that
    // freezes for couple of seconds when being open. Scannign network drives?
    QString result = QFileDialog::getSaveFileName(tree, title, existingFilePath.isEmpty() ? (gameFolder + "/") : existingFilePath, filter);
    if (!result.isEmpty())
    {
        QString relativeFilename = gameFolderDir.relativeFilePath(result);

        if (provider_->Create(type_.c_str(), relativeFilename.toLocal8Bit().data(), id_))
        {
            tree->model()->rowAboutToBeChanged(this);
            value_ = relativeFilename.toLocal8Bit().data();
            tree->model()->rowChanged(this);
        }
    }
    return true;
}

void PropertyRowResourceSelector::setValueAndContext(const Serialization::SStruct& ser, IArchive& ar)
{
    IResourceSelector* value = (IResourceSelector*)ser.pointer();
    if (type_ != value->resourceType)
    {
        type_ = value->resourceType;
        const char* resourceIconPath = GetIEditor()->GetResourceSelectorHost()->ResourceIconPath(type_.c_str());
        icon_ = resourceIconPath[0] ? QIcon(QString::fromLocal8Bit(resourceIconPath)) : QIcon();
    }
    value_ = value->GetValue();
    id_ = value->GetId();
    searchHandle_ = value->GetHandle();
    wrappedType_ = value->GetType();

    provider_ = ar.FindContext<Serialization::INavigationProvider>();
    if (!provider_ || !provider_->IsRegistered(type_))
    {
        provider_ = 0;
    }

    Serialization::TypeID contextObjectType = GetIEditor()->GetResourceSelectorHost()->ResourceContextType(type_.c_str());
    if (contextObjectType != Serialization::TypeID())
    {
        context_.contextObject = ar.FindContextByType(contextObjectType);
        context_.contextObjectType = contextObjectType;
    }

    if (Serialization::SNavigationContext* navigationContext = ar.FindContext<Serialization::SNavigationContext>())
    {
        defaultPath_ = navigationContext->path.c_str();
    }
    else
    {
        defaultPath_.clear();
    }
}

bool PropertyRowResourceSelector::assignTo(const Serialization::SStruct& ser) const
{
    ((IResourceSelector*)ser.pointer())->SetValue(value_.c_str());
    return true;
}

void PropertyRowResourceSelector::serializeValue(Serialization::IArchive& ar)
{
    ar(type_, "type");
    ar(value_, "value");
    ar(id_, "index");

    if (ar.IsInput())
    {
        const char* resourceIconPath = GetIEditor()->GetResourceSelectorHost()->ResourceIconPath(type_.c_str());
        icon_ = resourceIconPath[0] ? QIcon(QString::fromLocal8Bit(resourceIconPath)) : QIcon();
    }
}

const QIcon& PropertyRowResourceSelector::buttonIcon(const QPropertyTree* tree, int index) const
{
    switch (index)
    {
    case BUTTON_PICK:
    {
        if (provider_ != 0 || icon_.isNull())
        {
            #include "file_open.xpm"
            static QIcon defaultIcon(QPixmap::fromImage(*tree->_iconCache()->getImageForIcon(Serialization::IconXPM(file_open_xpm))));
            return defaultIcon;
        }
        else
        {
            return icon_;
        }
    }
    case BUTTON_CREATE:
    {
        static QIcon addIcon("Editor/Icons/animation/add.png");
        ;
        return addIcon;
    }
    default:
    {
        static QIcon defaultIcon;
        return defaultIcon;
    }
    }
}

string PropertyRowResourceSelector::valueAsString() const
{
    return value_;
}

void PropertyRowResourceSelector::clear()
{
    value_.clear();
}

bool PropertyRowResourceSelector::onContextMenu(QMenu& menu, QPropertyTree* tree)
{
    Serialization::SharedPtr<PropertyRow> selfPointer(this);

    ResourceSelectorMenuHandler* handler = new ResourceSelectorMenuHandler(tree, this);
    if (!multiValue() && provider_ && provider_->CanSelect(type_.c_str(), value_.c_str(), id_))
    {
        QAction* jumpToAction = menu.addAction("Jump to", handler, SLOT(onMenuJumpTo()));
        menu.setDefaultAction(jumpToAction);
    }
    if (!userReadOnly())
    {
        if (!provider_ || provider_->CanPickFile(type_.c_str(), id_))
        {
            menu.addAction(buttonIcon(tree, 0), "Pick Resource...", handler, SLOT(onMenuPickResource()))->setEnabled(!userReadOnly());
        }
        if (provider_ && provider_->CanCreate(type_.c_str(), id_))
        {
            menu.addAction(buttonIcon(tree, 1), "Create...", handler, SLOT(onMenuCreateFile()));
        }
        menu.addAction("Clear", handler, SLOT(onMenuClear()))->setEnabled(!userReadOnly());
    }
    tree->addMenuHandler(handler);

    PropertyRow::onContextMenu(menu, tree);
    return true;
}

static const wchar_t* getFilenameFromPath(const wchar_t* path)
{
    const wchar_t* lastSep = wcsrchr(path, L'/');
    if (!lastSep)
    {
        return path;
    }
    return lastSep + 1;
}

void PropertyRowResourceSelector::redraw(const PropertyDrawContext& context)
{
    ////! TOFIX: THIS CODE IS DUPLICATED IN PropertyRowField.cpp: PropertyRowField::redraw()!!!
    //
    int buttonCount = this->buttonCount();
    int offset = 0;
    for (int i = 0; i < buttonCount; ++i)
    {
        const QIcon& icon = buttonIcon(context.tree, i);
        int width = 16;
        QRect iconRect(context.widgetRect.right() - offset - width, context.widgetRect.top(), width, context.widgetRect.height());
        icon.paint(context.painter, iconRect, Qt::AlignCenter, userReadOnly() ? QIcon::Disabled : QIcon::Normal);
        offset += width;
    }

    int iconSpace = offset ? offset + 2 : 0;
    //
    ////

    QRect rect = context.widgetRect;
    rect.setRight(rect.right() - iconSpace);
    bool pressed = context.m_pressed || (provider_ ? provider_->IsSelected(type_.c_str(), value_.c_str(), id_) : false);
    bool active = !provider_ || provider_->IsActive(type_.c_str(), value_.c_str(), id_);
    bool modified = provider_ && provider_->IsModified(type_.c_str(), value_.c_str(), id_);
    QIcon icon = icon_;
    if (provider_)
    {
        icon = QIcon(provider_->GetIcon(type_.c_str(), value_.c_str()));
    }
    bool canSelect = !multiValue() && provider_ && provider_->CanSelect(type_.c_str(), value_.c_str(), id_);

    wstring text = multiValue() ? L"..." : wstring(modified ? L"*" : L"") + getFilenameFromPath(valueAsWString());
    if (provider_)
    {
        if (canSelect || !text.empty())
        {
            context.drawButtonWithIcon(icon, rect, text.c_str(), selected(), pressed, selected(), !userReadOnly(), canSelect, active ? &context.tree->_boldFont() : &context.tree->font());
        }
    }
    else
    {
        context.drawEntry(text.c_str(), true, false, iconSpace);
    }
}

bool PropertyRowResourceSelector::processesKey(QPropertyTree* tree, const QKeyEvent* ev)
{
    if (ev->key() == Qt::Key_Delete)
    {
        return true;
    }

    return PropertyRowField::processesKey(tree, ev);
}

bool PropertyRowResourceSelector::onKeyDown(QPropertyTree* tree, const QKeyEvent* ev)
{
    if (ev->key() == Qt::Key_Delete)
    {
        tree->model()->rowAboutToBeChanged(this);
        clear();
        tree->model()->rowChanged(this);
        return true;
    }
    return PropertyRowField::onKeyDown(tree, ev);
}

REGISTER_PROPERTY_ROW(IResourceSelector, PropertyRowResourceSelector);
DECLARE_SEGMENT(PropertyRowResourceSelector)

#include <QPropertyTree/moc_PropertyRowResourceSelector.cpp>
