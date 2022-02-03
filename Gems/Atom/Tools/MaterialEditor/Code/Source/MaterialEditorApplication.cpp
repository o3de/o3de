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
#include <Document/MaterialDocument.h>
#include <Document/MaterialDocumentRequestBus.h>
#include <Document/MaterialDocumentSettings.h>
#include <MaterialEditorApplication.h>
#include <MaterialEditor_Traits_Platform.h>
#include <Viewport/MaterialViewportModule.h>
#include <Window/MaterialEditorWindow.h>
#include <Window/MaterialEditorWindowSettings.h>

void InitMaterialEditorResources()
{
    // Must register qt resources from other modules
    Q_INIT_RESOURCE(MaterialEditor);
    Q_INIT_RESOURCE(InspectorWidget);
    Q_INIT_RESOURCE(AtomToolsAssetBrowser);
}

namespace MaterialEditor
{
    MaterialEditorApplication::MaterialEditorApplication(int* argc, char*** argv)
        : Base(argc, argv)
    {
        InitMaterialEditorResources();

        QApplication::setApplicationName("O3DE Material Editor");

        // The settings registry has been created at this point, so add the CMake target
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddBuildSystemTargetSpecialization(
            *AZ::SettingsRegistry::Get(), GetBuildTargetName());

        AzToolsFramework::EditorWindowRequestBus::Handler::BusConnect();
        AtomToolsFramework::AtomToolsMainWindowFactoryRequestBus::Handler::BusConnect();
    }

    MaterialEditorApplication::~MaterialEditorApplication()
    {
        AtomToolsFramework::AtomToolsMainWindowFactoryRequestBus::Handler::BusDisconnect();
        AzToolsFramework::EditorWindowRequestBus::Handler::BusDisconnect();
        m_window.reset();
    }

    void MaterialEditorApplication::Reflect(AZ::ReflectContext* context)
    {
        Base::Reflect(context);
        MaterialDocumentSettings::Reflect(context);
        MaterialEditorWindowSettings::Reflect(context);

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<MaterialDocumentRequestBus>("MaterialDocumentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "materialeditor")
                ;
        }
    }

    void MaterialEditorApplication::CreateStaticModules(AZStd::vector<AZ::Module*>& outModules)
    {
        Base::CreateStaticModules(outModules);
        outModules.push_back(aznew MaterialViewportModule);
    }

    const char* MaterialEditorApplication::GetCurrentConfigurationName() const
    {
#if defined(_RELEASE)
        return "ReleaseMaterialEditor";
#elif defined(_DEBUG)
        return "DebugMaterialEditor";
#else
        return "ProfileMaterialEditor";
#endif
    }

    void MaterialEditorApplication::StartCommon(AZ::Entity* systemEntity)
    {
        Base::StartCommon(systemEntity);

        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Broadcast(
            &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Handler::RegisterDocumentType,
            []() { return aznew MaterialDocument(); });
    }

    AZStd::string MaterialEditorApplication::GetBuildTargetName() const
    {
#if !defined(LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        //! Returns the build system target name of "MaterialEditor"
        return AZStd::string{ LY_CMAKE_TARGET };
    }

    AZStd::vector<AZStd::string> MaterialEditorApplication::GetCriticalAssetFilters() const
    {
        return AZStd::vector<AZStd::string>({ "passes/", "config/", "MaterialEditor/" });
    }

    void MaterialEditorApplication::CreateMainWindow()
    {
        m_materialEditorBrowserInteractions.reset(aznew MaterialEditorBrowserInteractions);
        m_window.reset(aznew MaterialEditorWindow);
    }

    void MaterialEditorApplication::DestroyMainWindow()
    {
        m_window.reset();
    }

    QWidget* MaterialEditorApplication::GetAppMainWindow()
    {
        return m_window.get();
    }
} // namespace MaterialEditor
