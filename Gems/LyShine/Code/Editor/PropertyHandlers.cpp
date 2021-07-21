/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
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
