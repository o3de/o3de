/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Random.h"
#include "Vector.h"

namespace MCore
{
    unsigned int LcgRandom::GetRandom()
    {
        m_seed = (m_seed * 0x5DEECE66DLL + 0xBLL) & ((1LL << 48) - 1);
        return static_cast<unsigned int>(m_seed >> 16);
    }

    float LcgRandom::GetRandomFloat()
    {
        unsigned int r = GetRandom();
        r &= 0x007fffff; //sets mantissa to random bits
        r |= 0x3f800000; //result is in [1,2), uniformly distributed
        union
        {
            float f;
            unsigned int i;
        } u;
        u.i = r;
        return u.f - 1.0f;
    }

    // returns a random direction vector
    AZ::Vector3 Random::RandDirVecF()
    {
        float z = RandF(-1.0f, 1.0f);
        float a = RandF(0.0f, Math::twoPi);

        float r = Math::Sqrt(1.0f - z * z);

        float x = r * Math::Cos(a);
        float y = r * Math::Sin(a);

        return AZ::Vector3(x, y, z);
    }



    // returns a random direction vector, using <dir> as basis vector with a cone of <coneAngle> radians
    AZ::Vector3 Random::RandomDirVec(const AZ::Vector3& dir, float coneAngle)
    {
        // determine r, phi and theta of direction vector
        const float r = SafeLength(dir);

        // make sure the direction vector isn't a zero length vector
        if (r < Math::epsilon)
        {
            return AZ::Vector3(0.0f, 0.0f, 1.0f);
        }

        const float oneOverR        = 1.0f / r;
        const float theta           = Math::ACos(dir.GetZ() * oneOverR);
        const float phi             = atan2(dir.GetY(), dir.GetX());
        const float halfConeAngle   = 0.5f * coneAngle;             // set up random vector within a cone that points upwards
        const float newPhi          = RandF(0, Math::twoPi);
        const float newTheta        = RandF(0, halfConeAngle);
        float sa                    = Math::Sin(newTheta);

        // calculate the resulting random vector
        AZ::Vector3 result(Math::Cos(newPhi) * sa, Math::Sin(newPhi) * sa, Math::Cos(newTheta));

        // rotate the random vector towards the direction vector
        float ca    = Math::Cos(-theta);
        float sb    = Math::Sin(-phi);
        float cb    = Math::Cos(-phi);
        sa          = Math::Sin(-theta);

        AZ::Vector3 oldResult(result);
        result.SetZ(oldResult.GetX() * sa + oldResult.GetZ() * ca);
        oldResult.SetX(oldResult.GetX() * ca - oldResult.GetZ() * sa);
        result.SetX(sb * oldResult.GetY() + cb * oldResult.GetX());
        result.SetY(cb * oldResult.GetY() - sb * oldResult.GetX());

        return result;
    }




    // returns a random direction vector, using <dir> as basis vector with a cone of <coneAngle> radians, sampled on a <gridSizeX x gridSizeY> grid
    // and it will return a random position inside gridcell [xGridPos][yGridPos]
    AZ::Vector3 Random::RandomDirVec(const AZ::Vector3& dir, float coneAngle, uint32 gridSizeX, uint32 gridSizeY, uint32 xGridPos, uint32 yGridPos)
    {
        // determine r, phi and theta of direction vector
        const float r = SafeLength(dir);

        // make sure the direction vector isn't a zero length vector
        if (r < Math::epsilon)
        {
            return AZ::Vector3(0.0f, 0.0f, 1.0f);
        }

        const float oneOverR        = 1.0f / r;
        const float theta           = Math::ACos(dir.GetZ() * oneOverR);
        const float phi             = Math::ATan2(dir.GetY(), dir.GetX());
        const float halfConeAngle   = 0.5f * coneAngle;             // set up random vector within a cone that points upwards

        const float phiGridSize     = Math::twoPi / (float)gridSizeX;
        const float thetaGridSize   = halfConeAngle / (float)gridSizeY;

        const float newPhi          = (xGridPos * phiGridSize)   + RandF(0, phiGridSize);   // a random point in gridcell [xGridPos][yGridPos]
        const float newTheta        = (yGridPos * thetaGridSize) + RandF(0, thetaGridSize);
        float sa                    = Math::Sin(newTheta);

        // calculate the random generated vector
        AZ::Vector3 result(Math::Cos(newPhi) * sa, Math::Sin(newPhi) * sa, Math::Cos(newTheta));

        // rotate the random vector towards the direction vector
        const float ca  = Math::Cos(-theta);
        const float sb  = Math::Sin(-phi);
        const float cb  = Math::Cos(-phi);
        sa              = Math::Sin(-theta);

        AZ::Vector3 oldResult(result);
        result.SetZ(oldResult.GetX() * sa + oldResult.GetZ() * ca);
        oldResult.SetX(oldResult.GetX() * ca - oldResult.GetZ() * sa);
        result.SetX(sb * oldResult.GetY() + cb * oldResult.GetX());
        result.SetY(cb * oldResult.GetY() - sb * oldResult.GetX());

        return result;
    }




    AZ::Vector3 Random::RandomDirVec(const AZ::Vector3& dir, float startPhi, float endPhi, float startTheta, float endTheta, bool midPoint)
    {
        // determine r, phi and theta of direction vector
        const float r = SafeLength(dir);

        // make sure the direction vector isn't a zero length vector
        if (r < Math::epsilon)
        {
            return AZ::Vector3(0.0f, 0.0f, 1.0f);
        }

        const float oneOverR    = 1.0f / r;
        const float theta       = Math::ACos(dir.GetZ() * oneOverR);
        const float phi         = Math::ATan2(dir.GetY(), dir.GetX());
        //float halfConeAngle = 0.5f*coneAngle;                 // set up random vector within a cone that points upwards

        const float newPhi      = midPoint ? (endPhi + startPhi) * 0.5f : RandF(startPhi, endPhi);
        const float newTheta    = midPoint ? (endTheta + startTheta) * 0.5f : RandF(startTheta * 0.5f, endTheta * 0.5f);
        float sa                = Math::Sin(newTheta);

        // calculate the random direction vector
        AZ::Vector3 result(Math::Cos(newPhi) * sa, Math::Sin(newPhi) * sa, Math::Cos(newTheta));

        // rotate the random vector towards the direction vector
        const float ca  = Math::Cos(-theta);
        const float sb  = Math::Sin(-phi);
        const float cb  = Math::Cos(-phi);
        sa              = Math::Sin(-theta);

        AZ::Vector3 oldResult(result);
        result.SetZ(oldResult.GetX() * sa + oldResult.GetZ() * ca);
        oldResult.SetX(oldResult.GetX() * ca - oldResult.GetZ() * sa);
        result.SetX(sb * oldResult.GetY() + cb * oldResult.GetX());
        result.SetY(cb * oldResult.GetY() - sb * oldResult.GetX());

        return result;
    }



    // stratisfied random direction vectors
    AZStd::vector<AZ::Vector3> Random::RandomDirVectorsStratisfied(const AZ::Vector3& dir, float coneAngle, uint32 numVectors)
    {
        AZStd::vector<AZ::Vector3> result;

        uint32 num = (uint32)Math::Sqrt((float)numVectors);
        for (uint32 y = 0; y < num; ++y)
        {
            for (uint32 x = 0; x < num; ++x)
            {
                result.emplace_back(RandomDirVec(dir, coneAngle, num, num, x, y));
            }
        }

        return result;
    }



    AZStd::vector<AZ::Vector3> Random::RandomDirVectorsHammersley(const AZ::Vector3& dir, float coneAngle, uint32 numVectors)
    {
        AZStd::vector<AZ::Vector3> arrayResult(numVectors);

        // generate the uvset
        float* uvSet = new float[numVectors << 1]; // TODO: use memory manager instead
        PlaneHammersley(uvSet, numVectors);

        //  precalculate values
        const float r = SafeLength(dir);
        MCORE_ASSERT(r > 0);
        const float oneOverR        = 1.0f / r;
        const float theta           = Math::ACos(dir.GetZ() * oneOverR);
        const float phi             = Math::ATan2(dir.GetY(), dir.GetX());
        const float halfConeAngle   = 0.5f * coneAngle;             // set up random vector within a cone that points upwards
        const float ca              = Math::Cos(-theta);
        const float sb              = Math::Sin(-phi);
        const float cb              = Math::Cos(-phi);
        const float sa2             = Math::Sin(-theta);

        uint32 index = 0;
        for (uint32 i = 0; i < numVectors; ++i)
        {
            const float newPhi      = uvSet[index++] * Math::twoPi; // 0..2pi
            const float newTheta    = uvSet[index++] * halfConeAngle;// 0..halfConeAngle
            const float sa          = Math::Sin(newTheta);

            // calculate the random vector
            AZ::Vector3 result(Math::Cos(newPhi) * sa, Math::Sin(newPhi) * sa, Math::Cos(newTheta));

            // rotate the random vector towards the direction vector
            AZ::Vector3 oldResult(result);
            result.SetZ(oldResult.GetX() * sa2 + oldResult.GetZ() * ca);
            oldResult.SetX(oldResult.GetX() * ca - oldResult.GetZ() * sa2);
            result.SetX(sb * oldResult.GetY() + cb * oldResult.GetX());
            result.SetY(cb * oldResult.GetY() - sb * oldResult.GetX());

            arrayResult[i] = result;
        }

        // get rid of allocated data
        delete[] uvSet; // TODO: use memory manager?

        return arrayResult;
    }




    AZStd::vector<AZ::Vector3> Random::RandomDirVectorsHammersley2(const AZ::Vector3& dir, float coneAngle, uint32 numVectors, uint32 base)
    {
        AZStd::vector<AZ::Vector3> arrayResult(numVectors);

        // generate the uvset
        float* uvSet = new float[numVectors << 1];
        PlaneHammersley2(uvSet, numVectors, base);

        //  precalculate values
        const float r = SafeLength(dir);
        MCORE_ASSERT(r > 0);
        const float oneOverR        = 1.0f / r;
        const float theta           = Math::ACos(dir.GetZ() * oneOverR);
        const float phi             = Math::ATan2(dir.GetY(), dir.GetX());
        const float halfConeAngle   = 0.5f * coneAngle;             // set up random vector within a cone that points upwards
        const float ca              = Math::Cos(-theta);
        const float sb              = Math::Sin(-phi);
        const float cb              = Math::Cos(-phi);
        const float sa2             = Math::Sin(-theta);

        uint32 index = 0;
        for (uint32 i = 0; i < numVectors; ++i)
        {
            const float newPhi      = uvSet[index++] * Math::twoPi; // 0..2pi
            const float newTheta    = uvSet[index++] * halfConeAngle;// 0..halfConeAngle
            const float sa          = Math::Sin(newTheta);

            // calculate the random vector
            AZ::Vector3 result(Math::Cos(newPhi) * sa, Math::Sin(newPhi) * sa, Math::Cos(newTheta));

            // rotate the random vector towards the direction vector
            AZ::Vector3 oldResult(result);
            result.SetZ(oldResult.GetX() * sa2 + oldResult.GetZ() * ca);
            oldResult.SetX(oldResult.GetX() * ca - oldResult.GetZ() * sa2);
            result.SetX(sb * oldResult.GetY() + cb * oldResult.GetX());
            result.SetY(cb * oldResult.GetY() - sb * oldResult.GetX());

            arrayResult[i] = result;
        }

        // get rid of allocated data
        delete[] uvSet; // TODO: use memory manager?

        return arrayResult;
    }




    AZStd::vector<AZ::Vector3> Random::RandomDirVectorsHalton(const AZ::Vector3& dir, float coneAngle, size_t numVectors, uint32 p2)
    {
        AZStd::vector<AZ::Vector3> arrayResult(numVectors);

        // generate the uvset
        float* uvSet = new float[numVectors << 1];
        PlaneHalton(uvSet, static_cast<uint32>(numVectors), p2);

        //  precalculate values
        const float r = SafeLength(dir);
        MCORE_ASSERT(r > 0);
        const float oneOverR        = 1.0f / r;
        const float theta           = Math::ACos(dir.GetZ() * oneOverR);
        const float phi             = Math::ATan2(dir.GetY(), dir.GetX());
        const float halfConeAngle   = 0.5f * coneAngle;             // set up random vector within a cone that points upwards
        const float ca              = Math::Cos(-theta);
        const float sb              = Math::Sin(-phi);
        const float cb              = Math::Cos(-phi);
        const float sa2             = Math::Sin(-theta);

        uint32 index = 0;
        for (uint32 i = 0; i < numVectors; ++i)
        {
            const float newPhi      = uvSet[index++] * Math::twoPi; // 0..2pi
            const float newTheta    = uvSet[index++] * halfConeAngle;// 0..halfConeAngle
            const float sa          = Math::Sin(newTheta);

            // calculate the random vector
            AZ::Vector3 result(Math::Cos(newPhi) * sa, Math::Sin(newPhi) * sa, Math::Cos(newTheta));

            // rotate the random vector towards the direction vector
            AZ::Vector3 oldResult(result);
            result.SetZ(oldResult.GetX() * sa2 + oldResult.GetZ() * ca);
            oldResult.SetX(oldResult.GetX() * ca - oldResult.GetZ() * sa2);
            result.SetX(sb * oldResult.GetY() + cb * oldResult.GetX());
            result.SetY(cb * oldResult.GetY() - sb * oldResult.GetX());

            arrayResult[i] = result;
        }

        // get rid of allocated data
        delete[] uvSet; // TODO: use memory manager?

        return arrayResult;
    }



    AZStd::vector<AZ::Vector3> Random::RandomDirVectorsHalton2(const AZ::Vector3& dir, float coneAngle, size_t numVectors, uint32 baseA, uint32 baseB)
    {
        AZStd::vector<AZ::Vector3> arrayResult(numVectors);

        // generate the uvset
        float* uvSet = new float[numVectors << 1];
        PlaneHalton2(uvSet, static_cast<uint32>(numVectors), baseA, baseB);

        //  precalculate values
        const float r = SafeLength(dir);
        MCORE_ASSERT(r > 0);
        const float oneOverR        = 1.0f / r;
        const float theta           = Math::ACos(dir.GetZ() * oneOverR);
        const float phi             = Math::ATan2(dir.GetY(), dir.GetX());
        const float halfConeAngle   = 0.5f * coneAngle;             // set up random vector within a cone that points upwards
        const float ca              = Math::Cos(-theta);
        const float sb              = Math::Sin(-phi);
        const float cb              = Math::Cos(-phi);
        const float sa2             = Math::Sin(-theta);

        uint32 index = 0;
        for (uint32 i = 0; i < numVectors; ++i)
        {
            const float newPhi      = uvSet[index++] * Math::twoPi; // 0..2pi
            const float newTheta    = uvSet[index++] * halfConeAngle;// 0..halfConeAngle
            const float sa          = Math::Sin(newTheta);

            // calculate the random vector
            AZ::Vector3 result(Math::Cos(newPhi) * sa, Math::Sin(newPhi) * sa, Math::Cos(newTheta));

            // rotate the random vector towards the direction vector
            AZ::Vector3 oldResult(result);
            result.SetZ(oldResult.GetX() * sa2 + oldResult.GetZ() * ca);
            oldResult.SetX(oldResult.GetX() * ca - oldResult.GetZ() * sa2);
            result.SetX(sb * oldResult.GetY() + cb * oldResult.GetX());
            result.SetY(cb * oldResult.GetY() - sb * oldResult.GetX());

            arrayResult[i] = result;
        }

        // get rid of allocated data
        delete[] uvSet; // TODO: use memory manager?

        return arrayResult;
    }




    AZ::Vector3 Random::UVToVector(const AZ::Vector3& dir, float coneAngle, float u, float v)
    {
        // determine r, phi and theta of direction vector
        const float r = SafeLength(dir);
        if (r < MCore::Math::epsilon)
        {
            return AZ::Vector3(0.0f, 0.0f, 1.0f);
        }

        const float oneOverR        = 1.0f / r;
        const float theta           = Math::ACos(dir.GetZ() * oneOverR);
        const float phi             = Math::ATan2(dir.GetY(), dir.GetX());
        const float halfConeAngle   = 0.5f * coneAngle;             // set up random vector within a cone that points upwards
        const float newPhi          = u * Math::twoPi;
        const float newTheta        = v * halfConeAngle;
        float sa                    = Math::Sin(newTheta);

        AZ::Vector3 result(Math::Cos(newPhi) * sa, Math::Sin(newPhi) * sa, Math::Cos(newTheta));

        // rotate the random vector towards the direction vector
        const float ca  = Math::Cos(-theta);
        const float sb  = Math::Sin(-phi);
        const float cb  = Math::Cos(-phi);
        sa              = Math::Sin(-theta);

        AZ::Vector3 oldResult(result);
        result.SetZ(oldResult.GetX() * sa + oldResult.GetZ() * ca);
        oldResult.SetX(oldResult.GetX() * ca - oldResult.GetZ() * sa);
        result.SetX(sb * oldResult.GetY() + cb * oldResult.GetX());
        result.SetY(cb * oldResult.GetY() - sb * oldResult.GetX());

        return result;
    }







    /*
    // Hammersley point sets. Deterministic and look random.
    // Base p = 2, which is especially fast for computation.
    void GenerateSphereHammersley(float *result, int32 n)
    {
      float p, t, st, phi, phirad;
      int32 k, kk, pos;

      for (k=0, pos=0 ; k<n ; k++)
      {
        t = 0;
        for (p=0.5, kk=k ; kk ; p*=0.5, kk>>=1)
          if (kk & 1)                           // kk mod 2 == 1
        t += p;
        t = 2.0 * t  - 1.0;                     // map from [0,1] to [-1,1]

        phi = (k + 0.5) / n;                    // a slight shift
        phirad =  phi * 2.0 * MATH_PI;             // map to [0, 2 pi)

        st = Sqrt(1.0-t*t);
        result[pos++] = st * Cos(phirad);
        result[pos++] = st * Sin(phirad);
        result[pos++] = t;
      }
    }



    // Hammersley point sets for any base p1. Deterministic and look random.
    void SphereHammersley2(float *result, int32 n, int32 p1)
    {
      float a, p, ip, t, st, phi, phirad;
      int32 k, kk, pos;

      for (k=0, pos=0 ; k<n ; k++)
      {
        t = 0;
        ip = 1.0/p1;                           // recipical of p1
        for (p=ip, kk=k ; kk ; p*=ip, kk/=p1)  // kk = (int32)(kk/p1)
          if ((a = kk % p1))
        t += a * p;
        t = 2.0 * t  - 1.0;                    // map from [0,1] to [-1,1]

        phi = (k + 0.5) / n;
        phirad =  phi * 2.0 * MATH_PI;            // map to [0, 2 pi)

        st = Sqrt(1.0-t*t);
        result[pos++] = st * Cos(phirad);
        result[pos++] = st * Sin(phirad);
        result[pos++] = t;
      }
    }



    // Halton point set generation
    // two p-adic Van der Corport sequences
    // Useful for incremental approach.
    // p1 = 2, p2 = 3(default)
    void SphereHalton(float *result, int32 n, int32 p2)
    {
      float p, t, st, phi, phirad, ip;
      int32 k, kk, pos, a;

      for (k=0, pos=0 ; k<n ; k++)
      {
        t = 0;
        for (p=0.5, kk=k ; kk ; p*=0.5, kk>>=1)
          if (kk & 1)                          // kk mod 2 == 1
        t += p;
        t = 2.0 * t - 1.0;                     // map from [0,1] to [-1,1]
        st = Sqrt(1.0-t*t);

        phi = 0;
        ip = 1.0/p2;                           // recipical of p2
        for (p=ip, kk=k ; kk ; p*=ip, kk/=p2)  // kk = (int32)(kk/p2)
          if ((a = kk % p2))
        phi += a * p;
        phirad =  phi * 4.0 * MATH_PI;            // map from [0,0.5] to [0, 2 pi)

        result[pos++] = st * Cos(phirad);
        result[pos++] = st * Sin(phirad);
        result[pos++] = t;
      }
    }



    // Halton point set generation
    // two p-adic Van der Corport sequences
    // Useful for incremental approach.
    // p1 = 2(default), p2 = 3(default)
    void SphereHalton2(float *result, int32 n, int32 p1, int32 p2)
    {
      float p, t, st, phi, phirad, ip;
      int32 k, kk, pos, a;

      for (k=0, pos=0 ; k<n ; k++)
      {
        t = 0;
        ip = 1.0/p1;                           // recipical of p1
        for (p=ip, kk=k ; kk ; p*=ip, kk/=p1)  // kk = (int32)(kk/p1)
          if ((a = kk % p1))
        t += a * p;
        t = 2.0 * t  - 1.0;                    // map from [0,1] to [-1,1]
        st = Sqrt(1.0-t*t);

        phi = 0;
        ip = 1.0/p2;                           // recipical of p2
        for (p=ip, kk=k ; kk ; p*=ip, kk/=p2)  // kk = (int32)(kk/p2)
          if ((a = kk % p2))
        phi += a * p;
        phirad =  phi * 4.0 * MATH_PI;            // map from [0,0.5] to [0, 2 pi)

        result[pos++] = st * Cos(phirad);
        result[pos++] = st * Sin(phirad);
        result[pos++] = t;
      }
    }
    */

    // Hammersley point sets. Deterministic and look random.
    // Base p = 2, which is especially fast for computation.
    void Random::PlaneHammersley(float* result, uint32 num)
    {
        float p, u, v;
        uint32 k, kk, pos;

        for (k = 0, pos = 0; k < num; k++)
        {
            u = 0;
            for (p = 0.5f, kk = k; kk; p *= 0.5f, kk >>= 1)
            {
                if (kk & 1)                       // kk mod 2 == 1
                {
                    u += p;
                }
            }

            v = (k + 0.5f) / num;

            result[pos++] = u;
            result[pos++] = v;
        }
    }



    // Hammersley point sets for any base p1. Deterministic and look random.
    void Random::PlaneHammersley2(float* result, uint32 num, uint32 base)
    {
        float p, ip, u, v;
        uint32 k, kk, pos, res;

        for (k = 0, pos = 0; k < num; k++)
        {
            u  = 0;
            ip = 1.0f / base;                   // recipical of base

            for (p = ip, kk = k; kk; p *= ip, kk /= base) // kk = (int32)(kk/base)
            {
                res = kk % base;
                if (res)
                {
                    u += (float)res * p;
                }
            }

            v = (k + 0.5f) / num;

            result[pos++] = u;
            result[pos++] = v;
        }
    }


    // Halton point set generation
    // two p-adic Van der Corport sequences
    // Useful for incremental approach.
    // p1 = 2, p2 = 3(default)
    void Random::PlaneHalton(float* result, uint32 num, uint32 p2)
    {
        float p, u, v, ip;
        uint32 k, kk, pos, a;

        for (k = 0, pos = 0; k < num; k++)
        {
            u = 0;
            for (p = 0.5f, kk = k; kk; p *= 0.5f, kk >>= 1)
            {
                if (kk & 1)                      // kk mod 2 == 1
                {
                    u += p;
                }
            }

            v = 0;
            ip = 1.0f / p2;                       // recipical of p2
            for (p = ip, kk = k; kk; p *= ip, kk /= p2) // kk = (int32)(kk/p2)
            {
                a = kk % p2;
                if (a)
                {
                    v += a * p;
                }
            }

            result[pos++] = u;
            result[pos++] = v;
        }
    }


    // Halton point set generation
    // two p-adic Van der Corport sequences
    // Useful for incremental approach.
    // p1 = 2(default), p2 = 3(default)
    void Random::PlaneHalton2(float* result, uint32 num, uint32 baseA, uint32 baseB)
    {
        float p, u, v, ip;
        uint32 k, kk, pos, a;

        for (k = 0, pos = 0; k < num; k++)
        {
            u = 0;
            ip = 1.0f / baseA;                     // recipical of baseA

            for (p = ip, kk = k; kk; p *= ip, kk /= baseA) // kk = (int32)(kk/baseA)
            {
                a = kk % baseA;
                if (a)
                {
                    u += a * p;
                }
            }

            v = 0;
            ip = 1.0f / baseB;                     // recipical of baseB
            for (p = ip, kk = k; kk; p *= ip, kk /= baseB) // kk = (int32)(kk/baseB)
            {
                a = kk % baseB;
                if (a)
                {
                    v += a * p;
                }
            }

            result[pos++] = u;
            result[pos++] = v;
        }
    }





    //-----------------------------------------------

    // default constructor
    HaltonSequence::HaltonSequence()
    {
        // init members
        mDimensions = 0;
        mNextDim    = 0;
        mMemory     = 0;
        mN          = 0;
        mN0         = 0;
        mX          = nullptr;
        mRadical    = nullptr;
        mBase       = nullptr;
        mOwnBase    = false;
    }


    // extended constructor
    HaltonSequence::HaltonSequence(uint32 dimensions, uint32 offset, uint32* primes)
    {
        // init members
        mDimensions = 0;
        mNextDim    = 0;
        mMemory     = 0;
        mN          = 0;
        mN0         = 0;
        mX          = nullptr;
        mRadical    = nullptr;
        mBase       = nullptr;
        mOwnBase    = false;

        // initialize
        Init(dimensions, offset, primes);
    }


    // initialize the sequence
    void HaltonSequence::Init(uint32 dimensions, uint32 offset, uint32* primes)
    {
        MCORE_ASSERT(dimensions > 0);

        mNextDim    = 0;
        mDimensions = dimensions;
        mX          = (double*)MCore::Allocate(dimensions * sizeof(double), MCORE_MEMCATEGORY_HALTONSEQ);
        mMemory     = sizeof(HaltonSequence) + dimensions * sizeof(double);

        mN          = offset;
        mN0         = offset;
        mRadical    = (double*)MCore::Allocate(dimensions * sizeof(double), MCORE_MEMCATEGORY_HALTONSEQ);

        mOwnBase    = (!primes);

        if (mOwnBase)
        {
            mBase = FirstPrimes((uint32)dimensions);
        }
        else
        {
            mBase = primes;
        }

        for (uint32 j = 0; j < dimensions; ++j)
        {
            mRadical[j] = 1.0 / (double)mBase[j];
            mX[j] = 0.0;
        }

        SetInstance(mN0);
    }


    // destructor
    HaltonSequence::~HaltonSequence()
    {
        Release();
    }


    void HaltonSequence::Release()
    {
        if (mOwnBase && mBase)
        {
            MCore::Free(mBase);
        }
        mBase = nullptr;

        if (mRadical)
        {
            MCore::Free(mRadical);
        }
        mRadical = nullptr;

        if (mX)
        {
            MCore::Free(mX);
        }
        mX = nullptr;
    }


    // operator for the next value
    void HaltonSequence::operator++()
    {
        const double one = 1.0 - 1e-10;
        double h, hh, remainder;

        mN++;

        if (mN & 8191)
        {
            for (uint32 j = 0; j < mDimensions; ++j)
            {
                remainder = one - mX[j];

                if (remainder < 0.0)
                {
                    mX[j] = 0.0;
                }
                else
                {
                    if (mRadical[j] < remainder)
                    {
                        mX[j] += mRadical[j];
                    }
                    else
                    {
                        h = mRadical[j];

                        do
                        {
                            hh = h;
                            h *= mRadical[j];
                        }
                        while (h >= remainder);

                        mX[j] += hh + h - 1.0;
                    }
                }
            }
        }
        else
        {
            if (mN >= 1073741824) // 2^30
            {
                SetInstance(0);
            }
            else
            {
                SetInstance(mN);
            }
        }
    }



    // set the instance
    void HaltonSequence::SetInstance(uint32 instance)
    {
        uint32  im;
        uint32  b;
        double  fac;

        mN = instance;
        for (uint32 j = 0; j < mDimensions; ++j)
        {
            mX[j]   = 0.0;
            fac     = mRadical[j];
            b       = mBase[j];

            for (im = mN; im > 0; im /= b, fac *= mRadical[j])
            {
                mX[j] += fac * (double)(im % b);
            }
        }
    }



    // get the n first number of primes
    uint32* HaltonSequence::FirstPrimes(uint32 n) const
    {
        uint32* prime, i, j, p, b;

        if (n == 0)
        {
            return nullptr;
        }

        prime = (uint32*)MCore::Allocate(n * sizeof(uint32), MCORE_MEMCATEGORY_HALTONSEQ);
        MCORE_ASSERT(prime);

        prime[0] = 2;

        for (p = 3, i = 1; i < n; p += 2)
        {
            prime[i] = p;

            j = 1;
            b = (prime[j] <= p / prime[j]);
            while (b && (p % prime[j]))
            {
                b = (prime[j] <= p / prime[j]);
                j++;
            }

            if (b == 0)
            {
                i++;
            }
        }

        return prime;
    }
}   // namespace MCore
