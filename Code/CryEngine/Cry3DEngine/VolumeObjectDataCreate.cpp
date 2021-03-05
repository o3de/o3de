// Modifications copyright Amazon.com, Inc. or its affiliates.

/*
Perlin Noise
https://mrl.nyu.edu/~perlin/doc/oscar.html

Copyright Ken Perlin

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
*/

#include "Cry3DEngine_precompiled.h"

#include "VolumeObjectDataCreate.h"

#include <vector>
#include <math.h>
#include "Cry_LegacyPhysUtils.h"

namespace
{
    //////////////////////////////////////////////////////////////////////////
    // Coherent noise function over 1, 2 or 3 dimensions
    // (copyright Ken Perlin)

    #define B 0x100
    #define BM 0xff
    #define N 0x1000
    #define NP 12   /* 2^N */
    #define NM 0xfff

    #define s_curve(t) (t * t * (3. - 2. * t))
    #define lerp(t, a, b) (a + t * (b - a))
    #define setup(i, b0, b1, r0, r1) \
    t = vec[i] + N;                  \
    b0 = ((int)t) & BM;              \
    b1 = (b0 + 1) & BM;              \
    r0 = t - (int)t;                 \
    r1 = r0 - 1.;
    #define at2(rx, ry) (rx * q[0] + ry * q[1])
    #define at3(rx, ry, rz) (rx * q[0] + ry * q[1] + rz * q[2])

    void init(void);
    //double noise1(double);
    //double noise2(double *);
    double noise3(double*);
    //void normalize2(double *);
    void normalize3(double*);

    //double PerlinNoise1D(double,double,double,int);
    //double PerlinNoise2D(double,double,double,double,int);
    float PerlinNoise3D(double, double, double, double, double, int);

    static int pp[B + B + 2];
    //static double g1[B + B + 2];
    //static double g2[B + B + 2][2];
    static double g3[B + B + 2][3];
    static int start = 1;

    //double noise1(double arg)
    //{
    //  int bx0, bx1;
    //  double rx0, rx1, sx, t, u, v, vec[1];

    //  vec[0] = arg;
    //  if (start) {
    //      start = 0;
    //      init();
    //  }

    //  setup(0,bx0,bx1,rx0,rx1);

    //  sx = s_curve(rx0);
    //  u = rx0 * g1[ p[ bx0 ] ];
    //  v = rx1 * g1[ p[ bx1 ] ];

    //  return(lerp(sx, u, v));
    //}

    //double noise2(double vec[2])
    //{
    //  int bx0, bx1, by0, by1, b00, b10, b01, b11;
    //  double rx0, rx1, ry0, ry1, *q, sx, sy, a, b, t, u, v;
    //  int i, j;

    //  if (start) {
    //      start = 0;
    //      init();
    //  }

    //  setup(0, bx0,bx1, rx0,rx1);
    //  setup(1, by0,by1, ry0,ry1);

    //  i = p[ bx0 ];
    //  j = p[ bx1 ];

    //  b00 = p[ i + by0 ];
    //  b10 = p[ j + by0 ];
    //  b01 = p[ i + by1 ];
    //  b11 = p[ j + by1 ];

    //  sx = s_curve(rx0);
    //  sy = s_curve(ry0);

    //  q = g2[ b00 ] ; u = at2(rx0,ry0);
    //  q = g2[ b10 ] ; v = at2(rx1,ry0);
    //  a = lerp(sx, u, v);

    //  q = g2[ b01 ] ; u = at2(rx0,ry1);
    //  q = g2[ b11 ] ; v = at2(rx1,ry1);
    //  b = lerp(sx, u, v);

    //  return lerp(sy, a, b);
    //}

    double noise3(double vec[3])
    {
        int bx0, bx1, by0, by1, bz0, bz1, b00, b10, b01, b11;
        double rx0, rx1, ry0, ry1, rz0, rz1, * q, sy, sz, a, b, c, d, t, u, v;
        int i, j;

        if (start)
        {
            start = 0;
            init();
        }

        setup(0, bx0, bx1, rx0, rx1);
        setup(1, by0, by1, ry0, ry1);
        setup(2, bz0, bz1, rz0, rz1);

        i = pp[ bx0 ];
        j = pp[ bx1 ];

        b00 = pp[ i + by0 ];
        b10 = pp[ j + by0 ];
        b01 = pp[ i + by1 ];
        b11 = pp[ j + by1 ];

        t  = s_curve(rx0);
        sy = s_curve(ry0);
        sz = s_curve(rz0);

        q = g3[ b00 + bz0 ];
        u = at3(rx0, ry0, rz0);
        q = g3[ b10 + bz0 ];
        v = at3(rx1, ry0, rz0);
        a = lerp(t, u, v);

        q = g3[ b01 + bz0 ];
        u = at3(rx0, ry1, rz0);
        q = g3[ b11 + bz0 ];
        v = at3(rx1, ry1, rz0);
        b = lerp(t, u, v);

        c = lerp(sy, a, b);

        q = g3[ b00 + bz1 ];
        u = at3(rx0, ry0, rz1);
        q = g3[ b10 + bz1 ];
        v = at3(rx1, ry0, rz1);
        a = lerp(t, u, v);

        q = g3[ b01 + bz1 ];
        u = at3(rx0, ry1, rz1);
        q = g3[ b11 + bz1 ];
        v = at3(rx1, ry1, rz1);
        b = lerp(t, u, v);

        d = lerp(sy, a, b);

        return lerp(sz, c, d);
    }

    //void normalize2(double v[2])
    //{
    //  double s;

    //  s = sqrt(v[0] * v[0] + v[1] * v[1]);
    //  v[0] = v[0] / s;
    //  v[1] = v[1] / s;
    //}

    void normalize3(double v[3])
    {
        double s;

        s = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        v[0] = v[0] / s;
        v[1] = v[1] / s;
        v[2] = v[2] / s;
    }

    void init(void)
    {
        int i, j, k;

        for (i = 0; i < B; i++)
        {
            pp[i] = i;
            //g1[i] = (double)((cry_rand() % (B + B)) - B) / B;

            //for (j = 0 ; j < 2 ; j++)
            //  g2[i][j] = (double)((cry_rand() % (B + B)) - B) / B;
            //normalize2(g2[i]);

            for (j = 0; j < 3; j++)
            {
                g3[i][j] = cry_random(-1.0f, 1.0f);
            }
            normalize3(g3[i]);
        }

        while (--i)
        {
            k = pp[i];
            pp[i] = pp[j = cry_random(0, B - 1)];
            pp[j] = k;
        }

        for (i = 0; i < B + 2; i++)
        {
            pp[B + i] = pp[i];
            //g1[B + i] = g1[i];
            //for (j = 0 ; j < 2 ; j++)
            //  g2[B + i][j] = g2[i][j];
            for (j = 0; j < 3; j++)
            {
                g3[B + i][j] = g3[i][j];
            }
        }
    }

    /* --- My harmonic summing functions - PDB --------------------------*/

    /*
    In what follows "alpha" is the weight when the sum is formed.
    Typically it is 2, As this approaches 1 the function is noisier.
    "beta" is the harmonic scaling/spacing, typically 2.
    */

    //double PerlinNoise1D(double x,double alpha,double beta,int n)
    //{
    //  int i;
    //  double val,sum = 0;
    //  double p,scale = 1;

    //  p = x;
    //  for (i=0;i<n;i++) {
    //      val = noise1(p);
    //      sum += val / scale;
    //      scale *= alpha;
    //      p *= beta;
    //  }
    //  return(sum);
    //}

    //double PerlinNoise2D(double x,double y,double alpha,double beta,int n)
    //{
    //  int i;
    //  double val,sum = 0;
    //  double p[2],scale = 1;

    //  p[0] = x;
    //  p[1] = y;
    //  for (i=0;i<n;i++) {
    //      val = noise2(p);
    //      sum += val / scale;
    //      scale *= alpha;
    //      p[0] *= beta;
    //      p[1] *= beta;
    //  }
    //  return(sum);
    //}

    float PerlinNoise3D(double x, double y, double z, double alpha, double beta, int n)
    {
        int i;
        double val, sum = 0;
        double p[3], scale = 1;

        p[0] = x;
        p[1] = y;
        p[2] = z;
        for (i = 0; i < n; i++)
        {
            val = noise3(p);
            sum += val / scale;
            scale *= alpha;
            p[0] *= beta;
            p[1] *= beta;
            p[2] *= beta;
        }
        return aznumeric_cast<float>(sum);
    }

    #undef B
    #undef BM
    #undef N
    #undef NP
    #undef NM

    //////////////////////////////////////////////////////////////////////////

    struct VolumeParticle
    {
        Vec3 p;
        float r;

        VolumeParticle()
            : p(0, 0, 0)
            , r(0)
        {
        }
    };

    typedef std::vector<VolumeParticle> VolumeDesc;

    //////////////////////////////////////////////////////////////////////////

    bool ReadVolumeDescription(const char* pFilePath, VolumeDesc& volDesc, float& globalDensity)
    {
        volDesc.clear();
        globalDensity = 1;

        XmlNodeRef root(gEnv->pSystem->LoadXmlFromFile(pFilePath));
        if (root)
        {
            int numSprites(root->getChildCount());
            if (numSprites > 0)
            {
                root->getAttr("Density", globalDensity);
                globalDensity = clamp_tpl(globalDensity, 0.0f, 1.0f);

                volDesc.reserve(numSprites);
                for (int i(0); i < numSprites; ++i)
                {
                    XmlNodeRef child(root->getChild(i));
                    if (child)
                    {
                        VolumeParticle vp;
                        child->getAttr("Pos", vp.p);
                        child->getAttr("Radius", vp.r);
                        volDesc.push_back(vp);
                    }
                }
            }
        }

        return !volDesc.empty();
    }

    //////////////////////////////////////////////////////////////////////////

    void CalcBoundingBox(const VolumeDesc& volDesc, AABB& bbox)
    {
        bbox.Reset();

        if (volDesc.empty())
        {
            return;
        }

        for (size_t i = 0; i < volDesc.size(); ++i)
        {
            bbox.Add(volDesc[i].p, volDesc[i].r);
        }
    }

    //////////////////////////////////////////////////////////////////////////

    void CalcTightBounds(const AABB& bbox, AABB& tightBounds, float& scale)
    {
        float max = bbox.max.x - bbox.min.x;

        if (bbox.max.y - bbox.min.y > max)
        {
            max = bbox.max.y - bbox.min.y;
        }

        if (bbox.max.z - bbox.min.z > max)
        {
            max = bbox.max.z - bbox.min.z;
        }

        tightBounds.min.x = -(bbox.max.x - bbox.min.x) / max;
        tightBounds.max.x =  (bbox.max.x - bbox.min.x) / max;

        tightBounds.min.y = -(bbox.max.y - bbox.min.y) / max;
        tightBounds.max.y =  (bbox.max.y - bbox.min.y) / max;

        tightBounds.min.z = -(bbox.max.z - bbox.min.z) / max;
        tightBounds.max.z =  (bbox.max.z - bbox.min.z) / max;

        scale = max * 0.5f;
    }

    //////////////////////////////////////////////////////////////////////////

    void AdjustBoundingBox(AABB& bbox)
    {
        float max = bbox.max.x - bbox.min.x;

        if (bbox.max.y - bbox.min.y > max)
        {
            max = bbox.max.y - bbox.min.y;
        }

        if (bbox.max.z - bbox.min.z > max)
        {
            max = bbox.max.z - bbox.min.z;
        }

        float adj = (max - (bbox.max.x - bbox.min.x)) * 0.5f;
        bbox.min.x -= adj;
        bbox.max.x += adj;

        adj = (max - (bbox.max.y - bbox.min.y)) * 0.5f;
        bbox.min.y -= adj;
        bbox.max.y += adj;

        adj = (max - (bbox.max.z - bbox.min.z)) * 0.5f;
        bbox.min.z -= adj;
        bbox.max.z += adj;
    }

    //////////////////////////////////////////////////////////////////////////

    inline uint8 TrilinearFilteredLookup(const SVolumeDataSrcB& density, const float lx, const float ly, const float lz)
    {
        if (lx < 0 || ly < 0 || lz < 0)
        {
            return 0;
        }

        int x = (int) lx;
        int y = (int) ly;
        int z = (int) lz;

        if (x > (int)density.m_width - 2 || y > (int)density.m_height - 2 || z > (int)density.m_depth - 2)
        {
            return 0;
        }

        const uint8* src = &density[density.Idx(x, y, z)];

        int lerpX = (int) ((lx - x) * 256.0f);
        int lerpY = (int) ((ly - y) * 256.0f);
        int lerpZ = (int) ((lz - z) * 256.0f);

        int _s000 = src[0];
        int _s001 = src[1];
        int _s010 = src[density.m_width];
        int _s011 = src[1 + density.m_width];

        src += density.m_slice;

        int _s100 = src[0];
        int _s101 = src[1];
        int _s110 = src[density.m_width];
        int _s111 = src[1 + density.m_width];

        int s00 = (_s000 << 8) + (_s001 - _s000) * lerpX;
        int s01 = (_s010 << 8) + (_s011 - _s010) * lerpX;
        int s0 = ((s00 << 8) + (s01 - s00) * lerpY) >> 8;

        int s10 = (_s100 << 8) + (_s101 - _s100) * lerpX;
        int s11 = (_s110 << 8) + (_s111 - _s110) * lerpX;
        int s1 = ((s10 << 8) + (s11 - s10) * lerpY) >> 8;

        return ((s0 << 8) + (s1 - s0) * lerpZ) >> 16;
    }

    inline int sat(int f)
    {
        return (f < 0) ? 0 : ((f > 255) ? 255 : f);
    }

    void Voxelize(const VolumeDesc& volDesc, float globalDensity, const AABB& bbox, SVolumeDataSrcB& trg)
    {
        SVolumeDataSrcB tmp(trg.m_width + 2, trg.m_height + 2, trg.m_depth + 2);

        // clear temporary volume
        for (size_t i(0); i < tmp.size(); ++i)
        {
            tmp[i] = 0;
        }

        // rasterize spheres
        for (size_t i = 0; i < volDesc.size(); ++i)
        {
            const VolumeParticle& vp(volDesc[i]);

            int sz = (int) floor((float) (trg.m_depth - 1) * ((vp.p.z - vp.r) - bbox.min.z) / (bbox.max.z - bbox.min.z));
            int ez = (int) ceil((float) (trg.m_depth - 1) * ((vp.p.z + vp.r) - bbox.min.z) / (bbox.max.z - bbox.min.z));

            int sy = (int) floor((float) (trg.m_height - 1) * ((vp.p.y - vp.r) - bbox.min.y) / (bbox.max.y - bbox.min.y));
            int ey = (int) ceil((float) (trg.m_height - 1) * ((vp.p.y + vp.r) - bbox.min.y) / (bbox.max.y - bbox.min.y));

            int sx = (int) floor((float) (trg.m_width - 1) * ((vp.p.x - vp.r) - bbox.min.x) / (bbox.max.x - bbox.min.x));
            int ex = (int) ceil((float) (trg.m_width - 1) * ((vp.p.x + vp.r) - bbox.min.x) / (bbox.max.x - bbox.min.x));

            float stepZ = (bbox.max.z - bbox.min.z) / (float) trg.m_depth;
            float wz = vp.p.z - (bbox.min.z + ((float) sz + 0.5f) * stepZ);

            for (int z = sz; z <= ez; ++z, wz -= stepZ)
            {
                float dz2 = wz * wz;

                float stepY = (bbox.max.y - bbox.min.y) / (float) trg.m_height;
                float wy = vp.p.y - (bbox.min.y + ((float) sy + 0.5f) * stepY);

                for (int y = sy; y <= ey; ++y, wy -= stepY)
                {
                    float dy2 = wy * wy;

                    float stepX = (bbox.max.x - bbox.min.x) / (float) trg.m_width;
                    float wx = vp.p.x - (bbox.min.x + ((float) sx + 0.5f) * stepX);

                    size_t idx = tmp.Idx(sx + 1, y + 1, z + 1);
                    for (int x = sx; x <= ex; ++x, wx -= stepX, ++idx)
                    {
                        float dx2 = wx * wx;
                        float d = sqrt_tpl(dx2 + dy2 + dz2);
                        float v = max(1.0f - d / vp.r, 0.0f) * globalDensity;
                        tmp[idx] = max(tmp[idx], (uint8) (v * 255.0f));
                    }
                }
            }
        }

        // perturb volume using Perlin noise
        {
            float stepGx = 5.0f / (float) trg.m_width;
            float stepGy = 5.0f / (float) trg.m_height;
            float stepGz = 5.0f / (float) trg.m_depth;

            const float origBias = 0.25f;
            const float origFillDens = 1.2f;

            const uint8 bias = (uint8) (origBias * 256.0f);
            const uint32 biasNorm = (uint32) (256.0f * 256.0f * (origFillDens / (1.0f - origBias)));

            size_t idx = 0;

            float nz = 0;
            float gz = 0;
            for (unsigned int z = 0; z < trg.m_depth; ++z, nz += 1.0f, gz += stepGz)
            {
                float ny = 0;
                float gy = 0;
                for (unsigned int y = 0; y < trg.m_height; ++y, ny += 1.0f, gy += stepGy)
                {
                    float nx = 0;
                    float gx = 0;
                    for (unsigned int x = 0; x < trg.m_width; ++x, nx += 1.0f, gx += stepGx, ++idx)
                    {
                        float gtx = (float)nx + 5.0f * PerlinNoise3D(gx, gy, gz, 2.0f, 2.1525f, 5);
                        float gty = (float)ny + 5.0f * PerlinNoise3D(gx + 21.132f, gy, gz, 2.0f, 2.1525f, 5);
                        float gtz = (float)nz + 5.0f * PerlinNoise3D(gx, gy + 3.412f, gz, 2.0f, 2.1525f, 5);

                        //float val = (float) TrilinearFilteredLookup(tmp, gtx + 1.0f, gty + 1.0f, gtz + 1.0f) / 255.0f;
                        //trg[idx] = (uint8) (saturate(saturate(val - origBias) / (1.0f - origBias) * origFillDens) * 255.0f);

                        uint8 val = TrilinearFilteredLookup(tmp, gtx + 1.0f, gty + 1.0f, gtz + 1.0f);
                        trg[idx] = sat(sat(val - bias) * biasNorm >> 16);

                        //trg[idx] = (uint8) TrilinearFilteredLookup(tmp, gtx + 1.0f, gty + 1.0f, gtz + 1.0f);
                    }
                }
            }
        }

        //// low pass filter
        //{
        //  {
        //      size_t srcIdx = 0;
        //      for (unsigned int z = 0; z < trg.m_depth; ++z)
        //      {
        //          for (unsigned int y = 0; y < trg.m_height; ++y)
        //          {
        //              size_t dstIdx = tmp.Idx(1, y+1, z+1);
        //              for (unsigned int x = 0; x < trg.m_width; ++x, ++srcIdx, ++dstIdx)
        //                  tmp[dstIdx] = trg[srcIdx];
        //          }
        //      }
        //  }
        //  {
        //      size_t dstIdx = 0;
        //      for (unsigned int z = 0; z < trg.m_depth; ++z)
        //      {
        //          for (unsigned int y = 0; y < trg.m_height; ++y)
        //          {
        //              const uint8* src = &tmp[tmp.Idx(1, y+1, z+1)];
        //              for (unsigned int x = 0; x < trg.m_width; ++x, ++dstIdx, ++src)
        //              {
        //                  const uint8* srcRow1 = src - tmp.m_slice - 1;
        //                  const uint8* srcRow0 = srcRow1 - tmp.m_width;
        //                  const uint8* srcRow2 = srcRow1 + tmp.m_width;

        //                  uint32 sum = 0;
        //                  sum += srcRow0[0] + srcRow0[1] + srcRow0[2];
        //                  sum += srcRow1[0] + (srcRow1[1] << 1) + srcRow1[2];
        //                  sum += srcRow2[0] + srcRow2[1] + srcRow2[2];

        //                  srcRow0 += tmp.m_slice;
        //                  srcRow1 += tmp.m_slice;
        //                  srcRow2 += tmp.m_slice;

        //                  sum += srcRow0[0] + (srcRow0[1] << 1) + srcRow0[2];
        //                  sum += (srcRow1[0] << 1) + (srcRow1[1] << 4) + (srcRow1[2] << 1);
        //                  sum += srcRow2[0] + (srcRow2[1] << 1) + srcRow2[2];

        //                  srcRow0 += tmp.m_slice;
        //                  srcRow1 += tmp.m_slice;
        //                  srcRow2 += tmp.m_slice;

        //                  sum += srcRow0[0] + srcRow0[1] + srcRow0[2];
        //                  sum += srcRow1[0] + (srcRow1[1] << 1) + srcRow1[2];
        //                  sum += srcRow2[0] + srcRow2[1] + srcRow2[2];

        //                  const uint32 weight = 65536 / 48;
        //                  sum *= weight;
        //                  sum >>= 16;

        //                  trg[dstIdx] = sum;
        //              }
        //          }
        //      }
        //  }
        //}
    }

    //////////////////////////////////////////////////////////////////////////
    /*
        void DumpAsRAW(SVolumeDataSrcB& v, const char* name)
        {
            FILE* f(fopen(name, "wb"));

            if (f)
            {
                for (unsigned int y = 0; y < 64; ++y)
                {
                    for (unsigned int z = 0; z < 64; ++z)
                    {
                        for (unsigned int x = 0; x < 64; ++x)
                        {
                            uint8 val = v[v.Idx(x,y,z)];

                            fwrite(&val, 1, 1, f);
                        }
                    }
                }
                fclose(f);
            }
        }
    */
    //////////////////////////////////////////////////////////////////////////

    inline void PerPixelFilteredLookup(uint8* pShadow, const uint8* pDensity, const int s00, const int s01,
        const int s10, const int s11, const int lerpX, const int lerpY, const int shadowStrength)
    {
        int _s00 = pShadow[s00];
        int _s01 = pShadow[s01];
        int _s10 = pShadow[s10];
        int _s11 = pShadow[s11];

        int s0 = (_s00 << 8) + (_s01 - _s00) * lerpX;
        int s1 = (_s10 << 8) + (_s11 - _s10) * lerpX;
        int s = ((s0 << 8) + (s1 - s0) * lerpY) >> 8;
        //int d = *pDensity * 103; // 103 = 0.4 * 256.0
        int d = *pDensity * shadowStrength;

        // todo: store data 1-
        //*pShadow = fastround_positive(s * (1.0f - 0.2f * d));
        *pShadow = s * (65280 - d) >> 24; // 65280 = 1.0 * 255 * 256
    }

    //shadow is propagated from one slice to the next
    inline void PerSliceFilteredLookup(uint8* pShadow, const uint8* pDensity, const int duOffset, const int dvOffset,
        const int s00, const int s01, const int s10, const int s11, const int lerpX, const int lerpY, const int shadowStrength)
    {
        for (int v = 0; v < VOLUME_SHADOW_SIZE - 1; ++v)
        {
            for (int u = 0; u < VOLUME_SHADOW_SIZE - 1; ++u)
            {
                int offset(duOffset * u + dvOffset * v);
                PerPixelFilteredLookup(&pShadow[offset], &pDensity[offset], s00, s01, s10, s11, lerpX, lerpY, shadowStrength);
            }
        }
    }

    inline void PerBlockFilteredLookup(uint8* pShadow, const uint8* pDensity, const int duOffset, const int dvOffset,
        const int dwOffset, const int s00, const int s01, const int s10, const int s11, const int lerpX, const int lerpY, const int shadowStrength)
    {
        for (int w = 1; w < VOLUME_SHADOW_SIZE; ++w)
        {
            int offset(dwOffset * w);
            PerSliceFilteredLookup(&pShadow[offset], &pDensity[offset], duOffset, dvOffset, s00, s01, s10, s11, lerpX, lerpY, shadowStrength);
        }
    }
} // anonymous namespace


bool CreateVolumeObject(const char* filePath, SVolumeDataSrcB& trg, AABB& tightBounds, float& scale)
{
    VolumeDesc volDesc;
    float globalDensity(1);
    if (ReadVolumeDescription(filePath, volDesc, globalDensity))
    {
        AABB bbox;

        CalcBoundingBox(volDesc, bbox);
        CalcTightBounds(bbox, tightBounds, scale);
        AdjustBoundingBox(bbox);
        Voxelize(volDesc, globalDensity, bbox, trg);
        //DumpAsRAW(trg, "E:\\TestDensity.raw");
        return true;
    }

    return false;
}


bool CreateVolumeShadow(const Vec3& lightDir, float shadowStrength, const SVolumeDataSrcB& density, SVolumeDataSrcB& shadows)
{
    for (size_t i(0); i < shadows.size(); ++i)
    {
        shadows[i] = 255;
    }

    if (density.m_width != VOLUME_SHADOW_SIZE || density.m_height != VOLUME_SHADOW_SIZE || density.m_depth != VOLUME_SHADOW_SIZE ||
        shadows.m_width != VOLUME_SHADOW_SIZE || shadows.m_height != VOLUME_SHADOW_SIZE || shadows.m_depth != VOLUME_SHADOW_SIZE)
    {
        assert(!"CreateVolumeShadow() -- density and/or shadow volume has invalid dimension!");
        return false;
    }

    float sun[3] = {lightDir.x, lightDir.y, lightDir.z};

    int majAxis = 0;
    {
        float abssun[3] = { fabsf(sun[0]), fabsf(sun[1]), fabsf(sun[2]) };

        if (abssun[1] > abssun[0])
        {
            majAxis = 1;
        }
        if (abssun[2] > abssun[majAxis])
        {
            majAxis = 2;
        }

        // project direction onto the -1..1 cube sides
        float fInv = 1.0f / abssun[majAxis];

        sun[0] *= fInv;
        sun[1] *= fInv;
        sun[2] *= fInv;
    }

    int du[3] = { 0 };
    int dv[3] = { 0 };
    int dw[3] = { 0 };

    int w = 0;

    if (sun[majAxis] > 0)
    {
        dw[majAxis] = 1;
    }
    else
    {
        dw[majAxis] = -1;
        w = VOLUME_SHADOW_SIZE - 1;
    }

    int secAxis = (majAxis + 1) % 3;
    int thirdAxis = (majAxis + 2) % 3;

    du[secAxis] = 1;
    dv[thirdAxis] = 1;

    int duOffset = du[0] + (du[1] + du[2] * VOLUME_SHADOW_SIZE) * VOLUME_SHADOW_SIZE;
    int dvOffset = dv[0] + (dv[1] + dv[2] * VOLUME_SHADOW_SIZE) * VOLUME_SHADOW_SIZE;
    int dwOffset = dw[0] + (dw[1] + dw[2] * VOLUME_SHADOW_SIZE) * VOLUME_SHADOW_SIZE;


    float lerpX = sun[secAxis];
    float lerpY = sun[thirdAxis];

    //if(sun[majAxis]>0)
    {
        lerpX = -lerpX;
        lerpY = -lerpY;
    }

    int prevSliceOffset = -dwOffset;
    int offset = -w * dwOffset;

    if (lerpX < 0)
    {
        lerpX += 1.0f;
        prevSliceOffset -= duOffset;
        offset += duOffset;
    }

    if (lerpY < 0)
    {
        lerpY += 1.0f;
        prevSliceOffset -= dvOffset;
        offset += dvOffset;
    }

    PerBlockFilteredLookup(&shadows[offset], &density[offset], duOffset, dvOffset, dwOffset,
        prevSliceOffset, duOffset + prevSliceOffset, dvOffset + prevSliceOffset, duOffset + dvOffset + prevSliceOffset,
        (int) (lerpX * 256.0f), (int) (lerpY * 256.0f), (int) (clamp_tpl(shadowStrength, 0.0f, 1.0f) * 256.0f));

    return true;
}


bool CreateDownscaledVolumeObject(const SVolumeDataSrcB& src, SVolumeDataSrcB& trg)
{
    if (src.m_width != 2 * trg.m_width || src.m_height != 2 * trg.m_height || src.m_depth != 2 * trg.m_height)
    {
        for (size_t i(0); i < trg.size(); ++i)
        {
            trg[i] = 255;
        }

        assert(!"CreateDownscaledVolumeObject() -- src and/or trg volume has invalid dimension!");
        return false;
    }

    for (unsigned int z = 0; z < trg.m_depth; ++z)
    {
        for (unsigned int y = 0; y < trg.m_height; ++y)
        {
            for (unsigned int x = 0; x < trg.m_width; ++x)
            {
                trg[trg.Idx(x, y, z)] = (uint8) (0.125f * (src[src.Idx(2 * x, 2 * y, 2 * z)] +
                                                           src[src.Idx(2 * x + 1, 2 * y, 2 * z)] +
                                                           src[src.Idx(2 * x, 2 * y + 1, 2 * z)] +
                                                           src[src.Idx(2 * x + 1, 2 * y + 1, 2 * z)] +
                                                           src[src.Idx(2 * x, 2 * y, 2 * z + 1)] +
                                                           src[src.Idx(2 * x + 1, 2 * y, 2 * z + 1)] +
                                                           src[src.Idx(2 * x, 2 * y + 1, 2 * z + 1)] +
                                                           src[src.Idx(2 * x + 1, 2 * y + 1, 2 * z + 1)]));
            }
        }
    }

    return true;
}


//////////////////////////////////////////////////////////////////////////


namespace
{
    typedef std::vector<Vec3> Points;
    typedef std::vector<uint8> PointIndexCache;

    bool IsPowerOfTwo(unsigned int x)
    {
        int cnt = 0;
        while (x > 0)
        {
            cnt += x & 1;
            x >>= 1;
        }
        return cnt == 1;
    }

    bool GeneratePoints(const SVolumeDataSrcB& src, Points& pts)
    {
        struct SPointGenerator
        {
        public:
            SPointGenerator(const SVolumeDataSrcB& _src, unsigned int _size, Points& _pts)
                : m_src(_src)
                , m_pts(_pts)
                , m_cache()
            {
                size_t cacheSize = (m_src.m_width * m_src.m_height * m_src.m_depth + 7) >> 3;
                m_cache.resize(cacheSize);

                for (size_t i = 0; i < cacheSize; ++i)
                {
                    m_cache[i] = 0;
                }

                if (Traverse(0, 0, 0, _size))
                {
                    PushPts(0, 0, 0, _size);
                }
            }

        private:
            void PushPt(unsigned int x, unsigned int y, unsigned int z)
            {
                size_t idx = m_src.Idx(x, y, z);
                if ((m_cache[idx >> 3] & (1 << (idx & 7))) == 0)
                {
                    Vec3 p;
                    p.x = 2.0f * ((float) x / (float) m_src.m_width) - 1.0f;
                    p.y = 2.0f * ((float) y / (float) m_src.m_height) - 1.0f;
                    p.z = 2.0f * ((float) z / (float) m_src.m_depth) - 1.0f;
                    assert(fabs(p.x) <= 1.0f && fabs(p.y) <= 1.0f && fabs(p.z) <= 1.0f);
                    m_pts.push_back(p);
                    m_cache[idx >> 3] |= 1 << (idx & 7);
                }
            }

            void PushPts(unsigned int x, unsigned int y, unsigned int z, unsigned int size)
            {
                PushPt(x, y, z);
                PushPt(x + size, y, z);
                PushPt(x, y + size, z);
                PushPt(x + size, y + size, z);
                PushPt(x, y, z + size);
                PushPt(x + size, y, z + size);
                PushPt(x, y + size, z + size);
                PushPt(x + size, y + size, z + size);
            }

            bool Traverse(unsigned int x, unsigned int y, unsigned int z, unsigned int size)
            {
                if (size == 1)
                {
                    bool isSolid = false;

                    unsigned int zs = max((int) z - 1, 0);
                    unsigned int ze = min(z + 2, m_src.m_depth);
                    for (; zs < ze; ++zs)
                    {
                        unsigned int ys = max((int)y - 1, 0);
                        unsigned int ye = min(y + 2, m_src.m_height);
                        for (; ys < ye; ++ys)
                        {
                            unsigned int xs = max((int)x - 1, 0);
                            unsigned int xe = min(x + 2, m_src.m_width);
                            for (; xs < xe; ++xs)
                            {
                                isSolid |= m_src[m_src.Idx(xs, ys, zs)] != 0;
                                if (isSolid)
                                {
                                    return true;
                                }
                            }
                        }
                    }
                    return false;
                }

                unsigned int ns = size >> 1;

                bool isSolid[8];
                isSolid[0] = Traverse(x, y, z, ns);
                isSolid[1] = Traverse(x + ns, y, z, ns);
                isSolid[2] = Traverse(x, y + ns, z, ns);
                isSolid[3] = Traverse(x + ns, y + ns, z, ns);
                isSolid[4] = Traverse(x, y, z + ns, ns);
                isSolid[5] = Traverse(x + ns, y, z + ns, ns);
                isSolid[6] = Traverse(x, y + ns, z + ns, ns);
                isSolid[7] = Traverse(x + ns, y + ns, z + ns, ns);

                bool mergeSubtrees = isSolid[0];
                for (int i = 1; i < 8; ++i)
                {
                    mergeSubtrees &= isSolid[i];
                }

                if (!mergeSubtrees)
                {
                    if (isSolid[0])
                    {
                        PushPts(x, y, z, ns);
                    }
                    if (isSolid[1])
                    {
                        PushPts(x + ns, y, z, ns);
                    }
                    if (isSolid[2])
                    {
                        PushPts(x, y + ns, z, ns);
                    }
                    if (isSolid[3])
                    {
                        PushPts(x + ns, y + ns, z, ns);
                    }
                    if (isSolid[4])
                    {
                        PushPts(x, y, z + ns, ns);
                    }
                    if (isSolid[5])
                    {
                        PushPts(x + ns, y, z + ns, ns);
                    }
                    if (isSolid[6])
                    {
                        PushPts(x, y + ns, z + ns, ns);
                    }
                    if (isSolid[7])
                    {
                        PushPts(x + ns, y + ns, z + ns, ns);
                    }
                }

                return mergeSubtrees;
            }

        private:
            const SVolumeDataSrcB& m_src;
            Points& m_pts;
            PointIndexCache m_cache;
        };

        if (src.m_width != src.m_height || src.m_width != src.m_depth || src.m_height != src.m_depth)
        {
            return false;
        }

        unsigned int size = src.m_width;

        if (!IsPowerOfTwo(size))
        {
            return false;
        }

        SPointGenerator gen(src, size, pts);

        return true;
    }

    void* qhmalloc(size_t s)
    {
        return new uint8[s];
    }

    void qhfree(void* p)
    {
        delete [] (uint8*) p;
    }
} // anonymous namespace


bool CreateVolumeDataHull(const SVolumeDataSrcB& src, SVolumeDataHull& hull)
{
    Points pts;
    if (GeneratePoints(src, pts))
    {
        Vec3* pPts = &pts[0];
        size_t numPts = pts.size();

        // compute convex hull
        LegacyCryPhysicsUtils::index_t* pIndices = 0;
        int numTris = LegacyCryPhysicsUtils::qhull(pPts, (int) numPts, pIndices, qhmalloc);

        if (pIndices)
        {
            // map used vertices
            typedef std::map<int, int> Vtx2Vtx;
            Vtx2Vtx v2v;
            for (int i = 0, idx = 0; i < numTris; ++i)
            {
                if (v2v.find(pIndices[idx]) == v2v.end())
                {
                    v2v.insert(Vtx2Vtx::value_type(pIndices[idx], -1));
                }
                ++idx;

                if (v2v.find(pIndices[idx]) == v2v.end())
                {
                    v2v.insert(Vtx2Vtx::value_type(pIndices[idx], -1));
                }
                ++idx;

                if (v2v.find(pIndices[idx]) == v2v.end())
                {
                    v2v.insert(Vtx2Vtx::value_type(pIndices[idx], -1));
                }
                ++idx;
            }
            // generate new indices
            int newIdx = 0;
            {
                Vtx2Vtx::iterator it = v2v.begin();
                Vtx2Vtx::iterator itEnd = v2v.end();
                for (; it != itEnd; ++it, ++newIdx)
                {
                    (*it).second = newIdx;
                }
            }
            // write used vertices
            {
                hull.m_numPts = newIdx;
                SAFE_DELETE_ARRAY(hull.m_pPts);
                hull.m_pPts = new SVF_P3F[newIdx];

                Vtx2Vtx::iterator it = v2v.begin();
                Vtx2Vtx::iterator itEnd = v2v.end();
                for (size_t i = 0; it != itEnd; ++it, ++i)
                {
                    assert((*it).second == i);
                    hull.m_pPts[i].xyz = pPts[(*it).first];
                }
            }
            // write remapped indices
            {
                hull.m_numTris = numTris;
                SAFE_DELETE_ARRAY(hull.m_pIdx);
                hull.m_pIdx = new vtx_idx [numTris * 3];

                for (int i = 0; i < numTris * 3; ++i)
                {
                    assert(v2v.find(pIndices[i]) != v2v.end());
                    hull.m_pIdx[i] = v2v[pIndices[i]];
                }
            }
            // free temp hull index array
            qhfree(pIndices);
        }
        else
        {
            hull.m_numPts = 0;
            hull.m_numTris = 0;

            SAFE_DELETE_ARRAY(hull.m_pPts);
            SAFE_DELETE_ARRAY(hull.m_pIdx);
        }

        return true;
    }
    return false;
}
#undef lerp
