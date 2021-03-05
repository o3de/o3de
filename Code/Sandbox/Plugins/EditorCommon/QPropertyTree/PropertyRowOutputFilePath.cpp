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
#include "PropertyRowOutputFilePath.h"
#include "Serialization/ClassFactory.h"
#include <Serialization/Decorators/IconXPM.h>
#ifndef SERIALIZATION_STANDALONE
#include <Include/EditorCoreAPI.h>
#include <Util/PathUtil.h> // for Getting game folder
#endif
#include <QFileDialog>
#include <QIcon>
#include <QMenu>
#include <QKeyEvent>

OutputFilePathMenuHandler::OutputFilePathMenuHandler(QPropertyTree* tree, PropertyRowOutputFilePath* self)
    : self(self)
    , tree(tree)
{
}


void OutputFilePathMenuHandler::onMenuClear()
{
    tree->model()->rowAboutToBeChanged(self);
    self->clear();
    tree->model()->rowChanged(self);
}

QString convertMFCToQtFileFilter(QString* defaultSuffix, const char* mfcFilter)
{
    // convert filter from "All Files|*.*|Text files|*.txt||"
    //         format into "All files (*.*);;Text Files (*.txt)"
    QString filterMFC = QString::fromLocal8Bit(mfcFilter);
    QStringList filterItems = filterMFC.split("|");

    if (defaultSuffix && filterItems.size() > 1)
    {
        QString extensions = filterItems[1];
        QRegExp re("\\*\\.(\\w*)");
        if (extensions.indexOf(re) >= 0)
        {
            *defaultSuffix = re.cap(1);
        }
    }

    QString filter;
    for (int i = 0; i < int(filterItems.size()) / 2; ++i)
    {
        int bracketPos = filterItems[i].indexOf('(');
        QString desc = bracketPos >= 0 ? filterItems[i].left(bracketPos) : filterItems[i];
        int extIndex = i * 2 + 1;
        if (extIndex >= filterItems.size())
        {
            break;
        }
        if (!filter.isEmpty())
        {
            filter += ";;";
        }
        filter += desc;
        filter += " (";
        filter += filterItems[extIndex];
        filter += ")";
    }

    return filter;
}

bool PropertyRowOutputFilePath::onActivate(const PropertyActivationEvent& e)
{
    if (e.reason == e.REASON_RELEASE)
    {
        return false;
    }
#ifndef SERIALIZATION_STANDALONE
    if (!GetIEditor())
    {
        return true;
    }
#endif

    QString title;
    if (labelUndecorated())
    {
        title = QString("Choose file for '") + labelUndecorated() + "'";
    }
    else
    {
        title = "Choose file";
    }

#ifdef SERIALIZATION_STANDALONE
    QString gameFolder;
#else
    QString gameFolder = QString::fromLocal8Bit(Path::GetEditingGameDataFolder().c_str());
#endif
    QDir gameFolderDir(QDir::fromNativeSeparators(gameFolder));
    QString defaultSuffix;
    QString filter = convertMFCToQtFileFilter(&defaultSuffix, filter_.c_str());
    QString existingFile = QString::fromLocal8Bit(path_.c_str());

    QString existingFilePath = (existingFile.isEmpty() || QDir::isAbsolutePath(existingFile)) ? existingFile : gameFolderDir.absoluteFilePath(existingFile);
    QString startFolder = QString::fromLocal8Bit(startFolder_.c_str());

    // Not using QFileDialog().exec() as it implements custom file dialog that
    // freezes for couple of seconds when being open. Scannign network drives?
    QString result = QFileDialog::getSaveFileName(e.tree, title, existingFilePath.isEmpty() ? (gameFolder + "/" + startFolder) : existingFilePath, filter);
    if (!result.isEmpty())
    {
        e.tree->model()->rowAboutToBeChanged(this);
        QString relativeFilename = gameFolderDir.relativeFilePath(result);
        path_ = relativeFilename.toLocal8Bit().data();
        e.tree->model()->rowChanged(this);
    }
    return true;
}
void PropertyRowOutputFilePath::setValueAndContext(const Serialization::SStruct& ser, [[maybe_unused]] Serialization::IArchive& ar)
{
    OutputFilePath* value = (OutputFilePath*)ser.pointer();
    path_ = value->m_path->c_str();
    filter_ = value->filter.c_str();
    startFolder_ = value->startFolder.c_str();
    handle_ = value->m_path;
}

bool PropertyRowOutputFilePath::assignTo(const Serialization::SStruct& ser) const
{
    ((OutputFilePath*)ser.pointer())->SetPath(path_.c_str());
    return true;
}

const QIcon& PropertyRowOutputFilePath::buttonIcon(const QPropertyTree* tree, [[maybe_unused]] int index) const
{
    #include "file_save.xpm"
    static QIcon fileOpenIcon = QIcon(QPixmap::fromImage(*tree->_iconCache()->getImageForIcon(Serialization::IconXPM(file_save_xpm))));
    return fileOpenIcon;
}

string PropertyRowOutputFilePath::valueAsString() const
{
    return path_;
}


void PropertyRowOutputFilePath::clear()
{
    path_.clear();
}

bool PropertyRowOutputFilePath::onContextMenu(QMenu& menu, QPropertyTree* tree)
{
    QAction* action = menu.addAction("Clear");
    Serialization::SharedPtr<PropertyRow> selfPointer(this);

    OutputFilePathMenuHandler* handler = new OutputFilePathMenuHandler(tree, this);
    QObject::connect(action, SIGNAL(triggered()), handler, SLOT(onMenuClear()));
    tree->addMenuHandler(handler);
    return true;
}

void PropertyRowOutputFilePath::serializeValue(Serialization::IArchive& ar)
{
    ar(path_, "path");
    ar(filter_, "filter");
    ar(startFolder_, "startFolder");
}

bool PropertyRowOutputFilePath::processesKey(QPropertyTree* tree, const QKeyEvent* ev)
{
    if (ev->key() == Qt::Key_Delete)
    {
        return true;
    }

    return PropertyRowField::processesKey(tree, ev);
}

bool PropertyRowOutputFilePath::onKeyDown(QPropertyTree* tree, const QKeyEvent* ev)
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

REGISTER_PROPERTY_ROW(OutputFilePath, PropertyRowOutputFilePath);
DECLARE_SEGMENT(PropertyRowOutputFilePath)

#include <QPropertyTree/moc_PropertyRowOutputFilePath.cpp>
