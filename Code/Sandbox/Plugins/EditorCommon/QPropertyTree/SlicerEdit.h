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
#ifndef CRYINCLUDE_EDITORCOMMON_SLICEREDIT_H
#define CRYINCLUDE_EDITORCOMMON_SLICEREDIT_H

#if !defined(Q_MOC_RUN)
#include <QLineEdit>

#include "SpriteBorderEditorCommon.h"
#endif

class SlicerEdit
: public QLineEdit
{
    Q_OBJECT

public:

    SlicerEdit( SpriteBorder border,
                QSize& unscaledPixmapSize,
                ISprite* sprite );

    void SetManipulator(SlicerManipulator* manipulator);

    void setPixelPosition(float p);

private:

    SlicerManipulator* m_manipulator;
};

#endif // CRYINCLUDE_EDITORCOMMON_SLICEREDIT_H
