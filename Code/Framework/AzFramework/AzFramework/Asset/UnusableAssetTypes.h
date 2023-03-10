#pragma once

#include <AzCore/Asset/AssetCommon.h>

namespace AzFramework
{
    // A list of editor unusuable asset types that are filtered out through a toggle switch in the asset browser 

    inline constexpr AZ::Data::AssetType AzslOutcomeAssetType{ "{6977AEB1-17AD-4992-957B-23BB2E85B18B}" };
    inline constexpr AZ::Data::AssetType ModelLodAssetType{ "{65B5A801-B9B9-4160-9CB4-D40DAA50B15C}" };
    inline constexpr AZ::Data::AssetType ImageMipChainAssetType{ "{CB403C8A-6982-4C9F-8090-78C9C36FBEDB}" };
    inline constexpr AZ::Data::AssetType BufferAssetType{ "{F6C5EA8A-1DB3-456E-B970-B6E2AB262AED}" };
    inline constexpr AZ::Data::AssetType ShaderVariantAssetType{ "{51BED815-36D8-410E-90F0-1FA9FF765FBA}" };

} // namespace AzFramework
