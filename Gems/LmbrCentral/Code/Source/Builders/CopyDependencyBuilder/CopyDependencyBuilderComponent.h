/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Component/Component.h>

#include "CfgBuilderWorker/CfgBuilderWorker.h"
#include "EmfxWorkspaceBuilderWorker/EmfxWorkspaceBuilderWorker.h"
#include "FontBuilderWorker/FontBuilderWorker.h"
#include "SchemaBuilderWorker/SchemaBuilderWorker.h"
#include "XmlBuilderWorker/XmlBuilderWorker.h"

namespace CopyDependencyBuilder
{
    //! The CopyDependencyBuilderComponent is responsible for setting up the CopyDependencyBuilderWorker.
    class CopyDependencyBuilderComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(CopyDependencyBuilderComponent, "{020E806C-E153-4E3A-8F4B-A550E3730808}");

        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////


    private:
        FontBuilderWorker m_fontBuilderWorker;
        CfgBuilderWorker m_cfgBuilderWorker;
        XmlBuilderWorker m_xmlBuilderWorker;
        SchemaBuilderWorker m_schemaBuilderWorker;
        EmfxWorkspaceBuilderWorker m_emfxWorkspaceBuilderWorker;
    };
}
