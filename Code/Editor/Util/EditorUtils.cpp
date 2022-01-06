/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "EditorUtils.h"

#include "EditorToolsApplicationAPI.h"

// Qt
#include <QColor>
#include <QMessageBox>


#define GetAValue(rgba)      (LOBYTE((rgba)>>24)) // Microsoft does not provide this one so let's make our own.
#define RGBA(r,g,b,a)          ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)|(((DWORD)(BYTE)(a))<<24)))

//////////////////////////////////////////////////////////////////////////
void HeapCheck::Check([[maybe_unused]] const char* file, [[maybe_unused]] int line)
{
    #ifdef _DEBUG

#ifdef Q_OS_WIN
    _ASSERTE(_CrtCheckMemory());
#endif

    #endif
}

#ifdef LoadCursor
#undef LoadCursor
#endif
QCursor CMFCUtils::LoadCursor(unsigned int nIDResource, int hotX, int hotY)
{
    QString path;
    switch (nIDResource)
    {
    case IDC_HAND_INTERNAL:
        path = QStringLiteral("cursor1.cur");
        break;
    case IDC_ZOOM_INTERNAL:
        path = QStringLiteral("cur00001.cur");
        break;
    case IDC_BRUSH_INTERNAL:
        path = QStringLiteral("cur00002.cur");
        break;
    case IDC_ARRBLCK:
        path = QStringLiteral("cur00003.cur");
        break;
    case IDC_ARRBLCKCROSS:
        path = QStringLiteral("cur00004.cur");
        break;
    case IDC_ARRWHITE:
        path = QStringLiteral("cur00005.cur");
        break;
    case IDC_COLOR_PICKER:
        path = QStringLiteral("pick_cursor.cur");
        break;
    case IDC_HIT_CURSOR:
        path = QStringLiteral("hit.cur");
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
    case IDC_POINTER_LINK:
        path = QStringLiteral("pointer_link.cur");
        break;
    case IDC_POINTER_LINKNOW:
        path = QStringLiteral("pointer_linknow.cur");
        break;
    case IDC_POINTER_OBJECT_ROTATE:
        path = QStringLiteral("object_rotate.cur");
        break;
    case IDC_POINTER_OBJECT_SCALE:
        path = QStringLiteral("object_scale.cur");
        break;
    case IDC_POINTER_OBJECT_MOVE:
        path = QStringLiteral("object_move.cur");
        break;
    case IDC_POINTER_PLUS:
        path = QStringLiteral("pointer_plus.cur");
        break;
    case IDC_POINTER_MINUS:
        path = QStringLiteral("pointer_minus.cur");
        break;
    case IDC_POINTER_FLATTEN:
        path = QStringLiteral("pointer_flatten.cur");
        break;
    case IDC_POINTER_SMOOTH:
        path = QStringLiteral("pointer_smooth.cur");
        break;
    case IDC_POINTER_SO_SELECT:
        path = QStringLiteral("pointer_so_select.cur");
        break;
    case IDC_POINTER_SO_SELECT_PLUS:
        path = QStringLiteral("pointer_so_sel_plus.cur");
        break;
    case IDC_POINTER_SO_SELECT_MINUS:
        path = QStringLiteral("pointer_.cur");
        break;
    case IDC_POINTER_DRAG_ITEM:
        path = QStringLiteral("pointerDragItem.cur");
        break;
    case IDC_CURSOR_HAND_DRAG:
        path = QStringLiteral("handDrag.cur");
        break;
    case IDC_CURSOR_HAND_FINGER:
        path = QStringLiteral("cursor2.cur");
        break;
    case IDC_ARROW_UP:
        path = QStringLiteral("arrow_up.cur");
        break;
    case IDC_ARROW_DOWN:
        path = QStringLiteral("arrow_down.cur");
        break;
    case IDC_ARROW_DOWNRIGHT:
        path = QStringLiteral("arrow_downright.cur");
        break;
    case IDC_ARROW_UPRIGHT:
        path = QStringLiteral("arrow_upright.cur");
        break;
    case IDC_POINTER_GET_HEIGHT:
        path = QStringLiteral("pointer_getheight.cur");
        break;
    default:
        return QCursor();
    }
    path = QStringLiteral(":/cursors/res/") + path;
    QPixmap pm(path);
    if (!pm.isNull() && (hotX < 0 || hotY < 0))
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
// This function is supposed to format float in user-friendly way,
// omitting the exponent notation.
//
// Why not using printf? Its formatting rules has following drawbacks:
//  %g   - will use exponent for small numbers;
//  %.Nf - doesn't allow to control total amount of significant numbers,
//         which exposes limited precision during binary-to-decimal fraction
//         conversion.
//////////////////////////////////////////////////////////////////////////
void FormatFloatForUI(QString& str, int significantDigits, double value)
{
    str = TrimTrailingZeros(QString::number(value, 'f', significantDigits));
    return;
}
//////////////////////////////////////////////////////////////////////////-

//////////////////////////////////////////////////////////////////////////
QColor ColorLinearToGamma(ColorF col)
{
    float r = clamp_tpl(col.r, 0.0f, 1.0f);
    float g = clamp_tpl(col.g, 0.0f, 1.0f);
    float b = clamp_tpl(col.b, 0.0f, 1.0f);
    float a = clamp_tpl(col.a, 0.0f, 1.0f);

    r = (float)(r <= 0.0031308 ? (12.92 * r) : (1.055 * pow((double)r, 1.0 / 2.4) - 0.055));
    g = (float)(g <= 0.0031308 ? (12.92 * g) : (1.055 * pow((double)g, 1.0 / 2.4) - 0.055));
    b = (float)(b <= 0.0031308 ? (12.92 * b) : (1.055 * pow((double)b, 1.0 / 2.4) - 0.055));

    return QColor(int(r * 255.0f), int(g * 255.0f), int(b * 255.0f), int(a * 255.0f));
}

//////////////////////////////////////////////////////////////////////////
ColorF ColorGammaToLinear(const QColor& col)
{
    float r = (float)col.red() / 255.0f;
    float g = (float)col.green() / 255.0f;
    float b = (float)col.blue() / 255.0f;
    float a = (float)col.alpha() / 255.0f;

    return ColorF((float)(r <= 0.04045 ? (r / 12.92) : pow(((double)r + 0.055) / 1.055, 2.4)),
        (float)(g <= 0.04045 ? (g / 12.92) : pow(((double)g + 0.055) / 1.055, 2.4)),
        (float)(b <= 0.04045 ? (b / 12.92) : pow(((double)b + 0.055) / 1.055, 2.4)), a);
}

QColor ColorToQColor(uint32 color)
{
    return QColor::fromRgbF((float)GetRValue(color) / 255.0f,
        (float)GetGValue(color) / 255.0f,
        (float)GetBValue(color) / 255.0f);
}

namespace EditorUtils
{
    AZ_PUSH_DISABLE_WARNING(4273, "-Wunknown-warning-option")
    AzWarningAbsorber::AzWarningAbsorber(const char* window)
        : m_window(window)
    AZ_POP_DISABLE_WARNING
    {
        BusConnect();
    }

    AzWarningAbsorber::~AzWarningAbsorber()
    {
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
    }

    bool AzWarningAbsorber::OnPreWarning(const char* window, const char*, int, const char*, const char*)
    {
        if ((window)&&(m_window == window))
        {
            return true;
        }

        return false;
    }

    const char* LevelFile::GetOldCryFileExtension()
    {
        const char* oldCryExtension = nullptr;
        EditorInternal::EditorToolsApplicationRequestBus::BroadcastResult(
            oldCryExtension, &EditorInternal::EditorToolsApplicationRequests::GetOldCryLevelExtension);

        AZ_Assert(oldCryExtension, "Cannot retrieve file extension");
        return oldCryExtension;
    }

    const char* LevelFile::GetDefaultFileExtension()
    {
        const char* levelExtension = nullptr;
        EditorInternal::EditorToolsApplicationRequestBus::BroadcastResult(
            levelExtension, &EditorInternal::EditorToolsApplicationRequests::GetLevelExtension);

        AZ_Assert(levelExtension, "Cannot retrieve file extension");
        return levelExtension;
    }

} // namespace EditorUtils

