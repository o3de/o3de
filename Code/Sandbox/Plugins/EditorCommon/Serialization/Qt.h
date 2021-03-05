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

#ifndef CRYINCLUDE_EDITORCOMMON_SERIALIZATION_QT_H
#define CRYINCLUDE_EDITORCOMMON_SERIALIZATION_QT_H
#pragma once

#include "EditorCommonAPI.h"

class QByteArray;
class QColor;
class QPalette;
class QSplitter;
class QString;
class QTreeView;

bool EDITOR_COMMON_API Serialize(Serialization::IArchive& ar, QSplitter* splitter, const char* name, const char* label);
bool EDITOR_COMMON_API Serialize(Serialization::IArchive& ar, QByteArray& value, const char* name, const char* label);
bool EDITOR_COMMON_API Serialize(Serialization::IArchive& ar, QString& value, const char* name, const char* label);
bool EDITOR_COMMON_API Serialize(Serialization::IArchive& ar, QTreeView* treeViewState, const char* name, const char* label);
bool EDITOR_COMMON_API Serialize(Serialization::IArchive& ar, QPalette& palette, const char* name, const char* label);
bool EDITOR_COMMON_API Serialize(Serialization::IArchive& ar, QColor& color, const char* name, const char* label);
#endif // CRYINCLUDE_EDITORCOMMON_SERIALIZATION_QT_H
