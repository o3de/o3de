/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLERCONTEXTINTERFACE_H
#define DRILLERCONTEXTINTERFACE_H

#include <AzCore/base.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/UI/LegacyFramework/Core/EditorContextBus.h>

namespace Driller
{
    class ContextInterface
        : public LegacyFramework::EditorContextMessages
    {
    public:
        typedef AZ::EBus<ContextInterface> Bus;
        typedef Bus::Handler Handler;

        virtual void ShowDrillerView() = 0;
    };
}

#endif //DRILLERCONTEXTINTERFACE_H
