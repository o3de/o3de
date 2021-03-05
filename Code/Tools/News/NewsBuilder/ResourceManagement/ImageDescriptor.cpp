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
        int nBytes = image.sizeInBytes();

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
