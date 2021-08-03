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

#include "LegacyTerrain_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "LegacyTerrainEditorLevelComponent.h"

//From EditorLib/Include
#include <EditorDefs.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <Terrain/Bus/LegacyTerrainBus.h>

#include <IEditor.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <AzToolsFramework/API/EditorVegetationRequestsBus.h>

#include "EditorDefs.h"
#include "MathConversion.h"
#include "VegetationMap.h"
#include "Terrain/Heightmap.h"
#include "VegetationObject.h"

#include "Include/EditorCoreAPI.h"


namespace LegacyTerrain
{
    void LegacyTerrainEditorLevelComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClassType::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<LegacyTerrainEditorLevelComponent, BaseClassType>()
                ->Version(0)
            ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<LegacyTerrainEditorLevelComponent>(
                    "Legacy Terrain", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Terrain")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC("Level", 0x9aeacc13) }))
                ;
            }
        }
    }


    void LegacyTerrainEditorLevelComponent::Init()
    {
        BaseClassType::Init();
    }

    void LegacyTerrainEditorLevelComponent::Activate()
    {
        bool isInstantiated = false;
        LegacyTerrainInstanceRequestBus::BroadcastResult(isInstantiated, &LegacyTerrainInstanceRequests::IsTerrainSystemInstantiated);
        if (isInstantiated)
        {
            AZ_Warning("LegacyTerrain", false, "The legacy terrain system was already instantiated");
            return;
        }

        AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(&AzFramework::Terrain::TerrainDataNotifications::OnTerrainDataCreateBegin);

        // We use the Editor version of this bus instead of the LegacyTerrainInstanceRequestBus because we'd like to use the Editor version
        // of the data in memory to initialize our runtime system.  Otherwise, it would use the last exported version.
        LegacyTerrainEditorDataRequestBus::BroadcastResult(isInstantiated, &LegacyTerrainEditorDataRequests::CreateTerrainSystemFromEditorData);
        AZ_Error("LegacyTerrain", isInstantiated, "Failed to initialize the legacy terrain system");

        Terrain::TerrainSystemServiceRequestBus::Handler::BusConnect();

        Terrain::TerrainAreaRequestBus::Broadcast(&Terrain::TerrainAreaRequestBus::Events::RegisterArea);
        
        if (isInstantiated)
        {
            AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(&AzFramework::Terrain::TerrainDataNotifications::OnTerrainDataCreateEnd);
        }
    }

    void LegacyTerrainEditorLevelComponent::Deactivate()
    {
        Terrain::TerrainSystemServiceRequestBus::Handler::BusDisconnect();

        bool isInstantiated = false;
        LegacyTerrainInstanceRequestBus::BroadcastResult(isInstantiated, &LegacyTerrainInstanceRequests::IsTerrainSystemInstantiated);
        if (!isInstantiated)
        {
            return;
        }

        AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(&AzFramework::Terrain::TerrainDataNotifications::OnTerrainDataDestroyBegin);

        //Before removing the terrain system from memory, it is important to make sure
        //there are no pending culling jobs because removing the terrain causes the Octree culling jobs
        //to recalculate and those jobs may access Octree nodes that don't exist anymore.
        GetIEditor()->Get3DEngine()->WaitForCullingJobsCompletion();

        // Make sure we use the Editor version of the bus so that both the Editor and the runtime versions of terrain know that we're destroying
        // the terrain.
        LegacyTerrainEditorDataRequestBus::Broadcast(&LegacyTerrainEditorDataRequests::DestroyTerrainSystem);

        AzFramework::Terrain::TerrainDataNotificationBus::Broadcast(&AzFramework::Terrain::TerrainDataNotifications::OnTerrainDataDestroyEnd);
    }

    AZ::u32 LegacyTerrainEditorLevelComponent::ConfigurationChanged()
    {
        return BaseClassType::ConfigurationChanged();
    }


    void LegacyTerrainEditorLevelComponent::RegisterArea(AZ::EntityId areaId)
    {
        AZ::Aabb aabb = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(aabb, areaId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        m_registeredAreas[areaId] = aabb;
        RefreshArea(areaId);
    }

    void LegacyTerrainEditorLevelComponent::UnregisterArea(AZ::EntityId areaId)
    {
        AZ::Aabb aabb = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(aabb, areaId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        m_registeredAreas.erase(areaId);
        RefreshArea(areaId);
    }

    void LegacyTerrainEditorLevelComponent::RefreshArea(AZ::EntityId areaId)
    {
        AZ::Aabb oldAabb = m_registeredAreas[areaId];
        AZ::Aabb newAabb = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(newAabb, areaId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        m_registeredAreas[areaId] = newAabb;

        AZ::Aabb expandedAabb = oldAabb;
        expandedAabb.AddAabb(newAabb);

        IEditor* editor = nullptr;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::GetEditor);
        CHeightmap* heightmap = editor->GetHeightmap();

        int unitSize = heightmap->GetUnitSize();
        float zMax = heightmap->GetMaxHeight();

        int xMin = max(0, (int)expandedAabb.GetMin().GetX());
        int yMin = max(0, (int)expandedAabb.GetMin().GetY());

        int yMax = min((int)heightmap->GetHeight() * unitSize, (int)expandedAabb.GetMax().GetY());
        int xMax = min((int)heightmap->GetWidth() * unitSize, (int)expandedAabb.GetMax().GetX());

        for (int y = yMin; y < yMax; y++)
        {
            for (int x = xMin; x < xMax; x++)
            {
                AZ::Vector3 inPosition((float)x, (float)y, 0.0f);
                AZ::Vector3 outPosition((float)x, (float)y, 0.0f);

                for (auto& entry : m_registeredAreas)
                {
                    Terrain::TerrainAreaHeightRequestBus::Event(entry.first, &Terrain::TerrainAreaHeightRequestBus::Events::GetHeight, inPosition, outPosition, Terrain::TerrainAreaHeightRequestBus::Events::Sampler::DEFAULT);
                }

                // Flip y and x because heightmaps are stored with a different rotation than the rest of the engine. :(
                heightmap->SetXY(y / unitSize, x / unitSize, AZ::GetClamp((float)outPosition.GetZ(), 0.0f, zMax));
            }
        }

        editor->SetModifiedFlag();
        editor->SetModifiedModule(eModifiedTerrain);
        heightmap->UpdateEngineTerrain(xMin, yMin, xMax, yMax, true, false);
        //editor->GetGameEngine()->ReloadEnvironment();
        editor->UpdateViews(eUpdateHeightmap);
        //InvalidateViewport();
        //Physics::EditorTerrainComponentRequestsBus::Broadcast(&Physics::EditorTerrainComponentRequests::UpdateHeightFieldAsset);


    }
}
