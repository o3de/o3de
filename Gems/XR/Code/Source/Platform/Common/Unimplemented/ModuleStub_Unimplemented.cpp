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
    namespace XR
    {
        class Module
            : public AZ::Module
        {
        public:
            AZ_RTTI(Module, "{3A378AD3-B176-43F4-A49B-9D9FE053243A}", AZ::Module);

            Module() = default;
            ~Module() override = default;

            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return AZ::ComponentTypeList();
            }
        };
    }
}

AZ_DECLARE_MODULE_CLASS(Gem_XR_Private, AZ::XR::Module)
