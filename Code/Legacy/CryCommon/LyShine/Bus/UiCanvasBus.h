/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzFramework/Input/Channels/InputChannelDigitalWithSharedModifierKeyStates.h>
#include <AzFramework/Input/User/LocalUserId.h>
#include <LyShine/UiBase.h>

// Forward declarations
struct IUiAnimationSystem;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiCanvasInterface
    : public AZ::ComponentBus
{
public: // member functions

    //! Deleting a canvas will delete all its child elements recursively and all of their components
    virtual ~UiCanvasInterface() {}

    //! Get the asset ID path name of this canvas. If not loaded or saved yet this will be ""
    virtual const AZStd::string& GetPathname() = 0;

    //! Get the ID of this canvas. This will remain the same while this canvas is loaded.
    virtual LyShine::CanvasId GetCanvasId() = 0;

    //! Get the unique ID of this canvas
    virtual AZ::u64 GetUniqueCanvasId() = 0;

    //! Get the draw order of this canvas. Rendering is back-to-front, so higher numbers render in front of lower numbers.
    virtual int GetDrawOrder() = 0;

    //! Set the draw order of this canvas. Rendering is back-to-front, so higher numbers render in front of lower numbers.
    virtual void SetDrawOrder(int drawOrder) = 0;

    //! Get the flag indicating if this canvas will stay loaded through a level unload.
    virtual bool GetKeepLoadedOnLevelUnload() = 0;

    //! Set the flag indicating if this canvas will stay loaded through a level unload.
    virtual void SetKeepLoadedOnLevelUnload(bool keepLoaded) = 0;

    //! Force a layout recompute. Layouts marked for a recompute are handled on the canvas update,
    //! so this can be used if an immediate recompute is desired
    virtual void RecomputeChangedLayouts() = 0;

    //! Get the number child elements of this canvas
    virtual int GetNumChildElements() = 0;

    //! Get the specified child element, index must be less than GetNumChildElements()
    virtual AZ::Entity* GetChildElement(int index) = 0;

    //! Get the specified child entity Id, index must be less than GetNumChildElements()
    virtual AZ::EntityId GetChildElementEntityId(int index) = 0;

    //! Get the child elements of this canvas
    virtual LyShine::EntityArray GetChildElements() = 0;

    //! Get the child entity Ids of this canvas
    virtual AZStd::vector<AZ::EntityId> GetChildElementEntityIds() = 0;

    //! Create a new element that is a child of the canvas, the canvas has ownership of the child
    virtual AZ::Entity* CreateChildElement(const LyShine::NameType& name) = 0;

    //! Return the element on this canvas with the given id or nullptr if no match
    virtual AZ::Entity* FindElementById(LyShine::ElementId id) = 0;

    //! Return the first element on this canvas with the given name or nullptr if no match
    virtual AZ::Entity* FindElementByName(const LyShine::NameType& name) = 0;

    //! Return the first element on this canvas with the given name or nullptr if no match
    virtual AZ::EntityId FindElementEntityIdByName(const LyShine::NameType& name) = 0;

    //! Find all elements on this canvas with the given name
    virtual void FindElementsByName(const LyShine::NameType& name, LyShine::EntityArray& result) = 0;

    //! Return the element with the given hierarchical name or nullptr if no match
    //! \param name, a hierarchical name relative to the root with '/' as the separator
    virtual AZ::Entity* FindElementByHierarchicalName(const LyShine::NameType& name) = 0;

    //! Find all elements on this canvas matching the predicate
    virtual void FindElements(AZStd::function<bool(const AZ::Entity*)> predicate, LyShine::EntityArray& result) = 0;

    //! Get the front-most element whose bounds include the given point in canvas space
    //! \return nullptr if no match
    virtual AZ::Entity* PickElement(AZ::Vector2 point) = 0;

    //! Get all element whose bounds intersect with the given box in canvas space
    //! \return empty EntityArray if no match
    virtual LyShine::EntityArray PickElements(const AZ::Vector2& bound0, const AZ::Vector2& bound1) = 0;

    //! Look for an entity with interactable component to handle an event at given point
    virtual AZ::EntityId FindInteractableToHandleEvent(AZ::Vector2 point) = 0;

    //! Save this canvas to the given path in XML
    //! \return true if no error
    virtual bool SaveToXml(const AZStd::string& assetIdPathname, const AZStd::string& sourceAssetPathname) = 0;

    //! Initialize a set of entities that have been added to the canvas
    //! Used when instantiating a slice or for undo/redo, copy/paste
    //! \param topLevelEntities - The elements that were created
    //! \param makeUniqueNamesAndIds If false the entity names and ElementIds in the string are kept, else unique ones are generated
    //! \param insertionPoint The parent element for the created elements, if nullptr the root element is the parent
    virtual void FixupCreatedEntities(LyShine::EntityArray topLevelEntities, bool makeUniqueNamesAndIds, AZ::Entity* optionalInsertionPoint) = 0;

    //! Add an existing entity to the canvas (only for internal use from editor)
    //! \param element      The newly created element to add to the canvas
    //! \param parent       The parent element for the created element, if nullptr the root element is the parent
    //! \param insertBefore The sibling element to place this element before, if nullptr then add as last child
    virtual void AddElement(AZ::Entity* element, AZ::Entity* parent, AZ::Entity* insertBefore) = 0;

    //! Go through all elements in the canvas and reinitialize them
    //! This is done whenever a slice asset changes and the entity context is rebuilt from the root slice asset
    virtual void ReinitializeElements() = 0;

    //! Save this canvas to an XML string
    //! \return the resulting string
    virtual AZStd::string SaveToXmlString() = 0;

    //! Get an element name that is unique to the children of the specified parent and to an optional array of elements
    //! \param parentEntityId   The entityId of the parent who's children's names must not match the returned name
    //! \param baseName         The name to append a unique identifier to
    //! \param includedChildren An array of any other elements who's names must not match the returned name
    //! \return Unique name that does not match the specified parent's children or the optional array of children
    virtual AZStd::string GetUniqueChildName(AZ::EntityId parentEntityId, AZStd::string baseName, const LyShine::EntityArray* includeChildren) = 0;

    //! Clone an element and add it to this canvas as a child of the given parent element
    //! The entity and all its components/children are cloned and new IDs are generated
    //! NOTE: Only state that is persistent/reflected is cloned
    //! \param sourceEntity The entity to clone
    //! \param parentEntity The parent element for the created elements, if nullptr the root element is the parent
    //! \return The new entity
    virtual AZ::Entity* CloneElement(AZ::Entity* sourceEntity, AZ::Entity* parentEntity) = 0;

    //! Clone an element and add it to this canvas as a child of the given parent element
    //! The entity and all its components/children are cloned and new IDs are generated
    //! NOTE: Only state that is persistent/reflected is cloned
    //! \param sourceEntity The entity to clone (may be from a different canvas)
    //! \param parentEntity The parent element for the created elements, if invalid the root element is the parent
    //! \param insertBefore The child of the parent element that the new element should be inserted before, if invalid the new element is the last child element
    //! \return The new entity
    virtual AZ::EntityId CloneElementEntityId(AZ::EntityId sourceEntity, AZ::EntityId parentEntity, AZ::EntityId insertBefore) = 0;

    //! Create a clone of this canvas entity
    //! \param canvasSize The resolution to display the canvas at
    virtual AZ::Entity* CloneCanvas(const AZ::Vector2& canvasSize) = 0;

    //! Set the transformation from canvas space to viewport space
    virtual void SetCanvasToViewportMatrix(const AZ::Matrix4x4& matrix) = 0;

    //! Get the transformation from canvas space to viewport space
    virtual const AZ::Matrix4x4& GetCanvasToViewportMatrix() = 0;

    //! Get the transformation from viewport space to canvas space
    virtual void GetViewportToCanvasMatrix(AZ::Matrix4x4& matrix) = 0;

    //! Returns the "target" size of the canvas (in pixels)
    //
    //! The target canvas size changes depending on whether you're running in
    //! the UI Editor or in-game. While in-game, we assume that the canvas size
    //! fills the screen, so the target canvas size is the size of the viewport.
    //
    //! When using the editor, however, the target size is the "authored" size of
    //! the canvas. The canvas is authored in one resolution, but it may be
    //! displayed by the game at whatever the game resolution is set to.
    virtual AZ::Vector2 GetCanvasSize() = 0;

    //! Set the authored size of the canvas (in pixels)
    virtual void SetCanvasSize(const AZ::Vector2& canvasSize) = 0;

    //! Set the target size of the canvas (in pixels)
    //!
    //! This should be called before the UpdateCanvas and RenderCanvas methods.
    //! When running in game in full screen mode the target canvas size should be set to the viewport size
    virtual void SetTargetCanvasSize(bool isInGame, const AZ::Vector2& targetCanvasSize) = 0;

    //! Get scale to adjust for the difference between canvas size (authored size)
    //! and the viewport size (target canvas size) when running on current device
    virtual AZ::Vector2 GetDeviceScale() = 0;

    //! Get flag that indicates whether visual element's vertices should snap to the nearest pixel
    virtual bool GetIsPixelAligned() = 0;

    //! Set flag that indicates whether visual element's vertices should snap to the nearest pixel
    virtual void SetIsPixelAligned(bool isPixelAligned) = 0;

    //! Get flag that indicates whether text should snap to the nearest pixel
    virtual bool GetIsTextPixelAligned() = 0;

    //! Set flag that indicates whether text should snap to the nearest pixel
    virtual void SetIsTextPixelAligned(bool isTextPixelAligned) = 0;

    //! Get the animation system for this canvas
    virtual IUiAnimationSystem* GetAnimationSystem() = 0;

    //! Get flag that governs whether the canvas is enabled
    //
    //! A canvas that's enabled will be updated and rendered each frame.
    virtual bool GetEnabled() = 0;

    //! Set flag that governs whether the canvas is enabled
    //
    //! A canvas that's enabled will be updated and rendered each frame.
    virtual void SetEnabled(bool enabled) = 0;

    //! Get flag that controls whether the canvas is rendering to a texture
    virtual bool GetIsRenderToTexture() = 0;

    //! Set flag that controls whether the canvas is rendering to a texture
    virtual void SetIsRenderToTexture(bool isRenderToTexture) = 0;

    //! Get the render target name that this canvas will render to
    virtual AZStd::string GetRenderTargetName() = 0;

    //! Set the render target name that this canvas will render to
    virtual void SetRenderTargetName(const AZStd::string& name) = 0;

    //! Get flag that controls whether this canvas automatically handles positional input (mouse/touch)
    virtual bool GetIsPositionalInputSupported() = 0;

    //! Set flag that controls whether this canvas automatically handles positional input (mouse/touch)
    virtual void SetIsPositionalInputSupported(bool isSupported) = 0;

    //! Get flag that controls whether this canvas consumes all input events while it is enabled
    virtual bool GetIsConsumingAllInputEvents() = 0;

    //! Set flag that controls whether this canvas consumes all input events while it is enabled
    virtual void SetIsConsumingAllInputEvents(bool isConsuming) = 0;

    //! Get flag that controls whether this canvas automatically handles multi-touch input
    virtual bool GetIsMultiTouchSupported() = 0;

    //! Set flag that controls whether this canvas automatically handles multi-touch input
    virtual void SetIsMultiTouchSupported(bool isSupported) = 0;

    //! Get flag that controls whether this canvas automatically handles navigation input (via keyboard/gamepad)
    virtual bool GetIsNavigationSupported() = 0;

    //! Set flag that controls whether this canvas automatically handles navigation input (via keyboard/gamepad)
    virtual void SetIsNavigationSupported(bool isSupported) = 0;

    //! Get the analog (eg. thumb-stick) input value that must be exceeded before a navigation command will be processed
    virtual float GetNavigationThreshold() = 0;

    //! Set the analog (eg. thumb-stick) input value that must be exceeded before a navigation command will be processed
    virtual void SetNavigationThreshold(float navigationThreshold) = 0;

    //! Get the delay (milliseconds) before a held navigation command will begin repeating
    virtual AZ::u64 GetNavigationRepeatDelay() = 0;

    //! Set the delay (milliseconds) before a held navigation command will begin repeating
    virtual void SetNavigationRepeatDelay(AZ::u64 navigationRepeatDelay) = 0;

    //! Get the delay (milliseconds) before a held navigation command will continue repeating
    virtual AZ::u64 GetNavigationRepeatPeriod() = 0;

    //! Set the delay (milliseconds) before a held navigation command will continue repeating
    virtual void SetNavigationRepeatPeriod(AZ::u64 navigationRepeatPeriod) = 0;

    //! Get the local user id that is being used to filter incoming input events
    virtual AzFramework::LocalUserId GetLocalUserIdInputFilter() = 0;

    //! Set the local user id that will be used to filter incoming input events
    virtual void SetLocalUserIdInputFilter(AzFramework::LocalUserId localUserId) = 0;

    //! Handle an input event for the canvas
    virtual bool HandleInputEvent(const AzFramework::InputChannel::Snapshot& inputSnapshot,
        const AZ::Vector2* viewportPos = nullptr,
        AzFramework::ModifierKeyMask activeModifierKeys = AzFramework::ModifierKeyMask::None) = 0;

    //! Handle a unicode text event for the canvas
    virtual bool HandleTextEvent(const AZStd::string& textUTF8) = 0;

    //! Handle a positional input event for the canvas, this could come from
    //! a ray cast intersection for example
    virtual bool HandleInputPositionalEvent(const AzFramework::InputChannel::Snapshot& inputSnapshot, AZ::Vector2 viewportPos) = 0;

    //! Get the mouse position of the last input event
    virtual AZ::Vector2 GetMousePosition() = 0;

    //! Get the element to be displayed when hovering over an interactable
    virtual AZ::EntityId GetTooltipDisplayElement() = 0;

    //! Set the element to be displayed when hovering over an interactable
    virtual void SetTooltipDisplayElement(AZ::EntityId entityId) = 0;

    //! Force the active interactable for the canvas to be the given one,
    //! also force AutoActivation of interactable,
    //! intended for internal use by UI components
    virtual void ForceFocusInteractable(AZ::EntityId interactableId) = 0;

    //! Force the active interactable for the canvas to be the given one,
    //! also set last mouse pos to point,
    //! intended for internal use by UI components
    virtual void ForceActiveInteractable(AZ::EntityId interactableId, bool shouldStayActive, AZ::Vector2 point) = 0;

    //! Get the hover interactable
    virtual AZ::EntityId GetHoverInteractable() = 0;

    //! Force the hover interactable for the canvas to be the given one, this can be useful when using
    //! keyboard/gamepad navigation and the current hover interactable is deleted by a script and the script
    //! wants to specify the new hover interactable
    virtual void ForceHoverInteractable(AZ::EntityId interactableId) = 0;

    //! Clear all active interactables, and all hover interactables if last input was positional (mouse/touch).
    //! This is intended for internal use by UI components
    virtual void ClearAllInteractables() = 0;

    //! Generate Enter pressed/released input events on an interactable.
    //! Useful for automated testing to simulate button clicks
    virtual void ForceEnterInputEventOnInteractable(AZ::EntityId interactableId) = 0;

public: // static member data

    //! Only one component on an entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiCanvasInterface> UiCanvasBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! The canvas component implements this bus and it is provided for C++ implementations of
//! UI components to use to talk to the canvas
class UiCanvasComponentImplementationInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiCanvasComponentImplementationInterface() {}

    //! Mark the render graph for the canvas as dirty. This will cause the render graph to get
    //! cleared and rebuilt on the next render.
    virtual void MarkRenderGraphDirty() = 0;

public: // static member data

    //! Only one component on an entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiCanvasComponentImplementationInterface> UiCanvasComponentImplementationBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that listeners need to implement to be notified of canvas actions
class UiCanvasActionNotification
    : public AZ::ComponentBus
{
public:

    //////////////////////////////////////////////////////////////////////////
    // EBusTraits overrides
    static const bool EnableEventQueue = true;
    //////////////////////////////////////////////////////////////////////////

public: // member functions

    virtual ~UiCanvasActionNotification(){}

    //! Called when the canvas sends an action to the listener
    virtual void OnAction(AZ::EntityId entityId, const LyShine::ActionName& actionName) = 0;
};

typedef AZ::EBus<UiCanvasActionNotification> UiCanvasNotificationBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that listeners need to implement to be notified when the draw order of any
//! canvas changes
class UiCanvasOrderNotification
    : public AZ::EBusTraits
{
public: // member functions

    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

    virtual ~UiCanvasOrderNotification(){}

    //! Called when the draw order setting for a canvas changes
    //! Note this is used to update the order in the UiCanvasManager so that
    //! order has not been updated when this fires.
    virtual void OnCanvasDrawOrderChanged(AZ::EntityId canvasEntityId) = 0;
};

typedef AZ::EBus<UiCanvasOrderNotification> UiCanvasOrderNotificationBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that listeners need to implement to be notified when any canvas has been
//! enabled or disabled
class UiCanvasEnabledStateNotification
    : public AZ::EBusTraits
{
public: // member functions

    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

    virtual ~UiCanvasEnabledStateNotification() {}

    //! Called when the canvas was enabled or disabled
    virtual void OnCanvasEnabledStateChanged(AZ::EntityId canvasEntityId, bool enabled) = 0;
};

typedef AZ::EBus<UiCanvasEnabledStateNotification> UiCanvasEnabledStateNotificationBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that listeners need to implement to be notified of canvas size or scale changes
class UiCanvasSizeNotification
    : public AZ::EBusTraits
{
public:
    virtual ~UiCanvasSizeNotification() {}

    //! Called when the target canvas size or uniform device scale changes.
    virtual void OnCanvasSizeOrScaleChange(AZ::EntityId canvasEntityId) = 0;
};

typedef AZ::EBus<UiCanvasSizeNotification> UiCanvasSizeNotificationBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that listeners need to implement to be notified of changes to the canvas
//! pixel alignment settings
class UiCanvasPixelAlignmentNotification
    : public AZ::ComponentBus
{
public:
    virtual ~UiCanvasPixelAlignmentNotification() {}

    //! Called when the pixel alignment setting for the canvas changes
    virtual void OnCanvasPixelAlignmentChange() {}

    //! Called when the text pixel alignment setting for the canvas changes
    virtual void OnCanvasTextPixelAlignmentChange() {}
};

typedef AZ::EBus<UiCanvasPixelAlignmentNotification> UiCanvasPixelAlignmentNotificationBus;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface class that listeners need to implement to be notified of canvas input.
//! Note that interactables already get methods called on them when they themselves are interacted
//! with. This notification bus is intended for other entities or Lua to know when some other
//! entities are interacted with.
class UiCanvasInputNotifications
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiCanvasInputNotifications() {}

    //! Called when an element is pressed. Will return an invalid entity id if no interactable was
    //! pressed.
    virtual void OnCanvasPrimaryPressed([[maybe_unused]] AZ::EntityId entityId) {};

    //! Called when an element is released. The released entity that is sent is the entity that was
    //! active (if any).
    virtual void OnCanvasPrimaryReleased([[maybe_unused]] AZ::EntityId entityId) {};

    //! Called when an element is pressed. Will be an invalid entity id if no interactable was
    //! pressed.
    virtual void OnCanvasMultiTouchPressed([[maybe_unused]] AZ::EntityId entityId, [[maybe_unused]] int multiTouchIndex) {};

    //! Called when an element is released. The released entity that is sent is the entity that was
    //! active (if any).
    virtual void OnCanvasMultiTouchReleased([[maybe_unused]] AZ::EntityId entityId, [[maybe_unused]] int multiTouchIndex) {};

    //! Called when an element starts being hovered
    virtual void OnCanvasHoverStart([[maybe_unused]] AZ::EntityId entityId) {};

    //! Called when an element ends being hovered
    virtual void OnCanvasHoverEnd([[maybe_unused]] AZ::EntityId entityId) {};

    //! Called when the enter key is pressed
    virtual void OnCanvasEnterPressed([[maybe_unused]] AZ::EntityId entityId) {};

    //! Called when the enter key is released
    virtual void OnCanvasEnterReleased([[maybe_unused]] AZ::EntityId entityId) {};
};

typedef AZ::EBus<UiCanvasInputNotifications> UiCanvasInputNotificationBus;
