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

#include <GraphCanvas/Styling/Parser.h>
#include <GraphCanvas/Styling/Style.h>
#include <GraphCanvas/Styling/SelectorImplementations.h>
#include <GraphCanvas/Styling/StyleHelper.h>
#include <GraphCanvas/Styling/StyleManager.h>
#include <GraphCanvas/Styling/definitions.h>

#include <AzCore/JSON/error/en.h>
#include <AzCore/JSON/error/error.h>

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QFile>
#include <QByteArray>
#include <QColor>
#include <QFont>
#include <QFontInfo>
#include <QUrl>
#include <QRegularExpression>
#include <QScopedPointer>
#include <QResource>
#include <QDebug>
AZ_POP_DISABLE_WARNING



namespace
{

    namespace Styling = GraphCanvas::Styling;

    const QRegularExpression hexColor("^#([[:xdigit:]]{2}){3,4}$");
    const QRegularExpression rgbColor(R"_(^rgb\(([[:digit:]]{1,3})(,\s?[[:digit:]]{1,3}){2}\)$)_");
    const QRegularExpression rgbaColor(R"_(^rgba\(([[:digit:]]{1,3})(,\s?[[:digit:]]{1,3}){3}\)$)_");

    const QRegularExpression percentage(R"_(^([[:digit:]]{1,3})%$)_");
    const QRegularExpression hexNumber("[[:xdigit:]]{2}");
    const QRegularExpression decimalNumber("[[:digit:]]{1,3}");

    const QRegularExpression invalidStart(R"_(^\s*>\s*)_");
    const QRegularExpression invalidEnd(R"_(\s*>\s*$)_");
    const QRegularExpression splitNesting(R"_(\s*>\s*)_");
    const QRegularExpression selector(R"_(^(?:(?:(\w+)?(\.\w+)?)|(#\w+)?)(:\w+)?$)_");

    AZStd::string QtToAz(const QString& source)
    {
        QByteArray utf8 = source.toUtf8();
        return AZStd::string(utf8.constData(), utf8.size());
    }

    Styling::Attribute AttributeFromString(const QString& attribute)
    {
        if (attribute == Styling::Attributes::BackgroundColor)
        {
            return Styling::Attribute::BackgroundColor;
        }
        else if (attribute == Styling::Attributes::BackgroundImage)
        {
            return Styling::Attribute::BackgroundImage;
        }

        else if (attribute == Styling::Attributes::GridMajorWidth)
        {
            return Styling::Attribute::GridMajorWidth;
        }
        else if (attribute == Styling::Attributes::GridMajorStyle)
        {
            return Styling::Attribute::GridMajorStyle;
        }
        else if (attribute == Styling::Attributes::GridMajorColor)
        {
            return Styling::Attribute::GridMajorColor;
        }

        else if (attribute == Styling::Attributes::GridMinorWidth)
        {
            return Styling::Attribute::GridMinorWidth;
        }
        else if (attribute == Styling::Attributes::GridMinorStyle)
        {
            return Styling::Attribute::GridMinorStyle;
        }
        else if (attribute == Styling::Attributes::GridMinorColor)
        {
            return Styling::Attribute::GridMinorColor;
        }

        else if (attribute == Styling::Attributes::FontFamily)
        {
            return Styling::Attribute::FontFamily;
        }
        else if (attribute == Styling::Attributes::FontSize)
        {
            return Styling::Attribute::FontSize;
        }
        else if (attribute == Styling::Attributes::FontWeight)
        {
            return Styling::Attribute::FontWeight;
        }
        else if (attribute == Styling::Attributes::FontStyle)
        {
            return Styling::Attribute::FontStyle;
        }
        else if (attribute == Styling::Attributes::FontVariant)
        {
            return Styling::Attribute::FontVariant;
        }
        else if (attribute == Styling::Attributes::Color)
        {
            return Styling::Attribute::Color;
        }

        else if (attribute == Styling::Attributes::BorderWidth)
        {
            return Styling::Attribute::BorderWidth;
        }
        else if (attribute == Styling::Attributes::BorderStyle)
        {
            return Styling::Attribute::BorderStyle;
        }
        else if (attribute == Styling::Attributes::BorderColor)
        {
            return Styling::Attribute::BorderColor;
        }
        else if (attribute == Styling::Attributes::BorderRadius)
        {
            return Styling::Attribute::BorderRadius;
        }
        else if (attribute == Styling::Attributes::LineWidth)
        {
            return Styling::Attribute::LineWidth;
        }
        else if (attribute == Styling::Attributes::LineStyle)
        {
            return Styling::Attribute::LineStyle;
        }
        else if (attribute == Styling::Attributes::LineColor)
        {
            return Styling::Attribute::LineColor;
        }
        else if (attribute == Styling::Attributes::LineCurve)
        {
            return Styling::Attribute::LineCurve;
        }
        else if (attribute == Styling::Attributes::LineSelectionPadding)
        {
            return Styling::Attribute::LineSelectionPadding;
        }

        else if (attribute == Styling::Attributes::CapStyle)
        {
            return Styling::Attribute::CapStyle;
        }

        else if (attribute == Styling::Attributes::Margin)
        {
            return Styling::Attribute::Margin;
        }
        else if (attribute == Styling::Attributes::Padding)
        {
            return Styling::Attribute::Padding;
        }

        else if (attribute == Styling::Attributes::Width)
        {
            return Styling::Attribute::Width;
        }
        else if (attribute == Styling::Attributes::Height)
        {
            return Styling::Attribute::Height;
        }

        else if (attribute == Styling::Attributes::MinWidth)
        {
            return Styling::Attribute::MinWidth;
        }
        else if (attribute == Styling::Attributes::MaxWidth)
        {
            return Styling::Attribute::MaxWidth;
        }
        else if (attribute == Styling::Attributes::MinHeight)
        {
            return Styling::Attribute::MinHeight;
        }
        else if (attribute == Styling::Attributes::MaxHeight)
        {
            return Styling::Attribute::MaxHeight;
        }
        else if (attribute == Styling::Attributes::Spacing)
        {
            return Styling::Attribute::Spacing;
        }
        else if (attribute == Styling::Attributes::Selectors)
        {
            return Styling::Attribute::Selectors;
        }
        else if (attribute == Styling::Attributes::TextAlignment)
        {
            return Styling::Attribute::TextAlignment;
        }
        else if (attribute == Styling::Attributes::TextVerticalAlignment)
        {
            return Styling::Attribute::TextVerticalAlignment;
        }
        else if (attribute == Styling::Attributes::ConnectionJut)
        {
            return Styling::Attribute::ConnectionJut;
        }
        else if (attribute == Styling::Attributes::ConnectionDragMaximumDistance)
        {
            return Styling::Attribute::ConnectionDragMaximumDistance;
        }
        else if (attribute == Styling::Attributes::ConnectionDragPercent)
        {
            return Styling::Attribute::ConnectionDragPercent;
        }
        else if (attribute == Styling::Attributes::ConnectionDragMoveBuffer)
        {
            return Styling::Attribute::ConnectionDragMoveBuffer;
        }
        else if (attribute == Styling::Attributes::ConnectionDefaultMarquee)
        {
            return Styling::Attribute::ConnectionDefaultMarquee;
        }
        else if (attribute == Styling::Attributes::PaletteStyle)
        {
            return Styling::Attribute::PaletteStyle;
        }
        else if (attribute == Styling::Attributes::MaximumStripeSize)
        {
            return Styling::Attribute::MaximumStripeSize;
        }
        else if (attribute == Styling::Attributes::MinimumStripes)
        {
            return Styling::Attribute::MinimumStripes;
        }
        else if (attribute == Styling::Attributes::StripeAngle)
        {
            return Styling::Attribute::StripeAngle;
        }
        else if (attribute == Styling::Attributes::StripeColor)
        {
            return Styling::Attribute::StripeColor;
        }
        else if (attribute == Styling::Attributes::StripeOffset)
        {
            return Styling::Attribute::StripeOffset;
        }
        else if (attribute == Styling::Attributes::PatternTemplate)
        {
            return Styling::Attribute::PatternTemplate;
        }
        else if (attribute == Styling::Attributes::PatternPalettes)
        {
            return Styling::Attribute::PatternPalettes;
        }
        else if (attribute == Styling::Attributes::OddOffsetPercent)
        {
            return Styling::Attribute::OddOffsetPercent;
        }
        else if (attribute == Styling::Attributes::EvenOffsetPercent)
        {
            return Styling::Attribute::EvenOffsetPercent;
        }
        else if (attribute == Styling::Attributes::MinimumRepetitions)
        {
            return Styling::Attribute::MinimumRepetitions;
        }
        else if (attribute == Styling::Attributes::ZValue)
        {
            return Styling::Attribute::ZValue;
        }
        else if (attribute == Styling::Attributes::Opacity)
        {
            return Styling::Attribute::Opacity;
        }
        else if (attribute == Styling::Attributes::Steps)
        {
            return Styling::Attribute::Steps;
        }

        return Styling::Attribute::Invalid;
    }

    QColor ParseColor(const QString& color)
    {
        QColor result;

        QRegularExpressionMatch match;
        if ((match = hexColor.match(color)).hasMatch())
        {
            auto number = hexNumber.globalMatch(color);
            int r = number.next().captured(0).toInt(nullptr, 16);
            int g = number.next().captured(0).toInt(nullptr, 16);
            int b = number.next().captured(0).toInt(nullptr, 16);
            int alpha = (number.hasNext()) ? number.next().captured(0).toInt(nullptr, 16) : 255;

            return QColor(r, g, b, alpha);
        }
        else if ((match = rgbColor.match(color)).hasMatch())
        {
            auto number = decimalNumber.globalMatch(color);
            int r = number.next().captured(0).toInt();
            int g = number.next().captured(0).toInt();
            int b = number.next().captured(0).toInt();

            return QColor(r, g, b, 255);
        }
        else if ((match = rgbaColor.match(color)).hasMatch())
        {
            auto number = decimalNumber.globalMatch(color);
            int r = number.next().captured(0).toInt();
            int g = number.next().captured(0).toInt();
            int b = number.next().captured(0).toInt();
            int alpha = number.next().captured(0).toInt();

            return QColor(r, g, b, alpha);
        }
        else
        {
            return QColor(color);
        }

        return QColor();
    }

    bool IsColorValid(const QString& value)
    {
        return ParseColor(value).isValid();
    }

    Qt::AlignmentFlag ParseTextAlignment(const QString& value)
    {
        if (QString::compare(value, QLatin1String("left"), Qt::CaseInsensitive) == 0)
        {
            return Qt::AlignLeft;
        }
        else if (QString::compare(value, QLatin1String("right"), Qt::CaseInsensitive) == 0)
        {
            return Qt::AlignRight;
        }
        else if (QString::compare(value, QLatin1String("center"), Qt::CaseInsensitive) == 0)
        {
            return Qt::AlignHCenter;
        }
        else if (QString::compare(value, QLatin1String("justify"), Qt::CaseInsensitive) == 0)
        {
            return Qt::AlignJustify;
        }

        return {};
    }

    Qt::AlignmentFlag ParseTextVerticalAlignment(const QString& value)
    {
        if (QString::compare(value, QLatin1String("top"), Qt::CaseInsensitive) == 0)
        {
            return Qt::AlignTop;
        }
        else if (QString::compare(value, QLatin1String("bottom"), Qt::CaseInsensitive) == 0)
        {
            return Qt::AlignBottom;
        }
        else if (QString::compare(value, QLatin1String("center"), Qt::CaseInsensitive) == 0)
        {
            return Qt::AlignVCenter;
        }

        return {};
    }

    Qt::PenStyle ParseLineStyle(const QString& value)
    {
        if (QString::compare(value, QLatin1String("none"), Qt::CaseInsensitive) == 0)
        {
            return Qt::NoPen;
        }
        else if (QString::compare(value, QLatin1String("solid"), Qt::CaseInsensitive) == 0)
        {
            return Qt::SolidLine;
        }
        else if (QString::compare(value, QLatin1String("dashed"), Qt::CaseInsensitive) == 0)
        {
            return Qt::DashLine;
        }
        else if (QString::compare(value, QLatin1String("dotted"), Qt::CaseInsensitive) == 0)
        {
            return Qt::DotLine;
        }
        else if (QString::compare(value, QLatin1String("dash-dotted"), Qt::CaseInsensitive) == 0)
        {
            return Qt::DashDotLine;
        }
        else if (QString::compare(value, QLatin1String("dash-dot-dotted"), Qt::CaseInsensitive) == 0)
        {
            return Qt::DashDotDotLine;
        }
        else
        {
            return{};
        }
    }

    bool IsLineStyleValid(const QString& value)
    {
        if (QString::compare(value, QLatin1String("none"), Qt::CaseInsensitive) == 0 ||
            QString::compare(value, QLatin1String("solid"), Qt::CaseInsensitive) == 0 ||
            QString::compare(value, QLatin1String("dashed"), Qt::CaseInsensitive) == 0 ||
            QString::compare(value, QLatin1String("dotted"), Qt::CaseInsensitive) == 0 ||
            QString::compare(value, QLatin1String("dash-dotted"), Qt::CaseInsensitive) == 0 ||
            QString::compare(value, QLatin1String("dash-dot-dotted"), Qt::CaseInsensitive) == 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    Styling::ConnectionCurveType ParseLineCurve(const QString& value)
    {
        if (QString::compare(value, QLatin1String("straight"), Qt::CaseInsensitive) == 0)
        {
            return Styling::ConnectionCurveType::Straight;
        }
        else if (QString::compare(value, QLatin1String("curved"), Qt::CaseInsensitive) == 0)
        {
            return Styling::ConnectionCurveType::Curved;
        }
        else
        {
            return{};
        }
    }

    Styling::PaletteStyle ParsePaletteStyle(const QString& value)
    {
        if (QString::compare(value, QLatin1String("solid"), Qt::CaseInsensitive) == 0)
        {
            return Styling::PaletteStyle::Solid;
        }
        else if (QString::compare(value, QLatin1String("candystripe"), Qt::CaseInsensitive) == 0)
        {
            return Styling::PaletteStyle::CandyStripe;
        }
        else if (QString::compare(value, QLatin1String("pattern-fill"), Qt::CaseInsensitive) == 0)
        {
            return Styling::PaletteStyle::PatternFill;
        }

        return Styling::PaletteStyle::Solid;
    }

    bool IsLineCurveValid(const QString& value)
    {
        if (QString::compare(value, QLatin1String("straight"), Qt::CaseInsensitive) == 0 ||
            QString::compare(value, QLatin1String("curved"), Qt::CaseInsensitive) == 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    Qt::PenCapStyle ParseCapStyle(const QString& value)
    {
        if (QString::compare(value, QLatin1String("square"), Qt::CaseInsensitive) == 0)
        {
            return Qt::SquareCap;
        }
        else if (QString::compare(value, QLatin1String("flat"), Qt::CaseInsensitive) == 0)
        {
            return Qt::FlatCap;
        }
        else if (QString::compare(value, QLatin1String("round"), Qt::CaseInsensitive) == 0)
        {
            return Qt::RoundCap;
        }
        else
        {
            return{};
        }
    }

    bool IsCapStyleValid(const QString& value)
    {
        if (QString::compare(value, QLatin1String("square"), Qt::CaseInsensitive) == 0 ||
            QString::compare(value, QLatin1String("flat"), Qt::CaseInsensitive) == 0 ||
            QString::compare(value, QLatin1String("round"), Qt::CaseInsensitive) == 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool IsFontVariantValid(const QString& value)
    {
        if (QString::compare(value, QLatin1String("normal"), Qt::CaseInsensitive) == 0 ||
            QString::compare(value, QLatin1String("all-uppercase"), Qt::CaseInsensitive) == 0 ||
            QString::compare(value, QLatin1String("all-lowercase"), Qt::CaseInsensitive) == 0 ||
            QString::compare(value, QLatin1String("small-caps"), Qt::CaseInsensitive) == 0 ||
            QString::compare(value, QLatin1String("capitalize"), Qt::CaseInsensitive) == 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    QFont::Capitalization ParseFontVariant(const QString& value)
    {
        if (QString::compare(value, QLatin1String("normal"), Qt::CaseInsensitive) == 0)
        {
            return QFont::MixedCase;
        }
        else if (QString::compare(value, QLatin1String("all-uppercase"), Qt::CaseInsensitive) == 0)
        {
            return QFont::AllUppercase;
        }
        else if (QString::compare(value, QLatin1String("all-lowercase"), Qt::CaseInsensitive) == 0)
        {
            return QFont::AllLowercase;
        }
        else if (QString::compare(value, QLatin1String("small-caps"), Qt::CaseInsensitive) == 0)
        {
            return QFont::SmallCaps;
        }
        else if (QString::compare(value, QLatin1String("capitalize"), Qt::CaseInsensitive) == 0)
        {
            return QFont::Capitalize;
        }
        else
        {
            return{};
        }
    }

    bool IsFontStyleValid(const QString& value)
    {
        if (QString::compare(value, QLatin1String("normal"), Qt::CaseInsensitive) == 0 ||
            QString::compare(value, QLatin1String("italic"), Qt::CaseInsensitive) == 0 ||
            QString::compare(value, QLatin1String("oblique"), Qt::CaseInsensitive) == 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    QFont::Style ParseFontStyle(const QString& value)
    {
        if (QString::compare(value, QLatin1String("normal"), Qt::CaseInsensitive) == 0)
        {
            return QFont::StyleNormal;
        }
        else if (QString::compare(value, QLatin1String("italic"), Qt::CaseInsensitive) == 0)
        {
            return QFont::StyleItalic;
        }
        else if (QString::compare(value, QLatin1String("oblique"), Qt::CaseInsensitive) == 0)
        {
            return QFont::StyleOblique;
        }
        else
        {
            return{};
        }
    }

    bool IsFontWeightValid(const QString& value)
    {
        if (QString::compare(value, QLatin1String("thin"), Qt::CaseInsensitive) == 0 ||
            QString::compare(value, QLatin1String("extra-light"), Qt::CaseInsensitive) == 0 ||
            QString::compare(value, QLatin1String("light"), Qt::CaseInsensitive) == 0 ||
            QString::compare(value, QLatin1String("normal"), Qt::CaseInsensitive) == 0 ||
            QString::compare(value, QLatin1String("medium"), Qt::CaseInsensitive) == 0 ||
            QString::compare(value, QLatin1String("demi-bold"), Qt::CaseInsensitive) == 0 ||
            QString::compare(value, QLatin1String("bold"), Qt::CaseInsensitive) == 0 ||
            QString::compare(value, QLatin1String("extra-bold"), Qt::CaseInsensitive) == 0 ||
            QString::compare(value, QLatin1String("black"), Qt::CaseInsensitive) == 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    QFont::Weight ParseFontWeight(const QString& value)
    {
        if (QString::compare(value, QLatin1String("thin"), Qt::CaseInsensitive) == 0)
        {
            return QFont::Thin;
        }
        else if (QString::compare(value, QLatin1String("extra-light"), Qt::CaseInsensitive) == 0)
        {
            return QFont::ExtraLight;
        }
        else if (QString::compare(value, QLatin1String("light"), Qt::CaseInsensitive) == 0)
        {
            return QFont::Light;
        }
        else if (QString::compare(value, QLatin1String("normal"), Qt::CaseInsensitive) == 0)
        {
            return QFont::Normal;
        }
        else if (QString::compare(value, QLatin1String("medium"), Qt::CaseInsensitive) == 0)
        {
            return QFont::Medium;
        }
        else if (QString::compare(value, QLatin1String("demi-bold"), Qt::CaseInsensitive) == 0)
        {
            return QFont::DemiBold;
        }
        else if (QString::compare(value, QLatin1String("bold"), Qt::CaseInsensitive) == 0)
        {
            return QFont::Bold;
        }
        else if (QString::compare(value, QLatin1String("extra-bold"), Qt::CaseInsensitive) == 0)
        {
            return QFont::ExtraBold;
        }

        return{};
    }

    AZStd::string CreateStyleName(const Styling::Style& style)
    {
        const Styling::SelectorVector selectors = style.GetSelectors();
        return std::accumulate(selectors.cbegin(), selectors.cend(), AZStd::string(), [](const AZStd::string& a, const Styling::Selector& s) {
            return a + (a.empty() ? "" : ", ") + s.ToString();
        });
    }



} // namespace


namespace GraphCanvas
{
    namespace Styling
    {
        void Parser::Parse(StyleManager& styleManager, const AZStd::string& json)
        {
            rapidjson::Document document;

            QFile resource(QString::fromUtf8(json.c_str()));
            if (resource.exists())
            {
                resource.open(QIODevice::ReadOnly);
                document.Parse(resource.readAll().data());
            }
            else
            {
                document.Parse(json.c_str());
            }

            if (document.HasParseError())
            {
                qWarning() << "GraphCanvas styling: parse error: " << rapidjson::GetParseError_En(document.GetParseError());
                document.Parse("[]");
            }

            return Parse(styleManager, document);
        }

        void Parser::Parse(StyleManager& styleManager, const rapidjson::Document& json)
        {
            Q_ASSERT(json.IsArray());

            styleManager.ClearStyles();

            for (rapidjson::SizeType i = 0; i < json.Size(); ++i)
            {
                ParseStyle(styleManager, json[i]);
            }
        }

        void Parser::ParseStyle(StyleManager& styleManager, const rapidjson::Value& value)
        {
            QFont defaultFont;
            QFontInfo defaultFontInfo(defaultFont);

            SelectorVector selectors = ParseSelectors(value);
            if (selectors.empty())
            {
                qWarning() << "Style has no selectors, skipping";
                return;
            }

            QScopedPointer<Style> style(aznew Style(selectors));

            for (auto member = value.MemberBegin(); member != value.MemberEnd(); ++member)
            {
                Attribute attribute = AttributeFromString(member->name.GetString());

                switch (attribute)
                {
                case Attribute::BackgroundColor:
                case Attribute::GridMajorColor:
                case Attribute::GridMinorColor:
                case Attribute::Color:
                case Attribute::BorderColor:
                case Attribute::LineColor:
                case Attribute::StripeColor:
                {
                    QString value(member->value.GetString());

                    if (IsColorValid(value))
                    {
                        style->SetAttribute(attribute, ParseColor(value));
                    }
                    break;
                }
                case Attribute::BackgroundImage:
                {
                    QString value(member->value.GetString());

                    if (value.startsWith(QStringLiteral(":/")))
                    {
                        value = QString("qrc%1").arg(value);
                    }
                    QUrl url(value);
                    if (url.isValid())
                    {
                        style->SetAttribute(attribute, url);
                    }
                    break;
                }
                case Attribute::GridMajorWidth:
                case Attribute::GridMinorWidth:
                case Attribute::BorderWidth:
                case Attribute::LineWidth:
                case Attribute::LineSelectionPadding:
                case Attribute::FontSize:
                case Attribute::BorderRadius:
                case Attribute::Margin:
                case Attribute::Padding:
                case Attribute::Width:
                case Attribute::Height:
                case Attribute::MinWidth:
                case Attribute::MaxWidth:
                case Attribute::MinHeight:
                case Attribute::MaxHeight:
                case Attribute::Spacing:
                case Attribute::ConnectionJut:
                case Attribute::ConnectionDragMaximumDistance:
                case Attribute::ConnectionDragPercent:
                case Attribute::ConnectionDragMoveBuffer:
                case Attribute::ConnectionDefaultMarquee:
                case Attribute::ZValue:
                case Attribute::MaximumStripeSize:
                case Attribute::MinimumStripes:
                case Attribute::StripeAngle:
                case Attribute::StripeOffset:
                case Attribute::MinimumRepetitions:
                {
                    if (member->value.IsInt())
                    {
                        style->SetAttribute(attribute, member->value.GetInt());
                    }
                    else if (member->value.IsDouble())
                    {
                        style->SetAttribute(attribute, member->value.GetDouble());
                    }
                    else if (Attribute::FontSize == attribute)
                    {
                        QString theValue(member->value.GetString());

                        if (theValue == "default")
                        {
                            style->SetAttribute(attribute, defaultFontInfo.pixelSize());
                        }
                        else
                        {
                            QRegularExpressionMatch match = percentage.match(theValue);
                            if (match.isValid())
                            {
                                double extracted = match.captured(1).toDouble() / 100.0;
                                style->SetAttribute(attribute, int(double(defaultFontInfo.pixelSize() * extracted)));
                            }
                        }
                    }
                    else
                    {
                        // Handle percent cases
                        QString theValue(member->value.GetString());

                        QRegularExpressionMatch match = percentage.match(theValue);
                        if (match.isValid())
                        {
                            double extracted = match.captured(1).toDouble() / 100.0;
                            style->SetAttribute(attribute, extracted);
                        }
                        else
                        {
                            qWarning() << "Invalid number:" << member->value.GetString();
                        }
                    }
                    break;
                }
                case Attribute::Opacity:
                case Attribute::OddOffsetPercent:
                case Attribute::EvenOffsetPercent:
                {
                    // Handler percent case
                    QString theValue(member->value.GetString());

                    QRegularExpressionMatch match = percentage.match(theValue);
                    if (match.isValid())
                    {
                        double extracted = match.captured(1).toDouble() / 100.0;
                        style->SetAttribute(attribute, extracted);
                    }
                    else
                    {
                        qWarning() << "Invalid number:" << member->value.GetString();
                    }
                    break;
                }
                case Attribute::GridMajorStyle:
                case Attribute::GridMinorStyle:
                case Attribute::BorderStyle:
                case Attribute::LineStyle:
                {
                    QString value(member->value.GetString());

                    if (IsLineStyleValid(value))
                    {
                        style->SetAttribute(attribute, QVariant::fromValue(ParseLineStyle(value)));
                    }
                    break;
                }

                case Attribute::LineCurve:
                {
                    QString value(member->value.GetString());

                    if (IsLineCurveValid(value))
                    {
                        style->SetAttribute(attribute, QVariant::fromValue(ParseLineCurve(value)));
                    }
                    break;
                }

                case Attribute::CapStyle:
                {
                    QString value(member->value.GetString());

                    if (IsCapStyleValid(value))
                    {
                        style->SetAttribute(attribute, QVariant::fromValue(ParseCapStyle(value)));
                    }
                    break;
                }

                case Attribute::FontFamily:
                {
                    QString value(member->value.GetString());

                    if (QString::compare(value, QLatin1String("default"), Qt::CaseInsensitive) == 0)
                    {
                        value = defaultFontInfo.family();
                    }
                    else
                    {
                        QFont font(value);
                        QFontInfo info(font);
                        if (!info.exactMatch())
                        {
                            qWarning() << "Invalid font-family:" << value;
                        }
                    }
                    style->SetAttribute(attribute, value);
                }
                case Attribute::FontStyle:
                {
                    QString value(member->value.GetString());

                    if (QString::compare(value, QLatin1String("default"), Qt::CaseInsensitive) == 0)
                    {
                        style->SetAttribute(attribute, defaultFontInfo.style());
                    }
                    else
                    {
                        if (IsFontStyleValid(value))
                        {
                            style->SetAttribute(attribute, ParseFontStyle(value));
                        }
                    }
                    break;
                }
                case Attribute::FontWeight:
                {
                    QString value(member->value.GetString());

                    if (QString::compare(value, QLatin1String("default"), Qt::CaseInsensitive) == 0)
                    {
                        style->SetAttribute(attribute, defaultFontInfo.weight());
                    }
                    else
                    {
                        if (IsFontWeightValid(value))
                        {
                            style->SetAttribute(attribute, ParseFontWeight(value));
                        }
                    }
                    break;
                }
                case Attribute::FontVariant:
                {
                    QString value(member->value.GetString());

                    if (QString::compare(value, QLatin1String("default"), Qt::CaseInsensitive) == 0)
                    {
                        style->SetAttribute(attribute, defaultFont.capitalization());
                    }
                    else
                    {
                        if (IsFontVariantValid(value))
                        {
                            style->SetAttribute(attribute, value);
                        }
                    }
                    break;
                }
                case Attribute::TextAlignment:
                {
                    if (Qt::AlignmentFlag flag = ParseTextAlignment(member->value.GetString()))
                    {
                        style->SetAttribute(attribute, QVariant::fromValue(flag));
                    }
                    break;
                }
                case Attribute::TextVerticalAlignment:
                {
                    if (Qt::AlignmentFlag flag = ParseTextVerticalAlignment(member->value.GetString()))
                    {
                        style->SetAttribute(attribute, QVariant::fromValue(flag));
                    }
                    break;
                }
                case Attribute::Selectors:
                    break;
                case Attribute::PaletteStyle:
                {
                    QString value(member->value.GetString());
                    style->SetAttribute(attribute, QVariant::fromValue(ParsePaletteStyle(value)));
                    break;
                }
                case Attribute::PatternTemplate:
                case Attribute::PatternPalettes:
                {
                    QString value(member->value.GetString());
                    style->SetAttribute(attribute, value);
                    break;
                }
                case Attribute::Steps:
                {
                    QList<QVariant> stepList;
                    QString value(member->value.GetString());

                    QStringList splitValues = value.split("|");

                    for (QString currentString : splitValues)
                    {
                        stepList.append(currentString.toInt());
                    }

                    style->SetAttribute(attribute, stepList);
                    break;
                }
                default:
                    qWarning() << "Invalid attribute:" << member->name.GetString();
                }
            }

            if (style->IsEmpty())
            {
                qWarning() << "Style contains no rules";
                return;
            }
            else
            {
                styleManager.m_styles.push_back(style.take());
            }
        }

        SelectorVector Parser::ParseSelectors(const rapidjson::Value& value)
        {
            SelectorVector result;

            if (!value.HasMember(Styling::Attributes::Selectors))
            {
                qWarning() << "Style has no selectors";
                return result;
            }

            const rapidjson::Value& raw = value[Styling::Attributes::Selectors];

            if (!raw.IsArray())
            {
                qWarning() << "Expected an array of strings";
                return result;
            }

            for (auto rawSelector = raw.Begin(); rawSelector != raw.End(); ++rawSelector)
            {
                if (!rawSelector->IsString())
                {
                    qWarning() << "Selectors should be strings, skipping";
                    continue;
                }

                QString candidate = QString::fromUtf8(rawSelector->GetString());
                bool br = candidate == "slot";

                if (candidate.contains(invalidStart))
                {
                    qWarning() << "Selectors can't start with '>', skipping";
                    continue;
                }
                if (candidate.contains(invalidEnd))
                {
                    qWarning() << "Selectors can't end with '>', skipping";
                    continue;
                }

                const QStringList parts = candidate.split(splitNesting);
                if (parts.contains(QStringLiteral("")))
                {
                    qWarning() << "Empty nesting relation found, skipping";
                    continue;
                }

                SelectorVector nestedSelectors;
                nestedSelectors.reserve(parts.size());
                for (const QString& part : parts)
                {
                    auto matches = selector.match(part);
                    if (!matches.hasMatch())
                    {
                        qWarning() << "Invalid selector:" << part << "in" << candidate;
                        nestedSelectors.clear();
                        break;
                    }

                    AZStd::string element = QtToAz(matches.captured(1));
                    AZStd::string clazz = QtToAz(matches.captured(2));
                    AZStd::string id = QtToAz(matches.captured(3));
                    AZStd::string state = QtToAz(matches.captured(4));

                    if (!id.empty())
                    {
                        Selector selector = Selector::Get(id);
                        result.emplace_back(selector);
                        continue;
                    }

                    Selector elementSelector = Selector::Get(element);
                    Selector clazzSelector = Selector::Get(clazz);
                    Selector stateSelector = Selector::Get(state);

                    if (matches.capturedTexts().size() == 2)
                    {
                        if (elementSelector.IsValid())
                        {
                            nestedSelectors.emplace_back(elementSelector);
                        }
                        else if (clazzSelector.IsValid())
                        {
                            nestedSelectors.emplace_back(clazzSelector);
                        }
                        else if (stateSelector.IsValid())
                        {
                            nestedSelectors.emplace_back(stateSelector);
                        }
                    }
                    else
                    {
                        SelectorVector bits;
                        if (elementSelector.IsValid())
                        {
                            bits.emplace_back(elementSelector);
                        }
                        if (clazzSelector.IsValid())
                        {
                            bits.emplace_back(clazzSelector);
                        }
                        if (stateSelector.IsValid())
                        {
                            bits.emplace_back(stateSelector);
                        }
                        Selector selector = aznew CompoundSelector(std::move(bits));
                        nestedSelectors.emplace_back(selector);
                    }
                }

                if (nestedSelectors.size() == 1)
                {
                    result.push_back(nestedSelectors[0]);
                }
                else if (nestedSelectors.empty())
                {
                    continue;
                }
                else
                {
                    Selector nested = aznew NestedSelector(std::move(nestedSelectors));
                    result.emplace_back(nested);
                }
            }

            return result;
        }

    }
}
