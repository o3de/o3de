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
#include "UiCanvasEditor_precompiled.h"
#include "EditorCommon.h"

#include "PropertyHandlerAnchor.h"
#include "PropertyHandlerChar.h"
#include "PropertyHandlerDirectory.h"
#include "PropertyHandlerEntityIdComboBox.h"
#include "PropertyHandlerLayoutPadding.h"
#include "PropertyHandlerOffset.h"
#include "PropertyHandlerUiParticleColorKeyframe.h"
#include "PropertyHandlerUiParticleFloatKeyframe.h"
#include "PropertyHandlerPivot.h"
#include "PropertyHandlerSprite.h"
#include "PropertyHandlerVec.h"

void PropertyHandlers::Register()
{
    // This MUST be done only ONCE.
    {
        static bool hasBeenDone = false;

        if (hasBeenDone)
        {
            return;
        }

        hasBeenDone = true;
    }

    PropertyHandlerAnchor::Register();
    PropertyHandlerChar::Register();
    PropertyHandlerDirectory::Register();
    PropertyHandlerEntityIdComboBox::Register();
    PropertyHandlerLayoutPadding::Register();
    PropertyHandlerUiParticleColorKeyframe::Register();
    PropertyHandlerUiParticleFloatKeyframe::Register();
    PropertyHandlerOffset::Register();
    PropertyHandlerPivot::Register();
    PropertyHandlerSprite::Register();
    PropertyHandlerVecRegister();
}
