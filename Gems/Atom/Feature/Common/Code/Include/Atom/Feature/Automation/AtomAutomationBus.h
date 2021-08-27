/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

namespace AZ
{
    namespace Render
    {
        class AtomAutomationInterface
            : public EBusTraits
        {
        public:
            virtual ~AtomAutomationInterface() = default;

            //! Return type matches FindComponentTypeIds from EditorComponentAPIBus
            virtual AZStd::vector<AZ::Uuid> GetMeshComponentTypeId() = 0;
        };

        using AtomAutomationBus = EBus<AtomAutomationInterface>;

    } // namespace Render
} // namespace AZ
