/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/base.h>
#include <QTreeWidgetFilter.h>

#include <QString>

//-----------------------------------------------------------------------------------------------//
struct SImplNameFilter
    : public ITreeWidgetItemFilter
{
    SImplNameFilter(const QString& filter = "");
    bool IsItemValid(QTreeWidgetItem* pItem) override;
    void SetFilter(QString filter);
    bool IsNameValid(const QString& name);

private:
    QString m_filter;
};

//-----------------------------------------------------------------------------------------------//
struct SImplTypeFilter
    : public ITreeWidgetItemFilter
{
    SImplTypeFilter()
        : m_nAllowedControlsMask(std::numeric_limits<AZ::u32>::max())
    {}
    bool IsItemValid(QTreeWidgetItem* pItem) override;
    void SetAllowedControlsMask(AZ::u32 nAllowedControlsMask) { m_nAllowedControlsMask = nAllowedControlsMask; }
    AZ::u32 m_nAllowedControlsMask;
};

//-----------------------------------------------------------------------------------------------//
struct SHideConnectedFilter
    : public ITreeWidgetItemFilter
{
    SHideConnectedFilter();
    bool IsItemValid(QTreeWidgetItem* pItem) override;
    void SetHideConnected(bool bHideConnected);

private:
    bool m_bHideConnected;
};
