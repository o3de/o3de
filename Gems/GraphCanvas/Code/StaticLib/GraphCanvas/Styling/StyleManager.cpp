/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QBitmap>
#include <QPainter>
AZ_POP_DISABLE_WARNING

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <GraphCanvas/Styling/StyleManager.h>

#include <GraphCanvas/Editor/GraphCanvasProfiler.h>
#include <GraphCanvas/Styling/Style.h>
#include <GraphCanvas/Styling/StyleHelper.h>
#include <GraphCanvas/Styling/Parser.h>
#include <GraphCanvas/Utils/QtDrawingUtils.h>
#include <GraphCanvas/Utils/QtVectorMath.h>

namespace
{
    struct StyleMatch
    {
        GraphCanvas::Styling::Style* style;
        int complexity;

        bool operator<(const StyleMatch& o) const
        {
            if (complexity > 0 && o.complexity > 0)
            {
                return complexity > o.complexity;
            }
            else if (complexity < 0 && o.complexity < 0)
            {
                return complexity < o.complexity;
            }
            else if (complexity < 0)
            {
                return false;
            }
            else
            {
                return true;
            }
        }
    };

    class HexagonIcon
        : public GraphCanvas::TintableIcon
    {
    public:
        AZ_CLASS_ALLOCATOR(HexagonIcon, AZ::SystemAllocator, 0);

        HexagonIcon()
        {
            m_paletteSwatches.push_back(QColor(0, 0, 0));
            m_sourcePixmap = new QPixmap(16, 16);
            m_sourcePixmap->fill(Qt::transparent);

            QPainter painter(m_sourcePixmap);
            painter.setRenderHint(QPainter::RenderHint::Antialiasing);

            QPen pen;
            pen.setWidth(4);
            pen.setColor(QColor(0, 0, 0));

            painter.setPen(pen);
            painter.drawLine(QPointF(0, 16), QPointF(8, 10));
            painter.drawLine(QPointF(16, 16), QPointF(8, 10));
            painter.drawLine(QPointF(8, 0), QPointF(8, 10));
        }

        AZ::Crc32 GetIconId() const override { return AZ_CRC("HexagonIcon", 0x34053842); }
    };

    class CheckerboardIcon
        : public GraphCanvas::TintableIcon
    {
    public:
        AZ_CLASS_ALLOCATOR(CheckerboardIcon, AZ::SystemAllocator, 0);

        CheckerboardIcon()
        {
            m_paletteSwatches.push_back(QColor(0, 0, 0));

            m_sourcePixmap = new QPixmap(16, 16);
            m_sourcePixmap->fill(Qt::transparent);

            QPainter painter(m_sourcePixmap);
            painter.setRenderHint(QPainter::RenderHint::Antialiasing);

            painter.fillRect(QRectF(0, 0, 8, 8), QColor(0, 0, 0));
            painter.fillRect(QRectF(8, 8, 8, 8), QColor(0, 0, 0));
        }

        AZ::Crc32 GetIconId() const override { return AZ_CRC("CheckerboardIcon", 0x782adcad); }
    };

    class TriColorCheckerboardIcon
        : public GraphCanvas::TintableIcon
    {
    public:
        AZ_CLASS_ALLOCATOR(TriColorCheckerboardIcon, AZ::SystemAllocator, 0);

        TriColorCheckerboardIcon()
        {
            m_paletteSwatches.push_back(QColor(0, 0, 0));
            m_paletteSwatches.push_back(QColor(1, 1, 1));

            m_sourcePixmap = new QPixmap(16, 16);
            m_sourcePixmap->fill(Qt::transparent);

            QPainter painter(m_sourcePixmap);
            painter.setRenderHint(QPainter::RenderHint::Antialiasing);

            painter.fillRect(QRectF(0, 0, 8, 8), QColor(0, 0, 0));
            painter.fillRect(QRectF(8, 8, 8, 8), QColor(1, 1, 1));
        }

        AZ::Crc32 GetIconId() const override { return AZ_CRC("TriColorCheckerboardIcon", 0x9f93d99d); }
    };
}

namespace GraphCanvas
{
    ////////////////////////
    // StyleSheetComponent
    ////////////////////////

    namespace Deprecated
    {
        void StyleSheetComponent::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            Styling::Style::Reflect(context);

            serializeContext->Class<StyleSheetComponent, AZ::Component>()
                ->Version(3)
                ;
        }
    }

    /////////////////
    // TintableIcon
    /////////////////

    QPixmap* TintableIcon::CreatePixmap(const AZStd::vector< QColor >& colors) const
    {
        if (m_sourcePixmap == nullptr)
        {
            return nullptr;
        }

        QPixmap* pixmap = new QPixmap(m_sourcePixmap->size());
        pixmap->fill(Qt::transparent);

        if (colors.empty())
        {
            pixmap->fromImage(m_sourcePixmap->toImage());
            return pixmap;
        }

        QRectF drawRect = QRectF(0, 0, pixmap->width(), pixmap->height());

        QPainter painter(pixmap);
        painter.setRenderHint(QPainter::Antialiasing);

        for (int i = 0; i < m_paletteSwatches.size(); ++i)
        {
            QBitmap mask = m_sourcePixmap->createMaskFromColor(m_paletteSwatches[i], Qt::MaskOutColor);
            painter.setClipRegion(QRegion(mask));

            painter.fillRect(drawRect, colors[i % colors.size()]);
        }

        return pixmap;
    }

    QPixmap* TintableIcon::CreatePixmap(const AZStd::vector< QBrush >& brushes) const
    {
        if (m_sourcePixmap == nullptr)
        {
            return nullptr;
        }

        QPixmap* pixmap = new QPixmap(m_sourcePixmap->size());
        pixmap->fill(Qt::transparent);

        if (brushes.empty())
        {
            pixmap->fromImage(m_sourcePixmap->toImage());
            return pixmap;
        }

        QRectF drawRect = QRectF(0, 0, pixmap->width(), pixmap->height());

        QPainter painter(pixmap);
        painter.setRenderHint(QPainter::Antialiasing);

        for (int i=0; i < m_paletteSwatches.size(); ++i)
        {
            QBitmap mask = m_sourcePixmap->createMaskFromColor(m_paletteSwatches[i], Qt::MaskOutColor);
            painter.setClipRegion(QRegion(mask));

            painter.fillRect(drawRect, brushes[i % brushes.size()]);
        }

        return pixmap;
    }

    QPixmap* TintableIcon::CreatePixmap(const AZStd::vector< Styling::StyleHelper* >& palettes) const
    {
        if (m_sourcePixmap == nullptr)
        {
            return nullptr;
        }

        QPixmap* pixmap = new QPixmap(m_sourcePixmap->size());
        pixmap->fill(Qt::transparent);

        if (palettes.empty())
        {
            pixmap->fromImage(m_sourcePixmap->toImage());
            return pixmap;
        }

        QRectF drawRect = QRectF(0, 0, pixmap->width(), pixmap->height());

        QPainter painter(pixmap);
        painter.setRenderHint(QPainter::Antialiasing);

        for (int i = 0; i < m_paletteSwatches.size(); ++i)
        {
            QBitmap mask = m_sourcePixmap->createMaskFromColor(m_paletteSwatches[i], Qt::MaskOutColor);
            painter.setClipRegion(QRegion(mask));

            QtDrawingUtils::FillArea(painter, drawRect, (*palettes[i%palettes.size()]));
        }

        return pixmap;
    }

    /////////////////
    // StyleManager
    /////////////////

    StyleManager::StyleManager(const EditorId& editorId, AZStd::string_view assetPath)
        : m_editorId(editorId)
        , m_assetPath(assetPath)
    {
        StyleManagerRequestBus::Handler::BusConnect(m_editorId);

        AZ::Data::AssetInfo assetInfo;
        AZStd::string watchFolder;

        bool foundInfo = false;

        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(foundInfo, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, m_assetPath.c_str(), assetInfo, watchFolder);

        if (foundInfo)
        {
            m_styleAssetId = assetInfo.m_assetId;
            AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        }

        LoadStyleSheet();
        PopulateDataPaletteMapping();

        AddPatternIcon(aznew HexagonIcon());
        AddPatternIcon(aznew CheckerboardIcon());
        AddPatternIcon(aznew TriColorCheckerboardIcon());

        RefreshColorPalettes();
    }

    StyleManager::~StyleManager()
    {
        for (auto& mapPair : m_styleTypeHelpers)
        {
            delete mapPair.second;
        }

        ClearCache();
        ClearStyles();
    }

    void StyleManager::OnCatalogAssetChanged(const AZ::Data::AssetId& asset)
    {
        if (asset == m_styleAssetId)
        {
            LoadStyleSheet();
        }
    }

    void StyleManager::LoadStyleSheet()
    {
        AZStd::string file = AZStd::string::format("@products@/%s", m_assetPath.c_str());

        AZ::IO::FileIOBase* fileBase = AZ::IO::FileIOBase::GetInstance();

        if (fileBase->Exists(file.c_str()))
        {
            AZ::IO::FileIOStream fileStream;

            fileStream.Open(file.c_str(), AZ::IO::OpenMode::ModeRead);

            if (fileStream.IsOpen())
            {
                AZ::u64 fileSize = fileStream.GetLength();

                AZStd::vector<char> buffer;
                buffer.resize(fileSize + 1);

                if (fileStream.Read(fileSize, buffer.data()))
                {
                    rapidjson::Document styleSheet;
                    styleSheet.Parse(buffer.data());

                    if (styleSheet.HasParseError())
                    {
                        rapidjson::ParseErrorCode errCode = styleSheet.GetParseError();
                        QString errMessage = QString("Parse Error: %1 at offset: %2").arg(errCode).arg(styleSheet.GetErrorOffset());

                        AZ_Warning("GraphCanvas", false, "%s", errMessage.toUtf8().data());
                    }
                    else
                    {
                        m_widthStepHelper.clear();
                        m_heightStepHelper.clear();

                        Styling::Parser::Parse((*this), styleSheet);
                        RefreshColorPalettes();
                        ClearCache();
                        StyleManagerNotificationBus::Event(m_editorId, &StyleManagerNotifications::OnStylesLoaded);

                        {
                            Styling::StyleHelper* helper = FindCreateStyleHelper("Sizing_WidthSteps");

                            if (helper != nullptr)
                            {
                                m_widthStepHelper = helper->GetAttribute(Styling::Attribute::Steps, QList<QVariant>());
                            }
                        }

                        {
                            Styling::StyleHelper* helper = FindCreateStyleHelper("Sizing_HeightSteps");

                            if (helper != nullptr)
                            {
                                m_heightStepHelper = helper->GetAttribute(Styling::Attribute::Steps, QList<QVariant>());
                            }
                        }
                    }
                }
                else
                {
                    AZ_Error("GraphCanvas", false, "Failed to read StyleSheet at path(%s)", file.c_str());
                }
            }
            else
            {
                AZ_Error("GraphCanvas", false, "Failed to load StyleSheet at path(%s).", file.c_str());
            }
        }
        else
        {
            AZ_Error("GraphCanvas", false, "Could not find StyleSheet at path(%s)", file.c_str());
        }
    }

    AZ::EntityId StyleManager::ResolveStyles(const AZ::EntityId& object) const
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

        Styling::SelectorVector selectors;
        StyledEntityRequestBus::EventResult(selectors, object, &StyledEntityRequests::GetStyleSelectors);

        QVector<StyleMatch> matches;
        for (const auto& style : m_styles)
        {
            GRAPH_CANVAS_DETAILED_PROFILE_SCOPE("StyleManager::ResolveStyles::StyleMatching");
            int complexity = style->Matches(object);
            if (complexity != 0)
            {
                matches.push_back({ style, complexity });
            }
        }

        {
            GRAPH_CANVAS_DETAILED_PROFILE_SCOPE("StyleManager::ResolveStyles::Sorting");
            std::stable_sort(matches.begin(), matches.end());
        }
        Styling::StyleVector result;
        result.reserve(matches.size());
        const auto& constMatches = matches;
        for (auto& match : constMatches)
        {
            GRAPH_CANVAS_DETAILED_PROFILE_SCOPE("StyleManager::ResolveStyles::ResultConstruction");
            result.push_back(match.style);
        }

        auto computed = new Styling::ComputedStyle(m_editorId, selectors, std::move(result));

        AZ::Entity* entity = new AZ::Entity;
        entity->AddComponent(computed);
        entity->Init();
        entity->Activate();

        return entity->GetId();
    }

    void StyleManager::RegisterDataPaletteStyle(const AZ::Uuid& dataType, const AZStd::string& palette)
    {
        m_dataPaletteMapping[dataType] = palette;
    }

    AZStd::string StyleManager::GetDataPaletteStyle(const AZ::Uuid& dataType) const
    {
        if (dataType.IsNull())
        {
            return "UnknownDataColorPalette";
        }
        else
        {
            auto mapIter = m_dataPaletteMapping.find(dataType);

            if (mapIter == m_dataPaletteMapping.end())
            {
                return "ObjectDataColorPalette";
            }
            else
            {
                return mapIter->second;
            }
        }
    }

    const Styling::StyleHelper* StyleManager::FindDataColorPalette(const AZ::Uuid& dataType)
    {
        return FindCreateStyleHelper(GetDataPaletteStyle(dataType));
    }

    QColor StyleManager::GetDataTypeColor(const AZ::Uuid& dataType)
    {
        Styling::StyleHelper* style = FindCreateStyleHelper(GetDataPaletteStyle(dataType));
        QColor color = style->GetAttribute(Styling::Attribute::BackgroundColor, QColor());

        return color;
    }

    const QPixmap* StyleManager::GetDataTypeIcon(const AZ::Uuid& dataType)
    {
        AZStd::string paletteStyle = GetDataPaletteStyle(dataType);

        PaletteIconConfiguration configuration;
        configuration.m_iconPalette = "DataTypeIcon";
        configuration.AddColorPalette(paletteStyle);

        return GetConfiguredPaletteIcon(configuration);
    }

    const QPixmap* StyleManager::GetMultiDataTypeIcon(const AZStd::vector<AZ::Uuid>& dataTypes)
    {
        PaletteIconConfiguration configuration;
        configuration.m_iconPalette = "DataTypeIcon";
        configuration.ReservePalettes(dataTypes.size());

        for (const AZ::Uuid& dataType : dataTypes)
        {
            AZStd::string paletteStyle = GetDataPaletteStyle(dataType);
            configuration.AddColorPalette(paletteStyle);
        }

        return GetConfiguredPaletteIcon(configuration);
    }

    const Styling::StyleHelper* StyleManager::FindColorPalette(const AZStd::string& paletteString)
    {
        return FindCreateStyleHelper(paletteString);
    }

    QColor StyleManager::GetPaletteColor(const AZStd::string& palette)
    {
        Styling::StyleHelper* style = FindCreateStyleHelper(palette);
        QColor color = style->GetAttribute(Styling::Attribute::BackgroundColor, QColor());

        return color;
    }

    const QPixmap* StyleManager::GetPaletteIcon(const AZStd::string& iconStyle, const AZStd::string& palette)
    {
        PaletteIconConfiguration configuration;
        configuration.m_iconPalette = iconStyle;
        configuration.AddColorPalette(palette);

        return GetConfiguredPaletteIcon(configuration);
    }

    const QPixmap* StyleManager::GetConfiguredPaletteIcon(const PaletteIconConfiguration& paletteConfiguration)
    {
        Styling::StyleHelper* iconStyleHelper = FindCreateStyleHelper(paletteConfiguration.m_iconPalette);

        if (!iconStyleHelper)
        {
            return nullptr;
        }

        const QPixmap* pixmap = FindCachedIcon(paletteConfiguration);

        if (pixmap == nullptr)
        {
            pixmap = CreateAndCacheIcon(paletteConfiguration);
        }

        return pixmap;
    }

    const Styling::StyleHelper* StyleManager::FindPaletteIconStyleHelper(const PaletteIconConfiguration& paletteConfiguration)
    {
        auto colorPalettes = paletteConfiguration.GetColorPalettes();

        if (colorPalettes.size() == 1)
        {
            return FindCreateStyleHelper(colorPalettes.front());
        }

        return nullptr;
    }

    QPixmap* StyleManager::CreateIcon(const QColor& color, const AZStd::string& iconStyle)
    {
        QColor drawColor = color;
        drawColor.setAlpha(255);

        QBrush brush(drawColor);

        return CreateIcon(brush, iconStyle);
    }

    QPixmap* StyleManager::CreateIconFromConfiguration(const PaletteIconConfiguration& paletteConfiguration)
    {
        const AZStd::vector< AZStd::string >& colorPalettes = paletteConfiguration.GetColorPalettes();

        if (colorPalettes.size() == 1)
        {
            Styling::StyleHelper* colorStyleHelper = FindCreateStyleHelper(colorPalettes.front());

            if (colorStyleHelper)
            {
                return CreateIcon((*colorStyleHelper), paletteConfiguration.m_iconPalette);
            }
        }
        else if (!paletteConfiguration.GetColorPalettes().empty())
        {
            AZStd::vector<QColor> colors;
            colors.reserve(colorPalettes.size());

            for (const AZStd::string& colorPalette : colorPalettes)
            {
                Styling::StyleHelper* colorHelper = FindCreateStyleHelper(colorPalette);
                colors.push_back(colorHelper->GetColor(Styling::Attribute::BackgroundColor));
            }

            return CreateMultiColoredIcon(colors, paletteConfiguration.m_transitionPercent, paletteConfiguration.m_iconPalette);
        }

        return nullptr;
    }

    QPixmap* StyleManager::CreateMultiColoredIcon(const AZStd::vector<QColor>& colors, float transitionPercent, const AZStd::string& iconStyle)
    {
        Styling::StyleHelper* iconStyleHelper = FindCreateStyleHelper(iconStyle);

        if (!iconStyleHelper || colors.empty())
        {
            return nullptr;
        }

        qreal width = iconStyleHelper->GetAttribute(Styling::Attribute::Width, 12.0);
        qreal height = iconStyleHelper->GetAttribute(Styling::Attribute::Height, 8.0);
        qreal margin = iconStyleHelper->GetAttribute(Styling::Attribute::Margin, 2.0);

        QPainter painter;
        QPainterPath path;

        qreal borderWidth = iconStyleHelper->GetAttribute(Styling::Attribute::BorderWidth, 1.0);

        QRectF rect(margin, margin, width, height);
        QRectF adjusted = rect.marginsRemoved(QMarginsF(borderWidth / 2.0, borderWidth / 2.0, borderWidth / 2.0, borderWidth / 2.0));

        QPointF startPoint = adjusted.bottomLeft();
        QPointF endPoint = adjusted.topRight();

        QPointF slope = QtVectorMath::Transpose(endPoint - startPoint);
        slope = QtVectorMath::Normalize(slope);
        slope *= adjusted.width() * sin(atan(adjusted.height() / adjusted.width()));

        QLinearGradient fillGradient = QLinearGradient(adjusted.center() + slope, adjusted.center() - slope);

        double transition = transitionPercent * (1.0 / colors.size());

        fillGradient.setColorAt(0, colors[0]);

        for (size_t i = 1; i < colors.size(); ++i)
        {
            double transitionStart = AZStd::max(0.0, ((double)i / colors.size()) - (transition * 0.5));
            double transitionEnd = AZStd::min(1.0, ((double)i / colors.size()) + (transition * 0.5));

            fillGradient.setColorAt(transitionStart, colors[i-1]);
            fillGradient.setColorAt(transitionEnd, colors[i]);
        }

        fillGradient.setColorAt(1, colors[colors.size() - 1]);
        QBrush brush(fillGradient);

        return CreateIcon(brush, iconStyle);
    }

    QPixmap* StyleManager::CreateColoredPatternPixmap(const AZStd::vector<QColor>& colorTypes, const AZStd::string& patternName)
    {
        const TintableIcon* icon = FindPatternIcon(AZ::Crc32(patternName.c_str()));

        if (icon)
        {
            return icon->CreatePixmap(colorTypes);
        }

        return nullptr;
    }

    const QPixmap* StyleManager::CreatePatternPixmap(const AZStd::vector< AZStd::string>& palettes, const AZStd::string& patternName)
    {
        AZ::Crc32 iconKey = AZ::Crc32(patternName.c_str());
        AZ::Crc32 cacheKey = iconKey;

        for (const AZStd::string& paletteString : palettes)
        {
            cacheKey.Add(paletteString.c_str());
        }

        const QPixmap* cachePixmap = FindPatternCache(cacheKey);

        if (cachePixmap)
        {
            return cachePixmap;
        }

        const TintableIcon* icon = FindPatternIcon(iconKey);

        AZStd::vector< Styling::StyleHelper* > styleHelpers;
        styleHelpers.reserve(palettes.size());

        for (const AZStd::string& palette : palettes)
        {
            styleHelpers.push_back(FindCreateStyleHelper(palette));
        }

        QPixmap* newPixmap = icon->CreatePixmap(styleHelpers);

        AddPatternCache(cacheKey, newPixmap);

        return newPixmap;
    }

    AZStd::vector<AZStd::string> StyleManager::GetColorPaletteStyles() const
    {
        AZStd::vector<AZStd::string> loadedPaletteNames;
        for (const auto& styleHelperPair : m_styleTypeHelpers)
        {
            loadedPaletteNames.push_back(styleHelperPair.first);
        }

        return loadedPaletteNames;
    }

    QPixmap* StyleManager::FindPixmap(const AZ::Crc32& key)
    {
        auto cacheIter = m_pixmapCache.find(key);

        if (cacheIter != m_pixmapCache.end())
        {
            return cacheIter->second;
        }

        return nullptr;
    }

    void StyleManager::CachePixmap(const AZ::Crc32& key, QPixmap* pixmap)
    {
        auto cacheIter = m_pixmapCache.find(key);

        if (cacheIter != m_pixmapCache.end())
        {
            delete cacheIter->second;
        }

        m_pixmapCache[key] = pixmap;
    }

    int StyleManager::FindLayer(AZStd::string_view layer)
    {
        AZ::Crc32 layerId(layer.data());

        Styling::StyleHelper* styleHelper = FindCreateStyleHelper(layer);

        return styleHelper->GetAttribute(GraphCanvas::Styling::Attribute::Layer, 0);
    }

    int StyleManager::GetSteppedWidth(int gridSteps)
    {
        int result = gridSteps;

        for (QVariant variantStep : m_widthStepHelper)
        {
            int currentStep = variantStep.toInt();

            if (result < currentStep)
            {
                result = currentStep;
                break;
            }
        }

        return result;
    }

    int StyleManager::GetSteppedHeight(int gridSteps)
    {
        int result = gridSteps;

        for (QVariant variantStep : m_heightStepHelper)
        {
            int currentStep = variantStep.toInt();

            if (result < currentStep)
            {
                result = currentStep;
                break;
            }
        }

        return result;
    }

    QPixmap* StyleManager::CreateIcon(QBrush& brush, const AZStd::string& iconStyle)
    {
        Styling::StyleHelper* iconStyleHelper = FindCreateStyleHelper(iconStyle);

        if (!iconStyleHelper)
        {
            return nullptr;
        }

        qreal width = iconStyleHelper->GetAttribute(Styling::Attribute::Width, 12.0);
        qreal height = iconStyleHelper->GetAttribute(Styling::Attribute::Height, 8.0);
        qreal margin = iconStyleHelper->GetAttribute(Styling::Attribute::Margin, 2.0);

        QPixmap* icon = new QPixmap(aznumeric_cast<int>(width + 2 * margin), aznumeric_cast<int>(height + 2 * margin));
        icon->fill(Qt::transparent);

        QPainter painter;
        QPainterPath path;

        qreal borderWidth = iconStyleHelper->GetAttribute(Styling::Attribute::BorderWidth, 1.0);
        qreal borderRadius = iconStyleHelper->GetAttribute(Styling::Attribute::BorderRadius, 1.0);

        QRectF rect(margin, margin, width, height);
        QRectF adjusted = rect.marginsRemoved(QMarginsF(borderWidth / 2.0, borderWidth / 2.0, borderWidth / 2.0, borderWidth / 2.0));

        path.addRoundedRect(adjusted, borderRadius, borderRadius);

        QColor borderColor = iconStyleHelper->GetAttribute(Styling::Attribute::BorderColor, QColor());
        Qt::PenStyle borderStyle = iconStyleHelper->GetAttribute(Styling::Attribute::BorderStyle, Qt::PenStyle());

        QPen pen(borderColor, borderWidth);
        pen.setStyle(borderStyle);

        painter.begin(icon);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(pen);
        painter.fillPath(path, brush);
        painter.drawRoundedRect(adjusted, borderRadius, borderRadius);

        painter.end();

        return icon;
    }

    QPixmap* StyleManager::CreateIcon(const Styling::StyleHelper& styleHelper, const AZStd::string& iconStyle)
    {
        Styling::StyleHelper* iconStyleHelper = FindCreateStyleHelper(iconStyle);

        if (!iconStyleHelper)
        {
            return nullptr;
        }

        qreal width = iconStyleHelper->GetAttribute(Styling::Attribute::Width, 12.0);
        qreal height = iconStyleHelper->GetAttribute(Styling::Attribute::Height, 8.0);
        qreal margin = iconStyleHelper->GetAttribute(Styling::Attribute::Margin, 2.0);

        QPixmap* icon = new QPixmap(aznumeric_cast<int>(width + 2 * margin), aznumeric_cast<int>(height + 2 * margin));
        icon->fill(Qt::transparent);

        QPainter painter;
        QPainterPath path;

        qreal borderWidth = iconStyleHelper->GetAttribute(Styling::Attribute::BorderWidth, 1.0);
        qreal borderRadius = iconStyleHelper->GetAttribute(Styling::Attribute::BorderRadius, 1.0);

        QRectF rect(margin, margin, width, height);
        QRectF adjusted = rect.marginsRemoved(QMarginsF(borderWidth / 2.0, borderWidth / 2.0, borderWidth / 2.0, borderWidth / 2.0));

        path.addRoundedRect(adjusted, borderRadius, borderRadius);

        QPen borderPen = iconStyleHelper->GetPen(Styling::Attribute::BorderWidth, Styling::Attribute::BorderStyle, Styling::Attribute::BorderColor, Styling::Attribute::CapStyle);

        painter.begin(icon);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(borderPen);

        painter.save();
        painter.setClipPath(path);
        QtDrawingUtils::FillArea(painter, path.boundingRect(), styleHelper);
        painter.restore();

        painter.drawRoundedRect(adjusted, borderRadius, borderRadius);
        painter.end();

        return icon;
    }

    void StyleManager::ClearStyles()
    {
        StyleManagerNotificationBus::Event(m_editorId, &StyleManagerNotifications::OnStylesUnloaded);

        for (auto style : m_styles)
        {
            delete style;
        }

        m_styles.clear();
    }

    void StyleManager::ClearCache()
    {
        for (auto& mapPair : m_iconMapping)
        {
            for (auto& pixmapIter : mapPair.second)
            {
                delete pixmapIter.second;
            }
        }

        m_iconMapping.clear();

        for (auto& mapPair : m_patternCache)
        {
            delete mapPair.second;
        }

        m_patternCache.clear();
    }

    void StyleManager::RefreshColorPalettes()
    {
        for (auto& mapPair : m_styleTypeHelpers)
        {
            mapPair.second->SetEditorId(m_editorId);
            mapPair.second->SetStyle(mapPair.first);
        }
    }

    void StyleManager::PopulateDataPaletteMapping()
    {
        // Boolean
        m_dataPaletteMapping[azrtti_typeid<bool>()] = "BooleanDataColorPalette";

        // String
        m_dataPaletteMapping[azrtti_typeid<AZStd::string>()] = "StringDataColorPalette";

        // EntityId
        m_dataPaletteMapping[azrtti_typeid<AZ::EntityId>()] = "EntityIdDataColorPalette";

        // Numbers
        for (const AZ::Uuid& numberType : { azrtti_typeid<char>()
            , azrtti_typeid<AZ::s8>()
            , azrtti_typeid<short>()
            , azrtti_typeid<int>()
            , azrtti_typeid<long>()
            , azrtti_typeid<AZ::s64>()
            , azrtti_typeid<unsigned char>()
            , azrtti_typeid<unsigned short>()
            , azrtti_typeid<unsigned int>()
            , azrtti_typeid<unsigned long>()
            , azrtti_typeid<AZ::u64>()
            , azrtti_typeid<float>()
            , azrtti_typeid<double>()
        }
            )
        {
            m_dataPaletteMapping[numberType] = "NumberDataColorPalette";
        }

        // VectorTypes
        for (const AZ::Uuid& vectorType : { azrtti_typeid<AZ::Vector2>()
            , azrtti_typeid<AZ::Vector3>()
            , azrtti_typeid<AZ::Vector4>()
        }
            )
        {
            m_dataPaletteMapping[vectorType] = "VectorDataColorPalette";
        }

        // ColorType
        m_dataPaletteMapping[azrtti_typeid<AZ::Color>()] = "ColorDataColorPalette";

        // TransformType
        m_dataPaletteMapping[azrtti_typeid<AZ::Transform>()] = "TransformDataColorPalette";
    }

    const TintableIcon* StyleManager::FindPatternIcon(AZ::Crc32 patternIcon) const
    {
        auto patternIter = m_patternIcons.find(patternIcon);

        if (patternIter == m_patternIcons.end())
        {
            return nullptr;
        }

        return patternIter->second;
    }

    void StyleManager::AddPatternIcon(TintableIcon* icon)
    {
        auto patternIter = m_patternIcons.find(icon->GetIconId());

        if (patternIter == m_patternIcons.end())
        {
            m_patternIcons[icon->GetIconId()] = icon;
        }
        else
        {
            AZ_Warning("GraphCanvas", false, "Pattern Id %llu already in use", icon->GetIconId());
        }
    }

    Styling::StyleHelper* StyleManager::FindCreateStyleHelper(const AZStd::string& paletteString)
    {
        Styling::StyleHelper* styleHelper = nullptr;

        auto styleHelperIter = m_styleTypeHelpers.find(paletteString);

        if (styleHelperIter == m_styleTypeHelpers.end())
        {
            styleHelper = aznew Styling::StyleHelper();
            styleHelper->SetEditorId(m_editorId);
            styleHelper->SetStyle(paletteString);

            m_styleTypeHelpers[paletteString] = styleHelper;
        }
        else
        {
            styleHelper = styleHelperIter->second;
        }

        return styleHelper;
    }

    const QPixmap* StyleManager::CreateAndCacheIcon(const PaletteIconConfiguration& configuration)
    {
        auto iconIter = m_iconMapping.find(configuration.m_iconPalette);

        if (iconIter != m_iconMapping.end())
        {
            auto paletteIter = iconIter->second.find(configuration.GetPaletteCrc());

            if (paletteIter != iconIter->second.end())
            {
                delete paletteIter->second;
                iconIter->second.erase(paletteIter);
            }
        }

        QPixmap* icon = CreateIconFromConfiguration(configuration);

        m_iconMapping[configuration.m_iconPalette][configuration.GetPaletteCrc()] = icon;

        return icon;
    }

    const QPixmap* StyleManager::FindCachedIcon(const PaletteIconConfiguration& configuration)
    {
        auto iconIter = m_iconMapping.find(configuration.m_iconPalette);

        if (iconIter != m_iconMapping.end())
        {
            auto paletteIter = iconIter->second.find(configuration.GetPaletteCrc());

            if (paletteIter != iconIter->second.end())
            {
                return paletteIter->second;
            }
        }

        return nullptr;
    }

    const QPixmap* StyleManager::FindPatternCache(const AZ::Crc32& patternCache) const
    {
        auto patternIter = m_patternCache.find(patternCache);

        if (patternIter == m_patternCache.end())
        {
            return nullptr;
        }

        return patternIter->second;
    }

    void StyleManager::AddPatternCache(const AZ::Crc32& patternCache, QPixmap* pixmap)
    {
        auto patternIter = m_patternCache.find(patternCache);

        if (patternIter != m_patternCache.end())
        {
            delete patternIter->second;
        }

        m_patternCache[patternCache] = pixmap;
    }
}
