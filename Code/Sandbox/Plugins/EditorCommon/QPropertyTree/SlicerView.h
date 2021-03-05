//-------------------------------------------------------------------------------
// Copyright (C) Amazon.com, Inc. or its affiliates.
// All Rights Reserved.
//
// Licensed under the terms set out in the LICENSE.HTML file included at the
// root of the distribution; you may not use this file except in compliance
// with the License.
//
// Do not remove or modify this notice or the LICENSE.HTML file.  This file
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// either express or implied. See the License for the specific language
// governing permissions and limitations under the License.
//-------------------------------------------------------------------------------

// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once
#ifndef CRYINCLUDE_EDITORCOMMON_SLICERVIEW_H
#define CRYINCLUDE_EDITORCOMMON_SLICERVIEW_H

class SlicerView
: public QGraphicsView
{
public:

    SlicerView(QGraphicsScene* scene, QWidget* parent = nullptr);

protected:

    // This is intentionally empty.
    void scrollContentsBy([[maybe_unused]] int dx, [[maybe_unused]] int dy) override {};
};

#endif // CRYINCLUDE_EDITORCOMMON_SLICERVIEW_H
