/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzFramework/PaintBrush/PaintBrush.h>
#include <AzFramework/PaintBrush/PaintBrushNotificationBus.h>
#include <GradientSignal/Components/ImageGradientModification.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/GradientTransformRequestBus.h>
#include <GradientSignal/Ebuses/ImageGradientRequestBus.h>
#include <GradientSignal/Ebuses/ImageGradientModificationBus.h>
#include <GradientSignal/Util.h>

namespace GradientSignal
{
    // Custom JSON serializer for ImageGradientConfig to handle version conversion
    class JsonImageGradientConfigSerializer
        : public AZ::BaseJsonSerializer
    {
    public:
        AZ_RTTI(GradientSignal::JsonImageGradientConfigSerializer, "{C5B982C8-2E81-45C3-8932-B6F54B28F493}", AZ::BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

        AZ::JsonSerializationResult::Result Load(
            void* outputValue, const AZ::Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context) override;
    };

    enum class ChannelToUse : AZ::u8
    {
        Red,
        Green,
        Blue,
        Alpha,
        //! "Terrarium" is an image - based terrain file format as defined here:
        //!     https://www.mapzen.com/blog/terrain-tile-service/
        //! According to the website : "Terrarium format PNG tiles contain raw elevation
        //! data in meters, in Mercator projection (EPSG:3857)."
        Terrarium
    };

    //! Custom scaling to apply to the values retrieved from the image data
    enum class CustomScaleType : AZ::u8
    {
        None,                   //! Data left as-is, no scaling calculation performed
        Auto,                   //! Automatically scale based on the min/max values in the data
        Manual                  //! Scale according to m_scaleRangeMin and m_scaleRangeMax
    };

    //! Sampling type to use for the image data
    enum class SamplingType : AZ::u8
    {
        Point,                  //! Point sampling just queries the X,Y point as specified (Default)
        Bilinear,               //! Apply a bilinear filter to the image data
        Bicubic,                //! Apply a bicubic filter to the image data
    };

    class ImageGradientConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(ImageGradientConfig, AZ::SystemAllocator);
        AZ_RTTI(ImageGradientConfig, "{1BDB5DA4-A4A8-452B-BE6D-6BD451D4E7CD}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        bool GetManualScaleVisibility() const;

        bool IsImageAssetReadOnly() const;
        bool AreImageOptionsReadOnly() const;

        AZStd::string GetImageAssetPropertyName() const;
        void SetImageAssetPropertyName(const AZStd::string& imageAssetPropertyName);

        // Serialized properties that control the image data.

        //! The image asset used for the image gradient.
        AZ::Data::Asset<AZ::RPI::StreamingImageAsset> m_imageAsset = { AZ::Data::AssetLoadBehavior::QueueLoad };
        //! How often the image should repeat within its shape bounds.
        AZ::Vector2 m_tiling = AZ::Vector2::CreateOne();
        //! Which color channel to use from the image.
        ChannelToUse m_channelToUse = ChannelToUse::Red;
        //! Which mipmap level to use from the image.
        AZ::u32 m_mipIndex = 0;
        //! Scale type to apply to the image data. (Auto = auto-scale data to use full 0-1 range, Manual = use scaleRangeMin/Max)
        CustomScaleType m_customScaleType = CustomScaleType::None;
        float m_scaleRangeMin = 0.0f;
        float m_scaleRangeMax = 1.0f;
        //! Which sampling method to use for querying gradient values (Point = exact image data, Bilinear = interpolated image data)
        SamplingType m_samplingType = SamplingType::Point;

        // Non-serialized properties used by the Editor for display purposes.

        //! The number of active image modification sessions.
        int m_numImageModificationsActive = 0;

        //! Label to use for the image asset. This gets modified to show current asset loading/processing state.
        AZStd::string m_imageAssetPropertyLabel = "Image Asset";
    };

    inline constexpr AZ::TypeId ImageGradientComponentTypeId{ "{4741F079-157F-457E-93E0-D6BA4EAF76FE}" };

    /**
    * calculates a gradient value based on image data
    */
    class ImageGradientComponent
        : public AZ::Component
        , private AZ::Data::AssetBus::Handler
        , private GradientRequestBus::Handler
        , private ImageGradientRequestBus::Handler
        , private ImageGradientModificationBus::Handler
        , private AzFramework::PaintBrushNotificationBus::Handler
        , private GradientTransformNotificationBus::Handler
    {
    public:
        friend class EditorImageGradientComponent;
        AZ_COMPONENT(ImageGradientComponent, ImageGradientComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        ImageGradientComponent(const ImageGradientConfig& configuration);
        ImageGradientComponent() = default;
        ~ImageGradientComponent() = default;

        // AZ::Component overrides...
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        // GradientRequestBus overrides...
        float GetValue(const GradientSampleParams& sampleParams) const override;
        void GetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const override;

        // AZ::Data::AssetBus overrides...
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        // ImageGradientRequestBus overrides...
        AZStd::string GetImageAssetPath() const override;
        AZStd::string GetImageAssetSourcePath() const override;
        void SetImageAssetPath(const AZStd::string& assetPath) override;
        void SetImageAssetSourcePath(const AZStd::string& assetPath) override;
        uint32_t GetImageHeight() const override;
        uint32_t GetImageWidth() const override;
        AZ::Vector2 GetImagePixelsPerMeter() const override;

        // ImageGradientModificationBus overrides...
        void StartImageModification() override;
        void EndImageModification() override;
        void GetPixelValuesByPosition(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const override;
        void SetPixelValuesByPosition(AZStd::span<const AZ::Vector3> positions, AZStd::span<const float> values) override;

        void GetPixelIndicesForPositions(AZStd::span<const AZ::Vector3> positions, AZStd::span<PixelIndex> outIndices) const override;
        void GetPixelValuesByPixelIndex(AZStd::span<const PixelIndex> positions, AZStd::span<float> outValues) const override;
        void SetPixelValuesByPixelIndex(AZStd::span<const PixelIndex> positions, AZStd::span<const float> values) override;

        bool ImageIsModified() const;

    protected:
        AZ::Data::Asset<AZ::RPI::StreamingImageAsset> GetImageAsset() const;
        void SetImageAsset(const AZ::Data::Asset<AZ::RPI::StreamingImageAsset>& asset);

        AZStd::vector<float>* GetImageModificationBuffer();

        // PaintBrushNotificationBus overrides...
        void OnPaintModeBegin() override;
        void OnPaintModeEnd() override;
        AZ::Color OnGetColor(const AZ::Vector3& brushCenter) const override;

        // GradientTransformNotificationBus overrides...
        void OnGradientTransformChanged(const GradientTransform& newTransform) override;

        void CreateImageModificationBuffer();
        void ClearImageModificationBuffer();
        bool ModificationBufferIsActive() const;
        void UpdateCachedImageBufferData(const AZ::RHI::ImageDescriptor& imageDescriptor, AZStd::span<const uint8_t> imageData);

        void GetSubImageData();
        void GetValuesInternal(SamplingType samplingType, AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const;
        float GetValueFromImageData(SamplingType samplingType, const AZ::Vector3& uvw, float defaultValue) const;

        //! Read the pixel from our image data at the given XY coordinates.
        //! This will read from image modification buffer if it exists or else from the image asset, using the component's
        //! mip and channel settings.
        //! Note that image space Y is inverted from world space because in images, 0 is the top corner and +Y goes down, but in world
        //! space we want 0 to be the bottom, and +Y goes up. If you want to get the pixel value using a Y calculated from world space,
        //! call InvertYAndGetPixelValue() instead.
        //! @param x The X coordinate in image space.
        //! @param y The Y coordinate in image space.
        float GetPixelValue(AZ::u32 x, AZ::u32 y) const;

        //! Read the pixel from our image data at the given X coordinate and an inverted Y coordinate.
        //! This will read from image modification buffer if it exists or else from the image asset, using the component's
        //! mip and channel settings.
        //! This is a convenience method that will invert our Y coordinate before calling GetPixelValue() so that we can take
        //! a Y coordinate that was calculated from world space axes and invert it into image space as a part of doing the pixel lookup.
        //! @param x The X coordinate in image space.
        //! @param invertedY The inverted Y coordinate in image space.
        float InvertYAndGetPixelValue(AZ::u32 x, AZ::u32 invertedY) const;

        float GetTerrariumPixelValue(AZ::u32 x, AZ::u32 y) const;
        void SetupMultiplierAndOffset(float min, float max);
        void SetupDefaultMultiplierAndOffset();
        void SetupAutoScaleMultiplierAndOffset();
        void SetupManualScaleMultiplierAndOffset();
        void Get4x4Neighborhood(uint32_t x, uint32_t y, AZStd::array<AZStd::array<float, 4>, 4>& values) const;
        float GetClampedValue(int32_t x, int32_t y) const;
        float GetValueForSamplingType(SamplingType samplingType, AZ::u32 x0, AZ::u32 y0, float pixelX, float pixelY) const;

        float GetTilingX() const override;
        void SetTilingX(float tilingX) override;

        float GetTilingY() const override;
        void SetTilingY(float tilingY) override;

        bool PixelIndexIsValid(const PixelIndex& pixelIndex) const;
        PixelIndex GetPixelIndexForPositionInternal(const AZ::Vector3& position) const;

    private:
        ImageGradientConfig m_configuration;
        mutable AZStd::shared_mutex m_queryMutex;
        GradientTransform m_gradientTransform;
        ChannelToUse m_currentChannel = ChannelToUse::Red;
        CustomScaleType m_currentScaleType = CustomScaleType::None;

        //! The multiplier and offset values are used for scaling our input pixel values to different ranges.
        float m_multiplier = 1.0f;
        float m_offset = 0.0f;

        //! Keep track of the min/max values that occur in the data so that if we modify the pixel values, we can readjust
        //! the scaling values appropriately.
        float m_minValue = 0.0f;
        float m_maxValue = 1.0f;

        AZ::u32 m_currentMipIndex = 0;
        int32_t m_maxX = 0;
        int32_t m_maxY = 0;
        SamplingType m_currentSamplingType = SamplingType::Point;

        //! Cached information for our loaded image data.
        //! This can either contain information about the image data in the image asset or information about our in-memory modifications.
        AZ::RHI::ImageDescriptor m_imageDescriptor;
        AZStd::span<const uint8_t> m_imageData;

        //! Temporary buffer for runtime modifications of the image data.
        AZStd::vector<float> m_modifiedImageData;

        //! Track whether or not any data has been modified.
        bool m_imageIsModified = false;

        //! Logic for handling image modification requests from PaintBrush instances.
        //! This is only created and active between StartPaintSession / EndPaintSession calls.
        AZStd::unique_ptr<ImageGradientModifier> m_imageModifier;
    };
}
