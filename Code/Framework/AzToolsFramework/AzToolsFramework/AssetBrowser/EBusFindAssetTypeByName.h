/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <QString>

struct EBusFindAssetTypeByName
{
    explicit EBusFindAssetTypeByName(const char* name)
        : m_name(name)
        , m_found(false)
        , m_assetType(AZ::Data::AssetType::CreateNull())
    {
    }

    AZ_FORCE_INLINE void operator=(const AZ::Data::AssetType& assetType)
    {
        if (m_found)
        {
            return;
        }
        if (MatchesName(assetType))
        {
            m_assetType = assetType;
            m_found = true;
        }
    }

    AZ::Data::AssetType GetAssetType() const
    {
        return m_assetType;
    }

    bool Found() const
    {
        return m_found;
    }

private:
    QString m_name;
    bool m_found;
    AZ::Data::AssetType m_assetType;

    bool MatchesName(const AZ::Data::AssetType& assetType) const
    {
        QString name;
        AZ::AssetTypeInfoBus::EventResult(name, assetType, &AZ::AssetTypeInfo::GetAssetTypeDisplayName);
        return name.compare(m_name, Qt::CaseInsensitive) == 0;
    }
};

