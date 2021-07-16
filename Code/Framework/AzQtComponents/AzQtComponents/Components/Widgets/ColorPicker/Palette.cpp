/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/ColorPicker/Palette.h>
#include <AzQtComponents/Utilities/Conversions.h>
#include <AzCore/Casting/numeric_cast.h>
#include <QtXml/QDomDocument>
#include <QDataStream>
#include <QFile>
#include <QMimeData>
#include <QBuffer>
#include <QDebug>
#include <unordered_set>

static QDataStream& operator<<(QDataStream& out, const AZ::Color& color)
{
    out << static_cast<float>(color.GetR());
    out << static_cast<float>(color.GetG());
    out << static_cast<float>(color.GetB());
    out << static_cast<float>(color.GetA());
    return out;
}

static QDataStream& operator>>(QDataStream& in, AZ::Color& color)
{
    float r, g, b, a;
    in >> r >> g >> b >> a;
    color.Set(r, g, b, a);
    return in;
}

namespace AzQtComponents
{

Palette::Palette(const QVector<AZ::Color>& colors)
    : m_colors(colors)
{
}

bool Palette::save(const QString& fileName) const
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
    {
        return false;
    }
    return save(&file);
}

bool Palette::save(QIODevice* device) const
{
    QDomDocument document;

    QDomNode root = document.createElement("palette");

    QDomElement colorsNode = document.createElement("colors");
    for (const auto& color : m_colors)
    {
        QDomElement colorNode = document.createElement("color");
        colorNode.setAttribute("r", static_cast<float>(color.GetR()));
        colorNode.setAttribute("g", static_cast<float>(color.GetG()));
        colorNode.setAttribute("b", static_cast<float>(color.GetB()));
        colorNode.setAttribute("a", static_cast<float>(color.GetA()));
        colorsNode.appendChild(colorNode);
    }
    root.appendChild(colorsNode);

    document.appendChild(root);

    if (!device->isOpen() || !device->isWritable())
    {
        return false;
    }

    device->write(document.toString().toUtf8());
    return true;
}

bool Palette::save(QMimeData* mimeData) const
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    save(&buffer);
    buffer.close();

    mimeData->setData(MIME_TYPE_PALETTE, buffer.data());
    return true;
}

bool Palette::load(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }
    
    return load(&file);
}

bool Palette::load(QIODevice* device)
{
    if (!device->isOpen() || !device->isReadable())
    {
        qWarning() << device << "is not readable";
        return false;
    }

    const auto data = device->readAll();

    QDomDocument document;
    if (!document.setContent(data))
    {
        return false;
    }

    m_colors.clear();

    const auto colorNodes = document.elementsByTagName("color");
    for (int i = 0; i < colorNodes.count(); ++i) {
        const auto colorNode = colorNodes.item(i).toElement();
        const auto r = colorNode.attribute("r").toFloat();
        const auto g = colorNode.attribute("g").toFloat();
        const auto b = colorNode.attribute("b").toFloat();
        const auto a = colorNode.attribute("a").toFloat();
        m_colors.append(AZ::Color(r, g, b, a));
    }

    return true;
}

bool Palette::load(const QMimeData* mimeData)
{
    if (mimeData->hasFormat(MIME_TYPE_PALETTE))
    {
        QByteArray array = mimeData->data(MIME_TYPE_PALETTE);
        QBuffer buffer(&array);
        buffer.open(QIODevice::ReadOnly);
        return load(&buffer);
    }

    return false;
}

bool Palette::tryInsertColor(int index, const AZ::Color& color)
{
    if (containsColor(color))
    {
        return false;
    }

    m_colors.insert(index, color);
    return true;
}

bool Palette::tryInsertColors(int index, const QVector<AZ::Color>& colors)
{
    return tryInsertColors(index, colors.cbegin(), colors.cend());
}

bool Palette::tryInsertColors(int index, QVector<AZ::Color>::const_iterator first, QVector<AZ::Color>::const_iterator last)
{
    if (containsAnyColor(first, last))
    {
        return false;
    }

    insertColorsIgnoringDuplicates(index, first, last);
    return true;
}

bool Palette::tryAppendColor(const AZ::Color& color)
{
    return tryInsertColor(m_colors.size(), color);
}

bool Palette::tryAppendColors(const QVector<AZ::Color>& colors)
{
    return tryInsertColors(m_colors.size(), colors.cbegin(), colors.cend());
}

bool Palette::tryAppendColors(QVector<AZ::Color>::const_iterator first, QVector<AZ::Color>::const_iterator last)
{
    return tryInsertColors(m_colors.size(), first, last);
}

bool Palette::tryRemoveColors(int index, int count)
{
    if (index >= m_colors.size() || index + count > m_colors.size())
    {
        return false;
    }

    m_colors.remove(index, count);
    return true;
}

bool Palette::trySetColor(int index, const AZ::Color& color)
{
    if (containsColor(color))
    {
        return false;
    }
    
    m_colors[index] = color;
    return true;
}


bool Palette::containsColor(const AZ::Color& color)
{
    return std::any_of(m_colors.cbegin(), m_colors.cend(), [color](const AZ::Color& existing) { return existing.IsClose(color); });
}

bool Palette::containsAnyColor(const QVector<AZ::Color>& colors)
{
    return containsAnyColor(colors.cbegin(), colors.cend());
}

bool Palette::containsAnyColor(QVector<AZ::Color>::const_iterator first, QVector<AZ::Color>::const_iterator last)
{
    return std::any_of(first, last, [this](const AZ::Color& color) { return containsColor(color); });
}

bool Palette::isValid() const
{
    // If a vector of only unique colors is smaller than our colors, we have duplicates and are invalid
    QVector<AZ::Color> truth(m_colors);
    std::sort(truth.begin(), truth.end(), [](const AZ::Color& l, const AZ::Color& r) { return l.IsLessThan(r); });
    truth.erase(std::unique(truth.begin(), truth.end()), truth.end());
    return truth.size() == m_colors.size();
}

const QVector<AZ::Color>& Palette::colors() const
{
    return m_colors;
}

void Palette::insertColorsIgnoringDuplicates(int index, QVector<AZ::Color>::const_iterator first, QVector<AZ::Color>::const_iterator last)
{
    m_colors.insert(index, aznumeric_cast<int>(std::distance(first, last)), {});
    auto target = m_colors.begin() + index;
    while (first != last) {
        *target = *first;
        ++target;
        ++first;
    }
}

QDataStream& operator<<(QDataStream& out, const Palette& palette)
{
    out << palette.m_colors;
    return out;
}

QDataStream& operator>>(QDataStream& in, Palette& palette)
{
    in >> palette.m_colors;
    return in;
}

}
