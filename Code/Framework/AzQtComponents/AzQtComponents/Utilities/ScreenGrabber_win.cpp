/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <Magnification.h>

#include <QApplication>
#include <QScreen>
#include <QWindow>
#include <QtWinExtras/QtWin>
#include <QGlobalStatic>
#include <QHash>
#include <QMutex>
#include <QtMath>

#include <string>
#include <AzCore/Debug/Trace.h>

#include <AzQtComponents/Components/Widgets/Eyedropper.h>
#include <AzQtComponents/Utilities/ScreenGrabber.h>

namespace AzQtComponents
{

    namespace
    {
        [[maybe_unused]] std::string GetLastErrorString()
        {
            DWORD error = GetLastError();
            if (!error)
            {
                return{};
            }

            char* buffer = nullptr;
            DWORD length = FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr,
                error,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                reinterpret_cast<LPSTR>(&buffer),
                0,
                nullptr);
            if (!length)
            {
                return{};
            }

            std::string result(buffer, length);
            LocalFree(buffer);
            return result;
        }

        using Hash = QHash<HWND, ScreenGrabber::Internal*>;
        Hash* g_internalInstances = nullptr;
        // This is a POD type with size == sizeof(void*)
        QBasicMutex g_internalInstancesLock;
    }

    class ScreenGrabber::Internal
    {
    public:
        Internal(QSize size);
        virtual ~Internal();

        QWidget* host() const { return m_host.data(); }
        HWND magnifier() const { return m_magnifier; }
        const QImage captured() const { return m_captured; }

    private:
        static BOOL WINAPI callback(HWND hwnd, void* srcdata, MAGIMAGEHEADER srcheader, void* destdata, MAGIMAGEHEADER destheader, RECT unclipped, RECT clipped, HRGN dirty);

        QScopedPointer<QWidget> m_host;
        HWND m_magnifier;
        QImage m_captured;
    };

    ScreenGrabber::Internal::Internal(QSize size)
    {
        QMutexLocker locker(&g_internalInstancesLock);

        if (!g_internalInstances)
        {
            BOOL success = MagInitialize();
            Q_UNUSED(success);
            AZ_Assert(success, "Failed to initialize Windows Magnification API: %s", GetLastErrorString().c_str());
        }

        // Make a window to host the magnification "control"
        m_host.reset(new QWidget(nullptr, Qt::FramelessWindowHint));
        m_host->setFixedSize(size / m_host->devicePixelRatioF());
        m_host->show();

        // move it to the top left
        m_host->move(0, 0);

        HWND hostHandle = reinterpret_cast<HWND>(m_host->effectiveWinId());

        // Add the WS_EX_LAYERED extended window style
        SetWindowLong(hostHandle, GWL_EXSTYLE, GetWindowLong(hostHandle, GWL_EXSTYLE) | WS_EX_LAYERED);
        // Set full opacity
        SetLayeredWindowAttributes(hostHandle, 0, 255, LWA_ALPHA);

        // Create the magnifier "control". It's really just a HWND that the API dumps pixels into.
        // We add 1 pixel to the size after observing that the callback function receives images of
        // the correct size, but with garbage data in the last scanline. We crop this in the callback.
        m_magnifier = CreateWindowW(
            WC_MAGNIFIERW,
            L"Open 3D Engine Color Picker Eyedropper Helper",
            WS_CHILD | WS_VISIBLE,
            0,
            0,
            size.width(),
            size.height() + 1,
            hostHandle, nullptr, nullptr, nullptr);

        // HACK; The magnifier control must be "visible" to function, which means it has to have some pixels on
        // screen. HOWEVER, we can set a clip region on the parent (layered) window which hides it entirely
        // and allows us to have it on screen without being visible.
        // We still receive all the pixels in the callback.
        RECT hostSize = {};
        GetWindowRect(hostHandle, &hostSize);
        HRGN clip = CreateRectRgn(hostSize.right - hostSize.left, 0, hostSize.right - hostSize.left, hostSize.bottom - hostSize.top);
        SetWindowRgn(hostHandle, clip, true);

        if (!g_internalInstances)
        {
            g_internalInstances = new QHash<HWND, ScreenGrabber::Internal*>();
        }
        g_internalInstances->insert(m_magnifier, this);

        {
            // Although this mechanism is deprecated, we need to use it to get the pixel data because the GDI
            // functions that QScreen uses to read pixels don't work outside the visible desktop area. Ideally,
            // we'd be able to just read the pixels from the magnifier's HWND in the same way, but this just
            // results in white pixels due to the special way the magnification API writes the pixels to the
            // screen.
            BOOL success = MagSetImageScalingCallback(m_magnifier, &callback);
            Q_UNUSED(success)
            AZ_Assert(success, "Failed to initialize Windows Magnification imaging scaling callback: %s", GetLastErrorString().c_str());
        }
    }

    ScreenGrabber::Internal::~Internal()
    {
        g_internalInstancesLock.lock();
        g_internalInstances->remove(m_magnifier);
        if (g_internalInstances->isEmpty())
        {
            delete g_internalInstances;
            g_internalInstances = nullptr;
            MagUninitialize();
        }
        g_internalInstancesLock.unlock();
        DestroyWindow(m_magnifier);
    }

    BOOL ScreenGrabber::Internal::callback(HWND hwnd, void* srcdata, MAGIMAGEHEADER srcheader, void* destdata, MAGIMAGEHEADER destheader, RECT unclipped, RECT clipped, HRGN dirty)
    {
        Q_UNUSED(srcheader);
        Q_UNUSED(destdata);
        Q_UNUSED(destheader);
        Q_UNUSED(unclipped);
        Q_UNUSED(clipped);
        Q_UNUSED(dirty);

        // We are actually given some Magnification API made child window of our "magnification control".
        HWND magnifier = GetAncestor(hwnd, GA_PARENT);

        QImage::Format format = QImage::Format_Invalid;

        if (srcheader.format == GUID_WICPixelFormat24bppRGB || srcheader.format == GUID_WICPixelFormat24bppBGR)
        {
            format = QImage::Format_RGB888;
        }
        else if (srcheader.format == GUID_WICPixelFormat32bppRGBA || srcheader.format == GUID_WICPixelFormat32bppBGRA)
        {
            format = QImage::Format_RGBA8888;
        }
        AZ_Assert(format != QImage::Format_Invalid, "Unable to convert pixel format");

        // We subtract one from the height because the last scanline is always garbage
        auto result = QImage(((unsigned char*)srcdata) + srcheader.offset, srcheader.width, srcheader.height - 1, format);

        // With the above hack to make the magnifier window invisible, the R/B channels are inverted vs.
        // what the reported pixel format would suggest. If some of the pixels are visible, this doesn't
        // appear to be the case.
        if (srcheader.format != GUID_WICPixelFormat24bppBGR && srcheader.format != GUID_WICPixelFormat32bppBGRA)
        {
            result = result.rgbSwapped();
        }

        // We hold the lock until the end of the function as we don't know what thread we're being
        // called back from and the UI thread may delete the instance
        QMutexLocker locker(&g_internalInstancesLock);
        if (!g_internalInstances)
        {
            AZ_Warning("ScreenGrabber", false, "Callback for unknown Magnification API control handle");
            return false;
        }
        auto instance = g_internalInstances->find(magnifier);
        if (instance == g_internalInstances->end())
        {
            AZ_Warning("ScreenGrabber", false, "Callback for unknown Magnification API control handle");
            return false;
        }

        (*instance)->m_captured = result;
        return true;
    }

    ScreenGrabber::ScreenGrabber(const QSize size, Eyedropper* parent /* = nullptr */)
        : QObject(parent)
        , m_size(size)
        , m_owner(parent)
    {
        m_internal.reset(new Internal(size));
    }

    ScreenGrabber::~ScreenGrabber()
    {
        m_internal.reset();
    }

    QImage ScreenGrabber::grab(const QPoint& point) const
    {
        // We have to do this lazily, as we need the owning Eyedropper to have been shown
        // before we can get its HWND and exclude it.
        HWND hwnd = reinterpret_cast<HWND>(m_owner->effectiveWinId());
        BOOL success = MagSetWindowFilterList(m_internal->magnifier(), MW_FILTERMODE_EXCLUDE, 1, &hwnd);
        Q_UNUSED(success);
        AZ_Assert(success, "Couldn't add the grabber window to the magnification system's window filter list: %s", GetLastErrorString().c_str());

        // The part of the screen we want to scale up
        QRect region({}, m_size);
        region.moveCenter(point);
        RECT area{ region.left(), region.top(), region.right(), region.bottom() };

        success = MagSetWindowSource(m_internal->magnifier(), area);
        Q_UNUSED(success);
        AZ_Assert(success, "Couldn't update the part of the screen being magnified: %s", GetLastErrorString().c_str());

        // Cause the magnification target to be redrawn
        InvalidateRect(m_internal->magnifier(), nullptr, true);

        return m_internal->captured();
    }

} // namespace AzQtComponents

#include "Utilities/moc_ScreenGrabber.cpp"
