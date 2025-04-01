/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/EntityUtils.h>

#include <algorithm>

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QVariant>
#include <QFont>
#include <QFontInfo>
#include <QPen>
#include <QDebug>
AZ_POP_DISABLE_WARNING

#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Styling/definitions.h>
#include <GraphCanvas/Styling/Style.h>
#include <GraphCanvas/Styling/StyleHelper.h>

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(QVariant, "{EFF68052-50FF-4072-8047-31243EF95FEE}");
}

namespace
{
    static bool StyleVersionConverter(AZ::SerializeContext& /*serializeContext*/, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() <= 3)
        {
            auto selectorArrayElementIndex = classElement.FindElement(AZ_CRC_CE("Selectors"));
            if (selectorArrayElementIndex != -1)
            {
                classElement.RemoveElement(selectorArrayElementIndex);
            }
        }
        return true;
    }

    namespace Styling = GraphCanvas::Styling;

    QString AttributeName(Styling::Attribute attribute)
    {
        switch (attribute)
        {
        case Styling::Attribute::BackgroundColor:
            return Styling::Attributes::BackgroundColor;
        case Styling::Attribute::BackgroundImage:
            return Styling::Attributes::BackgroundImage;
        case Styling::Attribute::CapStyle:
            return Styling::Attributes::CapStyle;
        case Styling::Attribute::GridMajorWidth:
            return Styling::Attributes::GridMajorWidth;
        case Styling::Attribute::GridMajorStyle:
            return Styling::Attributes::GridMajorStyle;
        case Styling::Attribute::GridMajorColor:
            return Styling::Attributes::GridMajorColor;
        case Styling::Attribute::GridMinorWidth:
            return Styling::Attributes::GridMinorWidth;
        case Styling::Attribute::GridMinorStyle:
            return Styling::Attributes::GridMinorStyle;
        case Styling::Attribute::GridMinorColor:
            return Styling::Attributes::GridMinorColor;
        case Styling::Attribute::FontFamily:
            return Styling::Attributes::FontFamily;
        case Styling::Attribute::FontSize:
            return Styling::Attributes::FontSize;
        case Styling::Attribute::FontWeight:
            return Styling::Attributes::FontWeight;
        case Styling::Attribute::FontStyle:
            return Styling::Attributes::FontStyle;
        case Styling::Attribute::FontVariant:
            return Styling::Attributes::FontVariant;
        case Styling::Attribute::Color:
            return Styling::Attributes::Color;
        case Styling::Attribute::BorderWidth:
            return Styling::Attributes::BorderWidth;
        case Styling::Attribute::BorderStyle:
            return Styling::Attributes::BorderStyle;
        case Styling::Attribute::BorderColor:
            return Styling::Attributes::BorderColor;
        case Styling::Attribute::BorderRadius:
            return Styling::Attributes::BorderRadius;
        case Styling::Attribute::LineWidth:
            return Styling::Attributes::LineWidth;
        case Styling::Attribute::LineStyle:
            return Styling::Attributes::LineStyle;
        case Styling::Attribute::LineColor:
            return Styling::Attributes::LineColor;
        case Styling::Attribute::LineCurve:
            return Styling::Attributes::LineCurve;
        case Styling::Attribute::LineSelectionPadding:
            return Styling::Attributes::LineSelectionPadding;
        case Styling::Attribute::Margin:
            return Styling::Attributes::Margin;
        case Styling::Attribute::Padding:
            return Styling::Attributes::Padding;
        case Styling::Attribute::Width:
            return Styling::Attributes::Width;
        case Styling::Attribute::Height:
            return Styling::Attributes::Height;
        case Styling::Attribute::MinWidth:
            return Styling::Attributes::MinWidth;
        case Styling::Attribute::MaxWidth:
            return Styling::Attributes::MaxWidth;
        case Styling::Attribute::MinHeight:
            return Styling::Attributes::MinHeight;
        case Styling::Attribute::MaxHeight:
            return Styling::Attributes::MaxHeight;
        case Styling::Attribute::Selectors:
            return Styling::Attributes::Selectors;
        case Styling::Attribute::TextAlignment:
            return Styling::Attributes::TextAlignment;
        case Styling::Attribute::LayoutOrientation:
            return Styling::Attributes::LayoutOrientation;
        case Styling::Attribute::Invalid:
        default:
            return "Invalid Attribute";
        }
    }

    class QVariantSerializer
        : public AZ::SerializeContext::IDataSerializer
    {
        /// Store the class data into a binary buffer
        size_t Save(const void* classPtr, AZ::IO::GenericStream& stream, bool isDataBigEndian /*= false*/) override
        {
            auto variant = reinterpret_cast<const QVariant*>(classPtr);

            QByteArray buffer;
            QDataStream qtStream(&buffer, QIODevice::WriteOnly);
            qtStream.setByteOrder(isDataBigEndian ? QDataStream::BigEndian : QDataStream::LittleEndian);
            qtStream << *variant;

            return static_cast<size_t>(stream.Write(buffer.length(), reinterpret_cast<const void*>(buffer.data())));
        }

        /// Convert binary data to text
        size_t DataToText(AZ::IO::GenericStream& in, AZ::IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            (void)isDataBigEndian;

            QByteArray buffer = ReadAll(in);
            QByteArray&& base64 = buffer.toBase64();
            return static_cast<size_t>(out.Write(base64.size(), base64.data()));
        }

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        size_t TextToData(const char* text, unsigned int textVersion, AZ::IO::GenericStream& stream, bool isDataBigEndian = false) override
        {
            (void)textVersion;
            (void)isDataBigEndian;
            AZ_Assert(textVersion == 0, "Unknown char, short, int version!");

            QByteArray decoder = QByteArray::fromBase64(text);
            stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            return static_cast<size_t>(stream.Write(decoder.size(), decoder.constData()));
        }

        /// Load the class data from a stream.
        bool Load(void* classPtr, AZ::IO::GenericStream& in, unsigned int, bool isDataBigEndian = false) override
        {
            QByteArray buffer = ReadAll(in);
            QDataStream qtStream(&buffer, QIODevice::ReadOnly);
            qtStream.setByteOrder(isDataBigEndian ? QDataStream::BigEndian : QDataStream::LittleEndian);

            QVariant* variant = reinterpret_cast<QVariant*>(classPtr);
            qtStream >> *variant;

            return true;
        }

        bool CompareValueData(const void* left, const void* right) override
        {
            auto leftVariant = reinterpret_cast<const QVariant*>(left);
            auto rightVariant = reinterpret_cast<const QVariant*>(right);

            return *leftVariant == *rightVariant;
        }

    private:
        QByteArray ReadAll(AZ::IO::GenericStream& in)
        {
            const size_t length = in.GetLength();

            QByteArray buffer;
            buffer.reserve(static_cast<int>(length));

            size_t processed = 0;
            size_t read = 0;
            char* data = buffer.data();
            while (processed < length)
            {
                read = in.Read(length - processed, data + processed);
                if (!read)
                {
                    break;
                }
                processed += read;
            }

            AZ_Assert(processed == length, "Incorrect amount of data read from stream");

            buffer.resize(static_cast<int>(processed));

            return buffer;
        }
    };
} // namespace

QDataStream& operator<<(QDataStream& out, const Qt::PenStyle& pen)
{
    out << static_cast<int>(pen);
    return out;
}

QDataStream& operator>>(QDataStream& in, Qt::PenStyle& pen)
{
    int holder;
    in >> holder;
    pen = static_cast<Qt::PenStyle>(holder);

    return in;
}

QDataStream& operator<<(QDataStream& out, const Qt::PenCapStyle& cap)
{
    out << static_cast<int>(cap);
    return out;
}

QDataStream& operator>>(QDataStream& in, Qt::PenCapStyle& cap)
{
    int holder;
    in >> holder;
    cap = static_cast<Qt::PenCapStyle>(holder);

    return in;
}

QDataStream& operator<<(QDataStream& out, const Qt::AlignmentFlag& alignment)
{
    out << static_cast<int>(alignment);
    return out;
}

QDataStream& operator>>(QDataStream& in, Qt::AlignmentFlag& alignment)
{
    int holder;
    in >> holder;
    alignment = static_cast<Qt::AlignmentFlag>(holder);

    return in;
}

QDataStream& operator<<(QDataStream& out, const GraphCanvas::Styling::ConnectionCurveType& curve)
{
    out << static_cast<int>(curve);
    return out;
}

QDataStream& operator>>(QDataStream& in, GraphCanvas::Styling::ConnectionCurveType& curve)
{
    int holder;
    in >> holder;
    curve = static_cast<GraphCanvas::Styling::ConnectionCurveType>(holder);

    return in;
}

QDataStream& operator<<(QDataStream& out, const GraphCanvas::Styling::PaletteStyle& curve)
{
    out << static_cast<int>(curve);
    return out;
}

QDataStream& operator >> (QDataStream& in, GraphCanvas::Styling::PaletteStyle& curve)
{
    int holder;
    in >> holder;
    curve = static_cast<GraphCanvas::Styling::PaletteStyle>(holder);

    return in;
}

namespace GraphCanvas
{
    namespace Styling
    {
        //////////
        // Style
        //////////
        void Style::Reflect(AZ::ReflectContext* context)
        {
            static bool reflected = false;

            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext || (reflected && !serializeContext->IsRemovingReflection()))
            {
                return;
            }

            qRegisterMetaTypeStreamOperators<Qt::PenStyle>();
            qRegisterMetaTypeStreamOperators<Qt::PenCapStyle>();
            qRegisterMetaTypeStreamOperators<Qt::AlignmentFlag>();
            qRegisterMetaTypeStreamOperators<Styling::ConnectionCurveType>();
            qRegisterMetaTypeStreamOperators<Styling::PaletteStyle>();

            // Allow QVectors to be serialized
            serializeContext->Class<QVariant>()
                ->Serializer<QVariantSerializer>();

            //TODO port collection class away from QHash so it can be serialized
            serializeContext->Class<Style>()
                ->Version(4, &StyleVersionConverter)
                ->Field("Selectors", &Style::m_selectors)
                ->Field("SelectorsAsString", &Style::m_selectorsAsString)
                ->Field("Attributes", &Style::m_values)
                ;

            reflected = true;
        }

        Style::Style(const SelectorVector& selectors)
            : m_selectors(selectors)
            , m_selectorsAsString(SelectorsToString(selectors))
        {
        }

        Style::Style(std::initializer_list<Selector> selectors)
            : m_selectors(selectors)
            , m_selectorsAsString(SelectorsToString(m_selectors))
        {
        }

        Style::~Style()
        {
        }

        int Style::Matches(const AZ::EntityId& object) const
        {
            for (const Selector& selector : m_selectors)
            {
                if (selector.Matches(object))
                {
                    return selector.GetComplexity();
                }
            }
            return 0;
        }

        bool Style::HasAttribute(Attribute attribute) const
        {
            return m_values.find(attribute) != m_values.end();
        }

        inline QVariant Style::GetAttribute(Attribute attribute) const
        {
            return HasAttribute(attribute) ? m_values.at(attribute) : QVariant();
        }

        void Style::SetAttribute(Attribute attribute, const QVariant& value)
        {
            m_values[attribute] = value;
        }

        void Style::Dump() const
        {
            qDebug() << SelectorsToString(m_selectors).c_str();

            for (auto& entry : m_values)
            {
                qDebug() << AttributeName(entry.first) << entry.second;
            }

            qDebug() << "";
        }

        //////////////////
        // ComputedStyle
        //////////////////

        void ComputedStyle::Reflect(AZ::ReflectContext * context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            Style::Reflect(context);

            serializeContext->Class<ComputedStyle, AZ::Component>()
                ->Version(2)
                ->Field("ObjectSelectors", &ComputedStyle::m_objectSelectors)
                ->Field("ObjectSelectorsAsString", &ComputedStyle::m_objectSelectorsAsString)
                ->Field("Styles", &ComputedStyle::m_styles)
                ;
        }

        ComputedStyle::ComputedStyle(const EditorId& editorId, const SelectorVector& objectSelectors, StyleVector&& styles)
            : m_objectSelectors(objectSelectors)
            , m_objectSelectorsAsString(SelectorsToString(m_objectSelectors))
            , m_styles(std::move(styles))
        {
            StyleManagerNotificationBus::Handler::BusConnect(editorId);
        }

        ComputedStyle::~ComputedStyle()
        {
        }

        void ComputedStyle::Activate()
        {
            StyleRequestBus::Handler::BusConnect(GetEntityId());
        }

        void ComputedStyle::Deactivate()
        {
            StyleRequestBus::Handler::BusDisconnect();
        }

        AZStd::string ComputedStyle::GetDescription() const
        {
            AZStd::string result("Computed:\n");
            result += "\tObject selectors: " + m_objectSelectorsAsString + "\n";
            result += "\tStyles:\n";
            for (const Style* style : m_styles)
            {
                if (style)
                {
                    result += "\t\t" + style->GetSelectorsAsString() + "\n";
                }
            }

            result += "\n";
            return result;
        }

        bool ComputedStyle::HasAttribute(AZ::u32 attribute) const
        {
            Attribute typedAttribute = static_cast<Attribute>(attribute);
            return std::any_of(m_styles.cbegin(), m_styles.cend(), [=](const Style* s) {
                return s && s->HasAttribute(typedAttribute);
            });
        }

        QVariant ComputedStyle::GetAttribute(AZ::u32 attribute) const
        {
            Attribute typedAttribute = static_cast<Attribute>(attribute);
            for (const Style* style : m_styles)
            {
                if (style && style->HasAttribute(typedAttribute))
                {
                    return style->GetAttribute(typedAttribute);
                }
            }

            return QVariant();
        }

        void ComputedStyle::OnStylesUnloaded()
        {
            m_styles.clear();
        }

#if 0
        void ComputedStyle::Dump() const
        {
            for (const auto& style : m_styles)
            {
                style->Dump();
            }
        }
#endif
    }
}
