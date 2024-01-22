/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Module/Module.h>

namespace AZ
{
    namespace RPI
    {
        class BuilderModule final
            : public AZ::Module
        {
        public:
            AZ_RTTI(BuilderModule, "{CA15BD7F-01B4-4959-BEF2-81FA3AD2C834}", AZ::Module);

            BuilderModule() = default;
            ~BuilderModule() = default;

            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return AZ::ComponentTypeList();
            }
        };
    } // namespace RPI
} // namespace AZ

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Builders), AZ::RPI::BuilderModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Atom_RPI_Edit_Builders, AZ::RPI::BuilderModule)
#endif
