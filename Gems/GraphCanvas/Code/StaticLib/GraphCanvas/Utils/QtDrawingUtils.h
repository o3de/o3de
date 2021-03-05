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
#pragma once

#include <QPainter>
#include <QPixmap>

#include <AzCore/std/string/string_view.h>

#include <GraphCanvas/Types/Types.h>
#include <GraphCanvas/Editor/EditorTypes.h>

namespace GraphCanvas
{
    namespace Styling
    {
        class StyleHelper;
    }

    class QtDrawingUtils
    {
    public:
        static void GenerateGradients(const AZStd::vector< const Styling::StyleHelper* >& stylingHelpers, const QRectF& area, QLinearGradient& penGradient, QLinearGradient& fillGradient);

        static void FillArea(QPainter& painter, const QRectF& area, const Styling::StyleHelper& styleHelper);

        static void CandyStripeArea(QPainter& painter, const QRectF& area, const CandyStripeConfiguration& candyStripeConfiguration);
        static void PatternFillArea(QPainter& painter, const QRectF& area, const QPixmap& pixmap, const PatternFillConfiguration& patternFillConfiguration);
        static void PatternFillArea(QPainter& painter, const QRectF& area, const PatternedFillGenerator& patternedFillGenerator);
    };
}