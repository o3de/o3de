//=============================================================================
//CBBoxInt32
// 3D bounding box with int32 coordinates
//
//=============================================================================
// (C) 2005 ATI Research, Inc., All rights reserved.
//=============================================================================
// modifications by Crytek GmbH
// modifications by Amazon

#pragma once

namespace ImageProcessing
{
    //bounding box class with coords specified as int32
    class CBBoxInt32
    {
    public:
        int32 m_minCoord[3];  //upper left back corner
        int32 m_maxCoord[3];  //lower right front corner

        CBBoxInt32();
        bool Empty(void);
        void  Clear(void);
        void  Augment(int32 a_X, int32 a_Y, int32 a_Z);
        void  AugmentX(int32 a_X);
        void  AugmentY(int32 a_Y);
        void  AugmentZ(int32 a_Z);
        void  ClampMin(int32 a_X, int32 a_Y, int32 a_Z);
        void  ClampMax(int32 a_X, int32 a_Y, int32 a_Z);
    };
} //namespace ImageProcessing
