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

#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_QPROPERTYDIALOG_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_QPROPERTYDIALOG_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "../EditorCommonAPI.h"
#include "Strings.h"
#include <memory>

#include <QDialog>
#endif

namespace Serialization
{
    struct SStruct;
    struct SContextLink;
    class BinOArchive;
    class IArchive;
}

class QPropertyTree;
class QBoxLayout;

class EDITOR_COMMON_API QPropertyDialog
    : public QDialog
{
    Q_OBJECT
public:
    static bool edit(Serialization::SStruct& ser, const char* title, const char* windowStateFilename, QWidget* parent);

    QPropertyDialog(QWidget* parent);
    ~QPropertyDialog();

    void setSerializer(const Serialization::SStruct& ser);
    void setArchiveContext(Serialization::SContextLink* context);
    void setWindowStateFilename(const char* windowStateFilename);
    void setSizeHint(const QSize& sizeHint);
    void setStoreContent(bool storeContent);

    void revert();
    QBoxLayout* layout() { return m_layout; }

    void Serialize(Serialization::IArchive& ar);
protected slots:
    void onAccepted();
    void onRejected();

protected:
    QSize sizeHint() const override;
    void setVisible(bool visible) override;
private:
    QPropertyTree* m_propertyTree;
    QBoxLayout* m_layout;
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    std::unique_ptr<Serialization::SStruct> m_serializer;
    std::unique_ptr<Serialization::BinOArchive> m_backup;
    string m_windowStateFilename;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QSize m_sizeHint;
    bool m_storeContent;
};

#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_QPROPERTYDIALOG_H
