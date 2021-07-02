/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/DataPatchUpgradeManager.h>

namespace AZ
{
    DataPatchUpgradeManager::DataPatchUpgradeManager(const AZ::SerializeContext::DataPatchFieldUpgrades& upgrades, int startingVersion)
        : m_upgrades(upgrades)
        , m_startingVersion(startingVersion)
    {}

    // Upgrade a data patch
    void DataPatchUpgradeManager::UpgradeDataPatch(AZ::SerializeContext* context, AZ::TypeId targetClassID, unsigned int targetClassVersion, DataPatch::AddressType& address, AZStd::any& data)
    {
        if (address.empty())
        {
            return;
        }

        // Best-Effort attempt to recover a legacy data patch address
        // In a failure case, the original address will be preserved and the system will attempt to apply it.
        if (address.IsFromLegacyAddress())
        {
            return;
        }

        // Process the address from the root node up
        bool leafNode = true;

        // Single address value case
        if (address.size() == 1)
        {
            auto* parentClassData = context->FindClassData(targetClassID);
            if (parentClassData)
            {
                // It would be better to store this information in the data patch so we don't make assumptions.
                DataPatchUpgradeManager upgrader(parentClassData->m_dataPatchUpgrader.GetUpgrades(), targetClassVersion);
                upgrader.ApplyUpgrades(address.front(), data);
                return;
            }
        }

        for (auto addressElement = address.rbegin(); addressElement != address.rend() && AZStd::next(addressElement) != address.rend(); ++addressElement)
        {
            // Get the class data for the next element in the sequence (The parent)
            // So we can apply the upgrades contained there to the address in question.
            auto nextType = AZStd::next(addressElement)->GetElementTypeId();
            auto* parentClassData = context->FindClassData(nextType);

            if (parentClassData)
            {
                DataPatchUpgradeManager upgrader(parentClassData->m_dataPatchUpgrader.GetUpgrades(), AZStd::next(addressElement)->GetElementVersion());

                if (leafNode)
                {
                    upgrader.ApplyUpgrades(*addressElement, data);
                }
                else
                {
                    upgrader.ApplyUpgrades(*addressElement);
                }
            }

            leafNode = false;
        }

        // Finally, apply any root class upgrades to the address
        auto* parentClassData = context->FindClassData(targetClassID);
        if (parentClassData)
        {
            DataPatchUpgradeManager upgrader(parentClassData->m_dataPatchUpgrader.GetUpgrades(), targetClassVersion);
            upgrader.ApplyUpgrades(address.front());
        }
    }

    // Apply appropriate upgrades to an address. Returns a new address.
    void DataPatchUpgradeManager::ApplyUpgrades(AZ::DataPatch::AddressTypeElement& addressElement, AZStd::vector<AZ::SerializeContext::DataPatchUpgrade*> upgrades) const
    {
        if (upgrades.empty())
        {
            upgrades = GetApplicableUpgrades(addressElement);
        }

        for (auto* upgrade : upgrades)
        {
            if (upgrade->GetUpgradeType() == AZ::SerializeContext::TYPE_UPGRADE)
            {
                AZ_Assert(addressElement.GetElementTypeId() == upgrade->GetFromType(), "Data Patch Upgrade Failure: address element type does not match the from type of the upgrade.");
                addressElement.SetAddressClassTypeId(upgrade->GetToType());

                // The nature of data patch upgrades means that a name upgrade
                // could have already set the version of the element of the
                // type converter.
                if (addressElement.GetElementVersion() < upgrade->ToVersion())
                {
                    addressElement.SetAddressClassVersion(upgrade->ToVersion());
                }
            }
            else if (upgrade->GetUpgradeType() == AZ::SerializeContext::NAME_UPGRADE)
            {
                addressElement.SetPathElement(upgrade->GetNewName());
                addressElement.SetAddressElement(AZ::u32(AZ::Crc32(upgrade->GetNewName().c_str())));

                // The nature of data patch upgrades means that a type upgrade
                // could have already set the version of the element of the
                // name converter.
                if (addressElement.GetElementVersion() < upgrade->ToVersion())
                {
                    addressElement.SetAddressClassVersion(upgrade->ToVersion());
                }
            }
        }
    }

    // Apply appropriate upgrades to a value. Returns a new value.
    void DataPatchUpgradeManager::ApplyUpgrades(AZ::DataPatch::AddressTypeElement& addressElement, AZStd::any& value) const
    {
        auto upgrades = GetApplicableUpgrades(addressElement);

        // Fix the address
        ApplyUpgrades(addressElement, upgrades);

        for (auto* upgrade : upgrades)
        {
            // Ignore name upgrades
            if (upgrade->GetUpgradeType() == AZ::SerializeContext::TYPE_UPGRADE)
            {
                value = upgrade->Apply(value);
            }
        }
    }

    // Retrieve an ordered list of upgrades managed by this handler that should be applied
    // to the given address or an associated value. Returns an empty vector if there are none.
    AZStd::vector<AZ::SerializeContext::DataPatchUpgrade*> DataPatchUpgradeManager::GetApplicableUpgrades(const DataPatch::AddressTypeElement& address) const
    {
        if (m_upgrades.empty())
        {
            return AZStd::vector<AZ::SerializeContext::DataPatchUpgrade*>();
        }

        AZStd::vector<AZ::SerializeContext::DataPatchUpgrade*> result;
        unsigned int typeVersion = m_startingVersion;
        unsigned int nameVersion = m_startingVersion;
        AZ::Crc32 currentNameCRC = AZ::Crc32(u32(address.GetAddressElement()));

        auto nextUpgrade = GetNextUpgrade(typeVersion, nameVersion, currentNameCRC);
        while (nextUpgrade)
        {
            if (nextUpgrade->GetUpgradeType() == AZ::SerializeContext::TYPE_UPGRADE)
            {
                typeVersion = nextUpgrade->ToVersion();
            }
            else if (nextUpgrade->GetUpgradeType() == AZ::SerializeContext::NAME_UPGRADE)
            {
                nameVersion = nextUpgrade->ToVersion();
                currentNameCRC = AZ::Crc32(nextUpgrade->GetNewName().c_str());
            }
            else
            {
                AZ_Assert(false, "Unknown data patch upgrade type: %u", nextUpgrade->GetUpgradeType());
            }

            result.push_back(nextUpgrade);
            nextUpgrade = GetNextUpgrade(typeVersion, nameVersion, currentNameCRC);
        }

        return result;
    }

    const AZ::SerializeContext::DataPatchUpgradeMap* DataPatchUpgradeManager::GetPotentialUpgrades(const AZ::Crc32& fieldNameCRC) const
    {
        auto upgradesByField = m_upgrades.find(fieldNameCRC);
        if (upgradesByField == m_upgrades.end())
        {
            return nullptr;
        }

        // Get a pointer to a sorted map of upgrades keyed by their from value.
        return &(upgradesByField->second);
    }

    AZ::SerializeContext::DataPatchUpgrade* DataPatchUpgradeManager::GetNextUpgrade(unsigned int currentTypeVersion, unsigned int currentNameVersion, AZ::Crc32 currentNameCRC) const
    {
        if (m_upgrades.empty())
        {
            return {};
        }

        auto nextTypeUpgrade = GetNextTypeUpgrade(currentNameCRC, currentTypeVersion);
        auto nextNameUpgrade = GetNextNameUpgrade(currentNameCRC, currentNameVersion);

        if (!nextTypeUpgrade && !nextNameUpgrade)
        {
            return nullptr;
        }

        if (!nextNameUpgrade || (nextTypeUpgrade && nextTypeUpgrade->FromVersion() <= nextNameUpgrade->FromVersion()))
        {
            return nextTypeUpgrade;
        }

        return nextNameUpgrade;
    }

    AZ::SerializeContext::DataPatchUpgrade* DataPatchUpgradeManager::GetNextTypeUpgrade(AZ::Crc32 currentFieldNameCRC, unsigned int currentTypeVersion) const
    {
        if (m_upgrades.empty())
        {
            return {};
        }

        auto potentialUpgrades = GetPotentialUpgrades(currentFieldNameCRC);
        if (potentialUpgrades && !potentialUpgrades->empty())
        {
            for (auto mapIter = potentialUpgrades->begin(); mapIter != potentialUpgrades->end(); ++mapIter)
            {
                if (mapIter->first >= currentTypeVersion && (AZStd::next(mapIter) == potentialUpgrades->end() || AZStd::next(mapIter)->first > currentTypeVersion))
                {
                    auto possibleUpgrade = mapIter->second.rbegin();

                    while (possibleUpgrade != mapIter->second.rend())
                    {
                        AZ::SerializeContext::DataPatchUpgradeType upgradeType = (*possibleUpgrade)->GetUpgradeType();
                        unsigned int toVersion = (*possibleUpgrade)->ToVersion();
                        if (upgradeType == AZ::SerializeContext::TYPE_UPGRADE && toVersion > currentTypeVersion)
                        {
                            return (*possibleUpgrade);
                        }

                        ++possibleUpgrade;
                    }
                }
            }
        }

        return nullptr;
    }

    AZ::SerializeContext::DataPatchUpgrade* DataPatchUpgradeManager::GetNextNameUpgrade(AZ::Crc32 fieldNameCRC, unsigned int currentNameVersion) const
    {
        if (m_upgrades.empty())
        {
            return {};
        }

        auto potentialUpgrades = GetPotentialUpgrades(fieldNameCRC);
        if (potentialUpgrades && !potentialUpgrades->empty())
        {
            for (auto mapIter = potentialUpgrades->begin(), nextMapIter = potentialUpgrades->begin(); mapIter != potentialUpgrades->end(); ++mapIter)
            {
                ++nextMapIter;

                if (mapIter->first >= currentNameVersion && (nextMapIter == potentialUpgrades->end() || nextMapIter->first > currentNameVersion))
                {
                    auto possibleUpgrade = mapIter->second.rbegin();

                    if (!mapIter->second.empty())
                    {
                        ++possibleUpgrade;
                    }

                    while (possibleUpgrade != mapIter->second.rend())
                    {
                        AZ::SerializeContext::DataPatchUpgradeType upgradeType = (*possibleUpgrade)->GetUpgradeType();
                        unsigned int toVersion = (*possibleUpgrade)->ToVersion();
                        if (upgradeType == AZ::SerializeContext::NAME_UPGRADE && toVersion > currentNameVersion)
                        {
                            return (*possibleUpgrade);
                        }

                        ++possibleUpgrade;
                    }
                }
            }
        }

        return nullptr;
    }
}
