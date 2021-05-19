/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "precompiled.h"

#if !defined(SCRIPTCANVAS_EDITOR)

#include <ScriptCanvasGem.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/SimpleAsset.h>
#include <AzFramework/TargetManagement/TargetManagementComponent.h>

#include <SystemComponent.h>

#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Data/DataRegistry.h>

#include <ScriptCanvas/Libraries/Libraries.h>

#include <ScriptCanvas/Debugger/Debugger.h>
#include <ScriptCanvas/Variable/GraphVariableManagerComponent.h>

namespace ScriptCanvas
{
    ////////////////////////////////////////////////////////////////////////////
    // ScriptCanvasModule
    ////////////////////////////////////////////////////////////////////////////

    //! Create ComponentDescriptors and add them to the list.
    //! The descriptors will be registered at the appropriate time.
    //! The descriptors will be destroyed (and thus unregistered) at the appropriate time.
    ScriptCanvasModule::ScriptCanvasModule()
        : ScriptCanvasModuleCommon()
    {
    }

    AZ::ComponentTypeList ScriptCanvasModule::GetRequiredSystemComponents() const
    {
        return GetCommonSystemComponents();
    }
}

AZ_DECLARE_MODULE_CLASS(Gem_ScriptCanvas, ScriptCanvas::ScriptCanvasModule)

#endif // !SCRIPTCANVAS_EDITOR
