/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


//#include "CustomizeKeyboardPage.h"

#include <AzCore/Math/Color.h>

#include <Util/EditorUtils.h>

#include <Editor/Resource.h>

#include <QColor>

//////////////////////////////////////////////////////////////////////////
void HeapCheck::Check([[maybe_unused]] const char* file, [[maybe_unused]] int line)
{
}

#include <QCursor>
#include <QPixmap>

QCursor CMFCUtils::LoadCursor(UINT nIDResource, int hotX, int hotY)
{
    QString path;
    switch (nIDResource)
    {
    case IDC_ARRBLCK:
        path = QStringLiteral("cur00003.cur");
        break;
    case IDC_ARRBLCKCROSS:
        path = QStringLiteral("cur00004.cur");
        break;
    case IDC_ARRWHITE:
        path = QStringLiteral("cur00005.cur");
        break;
    case IDC_ARROW_ADDKEY:
        path = QStringLiteral("arr_addkey.cur");
        break;
    case IDC_LEFTRIGHT:
        path = QStringLiteral("leftright.cur");
        break;
    case IDC_POINTER_OBJHIT:
        path = QStringLiteral("pointerHit.cur");
        break;
    case IDC_POINTER_OBJECT_ROTATE:
        path = QStringLiteral("object_rotate.cur");
        break;
    default:
        AZ_Assert(0, "Calling LoadCursor with an unknown cursor type");
        return QCursor();
    }
    path = QStringLiteral(":/cursors/res/") + path;
    QPixmap pm(path);
    if (!pm.isNull() && (hotX < 0 || hotX < 0))
    {
        QFile f(path);
        f.open(QFile::ReadOnly);
        QDataStream stream(&f);
        stream.setByteOrder(QDataStream::LittleEndian);
        f.read(10);
        quint16 x;
        stream >> x;
        hotX = x;
        stream >> x;
        hotY = x;
    }
    return QCursor(pm, hotX, hotY);
}

//////////////////////////////////////////////////////////////////////////-
QString TrimTrailingZeros(QString str)
{
    if (str.contains('.'))
    {
        for (int p = str.size() - 1; p >= 0; --p)
        {
            if (str.at(p) == '.')
            {
                return str.left(p);
            }
            else if (str.at(p) != '0')
            {
                return str.left(p + 1);
            }
        }
        return QString("0");
    }
    return str;
}

//////////////////////////////////////////////////////////////////////////
QColor ColorLinearToGamma(const AZ::Color& col)
{
    AZ::Color gammaColor = col.GetSaturated().LinearToGamma();
    return QColor(gammaColor.GetR8(), gammaColor.GetG8(), gammaColor.GetB8(), gammaColor.GetA8());
}

//////////////////////////////////////////////////////////////////////////
AZ::Color ColorGammaToLinear(const QColor& col)
{
    AZ::Color gammaColor(col.red(), col.green(), col.blue(), col.alpha());
    return gammaColor.GammaToLinear();
}
