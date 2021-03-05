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

#pragma once

#if !defined(Q_MOC_RUN)
#include "EditorCommonAPI.h"
#include "Serialization/Strings.h"
#include "Serialization/DynArray.h"

#include <QDialog>
#endif
class QListWidget;

// Supposed to be used through
// ConfirmSaveDialog function.
class CUnsavedChangedDialog
    : public QDialog
{
    Q_OBJECT
public:
    CUnsavedChangedDialog(QWidget* parent);

    bool Exec(DynArray<string>* selectedFiles, const DynArray<string>& files);
private:
    QListWidget* m_list;
    int m_result;
};

// Returns true if window should be closed (Yes/No). False for Cancel.
// selectedFiles contains list of files that should be saved (empty in No case).
bool EDITOR_COMMON_API UnsavedChangesDialog(QWidget* parent, DynArray<string>* selectedFiles, const DynArray<string>& files);
