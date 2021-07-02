/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ImageDescriptor.h"
#include "NewsShared/ResourceManagement/Resource.h"

#include <QImageReader>
#include <QBuffer>
#include <QPixmap>
#include <QVariant>
#include <QFile>

namespace News
{

    ImageDescriptor::ImageDescriptor(
        Resource& resource)
        : Descriptor(resource)
    {
    }

    bool ImageDescriptor::Read(const QString& filename, QString& error) const
    {
        QImageReader reader(filename);
        if (!reader.canRead())
        {
            error = reader.errorString();
            return false;
        }
        QImage image = reader.read();

        QPixmap pixmap(filename);
        QByteArray data;
        QBuffer buffer(&data);
        buffer.open(QIODevice::WriteOnly);
        QFile test("test.png");
        if (pixmap.save(&buffer, "PNG"))
        {
            m_resource.SetData(data);
            return true;
        }
        error = QString("failed to save image %1").arg(filename);
        return false;
    }

}
