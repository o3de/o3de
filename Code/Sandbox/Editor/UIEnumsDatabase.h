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

#ifndef CRYINCLUDE_EDITOR_UIENUMSDATABASE_H
#define CRYINCLUDE_EDITOR_UIENUMSDATABASE_H
#pragma once

#include "Include/EditorCoreAPI.h"

struct EDITOR_CORE_API CUIEnumsDatabase_SEnum
{
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QString m_name;
    QStringList strings; // Display Strings.
    QStringList values;  // Corresponding Values.
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

    QString NameToValue(const QString& name);
    QString ValueToName(const QString& value);
};

//////////////////////////////////////////////////////////////////////////
// Stores string associates to the enumeration collections for UI.
//////////////////////////////////////////////////////////////////////////
class EDITOR_CORE_API CUIEnumsDatabase
{
public:
    CUIEnumsDatabase();
    ~CUIEnumsDatabase();

    void SetEnumStrings(const QString& enumName, const QStringList& sStringsArray);
    CUIEnumsDatabase_SEnum* FindEnum(const QString& enumName) const;

private:
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    typedef std::map<QString, CUIEnumsDatabase_SEnum*> Enums;
    Enums m_enums;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

#endif // CRYINCLUDE_EDITOR_UIENUMSDATABASE_H
