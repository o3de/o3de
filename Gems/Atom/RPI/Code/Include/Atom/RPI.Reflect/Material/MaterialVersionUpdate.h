/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Name/Name.h>

namespace AZ
{
    namespace RPI
    {
        class MaterialAsset;

        // This class contains a toVersion and a list of actions to specify what operations were performed to upgrade a materialType. 
        class MaterialVersionUpdate
        {
        public:
            AZ_TYPE_INFO(AZ::RPI::MaterialVersionUpdate, "{B36E7712-AED8-46AA-AFE0-01F8F884C44A}");

            static void Reflect(ReflectContext* context);

            // At this time, the only supported operation is rename. If/when we add more actions in the future,
            // we'll need to improve this, possibly with some virtual interface or union data.
            struct RenamePropertyAction
            {
                AZ_TYPE_INFO(AZ::RPI::MaterialVersionUpdate::RenameAction, "{A1FBEB19-EA05-40F0-9700-57D048DF572B}");
                
                static void Reflect(ReflectContext* context);

                AZ::Name m_fromPropertyId;
                AZ::Name m_toPropertyId;
            };

            explicit MaterialVersionUpdate() = default;
            explicit MaterialVersionUpdate(uint32_t toVersion);

            uint32_t GetVersion() const;
            void SetVersion(uint32_t toVersion);
            
            //! Possibly renames @propertyId based on the material version update steps.
            //! @return true if the property was renamed
            bool ApplyPropertyRenames(AZ::Name& propertyId) const;

            //! Apply version updates to the given material asset.
            //! @return true if any changes were made
            bool ApplyVersionUpdates(MaterialAsset& materialAsset) const;

            using Actions = AZStd::vector<RenamePropertyAction>;
            const Actions& GetActions() const;
            void AddAction(const RenamePropertyAction& action);

        private:
            uint32_t m_toVersion;
            Actions m_actions;
        };

    } // namespace RPI
} // namespace AZ
