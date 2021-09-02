/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Common Camera class implementation

#ifndef CRYINCLUDE_CRYCOMMON_CRY_CAMERA_H
#define CRYINCLUDE_CRYCOMMON_CRY_CAMERA_H
#pragma once


//DOC-IGNORE-BEGIN
#include <Cry_Math.h>
#include <Cry_Geo.h>
//DOC-IGNORE-END

//////////////////////////////////////////////////////////////////////
#define CAMERA_MIN_NEAR         0.001f
#define DEFAULT_NEAR            0.2f
#define DEFAULT_FAR     1024.0f
#define DEFAULT_FOV     (75.0f * gf_PI / 180.0f)
#define MIN_FOV 0.0000001f

//////////////////////////////////////////////////////////////////////

enum
{
    FR_PLANE_NEAR,
    FR_PLANE_FAR,
    FR_PLANE_RIGHT,
    FR_PLANE_LEFT,
    FR_PLANE_TOP,
    FR_PLANE_BOTTOM,
    FRUSTUM_PLANES
};

//////////////////////////////////////////////////////////////////////

enum cull
{
    CULL_EXCLUSION,     // The whole object is outside of frustum.
    CULL_OVERLAP,           // The object &  frustum overlap.
    CULL_INCLUSION      // The whole object is inside frustum.
};

///////////////////////////////////////////////////////////////////////////////
// Implements essential operations like calculation of a view-matrix and
// frustum-culling with simple geometric primitives (Point, Sphere, AABB, OBB).
// All calculation are based on the CryENGINE coordinate-system
//
// We are using a "right-handed" coordinate systems, where the positive X-Axis points
// to the right, the positive Y-Axis points away from the viewer and the positive
// Z-Axis points up. The following illustration shows our coordinate system.
//
// <PRE>
//  z-axis
//    ^
//    |
//    |   y-axis
//    |  /
//    | /
//    |/
//    +---------------->   x-axis
// </PRE>
//
// This same system is also used in 3D-Studio-MAX. It is not unusual for 3D-APIs like D3D9 or
// OpenGL to use a different coordinate system.  Currently in D3D9 we use a coordinate system
// in which the X-Axis points to the right, the Y-Axis points down and the Z-Axis points away
// from the viewer. To convert from the CryEngine system into D3D9 we are just doing a clockwise
// rotation of pi/2 about the X-Axis. This conversion happens in the renderer.
//
// The 6 DOFs (degrees-of-freedom) are stored in one single 3x4 matrix ("m_Matrix"). The 3
// orientation-DOFs are stored in the 3x3 part and the 3 position-DOFs are stored in the translation-
// vector. You can use the member-functions "GetMatrix()" or "SetMatrix(Matrix34(orientation,positon))"
// to change or access the 6 DOFs.
//
// There are helper-function in Cry_Math.h to create the orientation:
//
// This function builds a 3x3 orientation matrix using a view-direction and a radiant to rotate about Y-axis.
//  Matrix33 orientation=Matrix33::CreateOrientation( Vec3(0,1,0), 0 );
//
// This function builds a 3x3 orientation matrix using Yaw-Pitch-Roll angles.
//  Matrix33 orientation=CCamera::CreateOrientationYPR( Ang3(1.234f,0.342f,0) );
//
///////////////////////////////////////////////////////////////////////////////
class CCamera
{
public:
    ILINE static Matrix33 CreateOrientationYPR(const Ang3& ypr);
    ILINE static Ang3 CreateAnglesYPR(const Matrix33& m);
    ILINE static Ang3 CreateAnglesYPR(const Vec3& vdir, f32 r = 0);

    ILINE void SetMatrix(const Matrix34& mat) { assert(mat.IsOrthonormal()); m_Matrix = mat; UpdateFrustum(); };
    ILINE void SetMatrixNoUpdate(const Matrix34& mat) { assert(mat.IsOrthonormal()); m_Matrix = mat; };
    ILINE const Matrix34& GetMatrix() const { return m_Matrix; };
    ILINE Vec3 GetViewdir() const { return m_Matrix.GetColumn1(); };

    ILINE void SetEntityPos(const Vec3& entityPos) { m_entityPos = entityPos; }
    ILINE Vec3 GetEntityPos() const { return m_entityPos; };

    ILINE Matrix34 GetViewMatrix() const { return m_Matrix.GetInverted(); };
    ILINE Vec3 GetPosition() const { return m_Matrix.GetTranslation(); }
    ILINE void SetPosition(const Vec3& p)   { m_Matrix.SetTranslation(p); UpdateFrustum(); }
    ILINE void SetPositionNoUpdate(const Vec3& p)   { m_Matrix.SetTranslation(p); }
    ILINE Vec3 GetUp() const { return m_Matrix.GetColumn2(); }

    //------------------------------------------------------------

    void SetFrustum(int nWidth, int nHeight, f32 FOV = DEFAULT_FOV, f32 nearplane = DEFAULT_NEAR, f32 farplane = DEFAULT_FAR, f32 fPixelAspectRatio = 1.0f);

    ILINE int GetViewSurfaceX() const { return m_Width; }
    ILINE int GetViewSurfaceZ() const { return m_Height; }
    ILINE f32 GetFov() const { return m_fov; }
    ILINE f32 GetNearPlane() const { return m_edge_nlt.y; }
    ILINE f32 GetFarPlane() const { return m_edge_flt.y; }
    ILINE f32 GetAngularResolution() const { return m_Height / m_fov; }
    ILINE f32 GetPixelAspectRatio() const { return m_PixelAspectRatio; }

    ILINE Vec3 GetEdgeP() const { return m_edge_plt; }
    ILINE Vec3 GetEdgeN() const { return m_edge_nlt; }
    ILINE Vec3 GetEdgeF() const { return m_edge_flt; }

    //////////////////////////////////////////////////////////////////////////

    //-----------------------------------------------------------------------------------
    //--------                Frustum-Culling                ----------------------------
    //-----------------------------------------------------------------------------------

    //Check if a point lies within camera's frustum
    bool IsPointVisible(const Vec3& p) const;

    // AABB-frustum test
    // Fast
    bool IsAABBVisible_F(const ::AABB& aabb) const;

    //## constructor/destructor
    CCamera()
    {
        m_Matrix.SetIdentity();
        m_asymRight = 0;
        m_asymLeft = 0;
        m_asymBottom = 0;
        m_asymTop = 0;
        SetFrustum(640, 480);
        m_zrangeMin = 0.0f;
        m_zrangeMax = 1.0f;
        m_pPortal = NULL;
        m_JustActivated = 0;
        m_nPosX = m_nPosY = m_nSizeX = m_nSizeY = 0;
        m_entityPos = Vec3(0, 0, 0);
    }
    ~CCamera() {}

    void SetJustActivated(const bool justActivated) {m_JustActivated = (int)justActivated; }
    bool IsJustActivated() const {return m_JustActivated != 0; }

    void SetViewPort(int nPosX, int nPosY, int nSizeX, int nSizeY)
    {
        m_nPosX = nPosX;
        m_nPosY = nPosY;
        m_nSizeX = nSizeX;
        m_nSizeY = nSizeY;
    }

    void GetViewPort(int& nPosX, int& nPosY, int& nSizeX, int& nSizeY) const
    {
        nPosX = m_nPosX;
        nPosY = m_nPosY;
        nSizeX = m_nSizeX;
        nSizeY = m_nSizeY;
    }

    void UpdateFrustum();

private:
    bool AdditionalCheck(const AABB& aabb) const;
    bool AdditionalCheck(const Vec3& wpos, const OBB& obb, f32 uscale) const;

    Matrix34 m_Matrix;              // world space-matrix

    f32     m_fov;                          // vertical fov in radiants [0..1*PI[
    int     m_Width;                        // surface width-resolution
    int     m_Height;                       // surface height-resolution
    f32     m_PixelAspectRatio; // accounts for aspect ratio and non-square pixels

    Vec3    m_entityPos;                //The position of this camera's entity (does not include HMD position or stereo offsets)

    Vec3    m_edge_nlt;                 // this is the left/upper vertex of the near-plane
    Vec3    m_edge_plt;                 // this is the left/upper vertex of the projection-plane
    Vec3    m_edge_flt;                 // this is the left/upper vertex of the far-clip-plane

    f32     m_asymLeft, m_asymRight, m_asymBottom, m_asymTop;   // Shift to create asymmetric frustum (only used for GPU culling of tessellated objects)
    f32     m_asymLeftProj, m_asymRightProj, m_asymBottomProj, m_asymTopProj;
    f32     m_asymLeftFar, m_asymRightFar, m_asymBottomFar, m_asymTopFar;

    //usually we update these values every frame (they depend on m_Matrix)
    Vec3    m_cltp, m_crtp, m_clbp, m_crbp;        //this are the 4 vertices of the projection-plane in cam-space
    Vec3    m_cltn, m_crtn, m_clbn, m_crbn;        //this are the 4 vertices of the near-plane in cam-space
    Vec3    m_cltf, m_crtf, m_clbf, m_crbf;        //this are the 4 vertices of the farclip-plane in cam-space

    Plane_tpl<f32>   m_fp [FRUSTUM_PLANES]; //
    uint32  m_idx1[FRUSTUM_PLANES], m_idy1[FRUSTUM_PLANES], m_idz1[FRUSTUM_PLANES]; //
    uint32  m_idx2[FRUSTUM_PLANES], m_idy2[FRUSTUM_PLANES], m_idz2[FRUSTUM_PLANES]; //

    // Near Far range of the z-buffer to use for this camera.
    float m_zrangeMin;
    float m_zrangeMax;

    int m_nPosX, m_nPosY, m_nSizeX, m_nSizeY;

    //------------------------------------------------------------------------
    //---   OLD STUFF
    //------------------------------------------------------------------------
public:

    void SetFrustumVertices(Vec3* arrvVerts)
    {
        m_clbp = arrvVerts[0];
        m_cltp = arrvVerts[1];
        m_crtp = arrvVerts[2];
        m_crbp = arrvVerts[3];
    }
    inline void SetFrustumPlane(int i, const Plane_tpl<f32>& plane)
    {
        m_fp[i] = plane;
        //do not break strict aliasing rules, use union instead of reinterpret_casts
        union f32_u
        {
            float floatVal;
            uint32 uintVal;
        };

        {
            f32_u ux;
            ux.floatVal = m_fp[i].n.x;
            f32_u uy;
            uy.floatVal = m_fp[i].n.y;
            f32_u uz;
            uz.floatVal = m_fp[i].n.z;
            uint32 bitX =   ux.uintVal >> 31;
            uint32 bitY =   uy.uintVal >> 31;
            uint32 bitZ =   uz.uintVal >> 31;
            m_idx1[i] = bitX * 3 + 0;
            m_idx2[i] = (1 - bitX) * 3 + 0;
            m_idy1[i] = bitY * 3 + 1;
            m_idy2[i] = (1 - bitY) * 3 + 1;
            m_idz1[i] = bitZ * 3 + 2;
            m_idz2[i] = (1 - bitZ) * 3 + 2;
        }
    }

    inline Ang3 GetAngles()   const { return CreateAnglesYPR(Matrix33(m_Matrix)); }
    void SetAngles(const Ang3& angles)  { SetMatrix(Matrix34::CreateRotationXYZ(angles)); }


    struct IVisArea* m_pPortal; // pointer to portal used to create this camera
    struct ScissorInfo
    {
        ScissorInfo() { x1 = y1 = x2 = y2 = 0; }
        uint16 x1, y1, x2, y2;
    };
    ScissorInfo m_ScissorInfo;

    Vec3    m_OccPosition;      //Position for calculate occlusions (needed for portals rendering)
    inline const Vec3& GetOccPos() const { return(m_OccPosition); }

    int m_JustActivated;        //Camera activated in this frame, used for disabling motion blur effect at camera changes in movies
};

// Description
//  This function builds a 3x3 orientation matrix using YPR-angles
//  Rotation order for the orientation-matrix is Z-X-Y. (Zaxis=YAW / Xaxis=PITCH / Yaxis=ROLL)
//
// <PRE>
//  COORDINATE-SYSTEM
//
//  z-axis
//    ^
//    |
//    |  y-axis
//    |  /
//    | /
//    |/
//    +--------------->   x-axis
// </PRE>
//
//  Example:
//      Matrix33 orientation=CCamera::CreateOrientationYPR( Ang3(1,2,3) );
inline Matrix33 CCamera::CreateOrientationYPR(const Ang3& ypr)
{
    f32 sz, cz;
    sincos_tpl(ypr.x, &sz, &cz);            //Zaxis = YAW
    f32 sx, cx;
    sincos_tpl(ypr.y, &sx, &cx);            //Xaxis = PITCH
    f32 sy, cy;
    sincos_tpl(ypr.z, &sy, &cy);            //Yaxis = ROLL
    Matrix33 c;
    c.m00 = cy * cz - sy * sz * sx;
    c.m01 = -sz * cx;
    c.m02 = sy * cz + cy * sz * sx;
    c.m10 = cy * sz + sy * sx * cz;
    c.m11 = cz * cx;
    c.m12 = sy * sz - cy * sx * cz;
    c.m20 = -sy * cx;
    c.m21 = sx;
    c.m22 = cy * cx;
    return c;
}

// Description
//   <PRE>
//   x-YAW
//   y-PITCH (negative=looking down / positive=looking up)
//   z-ROLL
//   </PRE>
// Note: If we are looking along the z-axis, its not possible to specify the x and z-angle
inline Ang3 CCamera::CreateAnglesYPR(const Matrix33& m)
{
    assert(m.IsOrthonormal());
    float l = Vec3(m.m01, m.m11, 0.0f).GetLength();
    if (l > 0.0001)
    {
        return Ang3(atan2f(-m.m01 / l, m.m11 / l), atan2f(m.m21, l), atan2f(-m.m20 / l, m.m22 / l));
    }
    else
    {
        return Ang3(0, atan2f(m.m21, l), 0);
    }
}

// Description
//   <PRE>
//x-YAW
//y-PITCH (negative=looking down / positive=looking up)
//z-ROLL (its not possile to extract a "roll" from a view-vector)
// </PRE>
// Note: if we are looking along the z-axis, its not possible to specify the rotation about the z-axis
ILINE Ang3 CCamera::CreateAnglesYPR(const Vec3& vdir, f32 r)
{
    assert((fabs_tpl(1 - (vdir | vdir))) < 0.001);        //check if unit-vector
    f32 l = Vec3(vdir.x, vdir.y, 0.0f).GetLength(); //check if not zero
    if (l > 0.0001)
    {
        return Ang3(atan2f(-vdir.x / l, vdir.y / l), atan2f(vdir.z, l), r);
    }
    else
    {
        return Ang3(0, atan2f(vdir.z, l), r);
    }
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
inline void CCamera::SetFrustum(int nWidth, int nHeight, f32 FOV, f32 nearplane, f32 farplane, f32 fPixelAspectRatio)
{
    assert (nearplane >= CAMERA_MIN_NEAR);     //check if near-plane is valid
    assert (farplane >= 0.1f);              //check if far-plane is valid
    assert (farplane >= nearplane);     //check if far-plane bigger then near-plane
    assert (FOV >= MIN_FOV && FOV < gf_PI);    //check if specified FOV is valid

    m_fov = FOV;

    m_Width = nWidth;           //surface x-resolution
    m_Height = nHeight;     //surface z-resolution

    f32 fWidth  = (((f32)nWidth) / fPixelAspectRatio);
    f32 fHeight = (f32) nHeight;

    m_PixelAspectRatio = fPixelAspectRatio;

    //-------------------------------------------------------------------------
    //---  calculate the Left/Top edge of the Projection-Plane in EYE-SPACE ---
    //-------------------------------------------------------------------------
    f32 projLeftTopX = -fWidth * 0.5f;
    f32 projLeftTopY = static_cast<f32>((1.0f / tan_tpl(m_fov * 0.5f)) * (fHeight * 0.5f));
    f32 projLeftTopZ = fHeight * 0.5f;

    m_edge_plt.x = projLeftTopX;
    m_edge_plt.y = projLeftTopY;
    m_edge_plt.z = projLeftTopZ;

    assert(fabs(acos_tpl(Vec3d(0, m_edge_plt.y, m_edge_plt.z).GetNormalized().y) * 2 - m_fov) < 0.001);

    float invProjLeftTopY = 1.0f / projLeftTopY;

    //Apply asym shift to the camera frustum - Necessary for properly culling tessellated objects in VR
    //These are applied in UpdateFrustum to the camera space frustum planes
    //Can't apply asym shift to frustum edges here. That would only apply to the top left corner
    //rather than the whole frustum. It would also interfere with shadow map application

    //m_asym is at the near plane, we want it at the projection plane too
    m_asymLeftProj = (m_asymLeft / nearplane) * projLeftTopY;
    m_asymTopProj = (m_asymTop / nearplane) * projLeftTopY;
    m_asymRightProj = (m_asymRight / nearplane) * projLeftTopY;
    m_asymBottomProj = (m_asymBottom / nearplane) * projLeftTopY;

    //Also want m_asym at the far plane
    m_asymLeftFar = m_asymLeftProj * (farplane * invProjLeftTopY);
    m_asymTopFar = m_asymTopProj * (farplane * invProjLeftTopY);
    m_asymRightFar = m_asymRightProj * (farplane * invProjLeftTopY);
    m_asymBottomFar = m_asymBottomProj * (farplane * invProjLeftTopY);

    m_edge_nlt.x    = nearplane * projLeftTopX * invProjLeftTopY;
    m_edge_nlt.y    = nearplane;
    m_edge_nlt.z    = nearplane * projLeftTopZ * invProjLeftTopY;

    //calculate the left/upper edge of the far-plane (=not rotated)
    m_edge_flt.x    = projLeftTopX  * (farplane * invProjLeftTopY);
    m_edge_flt.y    = farplane;
    m_edge_flt.z    = projLeftTopZ  * (farplane * invProjLeftTopY);

    UpdateFrustum();
}

/*!
 *
 *  Updates all parameters required by the render-engine:
 *
 *  3d-view-frustum and all matrices
 *
 */
inline void CCamera::UpdateFrustum()
{
    //-------------------------------------------------------------------
    //--- calculate frustum-edges of projection-plane in CAMERA-SPACE ---
    //-------------------------------------------------------------------
    Matrix33 m33 = Matrix33(m_Matrix);
    m_cltp = m33 * Vec3(+m_edge_plt.x + m_asymLeftProj, +m_edge_plt.y, +m_edge_plt.z + m_asymTopProj);
    m_crtp = m33 * Vec3(-m_edge_plt.x + m_asymRightProj, +m_edge_plt.y, +m_edge_plt.z + m_asymTopProj);
    m_clbp = m33 * Vec3(+m_edge_plt.x + m_asymLeftProj, +m_edge_plt.y, -m_edge_plt.z + m_asymBottomProj);
    m_crbp = m33 * Vec3(-m_edge_plt.x + m_asymRightProj, +m_edge_plt.y, -m_edge_plt.z + m_asymBottomProj);

    m_cltn = m33 * Vec3(+m_edge_nlt.x + m_asymLeft, +m_edge_nlt.y, +m_edge_nlt.z + m_asymTop);
    m_crtn = m33 * Vec3(-m_edge_nlt.x + m_asymRight, +m_edge_nlt.y, +m_edge_nlt.z + m_asymTop);
    m_clbn = m33 * Vec3(+m_edge_nlt.x + m_asymLeft, +m_edge_nlt.y, -m_edge_nlt.z + m_asymBottom);
    m_crbn = m33 * Vec3(-m_edge_nlt.x + m_asymRight, +m_edge_nlt.y, -m_edge_nlt.z + m_asymBottom);

    m_cltf = m33 * Vec3(+m_edge_flt.x + m_asymLeftFar, +m_edge_flt.y, +m_edge_flt.z + m_asymTopFar);
    m_crtf = m33 * Vec3(-m_edge_flt.x + m_asymRightFar, +m_edge_flt.y, +m_edge_flt.z + m_asymTopFar);
    m_clbf = m33 * Vec3(+m_edge_flt.x + m_asymLeftFar, +m_edge_flt.y, -m_edge_flt.z + m_asymBottomFar);
    m_crbf = m33 * Vec3(-m_edge_flt.x + m_asymRightFar, +m_edge_flt.y, -m_edge_flt.z + m_asymBottomFar);

    //-------------------------------------------------------------------------------
    //---  calculate the six frustum-planes using the frustum edges in world-space ---
    //-------------------------------------------------------------------------------
    m_fp[FR_PLANE_NEAR  ]   = Plane_tpl<f32>::CreatePlane(m_crtn + GetPosition(), m_cltn + GetPosition(), m_crbn + GetPosition());
    m_fp[FR_PLANE_RIGHT ]   = Plane_tpl<f32>::CreatePlane(m_crbf + GetPosition(), m_crtf + GetPosition(), GetPosition());
    m_fp[FR_PLANE_LEFT  ]   = Plane_tpl<f32>::CreatePlane(m_cltf + GetPosition(), m_clbf + GetPosition(), GetPosition());
    m_fp[FR_PLANE_TOP   ]   = Plane_tpl<f32>::CreatePlane(m_crtf + GetPosition(), m_cltf + GetPosition(), GetPosition());
    m_fp[FR_PLANE_BOTTOM]   = Plane_tpl<f32>::CreatePlane(m_clbf + GetPosition(), m_crbf + GetPosition(), GetPosition());
    m_fp[FR_PLANE_FAR   ]   = Plane_tpl<f32>::CreatePlane(m_crtf + GetPosition(), m_crbf + GetPosition(), m_cltf + GetPosition());  //clip-plane

    uint32 rh = m_Matrix.IsOrthonormalRH();
    if (rh == 0)
    {
        m_fp[FR_PLANE_NEAR  ] = -m_fp[FR_PLANE_NEAR  ];
        m_fp[FR_PLANE_RIGHT ] = -m_fp[FR_PLANE_RIGHT ];
        m_fp[FR_PLANE_LEFT  ] = -m_fp[FR_PLANE_LEFT  ];
        m_fp[FR_PLANE_TOP   ] = -m_fp[FR_PLANE_TOP   ];
        m_fp[FR_PLANE_BOTTOM] = -m_fp[FR_PLANE_BOTTOM];
        m_fp[FR_PLANE_FAR   ] = -m_fp[FR_PLANE_FAR   ];  //clip-plane
    }

    union f32_u
    {
        float floatVal;
        uint32 uintVal;
    };

    for (int i = 0; i < FRUSTUM_PLANES; i++)
    {
        f32_u ux;
        ux.floatVal = m_fp[i].n.x;
        f32_u uy;
        uy.floatVal = m_fp[i].n.y;
        f32_u uz;
        uz.floatVal = m_fp[i].n.z;
        uint32 bitX =   ux.uintVal >> 31;
        uint32 bitY =   uy.uintVal >> 31;
        uint32 bitZ =   uz.uintVal >> 31;
        m_idx1[i] = bitX * 3 + 0;
        m_idx2[i] = (1 - bitX) * 3 + 0;
        m_idy1[i] = bitY * 3 + 1;
        m_idy2[i] = (1 - bitY) * 3 + 1;
        m_idz1[i] = bitZ * 3 + 2;
        m_idz2[i] = (1 - bitZ) * 3 + 2;
    }
    m_OccPosition = GetPosition();
}

// Description
//   Check if a point lies within camera's frustum
//
// Example
//   u8 InOut=camera.IsPointVisible(point);
//
// return values
//   CULL_EXCLUSION = point outside of frustum
//   CULL_INTERSECT = point inside of frustum
inline bool CCamera::IsPointVisible(const Vec3& p) const
{
    if ((m_fp[FR_PLANE_NEAR  ] | p) > 0)
    {
        return CULL_EXCLUSION;
    }
    if ((m_fp[FR_PLANE_RIGHT ] | p) > 0)
    {
        return CULL_EXCLUSION;
    }
    if ((m_fp[FR_PLANE_LEFT  ] | p) > 0)
    {
        return CULL_EXCLUSION;
    }
    if ((m_fp[FR_PLANE_TOP   ] | p) > 0)
    {
        return CULL_EXCLUSION;
    }
    if ((m_fp[FR_PLANE_BOTTOM] | p) > 0)
    {
        return CULL_EXCLUSION;
    }
    if ((m_fp[FR_PLANE_FAR   ] | p) > 0)
    {
        return CULL_EXCLUSION;
    }
    return CULL_OVERLAP;
}

// Description
//   Simple approach to check if an AABB and the camera-frustum overlap. The AABB
//   is assumed to be in world-space. This is a very fast method, just one single
//   dot-product is necessary to check an AABB against a plane. Actually there
//   is no significant speed-different between culling a sphere or an AABB.
//
// Example
//   bool InOut=camera.IsAABBVisible_F(aabb);
//
// return values
//   CULL_EXCLUSION   = AABB outside of frustum (very fast rejection-test)
//   CULL_OVERLAP     = AABB either intersects the borders of the frustum or is totally inside

inline bool CCamera::IsAABBVisible_F(const AABB& aabb) const
{
    const f32* p = &aabb.min.x;
    uint32 x, y, z;
    x = m_idx1[0];
    y = m_idy1[0];
    z = m_idz1[0];
    if ((m_fp[0] | Vec3(p[x], p[y], p[z])) > 0)
    {
        return CULL_EXCLUSION;
    }
    x = m_idx1[1];
    y = m_idy1[1];
    z = m_idz1[1];
    if ((m_fp[1] | Vec3(p[x], p[y], p[z])) > 0)
    {
        return CULL_EXCLUSION;
    }
    x = m_idx1[2];
    y = m_idy1[2];
    z = m_idz1[2];
    if ((m_fp[2] | Vec3(p[x], p[y], p[z])) > 0)
    {
        return CULL_EXCLUSION;
    }
    x = m_idx1[3];
    y = m_idy1[3];
    z = m_idz1[3];
    if ((m_fp[3] | Vec3(p[x], p[y], p[z])) > 0)
    {
        return CULL_EXCLUSION;
    }
    x = m_idx1[4];
    y = m_idy1[4];
    z = m_idz1[4];
    if ((m_fp[4] | Vec3(p[x], p[y], p[z])) > 0)
    {
        return CULL_EXCLUSION;
    }
    x = m_idx1[5];
    y = m_idy1[5];
    z = m_idz1[5];
    if ((m_fp[5] | Vec3(p[x], p[y], p[z])) > 0)
    {
        return CULL_EXCLUSION;
    }
    return CULL_OVERLAP;
}

//------------------------------------------------------------------------------
//---                         ADDITIONAL-TEST                                ---
//------------------------------------------------------------------------------

alignas(64) extern uint32 BoxSides[];

// Description:
//   A box can easily straddle one of the view-frustum planes far
//   outside the view-frustum and in this case the previous test would
//   return CULL_OVERLAP.
//
// Note:  With this check, we make sure the AABB is really not visble
NO_INLINE_WEAK bool CCamera::AdditionalCheck(const AABB& aabb) const
{
    Vec3 m = (aabb.min + aabb.max) * 0.5;
    uint32 o = 1; //will be reset to 0 if center is outside
    o &= isneg(m_fp[0] | m);
    o &= isneg(m_fp[2] | m);
    o &= isneg(m_fp[3] | m);
    o &= isneg(m_fp[4] | m);
    o &= isneg(m_fp[5] | m);
    o &= isneg(m_fp[1] | m);
    if (o)
    {
        return CULL_OVERLAP;    //if obb-center is in view-frustum, then stop further calculation
    }
    Vec3 vmin(aabb.min - GetPosition());  //AABB in camera-space
    Vec3 vmax(aabb.max - GetPosition());  //AABB in camera-space

    uint32 frontx8 = 0; // make the flags using the fact that the upper bit in f32 is its sign

    union f64_u
    {
        f64 floatVal;
        int64 intVal;
    };
    f64_u uminx, uminy, uminz, umaxx, umaxy, umaxz;
    uminx.floatVal = vmin.x;
    uminy.floatVal = vmin.y;
    uminz.floatVal = vmin.z;
    umaxx.floatVal = vmax.x;
    umaxy.floatVal = vmax.y;
    umaxz.floatVal = vmax.z;

    frontx8 |= (-uminx.intVal >> 0x3f) & 0x008; //if (AABB.min.x>0.0f)  frontx8|=0x008;
    frontx8 |= (umaxx.intVal >> 0x3f) & 0x010; //if (AABB.max.x<0.0f)  frontx8|=0x010;
    frontx8 |= (-uminy.intVal >> 0x3f) & 0x020; //if (AABB.min.y>0.0f)  frontx8|=0x020;
    frontx8 |= (umaxy.intVal >> 0x3f) & 0x040; //if (AABB.max.y<0.0f)  frontx8|=0x040;
    frontx8 |= (-uminz.intVal >> 0x3f) & 0x080; //if (AABB.min.z>0.0f)  frontx8|=0x080;
    frontx8 |= (umaxz.intVal >> 0x3f) & 0x100; //if (AABB.max.z<0.0f)  frontx8|=0x100;

    //check if camera is inside the aabb
    if (frontx8 == 0)
    {
        return CULL_OVERLAP;             //AABB is patially visible
    }
    Vec3 v[8] = {
        Vec3(vmin.x, vmin.y, vmin.z),
        Vec3(vmax.x, vmin.y, vmin.z),
        Vec3(vmin.x, vmax.y, vmin.z),
        Vec3(vmax.x, vmax.y, vmin.z),
        Vec3(vmin.x, vmin.y, vmax.z),
        Vec3(vmax.x, vmin.y, vmax.z),
        Vec3(vmin.x, vmax.y, vmax.z),
        Vec3(vmax.x, vmax.y, vmax.z)
    };

    //---------------------------------------------------------------------
    //---            find the silhouette-vertices of the AABB            ---
    //---------------------------------------------------------------------
    uint32 p0   =   BoxSides[frontx8 + 0];
    uint32 p1   =   BoxSides[frontx8 + 1];
    uint32 p2   =   BoxSides[frontx8 + 2];
    uint32 p3   =   BoxSides[frontx8 + 3];
    uint32 p4   =   BoxSides[frontx8 + 4];
    uint32 p5   =   BoxSides[frontx8 + 5];
    uint32 sideamount = BoxSides[frontx8 + 7];

    if (sideamount == 4)
    {
        //--------------------------------------------------------------------------
        //---     we take the 4 vertices of projection-plane in cam-space,       ---
        //-----  and clip them against the 4 side-frustum-planes of the AABB        -
        //--------------------------------------------------------------------------
        Vec3 s0 = v[p0] % v[p1];
        if ((s0 | m_cltp) > 0 && (s0 | m_crtp) > 0 && (s0 | m_crbp) > 0 && (s0 | m_clbp) > 0)
        {
            return CULL_EXCLUSION;
        }
        Vec3 s1 = v[p1] % v[p2];
        if ((s1 | m_cltp) > 0 && (s1 | m_crtp) > 0 && (s1 | m_crbp) > 0 && (s1 | m_clbp) > 0)
        {
            return CULL_EXCLUSION;
        }
        Vec3 s2 = v[p2] % v[p3];
        if ((s2 | m_cltp) > 0 && (s2 | m_crtp) > 0 && (s2 | m_crbp) > 0 && (s2 | m_clbp) > 0)
        {
            return CULL_EXCLUSION;
        }
        Vec3 s3 = v[p3] % v[p0];
        if ((s3 | m_cltp) > 0 && (s3 | m_crtp) > 0 && (s3 | m_crbp) > 0 && (s3 | m_clbp) > 0)
        {
            return CULL_EXCLUSION;
        }
    }

    if (sideamount == 6)
    {
        //--------------------------------------------------------------------------
        //---     we take the 4 vertices of projection-plane in cam-space,       ---
        //---    and clip them against the 6 side-frustum-planes of the AABB      ---
        //--------------------------------------------------------------------------
        Vec3 s0 = v[p0] % v[p1];
        if ((s0 | m_cltp) > 0 && (s0 | m_crtp) > 0 && (s0 | m_crbp) > 0 && (s0 | m_clbp) > 0)
        {
            return CULL_EXCLUSION;
        }
        Vec3 s1 = v[p1] % v[p2];
        if ((s1 | m_cltp) > 0 && (s1 | m_crtp) > 0 && (s1 | m_crbp) > 0 && (s1 | m_clbp) > 0)
        {
            return CULL_EXCLUSION;
        }
        Vec3 s2 = v[p2] % v[p3];
        if ((s2 | m_cltp) > 0 && (s2 | m_crtp) > 0 && (s2 | m_crbp) > 0 && (s2 | m_clbp) > 0)
        {
            return CULL_EXCLUSION;
        }
        Vec3 s3 = v[p3] % v[p4];
        if ((s3 | m_cltp) > 0 && (s3 | m_crtp) > 0 && (s3 | m_crbp) > 0 && (s3 | m_clbp) > 0)
        {
            return CULL_EXCLUSION;
        }
        Vec3 s4 = v[p4] % v[p5];
        if ((s4 | m_cltp) > 0 && (s4 | m_crtp) > 0 && (s4 | m_crbp) > 0 && (s4 | m_clbp) > 0)
        {
            return CULL_EXCLUSION;
        }
        Vec3 s5 = v[p5] % v[p0];
        if ((s5 | m_cltp) > 0 && (s5 | m_crtp) > 0 && (s5 | m_crbp) > 0 && (s5 | m_clbp) > 0)
        {
            return CULL_EXCLUSION;
        }
    }
    return CULL_OVERLAP; //AABB is patially visible
}

//------------------------------------------------------------------------------
//---                         ADDITIONAL-TEST                                ---
//------------------------------------------------------------------------------

// Description:
//   A box can easily straddle one of the view-frustum planes far
//   outside the view-frustum and in this case the previous test would
//   return CULL_OVERLAP.
// Note:
//   With this check, we make sure the OBB is really not visible
NO_INLINE_WEAK bool CCamera::AdditionalCheck(const Vec3& wpos, const OBB& obb, f32 uscale) const
{
    Vec3 CamInOBBSpace  = wpos - GetPosition();
    Vec3 iCamPos = -CamInOBBSpace * obb.m33;
    uint32 front8 = 0;
    AABB aabb = AABB((obb.c - obb.h) * uscale, (obb.c + obb.h) * uscale);
    if (iCamPos.x < aabb.min.x)
    {
        front8 |= 0x008;
    }
    if (iCamPos.x > aabb.max.x)
    {
        front8 |= 0x010;
    }
    if (iCamPos.y < aabb.min.y)
    {
        front8 |= 0x020;
    }
    if (iCamPos.y > aabb.max.y)
    {
        front8 |= 0x040;
    }
    if (iCamPos.z < aabb.min.z)
    {
        front8 |= 0x080;
    }
    if (iCamPos.z > aabb.max.z)
    {
        front8 |= 0x100;
    }

    if (front8 == 0)
    {
        return CULL_OVERLAP;
    }

    //the transformed OBB-vertices in cam-space
    Vec3 v[8] = {
        obb.m33 * Vec3(aabb.min.x, aabb.min.y, aabb.min.z) + CamInOBBSpace,
        obb.m33 * Vec3(aabb.max.x, aabb.min.y, aabb.min.z) + CamInOBBSpace,
        obb.m33 * Vec3(aabb.min.x, aabb.max.y, aabb.min.z) + CamInOBBSpace,
        obb.m33 * Vec3(aabb.max.x, aabb.max.y, aabb.min.z) + CamInOBBSpace,
        obb.m33 * Vec3(aabb.min.x, aabb.min.y, aabb.max.z) + CamInOBBSpace,
        obb.m33 * Vec3(aabb.max.x, aabb.min.y, aabb.max.z) + CamInOBBSpace,
        obb.m33 * Vec3(aabb.min.x, aabb.max.y, aabb.max.z) + CamInOBBSpace,
        obb.m33 * Vec3(aabb.max.x, aabb.max.y, aabb.max.z) + CamInOBBSpace
    };

    //---------------------------------------------------------------------
    //---            find the silhouette-vertices of the OBB            ---
    //---------------------------------------------------------------------
    uint32 p0   =   BoxSides[front8 + 0];
    uint32 p1   =   BoxSides[front8 + 1];
    uint32 p2   =   BoxSides[front8 + 2];
    uint32 p3   =   BoxSides[front8 + 3];
    uint32 p4   =   BoxSides[front8 + 4];
    uint32 p5   =   BoxSides[front8 + 5];
    uint32 sideamount = BoxSides[front8 + 7];

    if (sideamount == 4)
    {
        //--------------------------------------------------------------------------
        //---     we take the 4 vertices of projection-plane in cam-space,       ---
        //-----  and clip them against the 4 side-frustum-planes of the OBB        -
        //--------------------------------------------------------------------------
        Vec3 s0 = v[p0] % v[p1];
        if (((s0 | m_cltp) >= 0) && ((s0 | m_crtp) >= 0) && ((s0 | m_crbp) >= 0) && ((s0 | m_clbp) >= 0))
        {
            return CULL_EXCLUSION;
        }
        Vec3 s1 = v[p1] % v[p2];
        if (((s1 | m_cltp) >= 0) && ((s1 | m_crtp) >= 0) && ((s1 | m_crbp) >= 0) && ((s1 | m_clbp) >= 0))
        {
            return CULL_EXCLUSION;
        }
        Vec3 s2 = v[p2] % v[p3];
        if (((s2 | m_cltp) >= 0) && ((s2 | m_crtp) >= 0) && ((s2 | m_crbp) >= 0) && ((s2 | m_clbp) >= 0))
        {
            return CULL_EXCLUSION;
        }
        Vec3 s3 = v[p3] % v[p0];
        if (((s3 | m_cltp) >= 0) && ((s3 | m_crtp) >= 0) && ((s3 | m_crbp) >= 0) && ((s3 | m_clbp) >= 0))
        {
            return CULL_EXCLUSION;
        }
    }

    if (sideamount == 6)
    {
        //--------------------------------------------------------------------------
        //---     we take the 4 vertices of projection-plane in cam-space,       ---
        //---    and clip them against the 6 side-frustum-planes of the OBB      ---
        //--------------------------------------------------------------------------
        Vec3 s0 = v[p0] % v[p1];
        if (((s0 | m_cltp) >= 0) && ((s0 | m_crtp) >= 0) && ((s0 | m_crbp) >= 0) && ((s0 | m_clbp) >= 0))
        {
            return CULL_EXCLUSION;
        }
        Vec3 s1 = v[p1] % v[p2];
        if (((s1 | m_cltp) >= 0) && ((s1 | m_crtp) >= 0) && ((s1 | m_crbp) >= 0) && ((s1 | m_clbp) >= 0))
        {
            return CULL_EXCLUSION;
        }
        Vec3 s2 = v[p2] % v[p3];
        if (((s2 | m_cltp) >= 0) && ((s2 | m_crtp) >= 0) && ((s2 | m_crbp) >= 0) && ((s2 | m_clbp) >= 0))
        {
            return CULL_EXCLUSION;
        }
        Vec3 s3 = v[p3] % v[p4];
        if (((s3 | m_cltp) >= 0) && ((s3 | m_crtp) >= 0) && ((s3 | m_crbp) >= 0) && ((s3 | m_clbp) >= 0))
        {
            return CULL_EXCLUSION;
        }
        Vec3 s4 = v[p4] % v[p5];
        if (((s4 | m_cltp) >= 0) && ((s4 | m_crtp) >= 0) && ((s4 | m_crbp) >= 0) && ((s4 | m_clbp) >= 0))
        {
            return CULL_EXCLUSION;
        }
        Vec3 s5 = v[p5] % v[p0];
        if (((s5 | m_cltp) >= 0) && ((s5 | m_crtp) >= 0) && ((s5 | m_crbp) >= 0) && ((s5 | m_clbp) >= 0))
        {
            return CULL_EXCLUSION;
        }
    }
    //now we are 100% sure that the OBB is visible on the screen
    return CULL_OVERLAP;
}

#endif // CRYINCLUDE_CRYCOMMON_CRY_CAMERA_H
