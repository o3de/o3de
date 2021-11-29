/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiCanvasManager.h"
#include <LyShine/Draw2d.h>

#include "UiCanvasFileObject.h"
#include "UiCanvasComponent.h"
#include "UiGameEntityContext.h"

#include <CryCommon/StlUtils.h>
#include <LyShine/UiSerializeHelpers.h>

#include <AzCore/Debug/AssetTracking.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Input/Channels/InputChannel.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <LyShine/Bus/UiCursorBus.h>
#include <LyShine/Bus/World/UiCanvasOnMeshBus.h>
#include <LyShine/Bus/World/UiCanvasRefBus.h>

#include <Atom/RPI.Public/Image/ImageSystemInterface.h>

#ifndef _RELEASE
#include <AzFramework/IO/LocalFileIO.h>
#endif

#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Render/Intersector.h>
#include <MathConversion.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// Anonymous namespace
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Compare function used for sorting
    bool CompareCanvasDrawOrder(UiCanvasComponent* a, UiCanvasComponent* b)
    {
        return a->GetDrawOrder() < b->GetDrawOrder();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Transform the pathname so that a) it works for opening a file that could be in a Gem or in
    // a pak file, and b) so that it is in a consistent form that can be used for string comparison
    AZStd::string GetAssePathFromUserDefinedPath(const AZStd::string& userPath)
    {
        if (userPath.empty())
        {
            AZ_Warning("UI", false, "Given UI canvas path is empty.")
            return userPath;
        }

        AZStd::string assetPath = userPath;

        // Check if extension needs to be fixed up
        const AZStd::string canvasExtension("uicanvas");
        bool validExtension = AzFramework::StringFunc::Path::IsExtension(assetPath.c_str(), canvasExtension.c_str(), true);
        if (!validExtension)
        {
            // Fix extension
            AZ_Warning("UI", !AzFramework::StringFunc::Path::HasExtension(assetPath.c_str()), "Given UI canvas path \"%s\" has an invalid extension. Replacing extension with \"%s\".",
                userPath.c_str(), canvasExtension.c_str());
            AzFramework::StringFunc::Path::ReplaceExtension(assetPath, canvasExtension.c_str());
        }

        // Normalize path
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, assetPath);

        // Check for any leading slashes as the specified path should be a relative path to the @products@ alias.
        // This eliminates inconsistencies between lower level file opens on different platforms
        int numCharsToErase = 0;
        for (; numCharsToErase < assetPath.length(); ++numCharsToErase)
        {
            if (assetPath[numCharsToErase] != '/')
            {
                break;
            }
        }

        if (numCharsToErase > 0)
        {
            // Remove leading slashes
            AZ_Warning("UI", false, "Given UI canvas path \"%s\" has invalid leading slashes that make the path not relative. "
                "Removing the invalid leading slashes.", userPath.c_str());
            assetPath.erase(0, numCharsToErase);
        }

        return assetPath;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasManager::UiCanvasManager()
    : m_latestViewportSize(UiCanvasComponent::s_defaultCanvasSize)
    , m_localUserIdInputFilter(AzFramework::LocalUserIdAny)
{
    UiCanvasManagerBus::Handler::BusConnect();
    UiCanvasOrderNotificationBus::Handler::BusConnect();
    UiCanvasEnabledStateNotificationBus::Handler::BusConnect();
    FontNotificationBus::Handler::BusConnect();
    AzFramework::AssetCatalogEventBus::Handler::BusConnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasManager::~UiCanvasManager()
{
    UiCanvasManagerBus::Handler::BusDisconnect();
    UiCanvasOrderNotificationBus::Handler::BusDisconnect();
    UiCanvasEnabledStateNotificationBus::Handler::BusDisconnect();
    FontNotificationBus::Handler::BusDisconnect();
    AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();

    // destroy ALL the loaded canvases, whether loaded in game or in Editor
    for (auto canvas : (m_loadedCanvases))
    {
        delete canvas->GetEntity();
    }

    for (auto canvas : (m_loadedCanvasesInEditor))
    {
        delete canvas->GetEntity();
    }

    AZ_Assert(m_recursionGuardCount == 0, "Destroying the UiCanvasManager while it is processing canvases");
    DeleteCanvasesQueuedForDeletion();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasManager::CreateCanvas()
{
    // Prevent in-game canvas from being created when we are in the editor in a simulation mode
    // but not in game mode (ex. AI/Physics mode or Preview mode)
    if (gEnv && gEnv->IsEditor() && gEnv->IsEditing())
    {
        return AZ::EntityId();
    }

    UiGameEntityContext* entityContext = new UiGameEntityContext();

    UiCanvasComponent* canvasComponent = UiCanvasComponent::CreateCanvasInternal(entityContext, false);

    m_loadedCanvases.push_back(canvasComponent);
    SortCanvasesByDrawOrder();

    // The game entity context needs to know its corresponding canvas entity for instantiating dynamic slices
    entityContext->SetCanvasEntity(canvasComponent->GetEntityId());

    // When we create a canvas in game we want it to have the correct viewport size from the first frame rather
    // than having to wait a frame to have it updated
    canvasComponent->SetTargetCanvasSize(true, m_latestViewportSize);

    return canvasComponent->GetEntityId();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasManager::LoadCanvas(const AZStd::string& assetIdPathname)
{
    // Prevent canvas from being loaded when we are in the editor in a simulation mode
    // but not in game mode (ex. AI/Physics mode or Preview mode).
    // NOTE: Normal Preview mode load does not come through here since we clone the canvas rather than load it
    if (gEnv && gEnv->IsEditor() && gEnv->IsEditing())
    {
        return AZ::EntityId();
    }

    AZ_ASSET_NAMED_SCOPE(assetIdPathname.c_str());

    UiGameEntityContext* entityContext = new UiGameEntityContext();

    AZ::EntityId canvasEntityId = LoadCanvasInternal(assetIdPathname, false, "", entityContext);

    if (!canvasEntityId.IsValid())
    {
        delete entityContext;
    }
    else
    {
        // The game entity context needs to know its corresponding canvas entity for instantiating dynamic slices
        entityContext->SetCanvasEntity(canvasEntityId);

        EBUS_EVENT(UiCanvasManagerNotificationBus, OnCanvasLoaded, canvasEntityId);
    }

    return canvasEntityId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::UnloadCanvas(AZ::EntityId canvasEntityId)
{
    ReleaseCanvasDeferred(canvasEntityId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasManager::FindLoadedCanvasByPathName(const AZStd::string& assetIdPathName, bool loadIfNotFound)
{
    // this is only used for finding canvases loaded in game
    UiCanvasComponent* canvasComponent = FindCanvasComponentByPathname(assetIdPathName.c_str());
    AZ::EntityId canvasId = canvasComponent ? canvasComponent->GetEntityId() : AZ::EntityId();
    if (!canvasId.IsValid() && loadIfNotFound)
    {
        canvasId = LoadCanvas(assetIdPathName);
    }
    return canvasId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasManager::CanvasEntityList UiCanvasManager::GetLoadedCanvases()
{
    CanvasEntityList list;
    for (auto canvasComponent : m_loadedCanvases)
    {
        list.push_back(canvasComponent->GetEntityId());
    }

    return list;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::SetLocalUserIdInputFilterForAllCanvases(AzFramework::LocalUserId localUserId)
{
    m_localUserIdInputFilter = localUserId;
    for (auto canvasComponent : m_loadedCanvases)
    {
        canvasComponent->SetLocalUserIdInputFilter(m_localUserIdInputFilter);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::OnCanvasDrawOrderChanged([[maybe_unused]] AZ::EntityId canvasEntityId)
{
    SortCanvasesByDrawOrder();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::OnCanvasEnabledStateChanged(AZ::EntityId canvasEntityId, bool enabled)
{
    if (enabled)
    {
        bool isConsumingAllInputEvents = false;
        EBUS_EVENT_ID_RESULT(isConsumingAllInputEvents, canvasEntityId, UiCanvasBus, GetIsConsumingAllInputEvents);
        if (isConsumingAllInputEvents)
        {
            AzFramework::InputChannelRequestBus::Broadcast(&AzFramework::InputChannelRequests::ResetState);
            EBUS_EVENT(UiCanvasBus, ClearAllInteractables);
        }
    }

    // Update hover state for loaded canvases
    m_generateMousePositionInputEvent = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::GenerateMousePositionInputEvent()
{
    const AzFramework::InputDevice* mouseDevice = AzFramework::InputDeviceRequests::FindInputDevice(AzFramework::InputDeviceMouse::Id);
    if (mouseDevice && mouseDevice->IsConnected())
    {
        // Create a game input event for the system cursor position
        const AzFramework::InputChannel::Snapshot inputSnapshot(AzFramework::InputDeviceMouse::SystemCursorPosition,
            AzFramework::InputDeviceMouse::Id,
            AzFramework::InputChannel::State::Updated);

        // Get the current system cursor viewport position
        AZ::Vector2 systemCursorPositionNormalized(0.0f, 0.0f);
        AzFramework::InputSystemCursorRequestBus::EventResult(systemCursorPositionNormalized,
            AzFramework::InputDeviceMouse::Id,
            &AzFramework::InputSystemCursorRequests::GetSystemCursorPositionNormalized);
        AZ::Vector2 cursorViewportPos(systemCursorPositionNormalized.GetX() * m_latestViewportSize.GetX(),
            systemCursorPositionNormalized.GetY() * m_latestViewportSize.GetY());

        // Handle the input event
        HandleInputEventForLoadedCanvases(inputSnapshot, cursorViewportPos, AzFramework::ModifierKeyMask::None, true);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::OnFontsReloaded()
{
    m_fontTextureHasChanged = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::OnFontTextureUpdated([[maybe_unused]] IFFont* font)
{
    m_fontTextureHasChanged = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::GetRenderTargets(LyShine::AttachmentImagesAndDependencies& attachmentImagesAndDependencies)
{
    for (auto canvas : m_loadedCanvases)
    {
        LyShine::AttachmentImagesAndDependencies canvasTargets;
        canvas->GetRenderTargets(canvasTargets);
        attachmentImagesAndDependencies.insert(attachmentImagesAndDependencies.end(), canvasTargets.begin(), canvasTargets.end());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
{
    // get AssetInfo from asset id
    AZ::Data::AssetInfo assetInfo;
    EBUS_EVENT_RESULT(assetInfo, AZ::Data::AssetCatalogRequestBus, GetAssetInfoById, assetId);
    if (assetInfo.m_assetType != LyShine::CanvasAsset::TYPEINFO_Uuid())
    {
        // this is not a UI canvas asset
        return;
    }

    // get pathname from asset id
    AZStd::string assetPath;
    EBUS_EVENT_RESULT(assetPath, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, assetId);

    CanvasList reloadedCanvases;  // keep track of the reloaded canvases and add them to m_loadedCanvases after the loop
    AZStd::vector<AZ::EntityId> unloadedCanvases;  // also keep track of any canvases that fail to reload and are unloaded

    // loop over all canvases loaded in game and reload any canvases loaded from this canvas asset
    // NOTE: this could be improved by using AssetId for the comparison rather than pathnames
    auto iter = m_loadedCanvases.begin();
    while (iter != m_loadedCanvases.end())
    {
        UiCanvasComponent* canvasComponent = *iter;
        if (assetPath == canvasComponent->GetPathname())
        {
            AZ::EntityId existingCanvasEntityId = canvasComponent->GetEntityId();

            //Before unloading the existing canvas, make a copy of its mapping table
            AZ::SliceComponent::EntityIdToEntityIdMap existingRemapTable = canvasComponent->GetEditorToGameEntityIdMap();

            // unload the canvas, just deleting the canvas entity does this
            AZ::Entity* existingCanvasEntity = canvasComponent->GetEntity();
            delete existingCanvasEntity;

            // Remove the deleted canvas entry in the m_loadedCanvases vector and move the iterator to the next
            iter = m_loadedCanvases.erase(iter);

            // reload canvas with the same entity IDs (except for new entities, deleted entities etc)
            UiGameEntityContext* entityContext = new UiGameEntityContext();
            AZStd::string pathname(assetPath.c_str());
            UiCanvasComponent* newCanvasComponent = UiCanvasComponent::LoadCanvasInternal(pathname, false, "", entityContext, &existingRemapTable, existingCanvasEntityId);

            if (!newCanvasComponent)
            {
                delete entityContext;
                unloadedCanvases.push_back(existingCanvasEntityId);
            }
            else
            {
                // The game entity context needs to know its corresponding canvas entity for instantiating dynamic slices
                entityContext->SetCanvasEntity(newCanvasComponent->GetEntityId());
                reloadedCanvases.push_back(newCanvasComponent);
            }
        }
        else
        {
            ++iter; // we did not delete this canvas - move to next in list
        }
    }

    // add the successfully reloaded canvases at the end
    m_loadedCanvases.insert(m_loadedCanvases.end(), reloadedCanvases.begin(), reloadedCanvases.end());

    // In case any draw orders changed resort
    SortCanvasesByDrawOrder();

    // notify any listeners of any UI canvases that were reloaded
    for (auto reloadedCanvasComponent : reloadedCanvases)
    {
        EBUS_EVENT(UiCanvasManagerNotificationBus, OnCanvasReloaded, reloadedCanvasComponent->GetEntityId());
    }

    // notify any listeners of any UI canvases that were unloaded
    for (auto unloadedCanvas : unloadedCanvases)
    {
        EBUS_EVENT(UiCanvasManagerNotificationBus, OnCanvasUnloaded, unloadedCanvas);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasManager::CreateCanvasInEditor(UiEntityContext* entityContext)
{
    UiCanvasComponent* canvasComponent = UiCanvasComponent::CreateCanvasInternal(entityContext, true);

    m_loadedCanvasesInEditor.push_back(canvasComponent);

    return canvasComponent->GetEntityId();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasManager::LoadCanvasInEditor(const AZStd::string& assetIdPathname, const AZStd::string& sourceAssetPathname, UiEntityContext* entityContext)
{
    return LoadCanvasInternal(assetIdPathname.c_str(), true, sourceAssetPathname.c_str(), entityContext);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasManager::ReloadCanvasFromXml(const AZStd::string& xmlString, UiEntityContext* entityContext)
{
    // Load new canvas from xml
    AZ::IO::MemoryStream memoryStream(xmlString.c_str(), xmlString.size());
    AZ::Entity* rootSliceEntity = nullptr;
    AZ::Entity* newCanvasEntity = UiCanvasFileObject::LoadCanvasEntitiesFromStream(memoryStream, rootSliceEntity);
    if (newCanvasEntity && rootSliceEntity)
    {
        // Find the old canvas to replace
        UiCanvasComponent* oldCanvasComponent = nullptr;
        for (auto canvas : (m_loadedCanvasesInEditor))
        {
            if (canvas->GetEntityId() == newCanvasEntity->GetId())
            {
                oldCanvasComponent = canvas;
                break;
            }
        }

        AZ_Assert(oldCanvasComponent, "Canvas not found");
        if (oldCanvasComponent)
        {
            LyShine::CanvasId oldCanvasId = oldCanvasComponent->GetCanvasId();
            const AZStd::string oldPathname = oldCanvasComponent->GetPathname();
            AZ::Matrix4x4 oldCanvasToViewportMatrix = oldCanvasComponent->GetCanvasToViewportMatrix();

            // Delete the old canvas. We assume this is for editor
            ReleaseCanvas(oldCanvasComponent->GetEntityId(), true);

            // Complete initialization of new canvas. We assume this is for editor
            UiCanvasComponent* newCanvasComponent = UiCanvasComponent::FixupReloadedCanvasForEditorInternal(
                    newCanvasEntity, rootSliceEntity, entityContext, oldCanvasId, oldPathname);

            newCanvasComponent->SetCanvasToViewportMatrix(oldCanvasToViewportMatrix);

            // Add new canvas to the list of loaded canvases
            m_loadedCanvasesInEditor.push_back(newCanvasComponent);

            return newCanvasComponent->GetEntityId();
        }
        else
        {
            delete newCanvasEntity;
        }
    }

    return AZ::EntityId();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::ReleaseCanvas(AZ::EntityId canvasEntityId, bool forEditor)
{
    if(!canvasEntityId.IsValid())
    {
        AZ_Warning("UI", false, "%s has been invoked with an Invalid Canvas Entity ID. No Canvas can be released", AZ_FUNCTION_SIGNATURE);
        return;
    }

    // if we are currently processing canvases for input handling or update then defer the deletion of the canvas
    if (!forEditor && m_recursionGuardCount > 0)
    {
        ReleaseCanvasDeferred(canvasEntityId);
        return;
    }

    AZ::Entity* canvasEntity = nullptr;
    EBUS_EVENT_RESULT(canvasEntity, AZ::ComponentApplicationBus, FindEntity, canvasEntityId);
    AZ_Assert(canvasEntity, "Canvas entity not found by ID");

    if (canvasEntity)
    {
        UiCanvasComponent* canvasComponent = canvasEntity->FindComponent<UiCanvasComponent>();
        AZ_Assert(canvasComponent, "Canvas entity has no canvas component");

        if (canvasComponent)
        {
            if (forEditor)
            {
                stl::find_and_erase(m_loadedCanvasesInEditor, canvasComponent);
                delete canvasEntity;
            }
            else
            {
                stl::find_and_erase(m_loadedCanvases, canvasComponent);
                delete canvasEntity;

                EBUS_EVENT(UiCanvasManagerNotificationBus, OnCanvasUnloaded, canvasEntityId);

                // Update hover state for loaded canvases
                m_generateMousePositionInputEvent = true;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::ReleaseCanvasDeferred(AZ::EntityId canvasEntityId)
{
    if (!canvasEntityId.IsValid())
    {
        AZ_Warning("UI", false, "%s has been invoked with an Invalid Canvas Entity ID. No Canvas can be released", AZ_FUNCTION_SIGNATURE);
        return;
    }
    AZ::Entity* canvasEntity = nullptr;
    EBUS_EVENT_RESULT(canvasEntity, AZ::ComponentApplicationBus, FindEntity, canvasEntityId);
    AZ_Assert(canvasEntity, "Canvas entity not found by ID");

    if (canvasEntity)
    {
        UiCanvasComponent* canvasComponent = canvasEntity->FindComponent<UiCanvasComponent>();
        AZ_Assert(canvasComponent, "Canvas entity has no canvas component");

        if (canvasComponent)
        {
            // Remove canvas component from list of loaded canvases
            stl::find_and_erase(m_loadedCanvases, canvasComponent);

            // Deactivate elements of the canvas
            canvasComponent->DeactivateElements();

            // Deactivate the canvas element
            canvasEntity->Deactivate();

            EBUS_EVENT(UiCanvasManagerNotificationBus, OnCanvasUnloaded, canvasEntityId);

            // Queue UI canvas deletion. This is because this function could have been triggered in input processing of
            // a component within the canvas. i.e. there could be a member function of the canvas or one of it's child entities
            // on the callstack. Unfortunately, just delaying until the next tick is not enough - pressing a button could cause
            // unloading of an entire level which could flush the tick bus. So we have to use our own queue.
            QueueCanvasForDeletion(canvasEntityId);

            // Update hover state for loaded canvases
            m_generateMousePositionInputEvent = true;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasManager::FindCanvasById(LyShine::CanvasId id)
{
    // this is only used for finding canvases loaded in game
    for (auto canvas : m_loadedCanvases)
    {
        if (canvas->GetCanvasId() == id)
        {
            return canvas->GetEntityId();
        }
    }
    return AZ::EntityId();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::SetTargetSizeForLoadedCanvases(AZ::Vector2 viewportSize)
{
    for (auto canvas : m_loadedCanvases)
    {
        canvas->SetTargetCanvasSize(true, viewportSize);
    }

    m_latestViewportSize = viewportSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::UpdateLoadedCanvases(float deltaTime)
{
    // make a temporary copy of the list in case the update code ends up releasing or loading canvases during iterating over the list
    auto loadedCanvases = m_loadedCanvases;

    // Update all the canvases loaded in game.
    // It is unlikely this will call out to client code that could remove a canvas but we have no
    // control over what custom components do so we increment the count that will defer all canvas deletion
    m_recursionGuardCount++;
    if (m_generateMousePositionInputEvent)
    {
        // Update hover state for loaded canvases
        m_generateMousePositionInputEvent = false;
        GenerateMousePositionInputEvent();
    }
    for (auto canvas : loadedCanvases)
    {
        canvas->UpdateCanvas(deltaTime, true);
    }
    m_recursionGuardCount--;

    // If not being called recursively from other canvas processing then immediately do any deferred canvas deletes
    DeleteCanvasesQueuedForDeletion();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::RenderLoadedCanvases()
{
    // Render all the canvases loaded in game
    // canvases loaded in editor are rendered by the viewport window

    // If any font texture has changed we force all canvases to rebuild the render graph. Individual text components
    // that use this font will also have got the notification and will have set a flag in their render cache
    // to indicate that the font texture has changed. This allows them to regenerate the quads with no reallocation.
    if (m_fontTextureHasChanged)
    {
        for (auto canvas : m_loadedCanvases)
        {
            canvas->MarkRenderGraphDirty();
        }

        m_fontTextureHasChanged = false;
    }

    for (auto canvas : m_loadedCanvases)
    {
        if (!canvas->GetIsRenderToTexture())
        {
            // Rendering in game full screen so the viewport size and target canvas size are the same
            AZ::Vector2 viewportSize = canvas->GetTargetCanvasSize();

            canvas->RenderCanvas(true, viewportSize);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::DestroyLoadedCanvases(bool keepCrossLevelCanvases)
{
    // Delete all the canvases loaded in game (but not loaded in editor)
    for (auto iter = m_loadedCanvases.begin(); iter != m_loadedCanvases.end(); ++iter)
    {
        auto canvas = *iter;

        if (!(keepCrossLevelCanvases && canvas->GetKeepLoadedOnLevelUnload()))
        {
            // no longer used by game so delete the canvas
            delete canvas->GetEntity();
            *iter = nullptr;    // mark for removal from container
        }
    }

    // now remove the nullptr entries
    m_loadedCanvases.erase(
        std::remove(m_loadedCanvases.begin(), m_loadedCanvases.end(), nullptr),
        m_loadedCanvases.end());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::OnLoadScreenUnloaded()
{
    // Mark all render graphs dirty in case the loaded canvases were already rendered before their textures were
    // done loading. This happens when a load screen is being rendered during a level load. When other canvases
    // associated with the level are loaded, they also get rendered by the UiLoadScreenComponent, but their texture
    // loading is delayed until further down the level load process. Once a canvas is rendered, its render graph's
    // dirty flag is cleared, so the render graph needs to be marked dirty again after the textures are loaded
    for (auto canvas : m_loadedCanvases)
    {
        canvas->MarkRenderGraphDirty();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasManager::HandleInputEventForLoadedCanvases(const AzFramework::InputChannel& inputChannel)
{
    // Take a snapshot of the input channel instead of just passing through the channel itself.
    // This is necessary because UI input is currently simulated in the editor's UI Preview mode
    // by constructing 'fake' input events, which we can do with snapshots but not input channels.
    // Long term we should look to update the AzFramework input system while in UI Editor Preview
    // mode so that it works exactly the same as in-game input, but this is a larger task for later.
    const AzFramework::InputChannel::Snapshot inputSnapshot(inputChannel);

    // De-normalize the position (if any) of the input event, as the UI system expects it relative
    // to the viewport from here on.
    AZ::Vector2 viewportPos(0.0f, 0.0f);
    const AzFramework::InputChannel::PositionData2D* positionData2D = inputChannel.GetCustomData<AzFramework::InputChannel::PositionData2D>();
    if (positionData2D)
    {
        viewportPos.SetX(positionData2D->m_normalizedPosition.GetX() * m_latestViewportSize.GetX());
        viewportPos.SetY(positionData2D->m_normalizedPosition.GetY() * m_latestViewportSize.GetY());
    }

    // Get the active modifier keys (if any) of the input event. Will only exist for keyboard keys.
    const AzFramework::ModifierKeyStates* modifierKeyStates = inputChannel.GetCustomData<AzFramework::ModifierKeyStates>();
    const AzFramework::ModifierKeyMask activeModifierKeys = modifierKeyStates ?
        modifierKeyStates->GetActiveModifierKeys() :
        AzFramework::ModifierKeyMask::None;

    bool handled = HandleInputEventForLoadedCanvases(inputSnapshot, viewportPos, activeModifierKeys, positionData2D ? true : false);
    return handled;
}

bool UiCanvasManager::HandleInputEventForLoadedCanvases(const AzFramework::InputChannel::Snapshot& inputSnapshot,
    const AZ::Vector2& viewportPos,
    AzFramework::ModifierKeyMask activeModifierKeys,
    bool isPositional)
{
    bool handled = false;

    if (isPositional)
    {
        m_generateMousePositionInputEvent = false;
    }

    // make a temporary copy of the list in case the input handling ends up releasing or loading canvases during iterating over the list
    auto loadedCanvases = m_loadedCanvases;

    // reverse iterate over the loaded canvases so that the front most canvas gets first chance to
    // handle the event
    bool areAnyInWorldInputCanvasesLoaded = false;

    // HandleInputEvent is likely to call user code and scripts that could potentially cause a canvas to be released.
    // Setting this flag will cause any canvas deletions to be deferred. Due to the weird behavior when switching levels this function
    // (HandleInputEventForLoadedCanvases) can actually be called recursively because it can flush the input events.
    m_recursionGuardCount++;
    for (auto iter = loadedCanvases.rbegin(); iter != loadedCanvases.rend(); ++iter)
    {
        UiCanvasComponent* canvas = *iter;

        if (canvas->GetIsRenderToTexture() && canvas->GetIsPositionalInputSupported())
        {
            // keep track of whether any canvases are rendering to texture. Positional events for these
            // are ignored in HandleInputEvent and handled later in this function by HandleInputEventForInWorldCanvases
            areAnyInWorldInputCanvasesLoaded = true;
        }

        if (canvas->HandleInputEvent(inputSnapshot, &viewportPos, activeModifierKeys))
        {
            handled = true;
            break;
        }
    }
    m_recursionGuardCount--;

    // If not being called recursively from other canvas processing then immediately do any deferred canvas deletes
    DeleteCanvasesQueuedForDeletion();

    // if there are any canvases loaded that are rendering to texture we handle them seperately after the screen canvases
    // only do this for input events that are actually associated with a position
    if (!handled && areAnyInWorldInputCanvasesLoaded && isPositional)
    {
        if (HandleInputEventForInWorldCanvases(inputSnapshot, viewportPos))
        {
            handled = true;
        }
    }

    return handled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasManager::HandleTextEventForLoadedCanvases(const AZStd::string& textUTF8)
{
    // reverse iterate over the loaded canvases so that the front most canvas gets first chance to
    // handle the event
    for (auto iter = m_loadedCanvases.rbegin(); iter != m_loadedCanvases.rend(); ++iter)
    {
        if ((*iter)->HandleTextEvent(textUTF8))
        {
            return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::SortCanvasesByDrawOrder()
{
    std::stable_sort(m_loadedCanvases.begin(), m_loadedCanvases.end(), CompareCanvasDrawOrder);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasComponent* UiCanvasManager::FindCanvasComponentByPathname(const AZStd::string& name)
{
    AZStd::string adjustedSearchName = GetAssePathFromUserDefinedPath(name);
    for (auto canvas : (m_loadedCanvases))
    {
        if (adjustedSearchName == canvas->GetPathname())
        {
            return canvas;
        }
    }

    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasComponent* UiCanvasManager::FindEditorCanvasComponentByPathname(const AZStd::string& name)
{
    for (auto canvas : (m_loadedCanvasesInEditor))
    {
        if (name == canvas->GetPathname())
        {
            return canvas;
        }
    }

    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasManager::HandleInputEventForInWorldCanvases([[maybe_unused]] const AzFramework::InputChannel::Snapshot& inputSnapshot, [[maybe_unused]] const AZ::Vector2& viewportPos)
{
    // First we need to construct a ray from the either the center of the screen or the mouse position.
    // This requires knowledge of the camera
    // for initial testing we will just use a ray in the center of the viewport

    // ToDo: Re-implement by getting the camera from Atom. LYN-3680
    return false;
    //const CCamera cam;
    //
    //// construct a ray from the camera position in the view direction of the camera
    //const float rayLength = 5000.0f;
    //Vec3 rayOrigin(cam.GetPosition());
    //Vec3 rayDirection = cam.GetViewdir() * rayLength;
    //
    //// If the mouse cursor is visible we will assume that the ray should be in the direction of the
    //// mouse pointer. This is a temporary solution. A better solution is to be able to configure the
    //// LyShine system to say how ray input should be handled.
    //bool isCursorVisible = false;
    //UiCursorBus::BroadcastResult(isCursorVisible, &UiCursorInterface::IsUiCursorVisible);
    //if (isCursorVisible)
    //{
    //    // for some reason Unproject seems to work when given the viewport pos with (0,0) at the
    //    // bottom left as opposed to the top left - even though that function specifically sets top left
    //    // to (0,0).
    //    const float viewportYInverted = cam.GetViewSurfaceZ() - viewportPos.GetY();
    //
    //    // Unproject to get the screen position in world space, use arbitrary Z that is within the depth range
    //    Vec3 flippedViewportRayOrigin(viewportPos.GetX(), viewportYInverted, 0.f);
    //    Vec3 flippedViewportRayForward(viewportPos.GetX(), viewportYInverted, 1.f);
    //
    //    cam.Unproject(flippedViewportRayOrigin, rayOrigin);
    //
    //    Vec3 unprojectedPosForward;
    //    cam.Unproject(flippedViewportRayForward, unprojectedPosForward);
    //
    //    // We want a vector relative to the camera origin
    //    Vec3 rayVec = unprojectedPosForward - rayOrigin;
    //
    //    // we want to ensure that the ray is a certain length so normalize it and scale it
    //    rayVec.NormalizeSafe();
    //    rayDirection = rayVec * rayLength;
    //}
    //
    //
    //AzFramework::EntityContextId gameContextId;
    //AzFramework::GameEntityContextRequestBus::BroadcastResult(gameContextId,
    //    &AzFramework::GameEntityContextRequests::GetGameEntityContextId);
    //
    //AzFramework::RenderGeometry::RayRequest request;
    //request.m_startWorldPosition = LYVec3ToAZVec3(rayOrigin);
    //request.m_endWorldPosition = LYVec3ToAZVec3(rayOrigin + rayDirection);
    //
    //AzFramework::RenderGeometry::RayResult rayResult;
    //AzFramework::RenderGeometry::IntersectorBus::EventResult(rayResult, gameContextId,
    //    &AzFramework::RenderGeometry::IntersectorInterface::RayIntersect, request);
    //
    //if (rayResult)
    //{
    //    AZ::EntityId hitEntity = rayResult.m_entityAndComponent.GetEntityId();
    //    if (hitEntity.IsValid())
    //    {
    //        AZ::EntityId canvasEntityId;
    //        UiCanvasRefBus::EventResult(canvasEntityId, hitEntity, &UiCanvasRefInterface::GetCanvas);
    //        if (canvasEntityId.IsValid())
    //        {
    //            // Checkif the UI canvas referenced by the hit entity supports automatic input
    //            bool doesCanvasSupportInput = false;
    //            UiCanvasBus::EventResult(doesCanvasSupportInput, canvasEntityId, &UiCanvasInterface::GetIsPositionalInputSupported);
    //
    //            if (doesCanvasSupportInput)
    //            {
    //                // set the hit details to the hit entity, it will convert into canvas coords and send to canvas
    //                bool handled = false;
    //                UiCanvasOnMeshBus::EventResult(handled, hitEntity,
    //                    &UiCanvasOnMeshInterface::ProcessHitInputEvent, inputSnapshot, rayResult);
    //
    //                if (handled)
    //                {
    //                    return true;
    //                }
    //            }
    //        }
    //    }
    //}
    //
    //
    //return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiCanvasManager::LoadCanvasInternal(const AZStd::string& assetIdPathname, bool forEditor, const AZStd::string& fullSourceAssetPathname, UiEntityContext* entityContext,
    const AZ::SliceComponent::EntityIdToEntityIdMap* previousRemapTable, AZ::EntityId previousCanvasId)
{
    // Fix up user defined asset path for runtime. For editor, the asset path should already be valid
    AZStd::string assetIdPath = forEditor ? assetIdPathname : GetAssePathFromUserDefinedPath(assetIdPathname);

    // If loading from the editor we load the source asset path.
    // If loading in game this could be a path that a developer typed into a script.
    // However, it has to be a valid asset ID path. E.g. it can be resolved from the asset root
    // since at runtime we cannot convert from an arbitrary dev asset path to an asset ID
    const AZStd::string& pathToOpen = forEditor ? fullSourceAssetPathname : assetIdPath;

    // if the canvas is already loaded in the editor and we are running in game then we clone the
    // editor version so that the user can test their canvas without saving it
    UiCanvasComponent* canvasComponent = FindEditorCanvasComponentByPathname(assetIdPath);

    // This scope opened here intentionally to control the lifetime of the AZ_ASSET_NAMED_SCOPE
    {
        AZ_ASSET_NAMED_SCOPE(pathToOpen.c_str());
        if (canvasComponent)
        {
            // this canvas is already loaded in the editor
            if (forEditor)
            {
                // should never load a canvas in Editor if it is already loaded. The Editor should avoid loading the
                // same canvas twice in Editor. If the game is running it is not possible to load a canvas
                // from the editor.
                gEnv->pSystem->Warning(VALIDATOR_MODULE_SHINE, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE,
                    pathToOpen.c_str(),
                    "UI canvas file: %s is already loaded",
                    pathToOpen.c_str());
                return AZ::EntityId();
            }
            else
            {
                // we are loading from the game, the canvas is already open in the editor, so
                // we clone the canvas that is open in the editor.
                canvasComponent = canvasComponent->CloneAndInitializeCanvas(entityContext, assetIdPath);
            }
        }
        else
        {
            // not already loaded in editor, attempt to load...
            canvasComponent = UiCanvasComponent::LoadCanvasInternal(pathToOpen.c_str(), forEditor, assetIdPath.c_str(), entityContext, previousRemapTable, previousCanvasId);
        }

        if (canvasComponent)
        {
            // canvas loaded OK (or cloned from Editor canvas OK)

            // add to the list of loaded canvases
            if (forEditor)
            {
                m_loadedCanvasesInEditor.push_back(canvasComponent);
            }
            else
            {
                if (canvasComponent->GetEnabled() && canvasComponent->GetIsConsumingAllInputEvents())
                {
                    AzFramework::InputChannelRequestBus::Broadcast(&AzFramework::InputChannelRequests::ResetState);
                    EBUS_EVENT(UiCanvasBus, ClearAllInteractables);
                }
                m_loadedCanvases.push_back(canvasComponent);
                SortCanvasesByDrawOrder();

                // Update hover state for loaded canvases
                m_generateMousePositionInputEvent = true;
            }
            canvasComponent->SetLocalUserIdInputFilter(m_localUserIdInputFilter);
        }
    }

    return (canvasComponent) ? canvasComponent->GetEntityId() : AZ::EntityId();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::QueueCanvasForDeletion(AZ::EntityId canvasEntityId)
{
    m_canvasesQueuedForDeletion.push_back(canvasEntityId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::DeleteCanvasesQueuedForDeletion()
{
    // In weird cases like level unload HandleInputEventForLoadedCanvases can get called recursively
    // so do not delete any canvases until there is no recursion
    if (m_recursionGuardCount == 0)
    {
        for (auto canvasEntityId : m_canvasesQueuedForDeletion)
        {
            AZ::Entity* canvasEntity = nullptr;
            EBUS_EVENT_RESULT(canvasEntity, AZ::ComponentApplicationBus, FindEntity, canvasEntityId);
            delete canvasEntity;
        }

        m_canvasesQueuedForDeletion.clear();
    }
}

#ifndef _RELEASE
////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::DebugDisplayCanvasData(int setting) const
{
    bool onlyShowEnabledCanvases = (setting == 2) ? true : false;

    CDraw2d* draw2d = Draw2dHelper::GetDefaultDraw2d();

    float xOffset = 20.0f;
    float yOffset = 20.0f;

    const int elementNameFieldLength = 20;

    auto blackTexture = AZ::RPI::ImageSystemInterface::Get()->GetSystemImage(AZ::RPI::SystemImage::Black);

    float textOpacity = 1.0f;
    float backgroundRectOpacity = 0.75f;

    const AZ::Vector3 white(1.0f, 1.0f, 1.0f);
    const AZ::Vector3 grey(0.5f, 0.5f, 0.5f);
    const AZ::Vector3 red(1.0f, 0.3f, 0.3f);
    const AZ::Vector3 blue(0.3f, 0.3f, 1.0f);

    // If the viewport is narrow then a font size of 16 might be too large, so we use a size between 12 and 16 depending
    // on the viewport width.
    float fontSize(draw2d->GetViewportWidth() / 75.f);
    fontSize = AZ::GetClamp(fontSize, 12.f, 16.f);
    const float lineSpacing = fontSize;

    // local function to write a line of text (with a background rect) and increment Y offset
    AZStd::function<void(const char*, const AZ::Vector3&)> WriteLine = [&](const char* buffer, const AZ::Vector3& color)
    {
        CDraw2d::TextOptions textOptions = draw2d->GetDefaultTextOptions();
        textOptions.color = color;
        AZ::Vector2 textSize = draw2d->GetTextSize(buffer, fontSize, &textOptions);
        AZ::Vector2 rectTopLeft = AZ::Vector2(xOffset - 2, yOffset);
        AZ::Vector2 rectSize = AZ::Vector2(textSize.GetX() + 4, lineSpacing);
        draw2d->DrawImage(blackTexture, rectTopLeft, rectSize, backgroundRectOpacity);
        draw2d->DrawText(buffer, AZ::Vector2(xOffset, yOffset), fontSize, textOpacity, &textOptions);
        yOffset += lineSpacing;
    };

    char buffer[200];

    sprintf_s(buffer, "There are %d loaded UI canvases", static_cast<int>(m_loadedCanvases.size()));
    WriteLine(buffer, white);

    sprintf_s(buffer, "NN: %20s %2s %2s %2s %11s %5s %5s %5s %5s %5s %5s %5s %5s %5s %5s %20s %20s",
        "Name", "En", "Po", "Na", "DrawOrder",
        "nElem", "nEnab", "nRend", "nRCtl", "nImg", "nText", "nMask", "nFadr", "nIntr", "nUpdt", "ActiveInt", "HoverInt");
    WriteLine(buffer, white);

    int totalEnabled = 0;
    int totalPositionalInputs = 0;
    int totalNavigable = 0;
    int totalElements = 0;
    int totalEnabledElements = 0;
    int totalEnabledRenderables = 0;
    int totalEnabledRCtls = 0;
    int totalEnabledImages = 0;
    int totalEnabledTexts = 0;
    int totalEnabledMasks = 0;
    int totalEnabledFaders = 0;
    int totalEnabledIntrs = 0;
    int totalEnabledUpdates = 0;

    int i = 0;
    for (auto canvas : m_loadedCanvases)
    {
        // Name
        const AZStd::string pathname = canvas->GetPathname().c_str();
        size_t lastDot = pathname.find_last_of(".");
        size_t lastSlash = pathname.find_last_of("/");
        AZStd::string leafName = pathname;
        if (lastDot > lastSlash)
        {
            leafName = pathname.substr(lastSlash+1, lastDot-lastSlash-1);
        }
        leafName = leafName.substr(0, 20);

        // Enabled
        bool isCanvasEnabled = canvas->GetEnabled();
        if (onlyShowEnabledCanvases && !isCanvasEnabled)
        {
            continue;
        }
        const char* enabledString = isCanvasEnabled ? "Y" : "N";
        totalEnabled += isCanvasEnabled ? 1 : 0;

        bool posEnabled = canvas->GetIsPositionalInputSupported();
        const char* posEnabledString = posEnabled ? "Y" : "N";
        totalPositionalInputs += posEnabled ? 1 : 0;

        bool navEnabled = canvas->GetIsNavigationSupported();
        const char* navEnabledString = navEnabled ? "Y" : "N";
        totalNavigable += navEnabled ? 1 : 0;

        // Draw order
        int drawOrder = canvas->GetDrawOrder();

        // Active and hover
        AZ::EntityId activeInteractableId;
        AZ::EntityId hoverInteractableId;
        canvas->GetDebugInfoInteractables(activeInteractableId, hoverInteractableId);

        AZStd::string activeInteractableName = "None";
        AZStd::string hoverInteractableName = "None";
        if (activeInteractableId.IsValid())
        {
            activeInteractableName = DebugGetElementName(activeInteractableId, elementNameFieldLength);
        }
        if (hoverInteractableId.IsValid())
        {
            hoverInteractableName = DebugGetElementName(hoverInteractableId, elementNameFieldLength);
        }

        // Num elements
        UiCanvasComponent::DebugInfoNumElements info;
        canvas->GetDebugInfoNumElements(info);

        sprintf_s(buffer, "%2d: %20s %2s %2s %2s %11d %5d %5d %5d %5d %5d %5d %5d %5d %5d %5d %20s %20s",
            i, leafName.c_str(),
            enabledString, posEnabledString, navEnabledString,
            drawOrder,
            info.m_numElements, info.m_numEnabledElements,
            info.m_numRenderElements, info.m_numRenderControlElements,
            info.m_numImageElements, info.m_numTextElements,
            info.m_numMaskElements, info.m_numFaderElements,
            info.m_numInteractableElements,info.m_numUpdateElements,
            activeInteractableName.c_str(), hoverInteractableName.c_str());

        const AZ::Vector3& color = isCanvasEnabled ? white : grey;
        WriteLine(buffer, color);

        ++i;

        totalElements += info.m_numElements;
        totalEnabledElements += info.m_numEnabledElements;
        totalEnabledRenderables += info.m_numRenderElements;
        totalEnabledRCtls += info.m_numRenderControlElements;
        totalEnabledImages += info.m_numImageElements;
        totalEnabledTexts += info.m_numTextElements;
        totalEnabledMasks += info.m_numMaskElements;
        totalEnabledFaders += info.m_numFaderElements;
        totalEnabledIntrs += info.m_numInteractableElements;
        totalEnabledUpdates += info.m_numUpdateElements;
    }

    sprintf_s(buffer, "Totals: %16s %2d %2d %2d %11s %5d %5d %5d %5d %5d %5d %5d %5d %5d %5d",
        "",
        totalEnabled, totalPositionalInputs, totalNavigable,
        "",
        totalElements, totalEnabledElements,
        totalEnabledRenderables, totalEnabledRCtls,
        totalEnabledImages, totalEnabledTexts,
        totalEnabledMasks, totalEnabledFaders,
        totalEnabledIntrs, totalEnabledUpdates);

    WriteLine(buffer, red);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::DebugDisplayDrawCallData() const
{
    CDraw2d* draw2d = Draw2dHelper::GetDefaultDraw2d();

    float xOffset = 20.0f;
    float yOffset = 20.0f;

    auto blackTexture = AZ::RPI::ImageSystemInterface::Get()->GetSystemImage(AZ::RPI::SystemImage::Black);
    float textOpacity = 1.0f;
    float backgroundRectOpacity = 0.75f;
    const float lineSpacing = 20.0f;

    const AZ::Vector3 white(1,1,1);
    const AZ::Vector3 red(1,0.3f,0.3f);
    const AZ::Vector3 blue(0.3f,0.3f,1);
    const AZ::Vector3 green(0.3f,1,0.3f);
    const AZ::Vector3 yellow(0.7f,0.7f,0.2f);

    // local function to write a line of text (with a background rect) and increment Y offset
    AZStd::function<void(const char*, const AZ::Vector3&)> WriteLine = [&](const char* buffer, const AZ::Vector3& color)
    {
        CDraw2d::TextOptions textOptions = draw2d->GetDefaultTextOptions();
        textOptions.color = color;
        AZ::Vector2 textSize = draw2d->GetTextSize(buffer, 16, &textOptions);
        AZ::Vector2 rectTopLeft = AZ::Vector2(xOffset - 2, yOffset);
        AZ::Vector2 rectSize = AZ::Vector2(textSize.GetX() + 4, lineSpacing);
        draw2d->DrawImage(blackTexture, rectTopLeft, rectSize, backgroundRectOpacity);
        draw2d->DrawText(buffer, AZ::Vector2(xOffset, yOffset), 16, textOpacity, &textOptions);
        yOffset += lineSpacing;
    };

    char buffer[200];

    sprintf_s(buffer, "NN: %20s %5s   %5s %5s %5s %5s %5s   %5s %5s %5s %5s %5s %5s",
        "Canvas name", "nDraw",   "nPrim", "nTris", "nMask", "nRTs", "nUTex",   "XMask", "XRT", "XBlnd", "XSrgb", "XMaxV", "XTex");
    WriteLine(buffer, blue);

    int totalRenderNodes = 0;
    int totalPrimitives = 0;
    int totalTriangles = 0;
    int totalMasks = 0;
    int totalRTs = 0;
    int totalDueToMask = 0;
    int totalDueToRT = 0;
    int totalDueToBlendMode = 0;
    int totalDueToSrgb = 0;
    int totalDueToMaxVerts = 0;
    int totalDueToTextures = 0;

    int i = 0;
    for (auto canvas : m_loadedCanvases)
    {
        // Name
        const AZStd::string pathname = canvas->GetPathname().c_str();
        size_t lastDot = pathname.find_last_of(".");
        size_t lastSlash = pathname.find_last_of("/");
        AZStd::string leafName = pathname;
        if (lastDot > lastSlash)
        {
            leafName = pathname.substr(lastSlash+1, lastDot-lastSlash-1);
        }

        // Enabled
        bool isCanvasEnabled = canvas->GetEnabled();
        if (!isCanvasEnabled)
        {
            continue;
        }

        // Num elements
        LyShineDebug::DebugInfoRenderGraph info;
        canvas->GetDebugInfoRenderGraph(info);

        sprintf_s(buffer, "%2d: %20s %5d   %5d %5d %5d %5d %5d   %5d %5d %5d %5d %5d %5d",
            i, leafName.c_str(),
            info.m_numRenderNodes,
            info.m_numPrimitives, info.m_numTriangles,
            info.m_numMasks, info.m_numRTs, info.m_numUniqueTextures,
            info.m_numNodesDueToMask, info.m_numNodesDueToRT,
            info.m_numNodesDueToBlendMode, info.m_numNodesDueToSrgb,
            info.m_numNodesDueToMaxVerts, info.m_numNodesDueToTextures);

        AZ::u64 timeSinceBuiltMs = AZStd::GetTimeUTCMilliSecond() - info.m_timeGraphLastBuiltMs;
        if (timeSinceBuiltMs > 1000)
        {
            timeSinceBuiltMs = 1000;
        }
        float percentageOfSecSinceLastBuilt = static_cast<float>(timeSinceBuiltMs) / 1000.0f;

        AZ::Vector3 color;
        if (info.m_wasBuiltThisFrame)
        {
            color = white; // white used if the render graph was rebuilt this frame
        }
        else
        {
            if (info.m_isReusingRenderTargets)
            {
                color = yellow;  // yellow used if the render graph was  not rebuilt and render targets were reused
            }
            else
            {
                color = green;  // green used if render graph not regenerated this frame and no render targets reused
            }

            // When the render graph switches to not being built each frame we take 1 second to interpolate from white to
            // the desired color, otherwise it is not possible to see when the rendergraph gets rebuilt at high frame rates
            color = white + (color - white) * percentageOfSecSinceLastBuilt;
        }

        WriteLine(buffer, color);
        ++i;

        totalRenderNodes += info.m_numRenderNodes;
        totalPrimitives += info.m_numPrimitives;
        totalTriangles += info.m_numTriangles;
        totalMasks += info.m_numMasks;
        totalRTs += info.m_numRTs;
        totalDueToMask += info.m_numNodesDueToMask;
        totalDueToRT += info.m_numNodesDueToRT;
        totalDueToBlendMode += info.m_numNodesDueToBlendMode;
        totalDueToSrgb += info.m_numNodesDueToSrgb;
        totalDueToMaxVerts += info.m_numNodesDueToMaxVerts;
        totalDueToTextures += info.m_numNodesDueToTextures;
    }

    sprintf_s(buffer, "Totals:                  %5d   %5d %5d %5d %5d         %5d %5d %5d %5d %5d %5d",
        totalRenderNodes,
        totalPrimitives, totalTriangles, totalMasks, totalRTs,
        totalDueToMask, totalDueToRT,
        totalDueToBlendMode, totalDueToSrgb,
        totalDueToMaxVerts, totalDueToTextures);

    WriteLine(buffer, red);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string UiCanvasManager::DebugGetElementName(AZ::EntityId entityId, int maxLength) const
{
    AZStd::string name = "None";
    AZ::Entity* entity = nullptr;
    EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, entityId);
    if (entity)
    {
        name = entity->GetName();
        if (name.length() < maxLength)
        {
            AZ::Entity* parent = nullptr;
            EBUS_EVENT_ID_RESULT(parent, entityId, UiElementBus, GetParent);
            if (parent)
            {
                name = parent->GetName() + "/" + name;
                if (name.length() > maxLength)
                {
                    AZStd::string maxString = name.substr(name.length() - maxLength);
                    name = maxString;
                }
            }
        }
    }

    return name;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::DebugReportDrawCalls(const AZStd::string& name) const
{
    AZStd::string logFolder = AZStd::string::format("@log@/LyShine");
    AZ::IO::LocalFileIO::GetInstance()->CreatePath(logFolder.c_str());

    AZStd::string logFileLeafName = "DrawCallReport";
    if (!name.empty())
    {
        logFileLeafName += "_";
        logFileLeafName += name;
    }
    AZStd::string logFile = AZStd::string::format("%s/%s.txt", logFolder.c_str(), logFileLeafName.c_str());

    AZ::IO::HandleType logHandle;
    AZ::IO::Result result = AZ::IO::LocalFileIO::GetInstance()->Open(logFile.c_str(), AZ::IO::OpenMode::ModeWrite, logHandle);
    if (!result)
    {
        AZ_TracePrintf("UI", "Failed to open file for Draw Call Report at %s\n", logFile.c_str());
        return;
    }

    AZStd::string logLine = AZStd::string::format("Draw call report for \'%s\'\r\n", name.c_str());
    AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, logLine.c_str(), logLine.size());

    logLine = AZStd::string::format("Output by the ui_ReportDrawCalls console command\r\n\r\n");
    AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, logLine.c_str(), logLine.size());

    int numEnabledCanvases = 0;
    for (auto canvas : m_loadedCanvases)
    {
        if (canvas->GetEnabled())
        {
            numEnabledCanvases++;
        }
    }

    logLine = AZStd::string::format("There are %zu loaded UI canvases, %d of which are enabled.\r\nThe below report only includes the enabled canvases\r\n",
        m_loadedCanvases.size(), numEnabledCanvases);
    AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, logLine.c_str(), logLine.size());

    LyShineDebug::DebugInfoDrawCallReport reportInfo;

    for (auto canvas : m_loadedCanvases)
    {
        // Check enabled
        bool isCanvasEnabled = canvas->GetEnabled();
        if (!isCanvasEnabled)
        {
            continue;
        }

        // Name of canvas
        const AZStd::string pathname = canvas->GetPathname().c_str();
        logLine = "\r\n=====================================================================================\r\n";
        AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, logLine.c_str(), logLine.size());
        logLine = AZStd::string::format("Canvas: %s\r\n", pathname.c_str());
        AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, logLine.c_str(), logLine.size());
        logLine = "=====================================================================================\r\n\r\n";
        AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, logLine.c_str(), logLine.size());

        // Get canvas summary data
        LyShineDebug::DebugInfoRenderGraph renderGraphInfo;
        canvas->GetDebugInfoRenderGraph(renderGraphInfo);

        // Output a summary report
        if (renderGraphInfo.m_numRenderNodes > 0)
        {
            logLine = AZStd::string::format("Canvas has %d draw calls and %d primitives with a total of %d triangles\r\n",
                renderGraphInfo.m_numRenderNodes, renderGraphInfo.m_numPrimitives, renderGraphInfo.m_numTriangles);
            AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, logLine.c_str(), logLine.size());
            logLine = AZStd::string::format("There are %d unique textures used, %d mask render nodes and %d render target render nodes\r\n",
                renderGraphInfo.m_numUniqueTextures, renderGraphInfo.m_numMasks, renderGraphInfo.m_numRTs);
            AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, logLine.c_str(), logLine.size());
            logLine = AZStd::string::format(
                "Extra draw calls caused by... Masks: %d, RenderTargets: %d, BlendModes: %d, Srgb: %d, MaxVerts: %d, MaxTextures: %d\r\n\r\n",
                renderGraphInfo.m_numNodesDueToMask, renderGraphInfo.m_numNodesDueToRT,
                renderGraphInfo.m_numNodesDueToBlendMode,
                renderGraphInfo.m_numNodesDueToSrgb, renderGraphInfo.m_numNodesDueToMaxVerts, renderGraphInfo.m_numNodesDueToTextures);
            AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, logLine.c_str(), logLine.size());
        }

        // output the details on each draw call and gather info for all canvases
        canvas->DebugReportDrawCalls(logHandle, reportInfo, canvas);
    }

    AZStd::string fontTexturePrefix = "$AutoFont";

    logLine = "\r\n\r\n--------------------------------------------------------------------------------------------\r\n";
    AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, logLine.c_str(), logLine.size());
    logLine = AZStd::string::format("Textures used on multiple canvases that are causing extra draw calls\r\n");
    AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, logLine.c_str(), logLine.size());
    logLine = "--------------------------------------------------------------------------------------------\r\n\r\n";
    AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, logLine.c_str(), logLine.size());

    for (LyShineDebug::DebugInfoTextureUsage& reportTextureUsage : reportInfo.m_textures)
    {
        if (reportTextureUsage.m_numCanvasesUsed > 1 &&
            reportTextureUsage.m_numDrawCallsWhereExceedingMaxTextures)
        {
            AZStd::string textureName;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(textureName, &AZ::Data::AssetCatalogRequests::GetAssetPathById, reportTextureUsage.m_texture->GetAssetId());
            if (textureName.compare(0, fontTexturePrefix.length(), fontTexturePrefix) != 0)
            {
                logLine = AZStd::string::format("%s\r\n", textureName.c_str());
                AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, logLine.c_str(), logLine.size());
            }
        }
    }

    logLine = "\r\n\r\n--------------------------------------------------------------------------------------------\r\n";
    AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, logLine.c_str(), logLine.size());
    logLine = AZStd::string::format("Per canvas report of textures used on only that canvas that are causing extra draw calls\r\n");
    AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, logLine.c_str(), logLine.size());
    logLine = "--------------------------------------------------------------------------------------------\r\n\r\n";
    AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, logLine.c_str(), logLine.size());

    for (auto canvas : m_loadedCanvases)
    {
        // Check enabled
        bool isCanvasEnabled = canvas->GetEnabled();
        if (!isCanvasEnabled)
        {
            continue;
        }

        bool loggedCanvasHeader = false;
        for (LyShineDebug::DebugInfoTextureUsage& reportTextureUsage : reportInfo.m_textures)
        {
            if (reportTextureUsage.m_numCanvasesUsed == 1 &&
                reportTextureUsage.m_lastContextUsed == canvas &&
                reportTextureUsage.m_numDrawCallsWhereExceedingMaxTextures)
            {
                AZStd::string textureName;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(textureName, &AZ::Data::AssetCatalogRequests::GetAssetPathById, reportTextureUsage.m_texture->GetAssetId());

                // exclude font textures
                if (textureName.compare(0, fontTexturePrefix.length(), fontTexturePrefix) != 0)
                {
                    if (!loggedCanvasHeader)
                    {
                        const AZStd::string pathname = canvas->GetPathname().c_str();
                        logLine = AZStd::string::format("\r\nCanvas: %s\r\n\r\n", pathname.c_str());
                        AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, logLine.c_str(), logLine.size());
                        loggedCanvasHeader = true;
                    }

                    logLine = AZStd::string::format("%s\r\n", textureName.c_str());
                    AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, logLine.c_str(), logLine.size());
                }
            }
        }
    }

    logLine = "\r\n--------------------------------------------------------------------------------------------\r\n";
    AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, logLine.c_str(), logLine.size());
    logLine = "End of report\r\n";
    AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, logLine.c_str(), logLine.size());
    logLine = "--------------------------------------------------------------------------------------------\r\n";
    AZ::IO::LocalFileIO::GetInstance()->Write(logHandle, logLine.c_str(), logLine.size());

    AZ::IO::LocalFileIO::GetInstance()->Close(logHandle);

    AZ_TracePrintf("UI", "Wrote Draw Call Report to %s\n", logFile.c_str());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasManager::DebugDisplayElemBounds(int canvasIndexFilter) const
{
    CDraw2d* draw2d = Draw2dHelper::GetDefaultDraw2d();

    int canvasIndex = 0;
    for (auto canvas : m_loadedCanvases)
    {
        // Enabled
        bool isCanvasEnabled = canvas->GetEnabled();
        if (!isCanvasEnabled)
        {
            continue;
        }

        // filter canvas index
        if (canvasIndexFilter == -1 || canvasIndexFilter == canvasIndex)
        {
            // Display the elem bounds
            canvas->DebugDisplayElemBounds(draw2d);
        }

        ++canvasIndex;  // only increments for enabled canvases so index matches "ui_DisplayCanvasData 2"
    }
}

#endif
