/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/DocumentPropertyEditor/Reflection/Visitor.h>

namespace AZ::Reflection
{
    void IRead::Visit(bool, const IAttributes&)
    {
    }

    void IRead::Visit(int8_t, const IAttributes&)
    {
    }

    void IRead::Visit(int16_t, const IAttributes&)
    {
    }

    void IRead::Visit(int32_t, const IAttributes&)
    {
    }

    void IRead::Visit(int64_t, const IAttributes&)
    {
    }

    void IRead::Visit(uint8_t, const IAttributes&)
    {
    }

    void IRead::Visit(uint16_t, const IAttributes&)
    {
    }

    void IRead::Visit(uint32_t, const IAttributes&)
    {
    }

    void IRead::Visit(uint64_t, const IAttributes&)
    {
    }

    void IRead::Visit(float, const IAttributes&)
    {
    }

    void IRead::Visit(double, const IAttributes&)
    {
    }

    void IRead::VisitObjectBegin(const IObjectAccess&, const IAttributes&)
    {
    }

    void IRead::VisitObjectEnd()
    {
    }

    void IRead::Visit(const AZStd::string_view, const IStringAccess&, const IAttributes&)
    {
    }

    void IRead::Visit(const IArrayAccess&, const IAttributes&)
    {
    }

    void IRead::Visit(const IMapAccess&, const IAttributes&)
    {
    }

    void IRead::Visit(const IDictionaryAccess&, const IAttributes&)
    {
    }

    void IRead::Visit(const IPointerAccess&, const IAttributes&)
    {
    }

    void IRead::Visit(const IBufferAccess&, const IAttributes&)
    {
    }

    void IRead::Visit(const AZ::Data::Asset<AZ::Data::AssetData>&, const IAssetAccess&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(bool&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(int8_t&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(int16_t&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(int32_t&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(int64_t&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(uint8_t&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(uint16_t&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(uint32_t&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(uint64_t&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(float&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(double&, const IAttributes&)
    {
    }

    void IReadWrite::VisitObjectBegin(IObjectAccess&, const IAttributes&)
    {
    }

    void IReadWrite::VisitObjectEnd()
    {
    }

    void IReadWrite::Visit(const AZStd::string_view, IStringAccess&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(IArrayAccess&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(IMapAccess&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(IDictionaryAccess&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(int64_t, const IEnumAccess&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(IPointerAccess&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(IBufferAccess&, const IAttributes&)
    {
    }

    void IReadWrite::Visit(const AZ::Data::Asset<AZ::Data::AssetData>&, IAssetAccess&, const IAttributes&)
    {
    }
} // namespace AZ::Reflection
