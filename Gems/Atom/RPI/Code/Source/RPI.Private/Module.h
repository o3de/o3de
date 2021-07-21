/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Module/Module.h>

#include <RPI.Private/RPISystemComponent.h>

namespace AZ
{
    namespace RPI
    {
        class Module
            : public AZ::Module
        {
        public:
            AZ_RTTI(Module, "{CDB54E96-717D-4DFC-BEA6-F809BDE601AE}", AZ::Module);

            Module();
            ~Module() override = default;

            AZ::ComponentTypeList GetRequiredSystemComponents() const override;
        };
    } // namespace RPI
} // namespace RPI
