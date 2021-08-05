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
#include <QScopedPointer>

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPI/SceneUI/SceneUIConfiguration.h>
#endif

class QAction;
class QMenu;

namespace AZ
{
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
}

namespace Ui
{
    class ImporterRootDisplay;
}

class ImporterRootDisplay 
    : public QWidget
    , public AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler
{
    Q_OBJECT

public:
    AZ_CLASS_ALLOCATOR(ImporterRootDisplay, AZ::SystemAllocator, 0)

    ImporterRootDisplay(AZ::SerializeContext* serializeContext, QWidget* parent = nullptr);

    ~ImporterRootDisplay();

    AZ::SceneAPI::UI::ManifestWidget* GetManifestWidget();

    void SetSceneDisplay(const QString& headerText, const AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene>& scene);
    void HandleSceneWasReset(const AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene>& scene);
    void HandleSaveWasSuccessful();
    bool HasUnsavedChanges() const;


signals:
    void UpdateClicked();

private:
    // ManifestMetaInfoBus
    void ObjectUpdated(const AZ::SceneAPI::Containers::Scene& scene, const AZ::SceneAPI::DataTypes::IManifestObject* target, void* sender) override;

    Ui::ImporterRootDisplay* ui;
    QScopedPointer<AZ::SceneAPI::UI::ManifestWidget> m_manifestWidget;
    bool m_hasUnsavedChanges;
};
