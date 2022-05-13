/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorRecastNavigationMeshComponent.h"
#include "EditorRecastNavigationMeshConfig.h"

#include <DetourDebugDraw.h>
#include <QString>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Physics/Shape.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>
#include <Components/RecastNavigationMeshComponent.h>
#include <RecastNavigation/RecastNavigationSurveyorBus.h>
#include <SourceControl/SourceControlAPI.h>

AZ_DECLARE_BUDGET(Navigation);

#pragma optimize("", off)

namespace RecastNavigation
{
    EditorRecastNavigationMeshComponent::EditorRecastNavigationMeshComponent()
        : m_autoUpdateHandler([this](bool changed) { OnAutoUpdateChanged(changed); })
        , m_showMeshHandler([this](bool changed) { OnShowNavMeshChanged(changed); })
        , m_tickEvent([this]() { OnTick(); }, AZ::Name("EditorRecastNavigationDebugViewTick"))
        , m_updateNavMeshEvent([this]() { OnUpdateNavMeshEvent(); }, AZ::Name("EditorRecastNavigationUpdateNavMeshInEditor"))
    {
    }

    void EditorRecastNavigationMeshComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorRecastNavigationMeshConfig::Reflect(context);

        if (const auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<EditorRecastNavigationMeshComponent, AZ::Component>()
                ->Field("Configurations", &EditorRecastNavigationMeshComponent::m_meshConfig)
                ->Field("Debug Options", &EditorRecastNavigationMeshComponent::m_meshEditorConfig)
                ->Field("Update Navigation Mesh", &EditorRecastNavigationMeshComponent::m_updateNavigationMeshComponentFlag)
                ->Field("Navigation Mesh Asset", &EditorRecastNavigationMeshComponent::m_navigationAsset)
                ->Version(1)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<EditorRecastNavigationMeshComponent>("Recast Navigation Mesh", "[Calculates the walkable navigation mesh]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(nullptr, &EditorRecastNavigationMeshComponent::m_meshConfig,
                        "Configurations", "Navigation Mesh configuration")
                    ->DataElement(nullptr, &EditorRecastNavigationMeshComponent::m_meshEditorConfig,
                        "Debug Options", "Various helper options for Editor viewport")

                    ->DataElement(AZ::Edit::UIHandlers::Button, &EditorRecastNavigationMeshComponent::m_updateNavigationMeshComponentFlag,
                        "Update Navigation Mesh", "Recalculates and draws the debug view of the mesh in the Editor viewport")
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ::Edit::Attributes::ButtonText, "Update Navigation Mesh")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorRecastNavigationMeshComponent::UpdatedNavigationMeshInEditor)

                    ->UIElement(AZ::Edit::UIHandlers::Button, "", "Export to obj")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorRecastNavigationMeshComponent::ExportToFile)
                    ->Attribute(AZ::Edit::Attributes::ButtonText, "Export")

                    ->DataElement(nullptr, &EditorRecastNavigationMeshComponent::m_navigationAsset,
                        "Navigation Mesh Asset", "Pre-computed Baked navigation mesh data and saved to a disk as an asset")
                    ;
            }
        }
    }

    void EditorRecastNavigationMeshComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("RecastNavigationMeshComponent"));
    }

    void EditorRecastNavigationMeshComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("RecastNavigationMeshComponent"));
    }

    void EditorRecastNavigationMeshComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("RecastNavigationSurveyorService"));
    }

    AZ::Crc32 EditorRecastNavigationMeshComponent::UpdatedNavigationMeshInEditor()
    {
        OnUpdateNavMeshEvent();

        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    void EditorRecastNavigationMeshComponent::Activate()
    {
        EditorComponentBase::Activate();

        m_context = AZStd::make_unique<RecastCustomContext>();

        AZ::Vector3 position = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(position, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);

        bool usingTiledSurveyor = false;
        RecastNavigationSurveyorRequestBus::EventResult(usingTiledSurveyor, GetEntityId(), &RecastNavigationSurveyorRequests::IsTiled);
        if (!usingTiledSurveyor)
        {
            // We are using a non-tiled surveyor. Force the tile to cover the entire area.
            AZ::Aabb entireVolume = AZ::Aabb::CreateNull();
            RecastNavigationSurveyorRequestBus::EventResult(entireVolume, GetEntityId(), &RecastNavigationSurveyorRequests::GetWorldBounds);
            m_meshConfig.m_tileSize = AZStd::max(entireVolume.GetExtents().GetX(), entireVolume.GetExtents().GetY());
        }

        CreateNavigationMesh(GetEntityId(), m_meshConfig.m_tileSize);

        m_navigationTaskExecutor = AZStd::make_unique<AZ::TaskExecutor>(m_meshEditorConfig.m_backgroundThreadsToUse);

        m_meshEditorConfig.BindAutoUpdateChangedEventHandler(m_autoUpdateHandler);
        m_meshEditorConfig.BindShowNavMeshChangedEventHandler(m_showMeshHandler);

        OnAutoUpdateChanged(m_meshEditorConfig.m_autoUpdateNavigationMesh);
        OnShowNavMeshChanged(m_meshEditorConfig.m_showNavigationMesh);
    }

    void EditorRecastNavigationMeshComponent::Deactivate()
    {
        m_autoUpdateHandler.Disconnect();
        m_showMeshHandler.Disconnect();

        if (m_taskGraphEvent)
        {
            m_taskGraphEvent->Wait();
            m_taskGraphEvent = {};
        }
        m_navigationTaskExecutor = {};
        m_graph = {};

        m_tickEvent.RemoveFromQueue();
        m_updateNavMeshEvent.RemoveFromQueue();

        m_context = {};
        m_navQuery = {};
        m_navMesh = {};

        EditorComponentBase::Deactivate();
    }

    void EditorRecastNavigationMeshComponent::OnTick()
    {
        if (!m_navMesh)
        {
            return;
        }

        if (m_meshEditorConfig.m_showNavigationMesh)
        {
            duDebugDrawNavMesh(&m_customDebugDraw, *m_navMesh, DU_DRAWNAVMESH_COLOR_TILES);
        }
    }

    void EditorRecastNavigationMeshComponent::OnUpdateNavMeshEvent()
    {
        if (!m_navMesh)
        {
            return;
        }

        if (m_meshEditorConfig.m_showNavigationMesh)
        {
            bool updateNow = false;
            {
                AZStd::lock_guard lock(m_navigationMeshMutex);
                if (!m_updatingNavMeshInProgress)
                {
                    m_updatingNavMeshInProgress = true;
                    updateNow = true;
                }
            }

            if (updateNow)
            {
                AZ_PROFILE_SCOPE(Navigation, "Navigation: OnUpdateNavMeshEvent");

                m_graph = AZStd::make_unique<AZ::TaskGraph>();

                AZStd::vector<AZStd::shared_ptr<TileGeometry>> tiles;

                {
                    AZ_PROFILE_SCOPE(Navigation, "Navigation: CollectGeometry");
                    RecastNavigationSurveyorRequestBus::EventResult(tiles, GetEntityId(),
                        &RecastNavigationSurveyorRequests::CollectGeometry,
                        m_meshConfig.m_tileSize, m_meshConfig.m_borderSize * m_meshConfig.m_cellSize);
                }

                AZ::TaskToken updateDoneTask = m_graph->AddTask(
                    m_taskDescriptor,
                    [this]
                    {
                        AZ_PROFILE_SCOPE(Navigation, "Navigation: update finished");
                        AZStd::lock_guard lock(m_navigationMeshMutex);
                        m_updatingNavMeshInProgress = false;
                    });

                AZStd::vector<AZ::TaskToken*> tasks;

                for (AZStd::shared_ptr<TileGeometry>& tile : tiles)
                {
                    if (tile->IsEmpty())
                    {
                        continue;
                    }

                    AZ::TaskToken processAndAddTileTask = m_graph->AddTask(
                        m_taskDescriptor,
                        [this, tile]
                        {
                            AZ_PROFILE_SCOPE(Navigation, "Navigation: processing a tile");

                            NavigationTileData navigationTileData = CreateNavigationTile(
                                tile.get(),
                                m_meshConfig, m_context.get());

                            {
                                AZStd::lock_guard lock(m_navigationMeshMutex);
                                m_navMesh->removeTile(m_navMesh->getTileRefAt(tile->m_tileX, tile->m_tileY, 0), nullptr, nullptr);

                                if (navigationTileData.IsValid())
                                {
                                    AttachNavigationTileToMesh(navigationTileData);
                                }
                            }
                        });

                    tasks.push_back(&processAndAddTileTask);
                }

                for (AZ::TaskToken* task : tasks)
                {
                    task->Precedes(updateDoneTask);
                }

                m_taskGraphEvent = AZStd::make_unique<AZ::TaskGraphEvent>();
                m_graph->SubmitOnExecutor(*m_navigationTaskExecutor, m_taskGraphEvent.get());
            }
        }
    }

    void EditorRecastNavigationMeshComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<RecastNavigationMeshComponent>(m_meshConfig, m_meshEditorConfig.m_showNavigationMesh);
    }

    void EditorRecastNavigationMeshComponent::OnAutoUpdateChanged(bool changed)
    {
        if (changed)
        {
            m_updateNavMeshEvent.Enqueue(AZ::TimeMs{ 1000 }, true);
        }
        else
        {
            m_updateNavMeshEvent.RemoveFromQueue();
        }
    }

    void EditorRecastNavigationMeshComponent::OnShowNavMeshChanged(bool changed)
    {
        if (changed)
        {
            m_tickEvent.Enqueue(AZ::TimeMs{ 0 }, true);
        }
        else
        {
            m_tickEvent.RemoveFromQueue();
        }
    }

    static const char* const ObjExtension = "navmesh";
    static AZStd::string NavigationPathAtProjectRoot(const AZStd::string_view name, const AZStd::string_view extension)
    {
        AZ::IO::Path path;
        if (const auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            settingsRegistry->Get(path.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath);
        }
        path /= AZ::IO::FixedMaxPathString::format("%.*s.%.*s", AZ_STRING_ARG(name), AZ_STRING_ARG(extension));
        return path.Native();
    }

    void RequestEditSourceControl(const char* absoluteFilePath)
    {
        bool active = false;
        AzToolsFramework::SourceControlConnectionRequestBus::BroadcastResult(
            active, &AzToolsFramework::SourceControlConnectionRequestBus::Events::IsActive);

        if (active)
        {
            AzToolsFramework::SourceControlCommandBus::Broadcast(
                &AzToolsFramework::SourceControlCommandBus::Events::RequestEdit, absoluteFilePath, true,
                []([[maybe_unused]] bool success, [[maybe_unused]] AzToolsFramework::SourceControlFileInfo info)
                {
                });
        }
    }

    static const int NAVMESHSET_MAGIC = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T'; //'MSET';
    static const int NAVMESHSET_VERSION = 1;

    struct NavMeshSetHeader
    {
        int magic;
        int version;
        int numTiles;
        dtNavMeshParams params;
    };

    struct NavMeshTileHeader
    {
        dtTileRef tileRef;
        int dataSize;
    };

    bool SaveNavigationMesh(const char* path, const dtNavMesh* mesh)
    {
        if (!mesh)
        {
            return false;
        }

        AZ::IO::FileIOStream fileStream(path, AZ::IO::OpenMode::ModeWrite);
        if (!fileStream.IsOpen())
        {
            return false;
        }

        // Store header.
        NavMeshSetHeader header = {};
        header.magic = NAVMESHSET_MAGIC;
        header.version = NAVMESHSET_VERSION;
        header.numTiles = 0;
        for (int i = 0; i < mesh->getMaxTiles(); ++i)
        {
            const dtMeshTile* tile = mesh->getTile(i);
            if (!tile || !tile->header || !tile->dataSize) continue;
            header.numTiles++;
        }
        memcpy(&header.params, mesh->getParams(), sizeof(dtNavMeshParams));

        fileStream.Write(sizeof(NavMeshSetHeader), &header);
        //fwrite(&header, sizeof(NavMeshSetHeader), 1, fp);

        // Store tiles.
        for (int i = 0; i < mesh->getMaxTiles(); ++i)
        {
            const dtMeshTile* tile = mesh->getTile(i);
            if (!tile || !tile->header || !tile->dataSize) continue;

            NavMeshTileHeader tileHeader;
            tileHeader.tileRef = mesh->getTileRef(tile);
            tileHeader.dataSize = tile->dataSize;

            fileStream.Write(sizeof(tileHeader), &tileHeader);
            //fwrite(&tileHeader, sizeof(tileHeader), 1, fp);

            fileStream.Write(tile->dataSize, tile->data);
            //fwrite(tile->data, tile->dataSize, 1, fp);
        }

        fileStream.Close();
        //fclose(fp);

        return true;
    }

    void EditorRecastNavigationMeshComponent::ExportToFile()
    {
        const AZStd::string initialAbsolutePathToExport =
            NavigationPathAtProjectRoot(GetEntity()->GetName(), ObjExtension);

        const QString fileFilter = AZStd::string::format("*.%s", ObjExtension).c_str();
        const QString absoluteSaveFilePath = AzQtComponents::FileDialog::GetSaveFileName(
            nullptr, "Save As...", QString(initialAbsolutePathToExport.c_str()), fileFilter);

        const auto absoluteSaveFilePathUtf8 = absoluteSaveFilePath.toUtf8();
        const auto absoluteSaveFilePathCstr = absoluteSaveFilePathUtf8.constData();
        if (SaveNavigationMesh(absoluteSaveFilePathCstr, m_navMesh.get()))
        {
            AZ_Printf("EditorRecastNavigationMeshComponent", "Exported navigation mesh to: %s", absoluteSaveFilePathCstr);
            RequestEditSourceControl(absoluteSaveFilePathCstr);
        }
        else
        {
            AZ_Warning(
                "EditorRecastNavigationMeshComponent", false, "Failed to export navigation mesh to: %s", absoluteSaveFilePathCstr);
        }
    }
} // namespace RecastNavigation

#pragma optimize("", on)
