/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/ui/EnabledRelocationTypesModel.h>
#include <native/utilities/UuidManager.h>

namespace AssetProcessor
{
    int EnabledRelocationTypesModel::rowCount(const QModelIndex& /*parent*/) const
    {
        // If no types are enabled, then a message is displayed explaining that.
        if (m_enabledTypes.empty())
        {
            return 1;
        }
        return aznumeric_caster(m_enabledTypes.size());
    }

    QVariant EnabledRelocationTypesModel::data(const QModelIndex& index, int role) const
    {
        AZ_Assert(index.isValid(), "EnabledRelocationTypesModel index out of bounds");

        if (!index.isValid())
        {
            return {};
        }

        if (role == Qt::DisplayRole)
        {
            if (m_enabledTypes.empty())
            {
                return tr("No types are enabled for asset relocation.");
            }
            if (index.row() < m_enabledTypes.size())
            {
                return QString(m_enabledTypes[index.row()].c_str());
            }
        }

        return {};
    }

    void EnabledRelocationTypesModel::Reset()
    {
        beginResetModel();

        m_enabledTypes.clear();

        AZStd::unordered_set<AZStd::string> enabledRelocationTypes = AZ::Interface<AssetProcessor::IUuidRequests>::Get()->GetEnabledTypes();

        for (const auto& type : enabledRelocationTypes)
        {
            m_enabledTypes.insert(AZStd::upper_bound(m_enabledTypes.begin(), m_enabledTypes.end(), type), type);
        }

        endResetModel();
    }

} // namespace AssetProcessor
