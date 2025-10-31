/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/RTTI/TypeInfo.h>

#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiInteractableBus.h>
#include <LyShine/Bus/UiAnimationBus.h>
#include <LyShine/Bus/UiEditorCanvasBus.h>
#include <LyShine/UiEntityContext.h>
#include <LyShine/UiComponentTypes.h>

#include "UiElementComponent.h"
#include "UiSerialize.h"
#include "Animation/UiAnimationSystem.h"
#include "UiLayoutManager.h"
#include "UiNavigationHelpers.h"
#include "RenderGraph.h"

#ifndef _RELEASE
#include "LyShineDebug.h"
#endif
#include "TextureAtlas/TextureAtlas.h"
#include "TextureAtlas/TextureAtlasBus.h"
#include "TextureAtlas/TextureAtlasNotificationBus.h"

#include "RenderToTextureBus.h"

namespace AZ
{
    class SerializeContext;
}

struct SDepthTexture;
class CDraw2d;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiCanvasComponent
    : public AZ::Component
    , public UiCanvasBus::Handler
    , public AZ::EntityBus::Handler
    , public UiAnimationBus::Handler
    , public UiInteractableActiveNotificationBus::Handler
    , public IUiAnimationListener
    , public UiEditorCanvasBus::Handler
    , public UiCanvasComponentImplementationBus::Handler
    , public LyShine::RenderToTextureRequestBus::Handler
{
public: // constants
    static const AZ::Vector2 s_defaultCanvasSize;
    static const AZ::Color s_defaultGuideColor;

public: // member functions

    AZ_COMPONENT(UiCanvasComponent, LyShine::UiCanvasComponentUuid, AZ::Component);

    //! Constructor, constructed by the LyShine class
    UiCanvasComponent();
    ~UiCanvasComponent() override;

    // UiCanvasInterface

    const AZStd::string& GetPathname() override;
    LyShine::CanvasId GetCanvasId() override;

    AZ::u64 GetUniqueCanvasId() override;

    int GetDrawOrder() override;
    void SetDrawOrder(int drawOrder) override;

    bool GetKeepLoadedOnLevelUnload() override;
    void SetKeepLoadedOnLevelUnload(bool keepLoaded) override;

    void RecomputeChangedLayouts() override;

    int GetNumChildElements() override;
    AZ::Entity* GetChildElement(int index) override;
    AZ::EntityId GetChildElementEntityId(int index) override;

    LyShine::EntityArray GetChildElements() override;
    AZStd::vector<AZ::EntityId> GetChildElementEntityIds() override;

    AZ::Entity* CreateChildElement(const LyShine::NameType& name) override;

    AZ::Entity* FindElementById(LyShine::ElementId id) override;
    AZ::Entity* FindElementByName(const LyShine::NameType& name) override;
    void FindElementsByName(const LyShine::NameType& name, LyShine::EntityArray& result) override;
    AZ::EntityId FindElementEntityIdByName(const LyShine::NameType& name) override;
    AZ::Entity* FindElementByHierarchicalName(const LyShine::NameType& name) override;
    void FindElements(AZStd::function<bool(const AZ::Entity*)> predicate, LyShine::EntityArray& result) override;

    AZ::Entity* PickElement(AZ::Vector2 point) override;
    LyShine::EntityArray PickElements(const AZ::Vector2& bound0, const AZ::Vector2& bound1) override;
    AZ::EntityId FindInteractableToHandleEvent(AZ::Vector2 point) override;

    bool SaveToXml(const AZStd::string& assetIdPathname, const AZStd::string& sourceAssetPathname) override;
    void FixupCreatedEntities(LyShine::EntityArray topLevelEntities, bool makeUniqueNamesAndIds, AZ::Entity* optionalInsertionPoint) override;
    void AddElement(AZ::Entity* element, AZ::Entity* parent, AZ::Entity* insertBefore) override;
    void ReinitializeElements() override;
    AZStd::string SaveToXmlString() override;
    AZStd::string GetUniqueChildName(AZ::EntityId parentEntityId, AZStd::string baseName, const LyShine::EntityArray* includeChildren) override;
    AZ::Entity* CloneElement(AZ::Entity* sourceEntity, AZ::Entity* parentEntity) override;
    AZ::EntityId CloneElementEntityId(AZ::EntityId sourceEntity, AZ::EntityId parentEntity, AZ::EntityId insertBefore) override;
    AZ::Entity* CloneCanvas(const AZ::Vector2& canvasSize) override;
    void SetCanvasToViewportMatrix(const AZ::Matrix4x4& matrix) override;
    const AZ::Matrix4x4& GetCanvasToViewportMatrix() override;
    void GetViewportToCanvasMatrix(AZ::Matrix4x4& matrix) override;
    AZ::Vector2 GetCanvasSize() override;
    AZ::Vector2 GetAuthoredCanvasSize() override;
    void SetCanvasSize(const AZ::Vector2& canvasSize) override;
    void SetTargetCanvasSize(bool isInGame, const AZ::Vector2& targetCanvasSize) override;
    AZ::Vector2 GetDeviceScale() override;
    bool GetIsPixelAligned() override;
    void SetIsPixelAligned(bool isPixelAligned) override;
    bool GetIsTextPixelAligned() override;
    void SetIsTextPixelAligned(bool isTextPixelAligned) override;
    IUiAnimationSystem* GetAnimationSystem() override;
    bool GetEnabled() override;
    void SetEnabled(bool enabled) override;

    bool GetIsRenderToTexture() override;
    void SetIsRenderToTexture(bool isRenderToTexture) override;
    const AZ::Data::Asset<AZ::RPI::AttachmentImageAsset>& GetAttachmentImageAsset() override;
    void SetAttachmentImageAsset(const AZ::Data::Asset<AZ::RPI::AttachmentImageAsset>& attachmentImageAsset) override;

    bool GetIsPositionalInputSupported() override;
    void SetIsPositionalInputSupported(bool isSupported) override;
    bool GetIsConsumingAllInputEvents() override;
    void SetIsConsumingAllInputEvents(bool isConsuming) override;
    bool GetIsMultiTouchSupported() override;
    void SetIsMultiTouchSupported(bool isSupported) override;
    bool GetIsNavigationSupported() override;
    void SetIsNavigationSupported(bool isSupported) override;
    float GetNavigationThreshold() override;
    void SetNavigationThreshold(float navigationThreshold) override;
    AZ::u64 GetNavigationRepeatDelay() override;
    void SetNavigationRepeatDelay(AZ::u64 navigationRepeatDelay) override;
    AZ::u64 GetNavigationRepeatPeriod() override;
    void SetNavigationRepeatPeriod(AZ::u64 navigationRepeatPeriod) override;
    AzFramework::LocalUserId GetLocalUserIdInputFilter() override;
    void SetLocalUserIdInputFilter(AzFramework::LocalUserId localUserId) override;

    bool HandleInputEvent(const AzFramework::InputChannel::Snapshot& inputSnapshot,
        const AZ::Vector2* viewportPos = nullptr,
        AzFramework::ModifierKeyMask activeModifierKeys = AzFramework::ModifierKeyMask::None) override;
    bool HandleTextEvent(const AZStd::string& textUTF8) override;
    bool HandleInputPositionalEvent(const AzFramework::InputChannel::Snapshot& inputSnapshot, AZ::Vector2 viewportPos) override;

    AZ::Vector2 GetMousePosition() override;

    AZ::EntityId GetTooltipDisplayElement() override;
    void SetTooltipDisplayElement(AZ::EntityId entityId) override;

    void ForceFocusInteractable(AZ::EntityId interactableId) override;
    void ForceActiveInteractable(AZ::EntityId interactableId, bool shouldStayActive, AZ::Vector2 point) override;
    AZ::EntityId GetHoverInteractable() override;
    void ForceHoverInteractable(AZ::EntityId interactableId) override;
    void ClearAllInteractables() override;
    void ForceEnterInputEventOnInteractable(AZ::EntityId interactableId) override;

    // ~UiCanvasInterface

    // EntityEvents
    void OnEntityDeactivated(const AZ::EntityId& entityId) override;
    // ~EntityEvents

    // UiAnimationInterface
    void StartSequence(const AZStd::string& sequenceName) override;
    void PlaySequenceRange(const AZStd::string& sequenceName, float startTime, float endTime) override;
    void StopSequence(const AZStd::string& sequenceName) override;
    void AbortSequence(const AZStd::string& sequenceName) override;
    void PauseSequence(const AZStd::string& sequenceName) override;
    void ResumeSequence(const AZStd::string& sequenceName) override;
    void ResetSequence(const AZStd::string& sequenceName) override;
    float GetSequencePlayingSpeed(const AZStd::string& sequenceName) override;
    void SetSequencePlayingSpeed(const AZStd::string& sequenceName, float speed) override;
    float GetSequencePlayingTime(const AZStd::string& sequenceName) override;
    bool IsSequencePlaying(const AZStd::string& sequenceName) override;
    float GetSequenceLength(const AZStd::string& sequenceName) override;
    void SetSequenceStopBehavior(IUiAnimationSystem::ESequenceStopBehavior stopBehavior) override;
    // ~UiAnimationInterface

    // UiInteractableActiveNotifications
    void ActiveCancelled() override;
    void ActiveChanged(AZ::EntityId m_newActiveInteractable, bool shouldStayActive) override;
    // ~UiInteractableActiveNotifications

    // IUiAnimationListener
    void OnUiAnimationEvent(EUiAnimationEvent uiAnimationEvent, IUiAnimSequence* pAnimSequence) override;
    void OnUiTrackEvent(AZStd::string eventName, AZStd::string valueName, IUiAnimSequence* pAnimSequence) override;
    // ~IUiAnimationListener

    // UiEditorCanvasInterface
    bool GetIsSnapEnabled() override;
    void SetIsSnapEnabled(bool enabled) override;
    float GetSnapDistance() override;
    void SetSnapDistance(float distance) override;
    float GetSnapRotationDegrees() override;
    void SetSnapRotationDegrees(float degrees) override;

    AZStd::vector<float> GetHorizontalGuidePositions() override;
    void AddHorizontalGuide(float position) override;
    void RemoveHorizontalGuide(int index) override;
    void SetHorizontalGuidePosition(int index, float position) override;
    AZStd::vector<float> GetVerticalGuidePositions() override;
    void AddVerticalGuide(float position) override;
    void RemoveVerticalGuide(int index) override;
    void SetVerticalGuidePosition(int index, float position) override;
    void RemoveAllGuides() override;
    AZ::Color GetGuideColor() override;
    void SetGuideColor(const AZ::Color& color) override;
    bool GetGuidesAreLocked() override;
    void SetGuidesAreLocked(bool areLocked) override;
    bool CheckForOrphanedElements() override;
    void RecoverOrphanedElements() override;
    void RemoveOrphanedElements() override;

    void UpdateCanvasInEditorViewport(float deltaTime, bool isInGame) override;
    void RenderCanvasInEditorViewport(bool isInGame, AZ::Vector2 viewportSize) override;
    // ~UiEditorCanvasInterface

    // UiCanvasComponentImplementationInterface
    void MarkRenderGraphDirty() override;
    // ~UiCanvasComponentImplementationInterface

    // LyShine::RenderToTextureRequestBus overrides ...
    AZ::RHI::AttachmentId UseRenderTarget(const AZ::Name& renderTargetName, AZ::RHI::Size size) override;
    AZ::RHI::AttachmentId UseRenderTargetAsset(const AZ::Data::Asset<AZ::RPI::AttachmentImageAsset>& attachmentImageAsset) override;
    void ReleaseRenderTarget(const AZ::RHI::AttachmentId& attachmentId) override;
    AZ::Data::Instance<AZ::RPI::AttachmentImage> GetRenderTarget(const AZ::RHI::AttachmentId& attachmentId) override;

    void UpdateCanvas(float deltaTime, bool isInGame);
    void RenderCanvas(bool isInGame, AZ::Vector2 viewportSize, UiRenderer* uiRenderer = nullptr);

    AZ::Entity* GetRootElement() const;

    LyShine::ElementId GenerateId();

    //! Clone this canvas's entity and return the Canvas component
    //! (used when it is loaded from in game or for preview mode etc)
    UiCanvasComponent* CloneAndInitializeCanvas(UiEntityContext* entityContext, const AZStd::string& assetIdPathname, const AZ::Vector2* canvasSize = nullptr);

    //! Deactivate all elements. Used when queuing a canvas up for deletion
    void DeactivateElements();

    AZ::Vector2 GetTargetCanvasSize();

    //! Get the mapping from editor EntityId to game EntityId. This will be empty for canvases loaded for editing
    AZ::SliceComponent::EntityIdToEntityIdMap GetEditorToGameEntityIdMap() { return m_editorToGameEntityIdMap; }

    void ScheduleElementForTransformRecompute(UiElementComponent* elementComponent);
    void UnscheduleElementForTransformRecompute(UiElementComponent* elementComponent);

    //! Queue an element to be destroyed at end of frame
    void ScheduleElementDestroy(AZ::EntityId entityId);

    bool IsRenderGraphDirty() { return m_renderGraph.GetDirtyFlag(); }

    void GetRenderTargets(LyShine::AttachmentImagesAndDependencies& attachmentImagesAndDependencies);

#ifndef _RELEASE
    struct DebugInfoNumElements
    {
        int m_numElements;
        int m_numEnabledElements;
        int m_numRenderElements;
        int m_numRenderControlElements;
        int m_numImageElements;
        int m_numTextElements;
        int m_numMaskElements;
        int m_numFaderElements;
        int m_numInteractableElements;
        int m_numUpdateElements;
    };

    void GetDebugInfoInteractables(AZ::EntityId& activeInteractable,  AZ::EntityId& hoverInteractable) const;
    void GetDebugInfoNumElements(DebugInfoNumElements& info) const;
    void GetDebugInfoRenderGraph(LyShineDebug::DebugInfoRenderGraph& info) const;
    void DebugInfoCountChildren(const AZ::EntityId entity, bool parentEnabled, DebugInfoNumElements& info) const;

    void DebugReportDrawCalls(AZ::IO::HandleType fileHandle, LyShineDebug::DebugInfoDrawCallReport& reportInfo, void* context) const;

    void DebugDisplayElemBounds(IDraw2d* draw2d) const;
    void DebugDisplayChildElemBounds(IDraw2d* draw2d, const AZ::EntityId entity) const;
#endif

public: // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("UiCanvasService"));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("UiCanvasService"));
    }

    static void GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    static void Reflect(AZ::ReflectContext* context);

    // TODO: Move these static functions into a CanvasManager class

    static void Initialize();
    static void Shutdown();

    static UiCanvasComponent* CreateCanvasInternal(UiEntityContext* entityContext, bool forEditor);
    static UiCanvasComponent* LoadCanvasInternal(const AZStd::string& pathToOpen, bool forEditor, const AZStd::string& assetIdPathname, UiEntityContext* entityContext,
        const AZ::SliceComponent::EntityIdToEntityIdMap* previousRemapTable = nullptr, AZ::EntityId previousCanvasId = AZ::EntityId());
    static UiCanvasComponent* FixupReloadedCanvasForEditorInternal(AZ::Entity* newCanvasEntity,
        AZ::Entity* rootSliceEntity, UiEntityContext* entityContext,
        LyShine::CanvasId existingId, const AZStd::string& existingPathname);

protected: // member functions

    // AZ::Component
    void Init() override;
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

private: // member functions

    AZ_DISABLE_COPY_MOVE(UiCanvasComponent);

    // Texture Atlas loading
    void LoadAtlases();
    void UnloadAtlases();
    void ReloadAtlases();

    // handle events for this canvas
    bool HandleHoverInputEvent(AZ::Vector2 point);
    bool HandleKeyInputEvent(const AzFramework::InputChannel::Snapshot& inputSnapshot, AzFramework::ModifierKeyMask activeModifierKeys = AzFramework::ModifierKeyMask::None);
    bool HandleNavigationInputEvent(UiNavigationHelpers::Command command, const AzFramework::InputChannel::Snapshot& inputSnapshot);
    bool HandleEnterInputEvent(UiNavigationHelpers::Command command, const AzFramework::InputChannel::Snapshot& inputSnapshot);
    bool HandleBackInputEvent(UiNavigationHelpers::Command command, const AzFramework::InputChannel::Snapshot& inputSnapshot);

    // A key was pressed to deactivate the active interactable
    bool DeactivateInteractableByKeyInput(const AzFramework::InputChannel::Snapshot& inputSnapshot);

    // A key was pressed to transfer hover to the ancestor interactable
    bool PassHoverToAncestorByKeyInput(const AzFramework::InputChannel::Snapshot& inputSnapshot);

    // Code shared by all positional input events
    bool HandlePrimaryPress(AZ::Vector2 point);
    bool HandlePrimaryUpdate(AZ::Vector2 point);
    bool HandlePrimaryRelease(AZ::Vector2 point);
    bool HandleMultiTouchPress(AZ::Vector2 point, int multiTouchIndex);
    bool HandleMultiTouchRelease(AZ::Vector2 point, int multiTouchIndex);
    bool HandleMultiTouchUpdated(AZ::Vector2 point, int multiTouchIndex);
    bool IsInteractableActiveOrPressed(AZ::EntityId interactableId) const;

    // Functions to change the hover and active interactables
    void SetHoverInteractable(AZ::EntityId interactableId);
    void ClearHoverInteractable();
    void SetActiveInteractable(AZ::EntityId newActiveInteractable, bool shouldStayActive);
    void ClearActiveInteractable();

    //! Check if the hover interactable is set to auto-activate, and if so activate it
    void CheckHoverInteractableAndAutoActivate(AZ::EntityId prevHoverInteractable = AZ::EntityId(), UiNavigationHelpers::Command command = UiNavigationHelpers::Command::Unknown, bool forceAutoActivate = false);

    //! Check if the active interactable has a descendant interactable. If it does,
    //! make the descendant the hover interactable and clear the active interactable
    void CheckActiveInteractableAndPassHoverToDescendant(AZ::EntityId prevHoverInteractable = AZ::EntityId(), UiNavigationHelpers::Command command = UiNavigationHelpers::Command::Unknown);

    //! Find the first interactable ancestor of the specified element
    AZ::EntityId FindAncestorInteractable(AZ::EntityId entityId);

    //! Get the first hover interactable, defaulting to the one set by the canvas
    AZ::EntityId GetFirstHoverInteractable();

    //! Find the first interactable to receive focus
    AZ::EntityId FindFirstHoverInteractable(AZ::EntityId parentElement = AZ::EntityId());

    //! Set the hover interactable on canvas load
    void SetFirstHoverInteractable();

    //! Due to differences in their serialization systems we need to do some work before save
    void PrepareAnimationSystemForCanvasSave();

    //! Due to differences in their serialization systems we need to do some work after load
    void RestoreAnimationSystemAfterCanvasLoad(bool remapIds, UiElementComponent::EntityIdMap* entityIdMap);

    //! Get a list of entity IDs for this element and all its descendant elements
    AZStd::vector<AZ::EntityId> GetEntityIdsOfElementAndDescendants(AZ::Entity* entity);

    //! Calculate the target canvas size and uniform scale
    void SetTargetCanvasSizeAndUniformScale(bool isInGame, AZ::Vector2 canvasSize);

    //! Check if an element name is unique
    bool IsElementNameUnique(const AZStd::string& elementName, const LyShine::EntityArray& elements);

    //! Methods used for controlling the Edit Context (the properties pane)
    using EntityComboBoxVec = AZStd::vector< AZStd::pair< AZ::EntityId, AZStd::string > >;
    EntityComboBoxVec PopulateNavigableEntityList();
    EntityComboBoxVec PopulateTooltipDisplayEntityList();
    void OnPixelAlignmentChange();
    void OnTextPixelAlignmentChange();

    void CreateRenderTarget();
    void DestroyRenderTarget();

    bool SaveCanvasToFile(const AZStd::string& pathname, AZ::DataStream::StreamType streamType);
    bool SaveCanvasToStream(AZ::IO::GenericStream& stream, AZ::DataStream::StreamType streamType);

    //! Notify elements that their canvas space rect has changed since the last update, and recompute invalid layouts
    void SendRectChangeNotificationsAndRecomputeLayouts();

    //! Notify elements that their canvas space rect has changed since the last update
    void SendRectChangeNotifications();

    //! Compute the layout for all elements. Parents are computed first, then children.
    //! Called on canvas initialization
    void InitializeLayouts();

    //! Call InGamePostActivate on children first, then parent
    void InGamePostActivateBottomUp(AZ::Entity* entity);

    //! Internal function for cloning an element
    AZ::Entity* CloneAndAddElementInternal(AZ::Entity* sourceEntity, AZ::Entity* parentEntity, AZ::Entity* insertBeforeEntity);

    //! Get any orphaned elements caused by old bugs
    void GetOrphanedElements(AZ::SliceComponent::EntityList& orphanedEntities);

    void DestroyScheduledElements();

    //! Notify LyShine pass that it needs to rebuild its Rtt child passes
    void QueueRttPassRebuild();

private: // static member functions

    static AZ::u64 CreateUniqueId();

    static UiCanvasComponent* FixupPostLoad(AZ::Entity* canvasEntity, AZ::Entity* rootSliceEntity, bool forEditor, UiEntityContext* entityContext,
        const AZ::Vector2* canvasSize = nullptr, const AZ::SliceComponent::EntityIdToEntityIdMap* previousRemapTable = nullptr, AZ::EntityId previousCanvasId = AZ::EntityId());

    static bool VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement);

private: // types

    typedef std::vector<UiCanvasComponent*> CanvasList;   //!< Sorted by draw order

private: // data

    AZStd::string m_pathname;              //! This is an asset ID pathname
    AZ::u64 m_uniqueId;
    AZ::EntityId m_rootElement;
    LyShine::ElementId m_lastElementId;
    bool m_isPixelAligned = true;       //! if true all visual elements have their vertices snapped to the nearest pixel
    bool m_isTextPixelAligned = true;   //! if true all text is snapped to the nearest pixel
    AZ::EntityId m_firstHoverInteractable;
    bool m_isPositionalInputSupported = true;
    bool m_isConsumingAllInputEvents = false;
    bool m_isMultiTouchSupported = true;
    bool m_isNavigationSupported = true;
    float m_navigationThreshold = 0.4f;
    AZ::u64 m_navigationRepeatDelay = 300;
    AZ::u64 m_navigationRepeatPeriod = 150;
    AzFramework::LocalUserId m_localUserIdInputFilter = AzFramework::LocalUserIdAny;
    AZ::EntityId m_tooltipDisplayElement;

    AZ::Matrix4x4 m_canvasToViewportMatrix;
    AZ::Matrix4x4 m_viewportToCanvasMatrix;

    AZStd::vector <AzFramework::SimpleAssetReference<TextureAtlasNamespace::TextureAtlasAsset>> m_atlasPathNames;  //! This is a list of filepaths for TextureAtlases
    AZStd::vector<TextureAtlasNamespace::TextureAtlas*> m_atlases;

    // In the comments below an "interactable entity" is an entity that is connected to the
    // UiInteractableBus

    //! The hover interactable is the interactable entity that the mouse cursor is currently over
    //! (or is selected by keyboard/controller navigation). The canvas tracks it so that it can tell
    //! the hover interactable that it is no longer in hover state when the cursor moves outside its bounds
    AZ::EntityId m_hoverInteractable;

    //! The active interactable is the interactable entity that the canvas considers "active".
    //! The active interactable is initially the interactable that handled the pressed event.
    //! An interactable can request to be stay active after the released event by returning
    //! "shouldStayActive" from HandlePressed. In this case the active interactable remains
    //! the active interactable until another interactable becomes active
    //! or until the active interactable itself requests to cancel its active status.
    //! If there is an active interactable when a released event is received then it
    //! is sent to the active interactable.
    //! The active interactable gets sent the mouse/touch position each frame (to support drag).
    //! The active interactable gets sent any character input received by the canvas.
    AZ::EntityId m_activeInteractable;

    //! If this is true the active interactable stays active after the release event
    bool m_activeInteractableShouldStayActive;

    //! True if the mouse is pressed or the enter key is down and there is an active interactable
    bool m_isActiveInteractablePressed;

    //! The last mouse position. Used to detect mouse movement
    AZ::Vector2 m_lastMousePosition;

    //! A map of all interactables that have handled a multi-touch (non-primary) pressed
    //! event but that are still waiting to receive the corresponding released event
    AZStd::unordered_map<int, AZ::EntityId> m_multiTouchInteractablesByTouchIndex;

    struct NavigationStatus
    {
        AZ::u64 lastNavigationTime;
        int navigationCount;
        bool allowNavigation;
    };

    //! The status of navigation in each direction
    AZStd::unordered_map<UiNavigationHelpers::Command, NavigationStatus> m_navCommandStatus;

    LyShine::CanvasId m_id;
    int m_drawOrder;

    //! The authored canvas size, in pixels
    //
    //! While in the editor, this is the resolution that we display the canvas at. While in
    //! game, the authored canvas size is used to calculate m_uniformDeviceScale, which is
    //! used to apply the "scale to device" feature.
    AZ::Vector2 m_canvasSize;

    //! The target size of the canvas in pixels
    //
    //! The resolution that we display the canvas at. While in-game, we assume the canvas
    //! occupies the entire screen, so it is set to the viewport size. In the Editor, we
    //! set the target size to be the authored canvas size.
    AZ::Vector2 m_targetCanvasSize;

    //! The scale that will convert from canvasSize to viewportSize.
    //! In the editor this is always 1.0f
    //! In game it is the closer value to 1.0f of the two values m_viewportSize.x/m_canvasSize.x
    //! and m_viewportSize.y/m_canvasSize.y
    AZ::Vector2 m_deviceScale;

    //! True if this canvas is loaded in game (including for Ctrl-G in Sandbox), false if open in the UI Editor
    bool m_isLoadedInGame;

    //! This flag allows some UI canvases to stay loaded when transitioning from one level to another
    bool m_keepLoadedOnLevelUnload;

    //! True (default) if the canvas is visible and updated each frame, false otherwise
    bool m_enabled;

    //! Each canvas has its own animation system to manage all its animation sequences
    UiAnimationSystem m_uiAnimationSystem;

    UiSerialize::AnimationData m_serializedAnimationData;

    //! If true the canvas is not rendered to the screen but is instead rendered to a texture
    bool m_renderToTexture;

    //! The user-specified asset for the attachment image that we render to if m_renderToTexture is true
    AZ::Data::Asset<AZ::RPI::AttachmentImageAsset> m_attachmentImageAsset;

    //! When rendering to a texture this is the attachment image for the render target
    AZ::RHI::AttachmentId m_attachmentImageId;

    //! Each canvas has a layout manager to track and recompute layouts
    UiLayoutManager* m_layoutManager = nullptr;

    bool m_isSnapEnabled;
    float m_snapDistance;
    float m_snapRotationDegrees;

    //! guides (editor-only)
    AZStd::vector<float> m_horizontalGuidePositions;
    AZStd::vector<float> m_verticalGuidePositions;
    AZ::Color m_guideColor;
    bool m_guidesAreLocked;

    UiEntityContext* m_entityContext;
    AZ::SliceComponent::EntityIdToEntityIdMap m_editorToGameEntityIdMap;

    //! This is an optimization to avoid visiting all elements multiple times every frame to see if any of them need recomputing
    //! We use an intrusive_slist to avoid any memory allocations and also to cheaply be able to tell if an element is already in list
    using ElementComponentSlist = AZStd::intrusive_slist<UiElementComponent, AZStd::slist_base_hook<UiElementComponent>>;
    ElementComponentSlist m_elementsNeedingTransformRecompute;

    //! Holds elements that are queued up to be deleted at end of frame
    AZStd::vector<AZ::EntityId> m_elementsScheduledForDestroy;

private: // static data


    //! If true update which element should be the hover interactable based on input position
    //! and notify the active interactable of position changes. Set to false when a key event
    //! occurs and back to true on mouse/touch activity.
    static bool s_handleHoverInputEvents;

    //! If true, when handling hover input events, allow clearing the hover interactable if the mouse isn't
    //! over any interactables. Set to false when a key event occurs and back to true when handling hover
    //! input events and the input position hovers over an interactable.
    static bool s_allowClearingHoverInteractableOnHoverInput;

    LyShine::RenderGraph m_renderGraph; //!< the render graph for rendering the canvas, can be cached between frames
    bool m_isRendering = false;
    bool m_renderInEditor = false; //!< indicates whether this canvas will render in the Editor viewport or the Game viewport

    //! Map of attachments used by this canvas's elements
    AZStd::unordered_map<AZ::RHI::AttachmentId, AZ::Data::Instance<AZ::RPI::AttachmentImage>> m_attachmentImageMap;
};
