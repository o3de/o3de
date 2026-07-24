/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QImage>

namespace ProjectSettingsTool
{
    namespace Validators
    {
        static const char* pngMimeType = "image/png";

        // Returns true if path is empty or valid png file with specified dimensions
        template <int imageWidth, int imageHeight>
        FunctorValidator::ReturnType PngImageSetSizeOrEmpty(const QString& path)
        {
            using RetType = FunctorValidator::ReturnType;

            if (IsNotEmpty(path).first != QValidator::Acceptable)
            {
                return FunctorValidator::ReturnType(QValidator::Acceptable, "");
            }
            else
            {
                RetType correct = Internal::FileReadableAndCorrectType(path, pngMimeType);

                if (correct.first)
                {
                    QImage image(path);

                    if (imageWidth == image.width() && imageHeight == image.height())
                    {
                        return FunctorValidator::ReturnType(QValidator::Acceptable, "");
                    }
                    else
                    {
                        return RetType(QValidator::Intermediate, QObject::tr("Image is not %1x%2 pixels.")
                                .arg(QString::number(imageWidth)).arg(QString::number(imageHeight)));
                    }
                }
                else
                {
                    return correct;
                }
            }
        }
    } // namespace Validators
} // namespace ProjectSettingsTool
