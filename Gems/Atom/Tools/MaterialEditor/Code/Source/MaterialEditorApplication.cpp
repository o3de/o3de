/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <MaterialEditorApplication.h>
#include <MaterialEditor_Traits_Platform.h>

#include <Document/MaterialDocument.h>
#include <Window/MaterialEditorWindow.h>

void InitMaterialEditorResources()
{
    // Must register qt resources from other modules
    Q_INIT_RESOURCE(MaterialEditor);
    Q_INIT_RESOURCE(InspectorWidget);
    Q_INIT_RESOURCE(AtomToolsAssetBrowser);
}

namespace MaterialEditor
{
    static const char* GetBuildTargetName()
    {
#if !defined(LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        return LY_CMAKE_TARGET;
    }

    MaterialEditorApplication::MaterialEditorApplication(int* argc, char*** argv)
        : Base(GetBuildTargetName(), argc, argv)
    {
        InitMaterialEditorResources();

        QApplication::setOrganizationName("O3DE");
        QApplication::setApplicationName("O3DE Material Editor");

        AzToolsFramework::EditorWindowRequestBus::Handler::BusConnect();
    }

    MaterialEditorApplication::~MaterialEditorApplication()
    {
        AzToolsFramework::EditorWindowRequestBus::Handler::BusDisconnect();
        m_window.reset();
    }

    void MaterialEditorApplication::Reflect(AZ::ReflectContext* context)
    {
        Base::Reflect(context);
        MaterialDocument::Reflect(context);
        MaterialViewportSettingsSystem::Reflect(context);
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

        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
            m_toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Handler::RegisterDocumentType,
            MaterialDocument::BuildDocumentTypeInfo());

        m_viewportSettingsSystem.reset(aznew MaterialViewportSettingsSystem(m_toolId));

        m_window.reset(aznew MaterialEditorWindow(m_toolId));
        m_window->show();
    }

    void MaterialEditorApplication::Destroy()
    {
        m_window.reset();
        m_viewportSettingsSystem.reset();
        Base::Destroy();
    }

    AZStd::vector<AZStd::string> MaterialEditorApplication::GetCriticalAssetFilters() const
    {
        return AZStd::vector<AZStd::string>({ "passes/", "config/", "MaterialEditor/" });
    }

    QWidget* MaterialEditorApplication::GetAppMainWindow()
    {
        return m_window.get();
    }
} // namespace MaterialEditor
