/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#undef RC_INVOKED

#include <Atom/Feature/Utils/EditorModelPreset.h>
#include <Atom/Feature/Utils/ModelPreset.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace AZ
{
    namespace Render
    {
        void EditorModelPreset::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<ModelPreset>(
                        "ModelPreset", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ModelPreset::m_modelAsset, QT_TRANSLATE_NOOP("Atom::Feature", "Model Asset"), QT_TRANSLATE_NOOP("Atom::Feature", "Model asset reference"))
                        ;
                }
            }
        }
    }
}
