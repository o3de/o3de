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

#pragma once

#include <AzCore/Component/Component.h>

#include <CryCommon/IRenderer.h>
#include <CrySystemBus.h>

#include <ScriptCanvasDiagnosticLibrary/DebugDrawBus.h>


struct ISystem;

namespace ScriptCanvasDiagnostics
{
    class SystemComponent
        : public AZ::Component
        , public IRenderDebugListener
        , private CrySystemEventBus::Handler
        , private SystemRequestBus::Handler
    {
    public:
        AZ_COMPONENT(SystemComponent, "{6A90B0E7-EB47-48B5-910D-4881E429AC9D}");

        static void Reflect(AZ::ReflectContext* context);

        SystemComponent() = default;
        ~SystemComponent() override;
        void Init() override;

        void Activate() override;
        void Deactivate() override;

        // IRenderDebugListener
        void OnDebugDraw() override;

        // SystemRequestBus
        bool IsEditor() override;

    private:

        ISystem * m_system;

        // CrySystemEventBus
        void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams) override;
        void OnCrySystemShutdown(ISystem& system) override;

    };
}
