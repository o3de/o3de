/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <Document/MaterialCanvasDocument.h>
#include <Document/MaterialCanvasDocumentRequestBus.h>
#include <Document/MaterialCanvasDocumentSettings.h>
#include <MaterialCanvasApplication.h>
#include <MaterialCanvas_Traits_Platform.h>
#include <Viewport/MaterialCanvasViewportModule.h>
#include <Window/MaterialCanvasMainWindow.h>
#include <Window/MaterialCanvasMainWindowSettings.h>

void InitMaterialCanvasResources()
{
    // Must register qt resources from other modules
    Q_INIT_RESOURCE(MaterialCanvas);
    Q_INIT_RESOURCE(InspectorWidget);
    Q_INIT_RESOURCE(AtomToolsAssetBrowser);
}

namespace MaterialCanvas
{
    MaterialCanvasApplication::MaterialCanvasApplication(int* argc, char*** argv)
        : Base(argc, argv)
    {
        InitMaterialCanvasResources();

        QApplication::setApplicationName("O3DE Material Canvas");

        // The settings registry has been created at this point, so add the CMake target
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddBuildSystemTargetSpecialization(
            *AZ::SettingsRegistry::Get(), GetBuildTargetName());

        AzToolsFramework::EditorWindowRequestBus::Handler::BusConnect();
        AtomToolsFramework::AtomToolsMainWindowFactoryRequestBus::Handler::BusConnect();
    }

    MaterialCanvasApplication::~MaterialCanvasApplication()
    {
        AtomToolsFramework::AtomToolsMainWindowFactoryRequestBus::Handler::BusDisconnect();
        AzToolsFramework::EditorWindowRequestBus::Handler::BusDisconnect();
        m_window.reset();
    }

    void MaterialCanvasApplication::Reflect(AZ::ReflectContext* context)
    {
        Base::Reflect(context);
        MaterialCanvasDocumentSettings::Reflect(context);
        MaterialCanvasMainWindowSettings::Reflect(context);

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<MaterialCanvasDocumentRequestBus>("MaterialCanvasDocumentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "materialcanvas")
                ;
        }
    }

    void MaterialCanvasApplication::CreateStaticModules(AZStd::vector<AZ::Module*>& outModules)
    {
        Base::CreateStaticModules(outModules);
        outModules.push_back(aznew MaterialCanvasViewportModule);
    }

    const char* MaterialCanvasApplication::GetCurrentConfigurationName() const
    {
#if defined(_RELEASE)
        return "ReleaseMaterialCanvas";
#elif defined(_DEBUG)
        return "DebugMaterialCanvas";
#else
        return "ProfileMaterialCanvas";
#endif
    }

    void MaterialCanvasApplication::StartCommon(AZ::Entity* systemEntity)
    {
        Base::StartCommon(systemEntity);

        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Broadcast(
            &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Handler::RegisterDocumentType,
            []() { return aznew MaterialCanvasDocument(); });
    }

    AZStd::string MaterialCanvasApplication::GetBuildTargetName() const
    {
#if !defined(LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        //! Returns the build system target name of "MaterialCanvas"
        return AZStd::string{ LY_CMAKE_TARGET };
    }

    AZStd::vector<AZStd::string> MaterialCanvasApplication::GetCriticalAssetFilters() const
    {
        return AZStd::vector<AZStd::string>({ "passes/", "config/", "MaterialCanvas/" });
    }

    void MaterialCanvasApplication::CreateMainWindow()
    {
        m_materialCanvasBrowserInteractions.reset(aznew MaterialCanvasBrowserInteractions);
        m_window.reset(aznew MaterialCanvasMainWindow);
    }

    void MaterialCanvasApplication::DestroyMainWindow()
    {
        m_window.reset();
    }

    QWidget* MaterialCanvasApplication::GetAppMainWindow()
    {
        return m_window.get();
    }
} // namespace MaterialCanvas
