/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
