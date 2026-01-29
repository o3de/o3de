// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include "${SanitizedCppName}Asset.h"

#include <AzCore/RTTI/BehaviorContext.h>

namespace ${GemName}
{
    void ${SanitizedCppName}Asset::Reflect(AZ::ReflectContext* context)
    {
        if (auto sc = azrtti_cast<AZ::SerializeContext*>(context))
        {
            sc->Class<${SanitizedCppName}Asset>()
                ->Version(1)
                ->Attribute(AZ::Edit::Attributes::EnableForAssetEditor, true)
                ->Field("singleVariable", &${SanitizedCppName}Asset::singleVariable)
                ;

            if (AZ::EditContext* ec = sc->GetEditContext())
            {
                ec->Class<${SanitizedCppName}Asset>("${SanitizedCppName}Asset", "[Description of functionality provided by this asset]")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &${SanitizedCppName}Asset::singleVariable, "singleVariable", "Replace this variable with anything.")
                ;
            }
        }

    }
} // namespace ${GemName}
