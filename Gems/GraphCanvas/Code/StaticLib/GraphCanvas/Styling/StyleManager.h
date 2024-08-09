/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/Component.h>

#include <AzFramework/Asset/AssetCatalogBus.h>

#include <GraphCanvas/Components/LayerBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Styling/definitions.h>
#include <GraphCanvas/Styling/Selector.h>
#include <GraphCanvas/Styling/Style.h>

class QVariant;

namespace GraphCanvas
{
    namespace Deprecated
    {
        // DummyComponent here to allow for older graphs that accidentally
        // Serialized this out to continue to function.
        class StyleSheetComponent
            : public AZ::Component
        {        
        public:
            AZ_COMPONENT(StyleSheetComponent, "{34B81206-2C69-4886-945B-4A9ECC0FDAEE}");
            static void Reflect(AZ::ReflectContext* context);
            
            // AZ::Component
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.push_back(AZ_CRC_CE("GraphCanvas_StyleService"));
            }

            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
            {
                (void)dependent;
            }

            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
            {
                (void)required;
            }
            ////
            
            void Activate() {}
            void Deactivate() {}
        };
    }

    namespace Styling
    {
        class Parser;
    }

    class TintableIcon
    {
    public:
        virtual ~TintableIcon() = default;

        virtual AZ::Crc32 GetIconId() const = 0;

        QPixmap* CreatePixmap(const AZStd::vector< QColor >& colors) const;
        QPixmap* CreatePixmap(const AZStd::vector< QBrush >& brushes) const;
        QPixmap* CreatePixmap(const AZStd::vector< Styling::StyleHelper* >& palettes) const;

    protected:

        AZStd::vector< QColor > m_paletteSwatches;
        QPixmap* m_sourcePixmap;
    };
    
    class StyleManager
        : protected StyleManagerRequestBus::Handler
        , private AzFramework::AssetCatalogEventBus::Handler
    {
    private:
        typedef AZStd::unordered_map< AZ::Crc32, QPixmap*> PalettesToIconDescriptorMap;

    public:
        // Takes in the Id used to identify this StyleSheet and a relative path to the style sheet json file.
        //
        // This path should be relative to your gems <GraphCanvas GemRoot>/Assets folder.
        StyleManager(const EditorId& editorId, AZStd::string_view assetPath);
        virtual ~StyleManager();

       
        // AzFramework::AssetCatalogEventBus
        void OnCatalogAssetChanged(const AZ::Data::AssetId& asset) override;
        ////

        // StyleManagerRequestBus::Handler
        AZ::EntityId ResolveStyles(const AZ::EntityId& object) const override;

        void RegisterDataPaletteStyle(const AZ::Uuid& dataType, const AZStd::string& palette) override;

        AZStd::string GetDataPaletteStyle(const AZ::Uuid& dataType) const override;
        const Styling::StyleHelper* FindDataColorPalette(const AZ::Uuid& uuid) override;
        QColor GetDataTypeColor(const AZ::Uuid& dataType) override;
        const QPixmap* GetDataTypeIcon(const AZ::Uuid& dataType) override;
        const QPixmap* GetMultiDataTypeIcon(const AZStd::vector<AZ::Uuid>& dataTypes) override;

        const Styling::StyleHelper* FindColorPalette(const AZStd::string& paletteString) override;
        QColor GetPaletteColor(const AZStd::string& palette) override;
        const QPixmap* GetPaletteIcon(const AZStd::string& iconStyle, const AZStd::string& palette) override;
        const QPixmap* GetConfiguredPaletteIcon(const PaletteIconConfiguration& paletteConfiguration) override;
        const Styling::StyleHelper* FindPaletteIconStyleHelper(const PaletteIconConfiguration& paletteConfiguration) override;

        QPixmap* CreateIcon(const QColor& colorType, const AZStd::string& iconStyle) override;
        QPixmap* CreateIconFromConfiguration(const PaletteIconConfiguration& paletteConfiguration) override;
        QPixmap* CreateMultiColoredIcon(const AZStd::vector<QColor>& colorType, float transitionPercent, const AZStd::string& iconStyle) override;

        QPixmap* CreateColoredPatternPixmap(const AZStd::vector<QColor>& colorTypes, const AZStd::string& patternName) override;
        const QPixmap* CreatePatternPixmap(const AZStd::vector<AZStd::string>& palettes, const AZStd::string& patternName) override;

        AZStd::vector<AZStd::string> GetColorPaletteStyles() const override;

        QPixmap* FindPixmap(const AZ::Crc32& keyName) override;
        void CachePixmap(const AZ::Crc32& keyName, QPixmap* pixmap) override;

        int FindLayer(AZStd::string_view layer) override;

        int GetSteppedWidth(int width) override;
        int GetSteppedHeight(int height) override;
        ////

    protected:
        StyleManager() = delete;

    private:

        QPixmap* CreateIcon(QBrush& brush, const AZStd::string& iconStyle);
        QPixmap* CreateIcon(const Styling::StyleHelper& styleHelper, const AZStd::string& iconStyle);

        void LoadStyleSheet();

        void ClearStyles();

        void ClearCache();
        void RefreshColorPalettes();

        void PopulateDataPaletteMapping();

        const TintableIcon* FindPatternIcon(AZ::Crc32 patternIcon) const;
        void AddPatternIcon(TintableIcon* icon);

        Styling::StyleHelper* FindCreateStyleHelper(const AZStd::string& paletteString);

        const QPixmap* CreateAndCacheIcon(const PaletteIconConfiguration& configuration);
        const QPixmap* FindCachedIcon(const PaletteIconConfiguration& configuration);

        const QPixmap* FindPatternCache(const AZ::Crc32& patternCache) const;
        void AddPatternCache(const AZ::Crc32& patternCache, QPixmap* pixmap);

        EditorId m_editorId;
        Styling::StyleVector m_styles;
        
        AZStd::string m_assetPath;
        AZ::Data::AssetId m_styleAssetId;

        AZStd::unordered_map<AZStd::string, Styling::StyleHelper*> m_styleTypeHelpers;

        AZStd::unordered_map<AZ::Uuid, AZStd::string > m_dataPaletteMapping;
        AZStd::unordered_map<AZStd::string, PalettesToIconDescriptorMap> m_iconMapping;

        AZStd::unordered_map< AZ::Crc32, QPixmap* > m_pixmapCache;

        AZStd::unordered_map< AZ::Crc32, TintableIcon* > m_patternIcons;
        AZStd::unordered_map< AZ::Crc32, QPixmap* > m_patternCache;

        QList<QVariant> m_widthStepHelper;
        QList<QVariant> m_heightStepHelper;

        friend class Styling::Parser;
    };
}
