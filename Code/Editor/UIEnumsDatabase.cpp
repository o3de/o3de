/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "UIEnumsDatabase.h"

//////////////////////////////////////////////////////////////////////////
QString CUIEnumsDatabase_SEnum::NameToValue(const QString& name)
{
    int n = (int)strings.size();
    for (int i = 0; i < n; i++)
    {
        if (name == strings[i])
        {
            return values[i];
        }
    }
    return name;
}

//////////////////////////////////////////////////////////////////////////
QString CUIEnumsDatabase_SEnum::ValueToName(const QString& value)
{
    int n = (int)strings.size();
    for (int i = 0; i < n; i++)
    {
        if (value == values[i])
        {
            return strings[i];
        }
    }
    return value;
}

//////////////////////////////////////////////////////////////////////////
CUIEnumsDatabase::CUIEnumsDatabase()
{
}

//////////////////////////////////////////////////////////////////////////
CUIEnumsDatabase::~CUIEnumsDatabase()
{
    // Free enums.
    for (Enums::iterator it = m_enums.begin(); it != m_enums.end(); ++it)
    {
        delete it->second;
    }
}

//////////////////////////////////////////////////////////////////////////
void CUIEnumsDatabase::SetEnumStrings(const QString& enumName, const QStringList& sStringsArray)
{
    int nStringCount = sStringsArray.size();

    CUIEnumsDatabase_SEnum* pEnum = stl::find_in_map(m_enums, enumName, nullptr);
    if (!pEnum)
    {
        pEnum = new CUIEnumsDatabase_SEnum;
        pEnum->m_name = enumName;
        m_enums[enumName] = pEnum;
    }
    pEnum->strings.clear();
    pEnum->values.clear();
    for (int i = 0; i < nStringCount; i++)
    {
        QString str = sStringsArray[i];
        QString value = str;
        int pos = str.indexOf('=');
        if (pos >= 0)
        {
            value = str.mid(pos + 1);
            str = str.mid(0, pos);
        }
        pEnum->strings.push_back(str);
        pEnum->values.push_back(value);
    }
}

//////////////////////////////////////////////////////////////////////////
CUIEnumsDatabase_SEnum* CUIEnumsDatabase::FindEnum(const QString& enumName) const
{
    CUIEnumsDatabase_SEnum* pEnum = stl::find_in_map(m_enums, enumName, nullptr);
    return pEnum;
}
