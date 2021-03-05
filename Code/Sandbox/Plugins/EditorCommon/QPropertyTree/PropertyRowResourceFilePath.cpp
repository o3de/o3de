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

#include <AzCore/IO/FileIO.h>

#include "PropertyRowResourceFilePath.h"
#include "Serialization/ClassFactory.h"
#include <Include/EditorCoreAPI.h>
#include <QMenu>
#include <QFileDialog>
#include <QIcon>
#include <QKeyEvent>
#include <Util/PathUtil.h> // for getting game folder.

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>

ResourceFilePathMenuHandler::ResourceFilePathMenuHandler(QPropertyTree* tree, PropertyRowResourceFilePath* self)
    : self(self)
    , tree(tree)
{
}


void ResourceFilePathMenuHandler::onMenuClear()
{
    tree->model()->rowAboutToBeChanged(self);
    self->clear();
    tree->model()->rowChanged(self);
}

// Get filename relative to the asset folder,
// whether it came from the project or from a gem
QString AssetRelativePathFromAbsolutePath(const QString& absPath)
{
    return Path::FullPathToGamePath(absPath);
}

bool PropertyRowResourceFilePath::onActivate(const PropertyActivationEvent& e)
{
    using namespace AzToolsFramework::AssetBrowser;

    if (e.reason == e.REASON_RELEASE)
    {
        return false;
    }

    AssetSelectionModel selection;
    if (m_group)
    {
        selection = AssetSelectionModel::AssetGroupSelection(filter_);
    }
    else
    {
        selection = AssetSelectionModel::AssetTypeSelection(filter_);
    }

    AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);
    if (!selection.IsValid())
    {
        return true;
    }
    
    auto product = azrtti_cast<const ProductAssetBrowserEntry*>(selection.GetResult());
    if (!product)
    {
        return true;
    }

    AZStd::string relativeFilename = product->GetRelativePath();

    if (flags_ & ResourceFilePath::STRIP_EXTENSION)
    {
        size_t ext = relativeFilename.rfind('.');
        if (ext != relativeFilename.npos)
        {
            relativeFilename.erase(ext, relativeFilename.length() - ext);
        }
    }

    e.tree->model()->rowAboutToBeChanged(this);
    path_ = relativeFilename.c_str();
    e.tree->model()->rowChanged(this);
    return true;
}
void PropertyRowResourceFilePath::setValueAndContext(const Serialization::SStruct& ser, [[maybe_unused]] Serialization::IArchive& ar)
{
    ResourceFilePath* value = (ResourceFilePath*)ser.pointer();
    filter_ = value->filter.c_str();
    path_ = value->m_path->c_str();
    flags_ = value->flags;
    handle_ = value->m_path;
    m_group = value->group;
}

bool PropertyRowResourceFilePath::assignTo(const Serialization::SStruct& ser) const
{
    ((ResourceFilePath*)ser.pointer())->SetPath(path_.c_str());
    return true;
}

void PropertyRowResourceFilePath::serializeValue(Serialization::IArchive& ar)
{
    ar(filter_, "filter");
    ar(path_, "path");
    ar(startFolder_, "startFolder");
    ar(m_group, "group");
}

const QIcon& PropertyRowResourceFilePath::buttonIcon(const QPropertyTree* tree, [[maybe_unused]] int index) const
{
  #include "file_open.xpm"
    static QIcon fileOpenIcon = QIcon(QPixmap::fromImage(*tree->_iconCache()->getImageForIcon(Serialization::IconXPM(file_open_xpm))));
    return fileOpenIcon;
}

string PropertyRowResourceFilePath::valueAsString() const
{
    return path_;
}


void PropertyRowResourceFilePath::clear()
{
    path_.clear();
}

bool PropertyRowResourceFilePath::onContextMenu(QMenu& menu, QPropertyTree* tree)
{
    QAction* action = menu.addAction("Clear");
    Serialization::SharedPtr<PropertyRow> selfPointer(this);

    ResourceFilePathMenuHandler* handler = new ResourceFilePathMenuHandler(tree, this);
    QObject::connect(action, SIGNAL(triggered()), handler, SLOT(onMenuClear()));
    tree->addMenuHandler(handler);
    return true;
}

bool PropertyRowResourceFilePath::processesKey(QPropertyTree* tree, const QKeyEvent* ev)
{
    if (ev->key() == Qt::Key_Delete)
    {
        return true;
    }

    return PropertyRowField::processesKey(tree, ev);
}

bool PropertyRowResourceFilePath::onKeyDown(QPropertyTree* tree, const QKeyEvent* ev)
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

REGISTER_PROPERTY_ROW(ResourceFilePath, PropertyRowResourceFilePath);
DECLARE_SEGMENT(PropertyRowResourceFilePath)

#include <QPropertyTree/moc_PropertyRowResourceFilePath.cpp>
