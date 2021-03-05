#pragma once

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
