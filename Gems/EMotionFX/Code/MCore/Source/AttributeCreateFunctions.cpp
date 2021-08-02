/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "MCoreSystem.h"
#include "AttributeBool.h"
#include "AttributeColor.h"
#include "AttributeFloat.h"
#include "AttributeInt32.h"
#include "AttributePointer.h"
#include "AttributeQuaternion.h"
#include "AttributeString.h"
#include "AttributeVector2.h"
#include "AttributeVector3.h"
#include "AttributeVector4.h"


namespace MCore
{
    // static create functions
    AttributeBool* AttributeBool::Create(bool value)
    { 
        AttributeBool* result = aznew AttributeBool(); 
        result->SetValue(value); 
        return result; 
    }
    
    AttributeColor* AttributeColor::Create()
    { 
        return aznew AttributeColor(); 
    }
    
    AttributeColor* AttributeColor::Create(const RGBAColor& value)
    { 
        AttributeColor* result = aznew AttributeColor();
        result->SetValue(value); 
        return result; 
    }

    AttributeFloat* AttributeFloat::Create(float value)
    { 
        AttributeFloat* result = aznew AttributeFloat(); 
        result->SetValue(value); 
        return result; 
    }
    
    AttributeInt32* AttributeInt32::Create(int32 value)
    { 
        AttributeInt32* result = aznew AttributeInt32(); 
        result->SetValue(value); 
        return result; 
    }
    
    AttributePointer* AttributePointer::Create(void* value)
    { 
        AttributePointer* result = aznew AttributePointer();
        result->SetValue(value); 
        return result; 
    }
    
    AttributeQuaternion* AttributeQuaternion::Create()
    { 
        return aznew AttributeQuaternion();
    }
    
    AttributeQuaternion* AttributeQuaternion::Create(float x, float y, float z, float w)
    { 
        AttributeQuaternion* result = aznew AttributeQuaternion(); 
        result->SetValue(AZ::Quaternion(x, y, z, w)); 
        return result; 
    }
    
    AttributeQuaternion* AttributeQuaternion::Create(const AZ::Quaternion& value)
    { 
        AttributeQuaternion* result = aznew AttributeQuaternion(); 
        result->SetValue(value); 
        return result; 
    }
    
    AttributeString* AttributeString::Create(const AZStd::string& value)
    { 
        AttributeString* result = aznew AttributeString();
        result->SetValue(value); 
        return result; 
    }
    
    AttributeString* AttributeString::Create(const char* value)
    { 
        AttributeString* result = aznew AttributeString(); 
        result->SetValue(value); 
        return result; 
    }

    AttributeVector2* AttributeVector2::Create()
    { 
        return aznew AttributeVector2(); 
    }
    
    AttributeVector2* AttributeVector2::Create(float x, float y)
    { 
        AttributeVector2* result = aznew AttributeVector2();
        result->SetValue(AZ::Vector2(x, y)); 
        return result; 
    }
    
    AttributeVector2* AttributeVector2::Create(const AZ::Vector2& value)
    { 
        AttributeVector2* result = aznew AttributeVector2();
        result->SetValue(value); 
        return result; 
    }
    
    AttributeVector3* AttributeVector3::Create()
    { 
        return aznew AttributeVector3(); 
    }
    
    AttributeVector3* AttributeVector3::Create(float x, float y, float z)
    { 
        AttributeVector3* result = aznew AttributeVector3(); 
        result->SetValue(AZ::Vector3(x, y, z));
        return result; 
    }
    
    AttributeVector3* AttributeVector3::Create(const AZ::Vector3& value)
    { 
        AttributeVector3* result = aznew AttributeVector3(); 
        result->SetValue(value); 
        return result;  
    }
    
    AttributeVector4* AttributeVector4::Create()
    { 
        return aznew AttributeVector4();
    }
    
    AttributeVector4* AttributeVector4::Create(float x, float y, float z, float w)
    { 
        AttributeVector4* result = aznew AttributeVector4(); 
        result->SetValue(AZ::Vector4(x, y, z, w)); 
        return result; 
    }
    
    AttributeVector4* AttributeVector4::Create(const AZ::Vector4& value)
    { 
        AttributeVector4* result = aznew AttributeVector4(); 
        result->SetValue(value); 
        return result;  
    }

}   // namespace MCore

