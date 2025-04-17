/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/Bus/UiAnimateEntityBus.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/UiTransform2dBus.h>
#include <LyShine/Bus/UiLayoutFitterBus.h>
#include <LyShine/UiComponentTypes.h>
#include <LyShine/UiSerializeHelpers.h>

#include <AzCore/Component/Component.h>

// Only needed for internal unit-testing
#include <LyShine.h>

class UiCanvasComponent;
class UiElementComponent;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiTransform2dComponent
    : public AZ::Component
    , public UiTransformBus::Handler
    , public UiTransform2dBus::Handler
    , public UiAnimateEntityBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiTransform2dComponent, LyShine::UiTransform2dComponentUuid, AZ::Component);

    UiTransform2dComponent();
    ~UiTransform2dComponent() override;

    // UiTransformInterface
    float GetZRotation() override;
    void SetZRotation(float rotation) override;
    AZ::Vector2 GetScale() override;
    void SetScale(AZ::Vector2 scale) override;
    float GetScaleX() override;
    void SetScaleX(float scale) override;
    float GetScaleY() override;
    void SetScaleY(float scale) override;
    AZ::Vector2 GetPivot() override;
    void SetPivot(AZ::Vector2 pivot) override;
    float GetPivotX() override;
    void SetPivotX(float pivot) override;
    float GetPivotY() override;
    void SetPivotY(float pivot) override;
    ScaleToDeviceMode GetScaleToDeviceMode() override;
    void SetScaleToDeviceMode(ScaleToDeviceMode scaleToDeviceMode) override;

    bool GetIsFlooringOffsets() override;
    void SetIsFlooringOffsets(bool isFlooringOffsets) override;

    void GetViewportSpacePoints(RectPoints& points) final;
    AZ::Vector2 GetViewportSpacePivot() final;
    void GetTransformToViewport(AZ::Matrix4x4& mat) final;
    void GetTransformFromViewport(AZ::Matrix4x4& mat) final;
    void RotateAndScalePoints(RectPoints& points) final;

    void GetCanvasSpacePoints(RectPoints& points) final;
    AZ::Vector2 GetCanvasSpacePivot() final;
    void GetTransformToCanvasSpace(AZ::Matrix4x4& mat) final;
    void GetTransformFromCanvasSpace(AZ::Matrix4x4& mat) final;

    void GetCanvasSpaceRectNoScaleRotate(Rect& rect) final;
    void GetCanvasSpacePointsNoScaleRotate(RectPoints& points) final;
    AZ::Vector2 GetCanvasSpaceSizeNoScaleRotate() final;
    AZ::Vector2 GetCanvasSpacePivotNoScaleRotate() final;

    void GetLocalTransform(AZ::Matrix4x4& mat) final;
    void GetLocalInverseTransform(AZ::Matrix4x4& mat) final;
    bool HasScaleOrRotation() final;

    AZ::Vector2 GetViewportPosition() final;
    void SetViewportPosition(const AZ::Vector2& position) final;
    AZ::Vector2 GetCanvasPosition() final;
    void SetCanvasPosition(const AZ::Vector2& position) final;
    AZ::Vector2 GetLocalPosition() final;
    void SetLocalPosition(const AZ::Vector2& position) final;
    float GetLocalPositionX() final;
    void SetLocalPositionX(float position) final;
    float GetLocalPositionY() final;
    void SetLocalPositionY(float position) final;
    void MoveViewportPositionBy(const AZ::Vector2& offset) final;
    void MoveCanvasPositionBy(const AZ::Vector2& offset) final;
    void MoveLocalPositionBy(const AZ::Vector2& offset) final;

    bool IsPointInRect(AZ::Vector2 point) final;
    bool BoundsAreOverlappingRect(const AZ::Vector2& bound0, const AZ::Vector2& bound1) final;

    void SetRecomputeFlags(Recompute recompute) final;

    bool HasCanvasSpaceRectChanged() final;
    bool HasCanvasSpaceSizeChanged() final;
    bool HasCanvasSpaceRectChangedByInitialization() final;
    void NotifyAndResetCanvasSpaceRectChange() final;
    // ~UiTransformInterface

    // UiTransform2dInterface
    Anchors GetAnchors() override;
    void SetAnchors(Anchors anchors, bool adjustOffsets, bool allowPush) override;
    Offsets GetOffsets() override;
    void SetOffsets(Offsets offsets) override;
    void SetPivotAndAdjustOffsets(AZ::Vector2 pivot) override;
    void SetLocalWidth(float width) override;
    float GetLocalWidth() override;
    void SetLocalHeight(float height) override;
    float GetLocalHeight() override;
    // ~UiTransform2dInterface

    // UiAninmateEntityInterface
    void PropertyValuesChanged() override;
    // ~UiAninmateEntityInterface

    //! This is called from the canvas component during the update if the element was scheduled for a transform recompute
    void RecomputeTransformsAndSendNotifications();

#if defined(LYSHINE_INTERNAL_UNIT_TEST)
    static void UnitTest(CLyShine* lyshine, IConsoleCmdArgs* cmdArgs);
#endif

public:  // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("UiTransformService"));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("UiTransformService"));
    }

    static void GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    static void Reflect(AZ::ReflectContext* context);

protected: // member functions

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    //! Determine whether this element's transform is being overridden by a component on its parent
    virtual bool IsControlledByParent() const;

    //! Get the level of control of a layout fitter
    virtual UiLayoutFitterInterface::FitType GetLayoutFitterType() const;

    //! Determine whether this element's transform is not being overridden by a component on its parent
    //! This just exists to be called from the edit context setup
    bool IsNotControlledByParent() const;

    //! Get the first ancestor that has a scale to device mode affecting the same dimension as this
    //! element's scale to device mode
    AZ::EntityId GetAncestorWithSameDimensionScaleToDevice(ScaleToDeviceMode scaleToDeviceMode) const;

    //! Get a list of descendants that have a scale to device mode affecting the same dimension as this
    //! element's scale to device mode
    LyShine::EntityArray GetDescendantsWithSameDimensionScaleToDevice(ScaleToDeviceMode scaleToDeviceMode) const;

    //! Return whether there are anchors that are apart affecting the same dimension as this
    //! element's scale to device mode
    bool AreAnchorsApartInSameScaleToDeviceDimension(ScaleToDeviceMode scaleToDeviceMode) const;

    //! Return a short one line string that incudes a warning for the currently assigned scale to device mode.
    //! An empty string indicates no warnings
    AZStd::string GetScaleToDeviceModeWarningText() const;

    //! Return a tooltip string describing the warning for the currently assigned scale to device mode.
    //! An empty string indicates no warnings
    AZStd::string GetScaleToDeviceModeWarningTooltipText() const;

    //! This is used to dynamically change the label for the Anchor property in the properties pane
    //! as a way to display a "disabled" stated for this component when the transform is controlled by the
    //! parent.
    const char* GetAnchorPropertyLabel() const;

    //! Helper function to get the canvas entity ID for canvas containing this element
    AZ::EntityId GetCanvasEntityId();

    //! Helper function to get the canvas component for canvas containing this element
    UiCanvasComponent* GetCanvasComponent() const;

    //! ChangeNotify function for when a transform property is changed
    void OnTransformPropertyChanged();

    //! If m_recomputeTransformToViewport is true then recompute the transform and clear the flag
    void RecomputeTransformToViewportIfNeeded();

    //! If m_recomputeTransformToCanvasSpace is true then recompute the transform and clear the flag
    void RecomputeTransformToCanvasSpaceIfNeeded();

private: // member functions

    AZ_DISABLE_COPY_MOVE(UiTransform2dComponent);

    //! Get the scale with the uniform device scale factored in, if m_scaleToDevice is true
    AZ::Vector2 GetScaleAdjustedForDevice();

    //! Calculates the rect if m_recomputeCanvasSpaceRect dirty flag is set
    void CalculateCanvasSpaceRect();

    //! Get the position of the element's anchors in canvas space
    AZ::Vector2 GetCanvasSpaceAnchorsCenterNoScaleRotate();

    // Get a pointer to this entity's UiElementComponent.
    UiElementComponent* GetElementComponent() const;

    // Get a pointer to the parent element's transform component. Returns nullptr if no parent.
    UiTransform2dComponent* GetParentTransformComponent() const;

    // Get a pointer to the given child element's transform component. Returns nullptr if no parent.
    UiTransform2dComponent* GetChildTransformComponent(int index) const;

    // Used to check that FixupPostLoad has been called
    bool IsFullyInitialized() const;

    // Display a warning that the component is not yet fully initialized
    void EmitNotInitializedWarning() const;

    // Given a scale apply the canvases device scale to it according to the m_scaleToDeviceMode setting
    void ApplyDeviceScale(AZ::Vector2& scale);

private: // static member functions

    static bool VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement);

    //! Determine whether the specified scale to device mode affects horizontal scale
    static bool DoesScaleToDeviceModeAffectX(ScaleToDeviceMode scaleToDeviceMode);

    //! Determine whether the specified scale to device mode affects vertical scale
    static bool DoesScaleToDeviceModeAffectY(ScaleToDeviceMode scaleToDeviceMode);

private: // data

    Anchors m_anchors;  // initialized by Anchors constructor
    Offsets m_offsets;  // initialized by Offsets constructor
    AZ::Vector2 m_pivot;
    float m_rotation;
    AZ::Vector2 m_scale;

    bool m_isFlooringOffsets = false;

    //! Cached transform to viewport space. Gets recalculated if the m_recomputeTransformToViewport dirty flag is set
    AZ::Matrix4x4 m_transformToViewport;

    //! Cached transform to canvas space. Gets recalculated if the m_recomputeTransformToCanvasSpace dirty flag is set
    AZ::Matrix4x4 m_transformToCanvasSpace;

    //! Cached rect in CanvasNoScaleRotateSpace.
    //! Gets recalculated if the m_recomputeCanvasSpaceRect dirty flag is set
    Rect m_rect;

    //! The previously cached rect in CanvasNoScaleRotateSpace.
    //! Initialized when m_rect is calculated for the first time.
    //! Updated to m_rect when a rect change notification is sent
    Rect m_prevRect;

    //! True if m_rect has been calculated
    bool m_rectInitialized;

    //! True if the rect has changed due to it being calculated for the first time. In this
    //! case m_prevRect will equal m_rect
    bool m_rectChangedByInitialization;

    //! If this is not set to None then the canvas scale is applied, in addition to m_scale, according to this mode
    ScaleToDeviceMode m_scaleToDeviceMode;

    //! If this is true, then the transform stored in m_transformToViewport is dirty and needs to be recomputed
    bool m_recomputeTransformToViewport;

    //! If this is true, then the transform stored in m_transformToCanvasSpace is dirty and needs to be recomputed
    bool m_recomputeTransformToCanvasSpace;

    bool m_recomputeCanvasSpaceRect;

    //! Pointer directly to the UiElementComponent for this entity. Cached for performance after profiling.
    UiElementComponent* m_elementComponent = nullptr;
};
