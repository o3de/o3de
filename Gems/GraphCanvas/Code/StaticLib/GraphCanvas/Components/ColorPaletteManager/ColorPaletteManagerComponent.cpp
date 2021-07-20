/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QPainter>
AZ_POP_DISABLE_WARNING

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

#include <GraphCanvas/Components/ColorPaletteManager/ColorPaletteManagerComponent.h>
#include <GraphCanvas/Styling/StyleHelper.h>

namespace GraphCanvas
{   
    namespace Deprecated
    {
        /////////////////////////////////
        // ColorPaletteManagerComponent
        /////////////////////////////////    
        void ColorPaletteManagerComponent::Reflect(AZ::ReflectContext* reflectContext)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

            if (serializeContext)
            {
                serializeContext->Class<ColorPaletteManagerComponent, AZ::Component>()
                    ->Version(1)
                    ;
            }
        }
    }
}
