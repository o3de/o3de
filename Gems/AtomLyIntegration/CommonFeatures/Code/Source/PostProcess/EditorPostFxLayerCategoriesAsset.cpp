/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/EditorPostFxLayerCategoriesAsset.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace AZ
{
    namespace Render
    {
        void EditorPostFxLayerCategoriesAsset::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<EditorPostFxLayerCategoriesAsset>()
                    ->Attribute(AZ::Edit::Attributes::EnableForAssetEditor, true)
                    ->Version(0)
                    ->Field("Layer Categories", &EditorPostFxLayerCategoriesAsset::m_layerCategories)
                    ;

                if (AZ::EditContext* edit = serialize->GetEditContext())
                {
                    edit->Class<EditorPostFxLayerCategoriesAsset>(
                        QT_TRANSLATE_NOOP("AtomLyIntegration", "Layer Categories"), QT_TRANSLATE_NOOP("AtomLyIntegration", "Contains priority-indexed layer categories used by PostFX Layer Component"))
                        ->DataElement(0, &EditorPostFxLayerCategoriesAsset::m_layerCategories, QT_TRANSLATE_NOOP("AtomLyIntegration", "Layer Categories"), "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, true)
                        ->ElementAttribute(AZ::Edit::Attributes::MaxLength, 64)
                        ;
                }
            }
        }
    }
}
