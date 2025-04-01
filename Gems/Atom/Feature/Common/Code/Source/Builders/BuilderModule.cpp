/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Module/Module.h>

namespace AZ
{
    namespace Render
    {
        /**
         * @class BuilderModule
         * @brief Exposes Atom Building components to the Asset Processor
         */
        class BuilderModule final
            : public AZ::Module
        {
        public:
            AZ_RTTI(BuilderModule, "{CAF0E749-130B-4C1D-8C68-6BD98D41177E}", AZ::Module);

            BuilderModule()
            {}

            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return AZ::ComponentTypeList();
            }
        };
    } // namespace RPI
} // namespace AZ

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Builders), AZ::Render::BuilderModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Atom_Feature_Common_Builders, AZ::Render::BuilderModule)
#endif
