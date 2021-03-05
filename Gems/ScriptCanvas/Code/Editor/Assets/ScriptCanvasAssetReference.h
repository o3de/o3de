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
#include <AzCore/Serialization/DataPatch.h>
#include <AzCore/Asset/AssetCommon.h>

namespace ScriptCanvas
{
    class ScriptCanvasData;
}

namespace ScriptCanvasEditor
{
    class ScriptCanvasAsset;

    class ScriptCanvasAssetReference
    {
    public:
        AZ_TYPE_INFO(ScriptCanvasAssetReference, "{C1B24507-887C-4E20-A259-BFEEDD7EDF9D}");
        AZ_CLASS_ALLOCATOR(ScriptCanvasAssetReference, AZ::SystemAllocator, 0);

        ScriptCanvasAssetReference() = default;
        ScriptCanvasAssetReference(AZ::Data::Asset<ScriptCanvasAsset> scriptCanvasAsset, bool storeInObjectStream = false);
        static void Reflect(AZ::ReflectContext* context);

        /*!
        \param scriptCanvasAsset asset reference to store
        \param storeInObjectStream bool which determines if the asset data will serialized out as part of the ObjectStream when serializing
        */
        void SetAsset(AZ::Data::Asset<ScriptCanvasAsset> scriptCanvasAsset);
        const AZ::Data::Asset<ScriptCanvasAsset>& GetAsset() const;
        AZ::Data::Asset<ScriptCanvasAsset>& GetAsset();

        /*!
        \param storeInObjectStream bool which determines if the asset data will serialized out as part of the ObjectStream when serializing
        */
        void SetAssetDataStoredInternally(bool storeInObjectStream);

        /*!
        Indicates if this scriptCanvas reference contains an asset whose data is stored internally in this class.
        \return true if the asset is asset data is serialize as part of this class object stream
        */
        bool GetAssetDataStoredInternally() const;

    private:
        bool m_storeInObjectStream = false; ///< If true the asset data is stored in the object stream with this class
        AZ::Data::Asset<ScriptCanvasAsset> m_asset;
        friend class ScriptCanvasAssetReferenceContainer;
    };
}

namespace AZStd
{
    template<>
    struct hash<ScriptCanvasEditor::ScriptCanvasAssetReference>
    {
        using argument_type = ScriptCanvasEditor::ScriptCanvasAssetReference;
        using result_type = AZStd::size_t;
        AZ_FORCE_INLINE size_t operator()(const argument_type& key) const
        {
            AZStd::hash<AZ::Data::AssetId> hasher;
            return hasher(key.GetAsset().GetId());
        }
    };
} // namespace AZStd