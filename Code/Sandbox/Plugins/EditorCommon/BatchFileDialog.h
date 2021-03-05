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

#ifndef CRYINCLUDE_EDITORCOMMON_BATCHFILEDIALOG_H
#define CRYINCLUDE_EDITORCOMMON_BATCHFILEDIALOG_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "EditorCommonAPI.h"
#include <QObject>
#include <Serialization/StringList.h>
#endif

// Private class, should not be used directly
class QWidget;
class QPropertyDialog;
class CBatchFileDialog
    : public QObject
{
    Q_OBJECT

public slots:
    void OnSelectAll();
    void OnSelectNone();
    void OnLoadList();

public:
    QPropertyDialog* m_dialog;
    struct SContent;
    SContent* m_content;
};
// ^^^

struct SBatchFileSettings
{
    const char* scanExtension;
    const char* scanFolder;
    const char* title;
    const char* descriptionText;
    const char* listLabel;
    const char* stateFilename;
    bool useCryPak;
    bool allowListLoading;
    bool readonlyList;
    bool filesAreCheckable;
    Serialization::StringList explicitFileList;
    int defaultWidth;
    int defaultHeight;

    SBatchFileSettings()
        : useCryPak(true)
        , readonlyList(true)
        , filesAreCheckable(false)
        , allowListLoading(true)
        , descriptionText("Batch Selected Files")
        , listLabel("Files")
        , stateFilename("batchFileDialog.state")
        , title("Batch Files")
        , scanFolder("")
        , scanExtension("*")
    {
    }
};

bool EDITOR_COMMON_API ShowBatchFileDialog(Serialization::StringList* filenames, const SBatchFileSettings& settings, QWidget* parent);

#endif // CRYINCLUDE_EDITORCOMMON_BATCHFILEDIALOG_H
