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

#include <AudioControlFilters.h>

#include <ACEEnums.h>
#include <QTreeWidget>


//-----------------------------------------------------------------------------------------------//
SImplNameFilter::SImplNameFilter(const QString& filter)
    : m_filter(filter)
{
}

//-----------------------------------------------------------------------------------------------//
void SImplNameFilter::SetFilter(QString filter)
{
    m_filter = filter;
}

//-----------------------------------------------------------------------------------------------//
bool SImplNameFilter::IsNameValid(const QString& name)
{
    if ((m_filter.isEmpty()) || name.contains(m_filter, Qt::CaseInsensitive))
    {
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------------------------//
bool SImplNameFilter::IsItemValid(QTreeWidgetItem* pItem)
{
    if (pItem)
    {
        return IsNameValid(pItem->text(0));
    }
    return false;
}

//-----------------------------------------------------------------------------------------------//
bool SImplTypeFilter::IsItemValid(QTreeWidgetItem* pItem)
{
    if (pItem)
    {
        return pItem->data(0, eMDR_TYPE).toUInt() & m_nAllowedControlsMask;
    }
    return false;
}

//-----------------------------------------------------------------------------------------------//
SHideConnectedFilter::SHideConnectedFilter()
    : m_bHideConnected(false)
{
}

//-----------------------------------------------------------------------------------------------//
void SHideConnectedFilter::SetHideConnected(bool bHideConnected)
{
    m_bHideConnected = bHideConnected;
}

//-----------------------------------------------------------------------------------------------//
bool SHideConnectedFilter::IsItemValid(QTreeWidgetItem* pItem)
{
    if (pItem)
    {
        return !m_bHideConnected || !pItem->data(0, eMDR_CONNECTED).toBool();
    }
    return false;
}