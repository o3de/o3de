/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

namespace AZ::Debug
{
    class BudgetsComponent : public Component
    {
    public:
        AZ_COMPONENT(AZ::Debug::BudgetsComponent, "{52063706-5B36-4B24-A781-B49AFDBB5BC5}");

        static void Reflect(AZ::ReflectContext* context);

        BudgetsComponent();
        ~BudgetsComponent() override;

        void Init() override;
        void Activate() override;
        void Deactivate() override;

    private:
    };
} // namespace AZ::Debug
