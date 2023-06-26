#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(Q_MOC_RUN)
#include <QWidget>
#include <QMenu>
#include <QScopedPointer>

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPI/SceneUI/SceneUIConfiguration.h>
#endif

class ImporterRootDisplayWidget;
class QAction;
class QDockWidget;
class QMenu;

namespace AZ
{
    class ReflectContext;
    class SerializeContext;

    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }

        namespace UI
        {
            class ManifestWidget;

        }
    }
} // namespace AZ

namespace Ui
{
    class ImporterRootDisplay;
} // namespace UI

//! Python interface for the scene importer root display settings
class SceneSettingsRootDisplayScriptRequests
    : public AZ::EBusTraits
{
public:
    //////////////////////////////////////////////////////////////////////////
    // EBusTraits overrides
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    //////////////////////////////////////////////////////////////////////////

    //! Returns true if the open scene settings file has unsaved changes, false if not.
    virtual bool HasUnsavedChanges() const = 0;
};
using SceneSettingsRootDisplayScriptRequestBus = AZ::EBus<SceneSettingsRootDisplayScriptRequests>;

class SceneSettingsRootDisplayScriptRequestHandler : protected SceneSettingsRootDisplayScriptRequestBus::Handler
{
public:
    AZ_RTTI(SceneSettingsRootDisplayScriptRequestHandler, "{DF965807-DA41-4DFB-BD26-DD94E4955E8D}");
    SceneSettingsRootDisplayScriptRequestHandler();
    ~SceneSettingsRootDisplayScriptRequestHandler();

    static void Reflect(AZ::ReflectContext* context);
    bool HasUnsavedChanges() const override;

    void SetRootDisplay(ImporterRootDisplayWidget* importerRootDisplay);

private:
    ImporterRootDisplayWidget* m_importerRootDisplay = nullptr;
};

class ImporterRootDisplayWidget
    : public QWidget
    , public AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler
{
    Q_OBJECT

public:
    AZ_CLASS_ALLOCATOR(ImporterRootDisplayWidget, AZ::SystemAllocator);

    ImporterRootDisplayWidget(AZ::SerializeContext* serializeContext, QWidget* parent = nullptr);

    ~ImporterRootDisplayWidget();

    AZ::SceneAPI::UI::ManifestWidget* GetManifestWidget();

    void SetSceneDisplay(const QString& headerText, const AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene>& scene);
    void SetSceneHeaderText(const QString& headerText);
    void SetPythonBuilderText(QString pythonBuilderText);
    void HandleSceneWasReset(const AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene>& scene);
    void HandleSaveWasSuccessful();

    QString GetHeaderFileName() const;

    void UpdateTimeStamp(const QString& manifestFilePath, bool enableInspector);

    bool HasUnsavedChanges() const;

signals:
    void AppendUnsavedChangesToTitle(bool hasUnsavedChanges);

private:
    // ManifestMetaInfoBus
    void ObjectUpdated(const AZ::SceneAPI::Containers::Scene& scene, const AZ::SceneAPI::DataTypes::IManifestObject* target, void* sender) override;

    void SetUnsavedChanges(bool hasUnsavedChanges);

    Ui::ImporterRootDisplay* ui;
    QScopedPointer<AZ::SceneAPI::UI::ManifestWidget> m_manifestWidget;
    bool m_hasUnsavedChanges;
    QString m_filePath;
    AZStd::shared_ptr<SceneSettingsRootDisplayScriptRequestHandler> m_requestHandler;
};
