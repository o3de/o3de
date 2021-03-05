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
#include "QPropertyDialog.h"
#include "QPropertyTree.h"
#include "Serialization/IArchive.h"
#include "Serialization/BinArchive.h"
#include "Serialization/JSONIArchive.h"
#include "Serialization/JSONOArchive.h"
#include <QBoxLayout>
#include <QDialogButtonBox>
#include <QDir>

#include <CryPath.h>

#ifndef SERIALIZATION_STANDALONE
#include <CryFile.h>
#include <Include/EditorCoreAPI.h>
#else
namespace PathUtil
{
    string GetParentDirectory(const char* path)
    {
        const char* end = strrchr(path, '/');
        if (!end)
        {
            end = strrchr(path, '\\');
        }
        if (end)
        {
            return string(path, end);
        }
        else
        {
            return string();
        }
    }
};
#endif

#ifndef SERIALIZATION_STANDALONE
#include <IEditor.h>
#endif

static string getFullStateFilename(const char* filename)
{
#ifdef SERIALIZATION_STANDALONE
    // use current folder
    return filename;
#else
    string path = GetIEditor()->GetResolvedUserFolder().toUtf8().data();
    if (!path.empty() && path[path.size() - 1] != '\\' && path[path.size() - 1] != '/')
    {
        path.push_back('\\');
    }
    path += filename;
    return path;
#endif
}

bool QPropertyDialog::edit(Serialization::SStruct& ser, const char* title, const char* windowStateFilename, QWidget* parent)
{
    QPropertyDialog dialog(parent);
    dialog.setSerializer(ser);
    dialog.setWindowTitle(QString::fromLocal8Bit(title));
    dialog.setWindowStateFilename(windowStateFilename);

    return dialog.exec() == QDialog::Accepted;
}

QPropertyDialog::QPropertyDialog(QWidget* parent)
    : QDialog(parent)
    , m_sizeHint(440, 500)
    , m_layout(0)
    , m_storeContent(false)
{
    connect(this, SIGNAL(accepted()), this, SLOT(onAccepted()));
    connect(this, SIGNAL(rejected()), this, SLOT(onRejected()));
    setModal(true);
    setWindowModality(Qt::ApplicationModal);

    m_propertyTree = new QPropertyTree(this);
    m_propertyTree->setExpandLevels(1);

    m_layout = new QBoxLayout(QBoxLayout::TopToBottom, this);

    m_layout->addWidget(m_propertyTree, 1);
    QDialogButtonBox* buttons = new QDialogButtonBox(this);
    buttons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
    m_layout->addWidget(buttons, 0);
}

QPropertyDialog::~QPropertyDialog()
{
}

void QPropertyDialog::revert()
{
    if (m_propertyTree)
    {
        m_propertyTree->revert();
    }
}

void QPropertyDialog::setSerializer(const Serialization::SStruct& ser)
{
    if (!m_serializer)
    {
        m_serializer.reset(new Serialization::SStruct());
    }
    *m_serializer = ser;
}

void QPropertyDialog::setWindowStateFilename(const char* windowStateFilename)
{
    m_windowStateFilename = windowStateFilename;
}

void QPropertyDialog::setSizeHint(const QSize& size)
{
    m_sizeHint = size;
}

void QPropertyDialog::setStoreContent(bool storeContent)
{
    m_storeContent = storeContent;
}

QSize QPropertyDialog::sizeHint() const
{
    return m_sizeHint;
}

void QPropertyDialog::setVisible(bool visible)
{
    QDialog::setVisible(visible);

    if (visible)
    {
        string fullStateFilename = getFullStateFilename(m_windowStateFilename.c_str());
        if (!fullStateFilename.empty())
        {
            Serialization::JSONIArchive ia;
            if (ia.load(fullStateFilename.c_str()))
            {
                ia(*this);
            }
        }

        m_backup.reset(new Serialization::BinOArchive());
        if (m_serializer && *m_serializer)
        {
            const Serialization::SStruct& ser = *m_serializer;
            (*m_backup)(ser, "backup");
            m_propertyTree->attach(*m_serializer);
        }
    }
}

void QPropertyDialog::onAccepted()
{
    string fullStateFilename = getFullStateFilename(m_windowStateFilename.c_str());
    if (!fullStateFilename.empty())
    {
        Serialization::JSONOArchive oa;
        oa(*this);

        QDir().mkdir(QString::fromLocal8Bit(PathUtil::GetParentDirectory(fullStateFilename.c_str()).c_str()));
        oa.save(fullStateFilename.c_str());
    }
}

void QPropertyDialog::onRejected()
{
    if (m_backup.get() && m_serializer.get() && *m_serializer)
    {
        // restore previous object state
        Serialization::BinIArchive ia;
        if (ia.open(m_backup->buffer(), m_backup->length()))
        {
            const Serialization::SStruct& ser = *m_serializer;
            ia(ser, "backup");
        }
    }
}

void QPropertyDialog::setArchiveContext(Serialization::SContextLink* context)
{
    m_propertyTree->setArchiveContext(context);
}

void QPropertyDialog::Serialize(Serialization::IArchive& ar)
{
    if (m_storeContent && m_serializer.get())
    {
        ar(*m_serializer, "content");
    }

    QByteArray geometry;
    if (ar.IsOutput())
    {
        geometry = saveGeometry();
    }
    std::vector<char> geometryVec(geometry.begin(), geometry.end());
    ar(geometryVec, "geometry");
    if (ar.IsInput() && !geometryVec.empty())
    {
        restoreGeometry(QByteArray(geometryVec.data(), (int)geometryVec.size()));
    }

    ar(*m_propertyTree, "propertyTree");
}

#include <QPropertyTree/moc_QPropertyDialog.cpp>
