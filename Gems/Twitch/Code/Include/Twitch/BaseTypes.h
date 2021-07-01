/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace Twitch
{
    /*
    ** Results enum, Unknown must be the last element, and its value must be 0x7fffffff
    */
     
    enum class ResultCode {Success, InvalidParam, TwitchRESTError, TwitchChannelNoUpdatesToMake, Unknown=0x7ffffff};

    /*
    ** Our receipts
    */

    struct ReceiptID
    {
        AZ_TYPE_INFO(ReceiptID, "{19D10763-0513-4EAC-9C6D-59401F729F1A}");

        ReceiptID() 
            : m_id(0) 
        {}
        
        ReceiptID(const ReceiptID& id)
            : m_id(id.m_id) 
        {}

        bool operator!=(const ReceiptID& id) const
        {
            return m_id != id.m_id;
        }

        bool operator==(const ReceiptID& id) const
        {
            return m_id == id.m_id;
        };

        void SetID(AZ::u64 id)
        { 
            m_id = id;
        }

        AZ::u64 GetID() const
        { 
            return m_id;
        }
        
    private:
        AZ::u64    m_id;
    };

    struct ReturnValue : public ReceiptID
    {
        AZ_TYPE_INFO(ReturnValue, "{892C7B14-AEB2-4107-BA94-716D07EDB8D4}");

        ReturnValue()
            : Result(ResultCode::Unknown)
        {
        }
        
        ReturnValue(const ReceiptID& recieptID, ResultCode result = ResultCode::Success)
            : ReceiptID(recieptID)
            , Result(result)
        {
        }

        ResultCode      Result;
    };

    #define CreateReturnTypeClass(_valueType, _returnType, _ClassGUID)                                              \
        struct _valueType : public ReturnValue                                                                      \
        {                                                                                                           \
            AZ_TYPE_INFO(_valueType, _ClassGUID);                                                                   \
                                                                                                                    \
            _valueType(const _returnType& value, const ReceiptID& recieptID, ResultCode result=ResultCode::Success) \
                : ReturnValue(recieptID, result)                                                                    \
                , Value(value)                                                                                      \
                {}                                                                                                  \
            _returnType Value;                                                                                      \
            AZStd::string ToString() const;                                                                         \
        }

    /*
    ** base return types
    */

    CreateReturnTypeClass(Int64Value, AZ::s64, "{38087AE8-D809-446E-B781-F24AD4167356}");
    CreateReturnTypeClass(Uint64Value, AZ::u64, "{9D84E120-4E4A-4861-BA4A-0ECDD208FA78}");
    CreateReturnTypeClass(StringValue, AZStd::string, "{99F06BB7-FFB7-4907-BC54-E38991B1B6DE}");
}
