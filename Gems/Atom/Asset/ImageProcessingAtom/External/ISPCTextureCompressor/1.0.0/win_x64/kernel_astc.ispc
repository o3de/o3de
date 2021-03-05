///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016, Intel Corporation
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of 
// the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
// SOFTWARE.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef int8 int8_t;
typedef int32 int32_t;
typedef int64 int64_t;

typedef unsigned int8 uint8_t;
typedef unsigned int32 uint32_t;
typedef unsigned int64 uint64_t;

///////////////////////////
//   generic helpers

void swap(float& a, float& b)
{
    int t = a;
    a = b; b = t;
}

void swap(int& a, int& b)
{
    int t = a;
    a = b; b = t;
}

void swap(uint32_t& a, uint32_t& b)
{
    uint32_t t = a;
    a = b; b = t;
}

void swap(uint8_t& a, uint8_t& b)
{
    uint8_t t = a;
    a = b; b = t;
}

inline float sq(float v)
{
    return v*v;
}

inline float clamp(float v, int a, int b)
{
    return clamp(v, (float)a, (float)b);
}

inline float dot3(float a[3], float b[3])
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

inline float dot4(float a[4], float b[4])
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
}

// the following helpers isolate performance warnings

inline uint32_t gather_uint(const uniform uint32_t* const uniform ptr, int idx)
{
    return ptr[idx]; // (perf warning expected)
}

inline float gather_float(uniform float* uniform ptr, int idx)
{
    return ptr[idx]; // (perf warning expected)
}

inline float gather_float(varying float* uniform ptr, int idx)
{
    return ptr[idx]; // (perf warning expected)
}

inline void scatter_uint(uniform uint32_t* ptr, int idx, uint32_t value)
{
    ptr[idx] = value; // (perf warning expected)
}

inline void scatter_float(uniform float* uniform ptr, int idx, float value)
{
    ptr[idx] = value; // (perf warning expected)
}

inline void scatter_float(varying float* uniform ptr, int idx, float value)
{
    ptr[idx] = value; // (perf warning expected)
}

///////////////////////////////////////////////////////////
//				 ASTC shared functions

struct rgba_surface
{
    uint8_t* ptr;
    int width, height, stride;
};

inline void set_pixel(float pixels[], uniform int p, uniform int x, uniform int y, float value);

inline void load_block_interleaved(float pixels[], uniform rgba_surface src[], int xx, int yy, uniform int width, uniform int height)
{
    uniform int pitch = width * height;
    for (uniform int y = 0; y < height; y++)
    for (uniform int x = 0; x < width; x++)
    {
        uint32_t rgba = gather_uint((uint32_t*)src->ptr, ((yy * height + y)*src->stride + (xx * width + x) * 4)/4);

        set_pixel(pixels, 0, x, y, (int)((rgba >> 0) & 255));
        set_pixel(pixels, 1, x, y, (int)((rgba >> 8) & 255));
        set_pixel(pixels, 2, x, y, (int)((rgba >> 16) & 255));
        set_pixel(pixels, 3, x, y, (int)((rgba >> 24) & 255));
    }
}

struct astc_enc_settings
{
    int block_width;
    int block_height;
    int channels;

    int fastSkipTreshold;
    int refineIterations;
};

export uniform int get_programCount()
{
    return programCount;
} 

inline float get_pixel(float pixels[], uniform int p, uniform int x, uniform int y)
{
    uniform static const int ystride = 8;
    uniform static const int pstride = 64;

    return pixels[pstride * p + ystride * y + x];
}

inline void set_pixel(float pixels[], uniform int p, uniform int x, uniform int y, float value)
{
    uniform static const int ystride = 8;
    uniform static const int pstride = 64;

    pixels[pstride * p + ystride * y + x] = value;
}

struct pixel_set
{
    varying float* uniform pixels;

    uniform int width;
    uniform int height;
};

inline void clear_alpha(float pixels[], uniform int width, uniform int height)
{
    for (uniform int y = 0; y < height; y++)
    for (uniform int x = 0; x < width; x++)
    {
        set_pixel(pixels, 3, x, y, 255);
    }
}

void rotate_plane(pixel_set block[], int p)
{
    uniform int pitch = block->height * block->width;

    for (uniform int y = 0; y < block->height; y++)
    for (uniform int x = 0; x < block->width; x++)
    {
        float r = get_pixel(block->pixels, 0, x, y);
        float g = get_pixel(block->pixels, 1, x, y);
        float b = get_pixel(block->pixels, 2, x, y);
        float a = get_pixel(block->pixels, 3, x, y);
        
        if (p == 0) swap(a, r);
        if (p == 1) swap(a, g);
        if (p == 2) swap(a, b);

        set_pixel(block->pixels, 0, x, y, r);
        set_pixel(block->pixels, 1, x, y, g);
        set_pixel(block->pixels, 2, x, y, b);
        set_pixel(block->pixels, 3, x, y, a);        
    }
}

inline void compute_moments(float stats[15], pixel_set block[], uniform int channels)
{
    for (uniform int y = 0; y < block->height; y++)
    for (uniform int x = 0; x < block->width; x++)
    {
        float rgba[4];
        for (uniform int p = 0; p < channels; p++) rgba[p] = get_pixel(block->pixels, p, x, y);

        stats[10] += rgba[0];
        stats[11] += rgba[1];
        stats[12] += rgba[2];

        stats[0] += rgba[0] * rgba[0];
        stats[1] += rgba[0] * rgba[1];
        stats[2] += rgba[0] * rgba[2];

        stats[4] += rgba[1] * rgba[1];
        stats[5] += rgba[1] * rgba[2];

        stats[7] += rgba[2] * rgba[2];

        if (channels == 4)
        {
            stats[13] += rgba[3];

            stats[3] += rgba[0] * rgba[3];
            stats[6] += rgba[1] * rgba[3];
            stats[8] += rgba[2] * rgba[3];
            stats[9] += rgba[3] * rgba[3];
        }
    }

    stats[14] += block->height * block->width;
}

inline void covar_from_stats(float covar[10], float stats[15], uniform int channels)
{
    covar[0] = stats[0] - stats[10 + 0] * stats[10 + 0] / stats[14];
    covar[1] = stats[1] - stats[10 + 0] * stats[10 + 1] / stats[14];
    covar[2] = stats[2] - stats[10 + 0] * stats[10 + 2] / stats[14];

    covar[4] = stats[4] - stats[10 + 1] * stats[10 + 1] / stats[14];
    covar[5] = stats[5] - stats[10 + 1] * stats[10 + 2] / stats[14];

    covar[7] = stats[7] - stats[10 + 2] * stats[10 + 2] / stats[14];

    if (channels == 4)
    {
        covar[3] = stats[3] - stats[10 + 0] * stats[10 + 3] / stats[14];
        covar[6] = stats[6] - stats[10 + 1] * stats[10 + 3] / stats[14];
        covar[8] = stats[8] - stats[10 + 2] * stats[10 + 3] / stats[14];
        covar[9] = stats[9] - stats[10 + 3] * stats[10 + 3] / stats[14];
    }
}

inline void compute_covar_dc(float covar[], float dc[], pixel_set block[], bool zero_based, uniform int channels)
{
    float stats[15] = { 0 };
    compute_moments(stats, block, channels);

    if (zero_based)
    for (uniform int p = 0; p < 4; p++) stats[10 + p] = 0;

    covar_from_stats(covar, stats, channels);
    for (uniform int p = 0; p < channels; p++) dc[p] = stats[10 + p] / stats[14];
}

inline void ssymv3(float a[4], float covar[10], float b[4])
{
    a[0] = covar[0] * b[0] + covar[1] * b[1] + covar[2] * b[2];
    a[1] = covar[1] * b[0] + covar[4] * b[1] + covar[5] * b[2];
    a[2] = covar[2] * b[0] + covar[5] * b[1] + covar[7] * b[2];
}

inline void ssymv4(float a[4], float covar[10], float b[4])
{
    a[0] = covar[0] * b[0] + covar[1] * b[1] + covar[2] * b[2] + covar[3] * b[3];
    a[1] = covar[1] * b[0] + covar[4] * b[1] + covar[5] * b[2] + covar[6] * b[3];
    a[2] = covar[2] * b[0] + covar[5] * b[1] + covar[7] * b[2] + covar[8] * b[3];
    a[3] = covar[3] * b[0] + covar[6] * b[1] + covar[8] * b[2] + covar[9] * b[3];
}

inline void compute_axis(float axis[4], float covar[10], uniform const int powerIterations, uniform int channels)
{
    float vec[4] = { 1, 1, 1, 1 };

    for (uniform int i = 0; i < powerIterations; i++)
    {
        if (channels == 3) ssymv3(axis, covar, vec);
        if (channels == 4) ssymv4(axis, covar, vec);
        for (uniform int p = 0; p < channels; p++) vec[p] = axis[p];

        if (i % 2 == 1) // renormalize every other iteration
        {
            float norm_sq = 0;
            for (uniform int p = 0; p < channels; p++)
                norm_sq += axis[p] * axis[p];

            float rnorm = rsqrt(norm_sq);
            for (uniform int p = 0; p < channels; p++) vec[p] *= rnorm;
        }
    }

    for (uniform int p = 0; p < channels; p++) axis[p] = vec[p];
}

void compute_pca_endpoints(float ep[8], pixel_set block[], bool zero_based, uniform int channels)
{
    float dc[4];
    float cov[10];
    compute_covar_dc(cov, dc, block, zero_based, channels);

    uniform int powerIterations = 10;

    float eps = sq(0.001) * 1000;
    cov[0] += eps;
    cov[4] += eps;
    cov[7] += eps;
    cov[9] += eps;

    float dir[4];
    compute_axis(dir, cov, powerIterations, channels);

    float ext[2] = { 1000, -1000 };

    for (uniform int y = 0; y < block->height; y++)
    for (uniform int x = 0; x < block->width; x++)
    {
        float proj = 0;
        for (uniform int p = 0; p < channels; p++) proj += (get_pixel(block->pixels, p, x, y) - dc[p]) * dir[p];

        ext[0] = min(ext[0], proj);
        ext[1] = max(ext[1], proj);
    }

    if (ext[1] - 1.0f < ext[0])
    {
        ext[1] += 0.5f;
        ext[0] -= 0.5f;
    }
    
    for (uniform int i = 0; i < 2; i++)
    for (uniform int p = 0; p < channels; p++)
    {
        ep[p * 2 + i] = dc[p] + dir[p] * ext[i];
    }
}

uniform static const int range_table[][3] =
{
    //2^ 3^ 5^
    { 1, 0, 0 }, // 0..1
    { 0, 1, 0 }, // 0..2
    { 2, 0, 0 }, // 0..3

    { 0, 0, 1 }, // 0..4
    { 1, 1, 0 }, // 0..5
    { 3, 0, 0 }, // 0..7

    { 1, 0, 1 }, // 0..9
    { 2, 1, 0 }, // 0..11
    { 4, 0, 0 }, // 0..15

    { 2, 0, 1 }, // 0..19
    { 3, 1, 0 }, // 0..23
    { 5, 0, 0 }, // 0..31

    { 3, 0, 1 }, // 0..39
    { 4, 1, 0 }, // 0..47
    { 6, 0, 0 }, // 0..63

    { 4, 0, 1 }, // 0..79
    { 5, 1, 0 }, // 0..95
    { 7, 0, 0 }, // 0..127

    { 5, 0, 1 }, // 0..159
    { 6, 1, 0 }, // 0..191
    { 8, 0, 0 }, // 0..255
};

uniform int get_levels(uniform int range)
{
    return (1 + 2 * range_table[range][1] + 4 * range_table[range][2]) << range_table[range][0];
}

struct range_values
{
    int levels_m;
    int levels_m_rcp;
    int levels_e;
    int levels;
};

void fill_range_values(range_values values[], int _range[])
{
    int range = *_range;
    int range_div3 = (range * 21846) >> 16;
    int range_mod3 = range - range_div3 * 3;

    int levels_m = max(2, 5 - range_mod3 * 2);
    int levels_e = max(0, range_mod3 + range_div3 - 1);
    if (range == 0) levels_m = 2;

    int levels_m_rcp = 0x10000 / 2 + 1;
    if (levels_m == 3) levels_m_rcp = 0x10000 / 3 + 1;
    if (levels_m == 5) levels_m_rcp = 0x10000 / 5 + 1;

    values->levels_e = levels_e;
    values->levels_m = levels_m;
    values->levels_m_rcp = levels_m_rcp;

    values->levels = levels_m << levels_e;
}

range_values get_range_values(int range)
{
    range_values values;
    fill_range_values(&values, &range);
    return values;
}

int get_levels(int range)
{
    int range_div3 = (range * 21846) >> 16;
    int range_mod3 = range - range_div3 * 3;

    int levels_m = max(2, 5 - range_mod3 * 2);
    int levels_e = range_mod3 + range_div3 - 1;

    return (levels_m << (levels_e + 1)) >> 1;
}

uniform float get_sq_rcp_levels(uniform int range)
{
    uniform static const float table[] =
    {
        1.000000, 0.250000, 0.111111,
        0.062500, 0.040000, 0.020408,
        0.012346, 0.008264, 0.004444,
        0.002770, 0.001890, 0.001041,
        0.000657, 0.000453, 0.000252,
        0.000160, 0.000111, 0.000062,
        0.000040, 0.000027, 0.000015,
    };

    return table[range];
}

///////////////////////////////////////////////////////////
//				 ASTC candidate ranking

struct astc_rank_state
{
    float pixels[256];

    float pca_error[2][5];
    float alpha_error[2][5];
    float sq_norm[2][5];
    float scale_error[7][7]; // 2x2 to 8x8

    float best_scores[64];
    uint32_t best_modes[64];

    // settings
    uniform int block_width;
    uniform int block_height;
    uniform int pitch;

    uniform int fastSkipTreshold;
};

struct astc_mode
{
    int width;
    int height;
    bool dual_plane;
    int weight_range;
    int color_component_selector;
    int partitions;
    int partition_id;
    int color_endpoint_pairs;
    int color_endpoint_modes[2];
    int endpoint_range;
};

void dct_4(float values[], uniform int stride)
{
    uniform static const float scale[] = { 0.5, 0.707106769 };
    uniform static const float c[5] = { 1, 0.923879533, 0.707106769, 0.382683432, 0 };
        
    float data[4];
    for (uniform int i = 0; i < 2; i++)
    {
        float a = values[stride * i];
        float b = values[stride * (3 - i)];
        data[0 + i] = a + b;
        data[2 + i] = a - b;
    }

    for (uniform int i = 0; i < 4; i++)
    {
        float acc = 0;
        varying float* uniform input = &data[(i % 2) * 2];
        for (uniform int j = 0; j < 2; j++)
        {
            uniform int e = (2 * j + 1)*i;
            e = e % (4 * 4);
            uniform float w = 1;
            if (e>8) { e = 16 - e; }
            if (e>4) { w = -1;  e = 8 - e; }
            w *= c[e];
            acc += w * input[j];
        }

        values[stride * i] = acc * scale[i > 0];
    }
}

void dct_6(float values[], uniform int stride)
{
    uniform static const float scale[] = { 0.408248290, 0.577350269 };
    uniform static const float c[7] =
        { 1, 0.965925813, 0.866025388, 0.707106769, 0.500000000, 0.258819044, 0 };
    
    float data[6];
    for (uniform int i = 0; i < 3; i++)
    {
        float a = values[stride * i];
        float b = values[stride * (5 - i)];
        data[0 + i] = a + b;
        data[3 + i] = a - b;
    }

    for (uniform int i = 0; i < 6; i++)
    {
        float acc = 0;
        varying float* uniform input = &data[(i % 2) * 3];
        for (uniform int j = 0; j < 3; j++)
        {
            uniform int e = (2 * j + 1)*i;
            e = e % (4 * 6);
            uniform float w = 1;
            if (e>12) { e = 24 - e; }
            if (e>6) { w = -1;  e = 12 - e; }
            w *= c[e];
            acc += w * input[j];
        }

        values[stride * i] = acc * scale[i > 0];
    }
}

void dct_n(float values[], uniform int stride, uniform int n)
{
    uniform static const float pi = 3.14159265358979323846;

    assert(n <= 16);
    uniform float c[16 + 1];
    for (uniform int i = 0; i <= n; i++)
        c[i] = cos((i / (4.0 * n) * 2 * pi));

    uniform float scale[] = { 1 / sqrt(1.0*n), 1 / sqrt(n / 2.0), };

    float data[16];
    for (uniform int i = 0; i < n; i++)
        data[i] = values[stride * i];

    for (uniform int i = 0; i < n; i++)
    {
        float acc = 0;
        for (uniform int j = 0; j < n; j++)
        {
            uniform int e = (2 * j + 1)*i;
            e = e % (4 * n);
            float w = 1;
            if (e > 2 * n) { e = 4 * n - e; }
            if (e > n) { w = -1;  e = 2 * n - e; }
            assert(e <= n);
            w *= c[e];
            acc += w * data[j];
        }

        values[stride * i] = acc * scale[i > 0];
    }
}

void dct(float values[], uniform int stride, uniform int n)
{
    if (false) {}
    else if (n == 8) dct_n(values, stride, 8);
    else if (n == 6) dct_6(values, stride);
    else if (n == 5) dct_n(values, stride, 5);
    else if (n == 4) dct_4(values, stride);
    else
    {
        assert(false);
    }
}

void compute_dct_inplace(pixel_set block[], uniform int channels)
{
    uniform static const int stride = 8;
    uniform static const int pitch = 64;

    for (uniform int p = 0; p < channels; p++)
    {
        for (uniform int y = 0; y < block->height; y++)
            dct(&block->pixels[pitch * p + y * stride], 1, block->width);

        for (uniform int x = 0; x < block->width; x++)
            dct(&block->pixels[pitch * p + x], stride, block->height);
    }
}

void compute_metrics(astc_rank_state state[])
{
    float temp_pixels[256];
    pixel_set _pset; varying pixel_set* uniform pset = &_pset;
    pset->pixels = temp_pixels;
    pset->width = state->block_width;
    pset->height = state->block_height;

    for (uniform int p = 0; p < 4; p++)
    for (uniform int y = 0; y < state->block_height; y++)
    for (uniform int x = 0; x < state->block_width; x++)
    {
        float value = get_pixel(state->pixels, p, x, y);
        set_pixel(pset->pixels, p, x, y, value);
    }

    for (uniform int i = 0; i < 2; i++)
    {
        bool zero_based = (i == 1);
        float endpoints[8];
        compute_pca_endpoints(endpoints, pset, zero_based, 4);

        float base[4], dir[4];
        for (int p = 0; p < 4; p++) dir[p] = endpoints[p * 2 + 1] - endpoints[p * 2];
        for (int p = 0; p < 4; p++) base[p] = endpoints[p * 2];
        float sq_norm = dot4(dir, dir) + 0.00001;

        float pca_error = 0;
        float alpha_error = 0;
        float pca_alpha_error = 0;
        for (uniform int y = 0; y < state->block_height; y++)
        for (uniform int x = 0; x < state->block_width; x++)
        {
            float pixel[4];
            for (uniform int p = 0; p < 4; p++) pixel[p] = get_pixel(pset->pixels, p, x, y) - base[p];
            float proj = dot4(pixel, dir) / sq_norm;
            for (uniform int p = 0; p < 3; p++) pca_error += sq(get_pixel(pset->pixels, p, x, y) - (proj * dir[p] + base[p]));
            pca_alpha_error += sq(get_pixel(pset->pixels, 3, x, y) - (proj * dir[3] + base[3]));
            alpha_error += sq(get_pixel(pset->pixels, 3, x, y) - 255);
        }

        state->pca_error[i][0] = pca_error + pca_alpha_error;
        state->alpha_error[i][0] = alpha_error - pca_alpha_error;
        state->sq_norm[i][0] = sq_norm;
    }

    for (uniform int i = 0; i < 2; i++)
    for (uniform int c = 1; c < 5; c++)
    {
        rotate_plane(pset, c - 1);
        
        bool zero_based = (i == 1);
        float endpoints[8];
        compute_pca_endpoints(endpoints, pset, zero_based, 3);

        float base[3], dir[3];
        for (int p = 0; p < 3; p++) dir[p] = endpoints[p * 2 + 1] - endpoints[p * 2];
        for (int p = 0; p < 3; p++) base[p] = endpoints[p * 2];
        float sq_norm = dot3(dir, dir) + 0.00001;

        float pca_error = 0;
        float alpha_error = 0;
        float pca_alpha_error = 0;
        float ext[2] = { 1000, -1000 };
        for (uniform int y = 0; y < state->block_height; y++)
        for (uniform int x = 0; x < state->block_width; x++)
        {
            float pixel[3];
            for (uniform int p = 0; p < 3; p++) pixel[p] = get_pixel(pset->pixels, p, x, y) - base[p];
            float proj = dot3(pixel, dir) / sq_norm;
            for (uniform int p = 0; p < 3; p++)
            {
                if (p == c - 1) 
                {
                    pca_alpha_error += sq(get_pixel(pset->pixels, p, x, y) - (proj * dir[p] + base[p]));
                    alpha_error += sq(get_pixel(pset->pixels, p, x, y) - 255);
                }
                else
                {
                    pca_error += sq(get_pixel(pset->pixels, p, x, y) - (proj * dir[p] + base[p]));
                }
            }

            float value = get_pixel(pset->pixels, 3, x, y);
            ext[0] = min(ext[0], value);
            ext[1] = max(ext[1], value);
        }

        state->pca_error[i][c] = pca_error + pca_alpha_error;
        state->alpha_error[i][c] = alpha_error - pca_alpha_error;
        state->sq_norm[i][c] = sq_norm + sq(ext[1] - ext[0]);
        
        // rotate back
        rotate_plane(pset, c - 1);
    }
    
    compute_dct_inplace(pset, 4);
        
    for (uniform int h = 2; h <= state->block_height; h++)
    for (uniform int w = 2; w <= state->block_width; w++)
    {
        uniform int stride = 8;
        uniform int pitch = 64;
        
        float sq_sum = 0;

        for (uniform int y = 0; y < state->block_height; y++)
        for (uniform int x = 0; x < state->block_width; x++)
        {
            if (y < h && x < w) continue;

            for (uniform int p = 0; p < 4; p++)
                sq_sum += sq(pset->pixels[pitch * p + stride * y + x]);
        }

        state->scale_error[h - 2][w - 2] = sq_sum;
    }
}

float estimate_error(astc_rank_state state[], uniform astc_mode mode[])
{
    uniform int c = 0;
    if (mode->dual_plane) c = 1 + mode->color_component_selector;

    float scale_error = state->scale_error[mode->height - 2][mode->width - 2];
    
    uniform bool zero_based = (mode->color_endpoint_modes[0] % 4) == 2;
    float pca_error = state->pca_error[zero_based][c];
    float sq_norm = state->sq_norm[zero_based][c];

    if (mode->color_endpoint_modes[0] <= 8) pca_error += state->alpha_error[zero_based][c];

    uniform float sq_rcp_w_levels = get_sq_rcp_levels(mode->weight_range);
    uniform float sq_rcp_ep_levels = get_sq_rcp_levels(mode->endpoint_range);
    float quant_error = 0;

    quant_error += 2 * sq_norm * sq_rcp_w_levels;
    quant_error += 9000 * (state->block_height * state->block_width) * sq_rcp_ep_levels;
    
    float error = 0;
    error += scale_error;
    error += pca_error;
    error += quant_error;

    return error;
}

void insert_element(astc_rank_state state[], float error, uint32_t packed_mode, float threshold_error[])
{
    float max_error = 0;

    for (uniform int k = 0; k < state->fastSkipTreshold; k++)
    {
        if (state->best_scores[k] > error)
        {
            swap(state->best_scores[k], error);
            swap(state->best_modes[k], packed_mode);
        }

        max_error = max(max_error, state->best_scores[k]);
    }

    *threshold_error = max_error;
}

uniform static const int packed_modes_count = 3334;
uniform static const uint32_t packed_modes[3334] =
{
    0x0006D400, 0x0016D340, 0x0026D380, 0x0036CDC0, 0x00469400, 0x00569401, 0x00668702, 0x00769440,
    0x00868D41, 0x00969480, 0x00A68D81, 0x00B693C0, 0x00C688C1, 0x00D4D400, 0x00E4D401, 0x00F4C702,
    0x0104D440, 0x0114CD41, 0x0124D480, 0x0134CD81, 0x0144D3C0, 0x0154C8C1, 0x01667400, 0x01767401,
    0x01867302, 0x01966803, 0x01A67440, 0x01B67341, 0x01C66B42, 0x01D66443, 0x01E67480, 0x01F67381,
    0x02066B82, 0x02166483, 0x022674C0, 0x02366DC1, 0x024667C2, 0x0253D400, 0x0263D401, 0x0273D302,
    0x0283C803, 0x0293D440, 0x02A3D341, 0x02B3CB42, 0x02C3C443, 0x02D3D480, 0x02E3D381, 0x02F3CB82,
    0x0303C483, 0x0313D4C0, 0x0323CDC1, 0x0333C7C2, 0x03465400, 0x03565401, 0x03665402, 0x03765403,
    0x03865004, 0x03964705, 0x03A65440, 0x03B65441, 0x03C65342, 0x03D64E43, 0x03E64944, 0x03F65480,
    0x04065481, 0x04165382, 0x04264E83, 0x04364984, 0x044654C0, 0x045652C1, 0x04664DC2, 0x047649C3,
    0x048646C4, 0x049E5400, 0x04AE5240, 0x04BE5280, 0x04CE4DC0, 0x04DE5410, 0x04EE5250, 0x04FE5290,
    0x050E4DD0, 0x051E5420, 0x052E5260, 0x053E52A0, 0x054E4DE0, 0x055E52B0, 0x056E4DF0, 0x0572D400,
    0x0582D401, 0x0592D402, 0x05A2D403, 0x05B2D004, 0x05C2C705, 0x05D2D440, 0x05E2D441, 0x05F2D342,
    0x0602CE43, 0x0612C944, 0x0622D480, 0x0632D481, 0x0642D382, 0x0652CE83, 0x0662C984, 0x0672D4C0,
    0x0682D2C1, 0x0692CDC2, 0x06A2C9C3, 0x06B2C6C4, 0x06CAD400, 0x06DAD240, 0x06EAD280, 0x06FACDC0,
    0x070AD410, 0x071AD250, 0x072AD290, 0x073ACDD0, 0x074AD420, 0x075AD260, 0x076AD2A0, 0x077ACDE0,
    0x078AD2B0, 0x079ACDF0, 0x07A63400, 0x07B63401, 0x07C63402, 0x07D63403, 0x07E63404, 0x07F63405,
    0x08063306, 0x08162E07, 0x08262708, 0x08363440, 0x08463441, 0x08563442, 0x08663443, 0x08763444,
    0x08862F45, 0x08962B46, 0x08A62847, 0x08B63480, 0x08C63481, 0x08D63482, 0x08E63483, 0x08F63484,
    0x09062F85, 0x09162B86, 0x09262887, 0x093634C0, 0x094634C1, 0x095633C2, 0x096630C3, 0x09762EC4,
    0x09862AC5, 0x099627C6, 0x09A625C7, 0x09BE3400, 0x09CE3401, 0x09DE2502, 0x09EE3440, 0x09FE2C41,
    0x0A0E3480, 0x0A1E2C81, 0x0A2E33C0, 0x0A3E28C1, 0x0A4E3410, 0x0A5E3411, 0x0A6E2512, 0x0A7E3450,
    0x0A8E2C51, 0x0A9E3490, 0x0AAE2C91, 0x0ABE33D0, 0x0ACE28D1, 0x0ADE3420, 0x0AEE3421, 0x0AFE2522,
    0x0B0E3460, 0x0B1E2C61, 0x0B2E34A0, 0x0B3E2CA1, 0x0B4E33E0, 0x0B5E28E1, 0x0B6E34B0, 0x0B7E2CB1,
    0x0B8E33F0, 0x0B9E28F1, 0x0BA1D400, 0x0BB1D401, 0x0BC1D402, 0x0BD1D403, 0x0BE1D404, 0x0BF1D405,
    0x0C01D306, 0x0C11CE07, 0x0C21C708, 0x0C31D440, 0x0C41D441, 0x0C51D442, 0x0C61D443, 0x0C71D444,
    0x0C81CF45, 0x0C91CB46, 0x0CA1C847, 0x0CB1D480, 0x0CC1D481, 0x0CD1D482, 0x0CE1D483, 0x0CF1D484,
    0x0D01CF85, 0x0D11CB86, 0x0D21C887, 0x0D31D4C0, 0x0D41D4C1, 0x0D51D3C2, 0x0D61D0C3, 0x0D71CEC4,
    0x0D81CAC5, 0x0D91C7C6, 0x0DA1C5C7, 0x0DB9D400, 0x0DC9D401, 0x0DD9C502, 0x0DE9D440, 0x0DF9CC41,
    0x0E09D480, 0x0E19CC81, 0x0E29D3C0, 0x0E39C8C1, 0x0E49D410, 0x0E59D411, 0x0E69C512, 0x0E79D450,
    0x0E89CC51, 0x0E99D490, 0x0EA9CC91, 0x0EB9D3D0, 0x0EC9C8D1, 0x0ED9D420, 0x0EE9D421, 0x0EF9C522,
    0x0F09D460, 0x0F19CC61, 0x0F29D4A0, 0x0F39CCA1, 0x0F49D3E0, 0x0F59C8E1, 0x0F69D4B0, 0x0F79CCB1,
    0x0F89D3F0, 0x0F99C8F1, 0x0FA61401, 0x0FB61402, 0x0FC61403, 0x0FD61404, 0x0FE61405, 0x0FF61406,
    0x10061407, 0x10161408, 0x10261409, 0x1036140A, 0x1046130B, 0x10561441, 0x10661442, 0x10761443,
    0x10861444, 0x10961445, 0x10A61446, 0x10B61447, 0x10C61348, 0x10D61049, 0x10E60E4A, 0x10F60B4B,
    0x11061481, 0x11161482, 0x11261483, 0x11361484, 0x11461485, 0x11561486, 0x11661487, 0x11761388,
    0x11861089, 0x11960E8A, 0x11A60B8B, 0x11B614C1, 0x11C614C2, 0x11D614C3, 0x11E614C4, 0x11F613C5,
    0x120611C6, 0x121610C7, 0x12260DC8, 0x12360BC9, 0x12460ACA, 0x125607CB, 0x126E1400, 0x127E1401,
    0x128E1402, 0x129E1403, 0x12AE0E04, 0x12BE0505, 0x12CE1440, 0x12DE1441, 0x12EE1242, 0x12FE0D43,
    0x130E0844, 0x131E1480, 0x132E1481, 0x133E1282, 0x134E0D83, 0x135E0884, 0x136E14C0, 0x137E11C1,
    0x138E0DC2, 0x139E08C3, 0x13AE05C4, 0x13BE1410, 0x13CE1411, 0x13DE1412, 0x13EE1413, 0x13FE0E14,
    0x140E0515, 0x141E1450, 0x142E1451, 0x143E1252, 0x144E0D53, 0x145E0854, 0x146E1490, 0x147E1491,
    0x148E1292, 0x149E0D93, 0x14AE0894, 0x14BE14D0, 0x14CE11D1, 0x14DE0DD2, 0x14EE08D3, 0x14FE05D4,
    0x150E1420, 0x151E1421, 0x152E1422, 0x153E1423, 0x154E0E24, 0x155E0525, 0x156E1460, 0x157E1461,
    0x158E1262, 0x159E0D63, 0x15AE0864, 0x15BE14A0, 0x15CE14A1, 0x15DE12A2, 0x15EE0DA3, 0x15FE08A4,
    0x160E14E0, 0x161E11E1, 0x162E0DE2, 0x163E08E3, 0x164E05E4, 0x165E14B0, 0x166E14B1, 0x167E12B2,
    0x168E0DB3, 0x169E08B4, 0x16AE14F0, 0x16BE11F1, 0x16CE0DF2, 0x16DE08F3, 0x16EE05F4, 0x16F0D401,
    0x1700D402, 0x1710D403, 0x1720D404, 0x1730D405, 0x1740D406, 0x1750D407, 0x1760D408, 0x1770D409,
    0x1780D40A, 0x1790D30B, 0x17A0D441, 0x17B0D442, 0x17C0D443, 0x17D0D444, 0x17E0D445, 0x17F0D446,
    0x1800D447, 0x1810D348, 0x1820D049, 0x1830CE4A, 0x1840CB4B, 0x1850D481, 0x1860D482, 0x1870D483,
    0x1880D484, 0x1890D485, 0x18A0D486, 0x18B0D487, 0x18C0D388, 0x18D0D089, 0x18E0CE8A, 0x18F0CB8B,
    0x1900D4C1, 0x1910D4C2, 0x1920D4C3, 0x1930D4C4, 0x1940D3C5, 0x1950D1C6, 0x1960D0C7, 0x1970CDC8,
    0x1980CBC9, 0x1990CACA, 0x19A0C7CB, 0x19B8D400, 0x19C8D401, 0x19D8D402, 0x19E8D403, 0x19F8CE04,
    0x1A08C505, 0x1A18D440, 0x1A28D441, 0x1A38D242, 0x1A48CD43, 0x1A58C844, 0x1A68D480, 0x1A78D481,
    0x1A88D282, 0x1A98CD83, 0x1AA8C884, 0x1AB8D4C0, 0x1AC8D1C1, 0x1AD8CDC2, 0x1AE8C8C3, 0x1AF8C5C4,
    0x1B08D410, 0x1B18D411, 0x1B28D412, 0x1B38D413, 0x1B48CE14, 0x1B58C515, 0x1B68D450, 0x1B78D451,
    0x1B88D252, 0x1B98CD53, 0x1BA8C854, 0x1BB8D490, 0x1BC8D491, 0x1BD8D292, 0x1BE8CD93, 0x1BF8C894,
    0x1C08D4D0, 0x1C18D1D1, 0x1C28CDD2, 0x1C38C8D3, 0x1C48C5D4, 0x1C58D420, 0x1C68D421, 0x1C78D422,
    0x1C88D423, 0x1C98CE24, 0x1CA8C525, 0x1CB8D460, 0x1CC8D461, 0x1CD8D262, 0x1CE8CD63, 0x1CF8C864,
    0x1D08D4A0, 0x1D18D4A1, 0x1D28D2A2, 0x1D38CDA3, 0x1D48C8A4, 0x1D58D4E0, 0x1D68D1E1, 0x1D78CDE2,
    0x1D88C8E3, 0x1D98C5E4, 0x1DA8D4B0, 0x1DB8D4B1, 0x1DC8D2B2, 0x1DD8CDB3, 0x1DE8C8B4, 0x1DF8D4F0,
    0x1E08D1F1, 0x1E18CDF2, 0x1E28C8F3, 0x1E38C5F4, 0x1E449400, 0x1E549401, 0x1E649402, 0x1E749003,
    0x1E848804, 0x1E949440, 0x1EA49441, 0x1EB48F42, 0x1EC48943, 0x1ED48444, 0x1EE49480, 0x1EF49481,
    0x1F048F82, 0x1F148983, 0x1F248484, 0x1F3494C0, 0x1F4490C1, 0x1F548AC2, 0x1F6486C3, 0x1F747400,
    0x1F847401, 0x1F947402, 0x1FA47403, 0x1FB47404, 0x1FC46B05, 0x1FD47440, 0x1FE47441, 0x1FF47442,
    0x20047043, 0x20146C44, 0x20246645, 0x20347480, 0x20447481, 0x20547482, 0x20647083, 0x20746C84,
    0x20846685, 0x209474C0, 0x20A473C1, 0x20B46FC2, 0x20C46BC3, 0x20D468C4, 0x20E464C5, 0x20FC7400,
    0x210C6501, 0x211C7440, 0x212C7480, 0x213C6EC0, 0x214C7410, 0x215C6511, 0x216C7450, 0x217C7490,
    0x218C6ED0, 0x219C7420, 0x21AC6521, 0x21BC7460, 0x21CC74A0, 0x21DC6EE0, 0x21EC74B0, 0x21FC6EF0,
    0x22039400, 0x22139401, 0x22239402, 0x22339403, 0x22439404, 0x22538B05, 0x22639440, 0x22739441,
    0x22839442, 0x22939043, 0x22A38C44, 0x22B38645, 0x22C39480, 0x22D39481, 0x22E39482, 0x22F39083,
    0x23038C84, 0x23138685, 0x232394C0, 0x233393C1, 0x23438FC2, 0x23538BC3, 0x236388C4, 0x237384C5,
    0x238B9400, 0x239B8501, 0x23AB9440, 0x23BB9480, 0x23CB8EC0, 0x23DB9410, 0x23EB8511, 0x23FB9450,
    0x240B9490, 0x241B8ED0, 0x242B9420, 0x243B8521, 0x244B9460, 0x245B94A0, 0x246B8EE0, 0x247B94B0,
    0x248B8EF0, 0x24937400, 0x24A37401, 0x24B37402, 0x24C37403, 0x24D37404, 0x24E37405, 0x24F37006,
    0x25036B07, 0x25137440, 0x25237441, 0x25337442, 0x25437443, 0x25537344, 0x25636E45, 0x25736946,
    0x25836647, 0x25937480, 0x25A37481, 0x25B37482, 0x25C37483, 0x25D37384, 0x25E36E85, 0x25F36986,
    0x26036687, 0x261374C0, 0x262374C1, 0x263373C2, 0x26436FC3, 0x26536DC4, 0x266369C5, 0x267366C6,
    0x268364C7, 0x269B7400, 0x26AB7101, 0x26BB7440, 0x26CB6A41, 0x26DB7480, 0x26EB6A81, 0x26FB72C0,
    0x270B67C1, 0x271B7410, 0x272B7111, 0x273B7450, 0x274B6A51, 0x275B7490, 0x276B6A91, 0x277B72D0,
    0x278B67D1, 0x279B7420, 0x27AB7121, 0x27BB7460, 0x27CB6A61, 0x27DB74A0, 0x27EB6AA1, 0x27FB72E0,
    0x280B67E1, 0x281B74B0, 0x282B6AB1, 0x283B72F0, 0x284B67F1, 0x28545400, 0x28645401, 0x28745402,
    0x28845403, 0x28945404, 0x28A45405, 0x28B45306, 0x28C44E07, 0x28D44708, 0x28E45440, 0x28F45441,
    0x29045442, 0x29145443, 0x29245444, 0x29344F45, 0x29444B46, 0x29544847, 0x29645480, 0x29745481,
    0x29845482, 0x29945483, 0x29A45484, 0x29B44F85, 0x29C44B86, 0x29D44887, 0x29E454C0, 0x29F454C1,
    0x2A0453C2, 0x2A1450C3, 0x2A244EC4, 0x2A344AC5, 0x2A4447C6, 0x2A5445C7, 0x2A6C5400, 0x2A7C5401,
    0x2A8C4502, 0x2A9C5440, 0x2AAC4C41, 0x2ABC5480, 0x2ACC4C81, 0x2ADC53C0, 0x2AEC48C1, 0x2AFC5410,
    0x2B0C5411, 0x2B1C4512, 0x2B2C5450, 0x2B3C4C51, 0x2B4C5490, 0x2B5C4C91, 0x2B6C53D0, 0x2B7C48D1,
    0x2B8C5420, 0x2B9C5421, 0x2BAC4522, 0x2BBC5460, 0x2BCC4C61, 0x2BDC54A0, 0x2BEC4CA1, 0x2BFC53E0,
    0x2C0C48E1, 0x2C1C54B0, 0x2C2C4CB1, 0x2C3C53F0, 0x2C4C48F1, 0x2C529400, 0x2C629401, 0x2C729402,
    0x2C829403, 0x2C929404, 0x2CA29405, 0x2CB29306, 0x2CC28E07, 0x2CD28708, 0x2CE29440, 0x2CF29441,
    0x2D029442, 0x2D129443, 0x2D229444, 0x2D328F45, 0x2D428B46, 0x2D528847, 0x2D629480, 0x2D729481,
    0x2D829482, 0x2D929483, 0x2DA29484, 0x2DB28F85, 0x2DC28B86, 0x2DD28887, 0x2DE294C0, 0x2DF294C1,
    0x2E0293C2, 0x2E1290C3, 0x2E228EC4, 0x2E328AC5, 0x2E4287C6, 0x2E5285C7, 0x2E6A9400, 0x2E7A9401,
    0x2E8A8502, 0x2E9A9440, 0x2EAA8C41, 0x2EBA9480, 0x2ECA8C81, 0x2EDA93C0, 0x2EEA88C1, 0x2EFA9410,
    0x2F0A9411, 0x2F1A8512, 0x2F2A9450, 0x2F3A8C51, 0x2F4A9490, 0x2F5A8C91, 0x2F6A93D0, 0x2F7A88D1,
    0x2F8A9420, 0x2F9A9421, 0x2FAA8522, 0x2FBA9460, 0x2FCA8C61, 0x2FDA94A0, 0x2FEA8CA1, 0x2FFA93E0,
    0x300A88E1, 0x301A94B0, 0x302A8CB1, 0x303A93F0, 0x304A88F1, 0x30535401, 0x30635402, 0x30735403,
    0x30835404, 0x30935405, 0x30A35406, 0x30B35407, 0x30C35308, 0x30D34E09, 0x30E34A0A, 0x30F35441,
    0x31035442, 0x31135443, 0x31235444, 0x31335445, 0x31435246, 0x31534F47, 0x31634B48, 0x31734849,
    0x3183454A, 0x31935481, 0x31A35482, 0x31B35483, 0x31C35484, 0x31D35485, 0x31E35286, 0x31F34F87,
    0x32034B88, 0x32134889, 0x3223458A, 0x323354C1, 0x324354C2, 0x325354C3, 0x326352C4, 0x32734FC5,
    0x32834CC6, 0x32934AC7, 0x32A347C8, 0x32B345C9, 0x32CB5400, 0x32DB5401, 0x32EB5102, 0x32FB4703,
    0x330B5440, 0x331B5241, 0x332B4A42, 0x333B5480, 0x334B5281, 0x335B4A82, 0x336B54C0, 0x337B4DC1,
    0x338B47C2, 0x339B5410, 0x33AB5411, 0x33BB5112, 0x33CB4713, 0x33DB5450, 0x33EB5251, 0x33FB4A52,
    0x340B5490, 0x341B5291, 0x342B4A92, 0x343B54D0, 0x344B4DD1, 0x345B47D2, 0x346B5420, 0x347B5421,
    0x348B5122, 0x349B4723, 0x34AB5460, 0x34BB5261, 0x34CB4A62, 0x34DB54A0, 0x34EB52A1, 0x34FB4AA2,
    0x350B54E0, 0x351B4DE1, 0x352B47E2, 0x353B54B0, 0x354B52B1, 0x355B4AB2, 0x356B54F0, 0x357B4DF1,
    0x358B47F2, 0x35927401, 0x35A27402, 0x35B27403, 0x35C27404, 0x35D27405, 0x35E27406, 0x35F27407,
    0x36027308, 0x36126E09, 0x36226A0A, 0x36327441, 0x36427442, 0x36527443, 0x36627444, 0x36727445,
    0x36827246, 0x36926F47, 0x36A26B48, 0x36B26849, 0x36C2654A, 0x36D27481, 0x36E27482, 0x36F27483,
    0x37027484, 0x37127485, 0x37227286, 0x37326F87, 0x37426B88, 0x37526889, 0x3762658A, 0x377274C1,
    0x378274C2, 0x379274C3, 0x37A272C4, 0x37B26FC5, 0x37C26CC6, 0x37D26AC7, 0x37E267C8, 0x37F265C9,
    0x380A7400, 0x381A7401, 0x382A7102, 0x383A6703, 0x384A7440, 0x385A7241, 0x386A6A42, 0x387A7480,
    0x388A7281, 0x389A6A82, 0x38AA74C0, 0x38BA6DC1, 0x38CA67C2, 0x38DA7410, 0x38EA7411, 0x38FA7112,
    0x390A6713, 0x391A7450, 0x392A7251, 0x393A6A52, 0x394A7490, 0x395A7291, 0x396A6A92, 0x397A74D0,
    0x398A6DD1, 0x399A67D2, 0x39AA7420, 0x39BA7421, 0x39CA7122, 0x39DA6723, 0x39EA7460, 0x39FA7261,
    0x3A0A6A62, 0x3A1A74A0, 0x3A2A72A1, 0x3A3A6AA2, 0x3A4A74E0, 0x3A5A6DE1, 0x3A6A67E2, 0x3A7A74B0,
    0x3A8A72B1, 0x3A9A6AB2, 0x3AAA74F0, 0x3ABA6DF1, 0x3ACA67F2, 0x3AD25401, 0x3AE25402, 0x3AF25403,
    0x3B025404, 0x3B125405, 0x3B225406, 0x3B325407, 0x3B425408, 0x3B525409, 0x3B62540A, 0x3B72530B,
    0x3B825441, 0x3B925442, 0x3BA25443, 0x3BB25444, 0x3BC25445, 0x3BD25446, 0x3BE25447, 0x3BF25348,
    0x3C025049, 0x3C124E4A, 0x3C224B4B, 0x3C325481, 0x3C425482, 0x3C525483, 0x3C625484, 0x3C725485,
    0x3C825486, 0x3C925487, 0x3CA25388, 0x3CB25089, 0x3CC24E8A, 0x3CD24B8B, 0x3CE254C1, 0x3CF254C2,
    0x3D0254C3, 0x3D1254C4, 0x3D2253C5, 0x3D3251C6, 0x3D4250C7, 0x3D524DC8, 0x3D624BC9, 0x3D724ACA,
    0x3D8247CB, 0x3D9A5400, 0x3DAA5401, 0x3DBA5402, 0x3DCA5403, 0x3DDA4E04, 0x3DEA4505, 0x3DFA5440,
    0x3E0A5441, 0x3E1A5242, 0x3E2A4D43, 0x3E3A4844, 0x3E4A5480, 0x3E5A5481, 0x3E6A5282, 0x3E7A4D83,
    0x3E8A4884, 0x3E9A54C0, 0x3EAA51C1, 0x3EBA4DC2, 0x3ECA48C3, 0x3EDA45C4, 0x3EEA5410, 0x3EFA5411,
    0x3F0A5412, 0x3F1A5413, 0x3F2A4E14, 0x3F3A4515, 0x3F4A5450, 0x3F5A5451, 0x3F6A5252, 0x3F7A4D53,
    0x3F8A4854, 0x3F9A5490, 0x3FAA5491, 0x3FBA5292, 0x3FCA4D93, 0x3FDA4894, 0x3FEA54D0, 0x3FFA51D1,
    0x400A4DD2, 0x401A48D3, 0x402A45D4, 0x403A5420, 0x404A5421, 0x405A5422, 0x406A5423, 0x407A4E24,
    0x408A4525, 0x409A5460, 0x40AA5461, 0x40BA5262, 0x40CA4D63, 0x40DA4864, 0x40EA54A0, 0x40FA54A1,
    0x410A52A2, 0x411A4DA3, 0x412A48A4, 0x413A54E0, 0x414A51E1, 0x415A4DE2, 0x416A48E3, 0x417A45E4,
    0x418A54B0, 0x419A54B1, 0x41AA52B2, 0x41BA4DB3, 0x41CA48B4, 0x41DA54F0, 0x41EA51F1, 0x41FA4DF2,
    0x420A48F3, 0x421A45F4, 0x42243401, 0x42343402, 0x42443403, 0x42543404, 0x42643405, 0x42743406,
    0x42843407, 0x42943408, 0x42A43409, 0x42B4310A, 0x42C42B0B, 0x42D43441, 0x42E43442, 0x42F43443,
    0x43043444, 0x43143445, 0x43243446, 0x43343347, 0x43442F48, 0x43542C49, 0x43642A4A, 0x4374264B,
    0x43843481, 0x43943482, 0x43A43483, 0x43B43484, 0x43C43485, 0x43D43486, 0x43E43387, 0x43F42F88,
    0x44042C89, 0x44142A8A, 0x4424268B, 0x443434C1, 0x444434C2, 0x445434C3, 0x446434C4, 0x447431C5,
    0x44842FC6, 0x44942DC7, 0x44A42AC8, 0x44B428C9, 0x44C426CA, 0x44D424CB, 0x44EC3400, 0x44FC3401,
    0x450C3402, 0x451C2E03, 0x452C2704, 0x453C3440, 0x454C3441, 0x455C2E42, 0x456C2843, 0x457C3480,
    0x458C3481, 0x459C2E82, 0x45AC2883, 0x45BC34C0, 0x45CC2FC1, 0x45DC2AC2, 0x45EC25C3, 0x45FC3410,
    0x460C3411, 0x461C3412, 0x462C2E13, 0x463C2714, 0x464C3450, 0x465C3451, 0x466C2E52, 0x467C2853,
    0x468C3490, 0x469C3491, 0x46AC2E92, 0x46BC2893, 0x46CC34D0, 0x46DC2FD1, 0x46EC2AD2, 0x46FC25D3,
    0x470C3420, 0x471C3421, 0x472C3422, 0x473C2E23, 0x474C2724, 0x475C3460, 0x476C3461, 0x477C2E62,
    0x478C2863, 0x479C34A0, 0x47AC34A1, 0x47BC2EA2, 0x47CC28A3, 0x47DC34E0, 0x47EC2FE1, 0x47FC2AE2,
    0x480C25E3, 0x481C34B0, 0x482C34B1, 0x483C2EB2, 0x484C28B3, 0x485C34F0, 0x486C2FF1, 0x487C2AF2,
    0x488C25F3, 0x48919401, 0x48A19402, 0x48B19403, 0x48C19404, 0x48D19405, 0x48E19406, 0x48F19407,
    0x49019408, 0x49119409, 0x4921910A, 0x49318B0B, 0x49419441, 0x49519442, 0x49619443, 0x49719444,
    0x49819445, 0x49919446, 0x49A19347, 0x49B18F48, 0x49C18C49, 0x49D18A4A, 0x49E1864B, 0x49F19481,
    0x4A019482, 0x4A119483, 0x4A219484, 0x4A319485, 0x4A419486, 0x4A519387, 0x4A618F88, 0x4A718C89,
    0x4A818A8A, 0x4A91868B, 0x4AA194C1, 0x4AB194C2, 0x4AC194C3, 0x4AD194C4, 0x4AE191C5, 0x4AF18FC6,
    0x4B018DC7, 0x4B118AC8, 0x4B2188C9, 0x4B3186CA, 0x4B4184CB, 0x4B599400, 0x4B699401, 0x4B799402,
    0x4B898E03, 0x4B998704, 0x4BA99440, 0x4BB99441, 0x4BC98E42, 0x4BD98843, 0x4BE99480, 0x4BF99481,
    0x4C098E82, 0x4C198883, 0x4C2994C0, 0x4C398FC1, 0x4C498AC2, 0x4C5985C3, 0x4C699410, 0x4C799411,
    0x4C899412, 0x4C998E13, 0x4CA98714, 0x4CB99450, 0x4CC99451, 0x4CD98E52, 0x4CE98853, 0x4CF99490,
    0x4D099491, 0x4D198E92, 0x4D298893, 0x4D3994D0, 0x4D498FD1, 0x4D598AD2, 0x4D6985D3, 0x4D799420,
    0x4D899421, 0x4D999422, 0x4DA98E23, 0x4DB98724, 0x4DC99460, 0x4DD99461, 0x4DE98E62, 0x4DF98863,
    0x4E0994A0, 0x4E1994A1, 0x4E298EA2, 0x4E3988A3, 0x4E4994E0, 0x4E598FE1, 0x4E698AE2, 0x4E7985E3,
    0x4E8994B0, 0x4E9994B1, 0x4EA98EB2, 0x4EB988B3, 0x4EC994F0, 0x4ED98FF1, 0x4EE98AF2, 0x4EF985F3,
    0x4F033401, 0x4F133402, 0x4F233403, 0x4F333404, 0x4F433405, 0x4F533406, 0x4F633407, 0x4F733408,
    0x4F833409, 0x4F93340A, 0x4FA3340B, 0x4FB33441, 0x4FC33442, 0x4FD33443, 0x4FE33444, 0x4FF33445,
    0x50033446, 0x50133447, 0x50233448, 0x50333349, 0x5043314A, 0x50532E4B, 0x50633481, 0x50733482,
    0x50833483, 0x50933484, 0x50A33485, 0x50B33486, 0x50C33487, 0x50D33488, 0x50E33389, 0x50F3318A,
    0x51032E8B, 0x511334C1, 0x512334C2, 0x513334C3, 0x514334C4, 0x515334C5, 0x516333C6, 0x517331C7,
    0x51832FC8, 0x51932DC9, 0x51A32BCA, 0x51B329CB, 0x51CB3400, 0x51DB3401, 0x51EB3402, 0x51FB3403,
    0x520B3304, 0x521B2A05, 0x522B3440, 0x523B3441, 0x524B3442, 0x525B2F43, 0x526B2B44, 0x527B2545,
    0x528B3480, 0x529B3481, 0x52AB3482, 0x52BB2F83, 0x52CB2B84, 0x52DB2585, 0x52EB34C0, 0x52FB33C1,
    0x530B2EC2, 0x531B2AC3, 0x532B27C4, 0x533B3410, 0x534B3411, 0x535B3412, 0x536B3413, 0x537B3314,
    0x538B2A15, 0x539B3450, 0x53AB3451, 0x53BB3452, 0x53CB2F53, 0x53DB2B54, 0x53EB2555, 0x53FB3490,
    0x540B3491, 0x541B3492, 0x542B2F93, 0x543B2B94, 0x544B2595, 0x545B34D0, 0x546B33D1, 0x547B2ED2,
    0x548B2AD3, 0x549B27D4, 0x54AB3420, 0x54BB3421, 0x54CB3422, 0x54DB3423, 0x54EB3324, 0x54FB2A25,
    0x550B3460, 0x551B3461, 0x552B3462, 0x553B2F63, 0x554B2B64, 0x555B2565, 0x556B34A0, 0x557B34A1,
    0x558B34A2, 0x559B2FA3, 0x55AB2BA4, 0x55BB25A5, 0x55CB34E0, 0x55DB33E1, 0x55EB2EE2, 0x55FB2AE3,
    0x560B27E4, 0x561B34B0, 0x562B34B1, 0x563B34B2, 0x564B2FB3, 0x565B2BB4, 0x566B25B5, 0x567B34F0,
    0x568B33F1, 0x569B2EF2, 0x56AB2AF3, 0x56BB27F4, 0x56C17401, 0x56D17402, 0x56E17403, 0x56F17404,
    0x57017405, 0x57117406, 0x57217407, 0x57317408, 0x57417409, 0x5751740A, 0x5761740B, 0x57717441,
    0x57817442, 0x57917443, 0x57A17444, 0x57B17445, 0x57C17446, 0x57D17447, 0x57E17448, 0x57F17349,
    0x5801714A, 0x58116E4B, 0x58217481, 0x58317482, 0x58417483, 0x58517484, 0x58617485, 0x58717486,
    0x58817487, 0x58917488, 0x58A17389, 0x58B1718A, 0x58C16E8B, 0x58D174C1, 0x58E174C2, 0x58F174C3,
    0x590174C4, 0x591174C5, 0x592173C6, 0x593171C7, 0x59416FC8, 0x59516DC9, 0x59616BCA, 0x597169CB,
    0x59897400, 0x59997401, 0x59A97402, 0x59B97403, 0x59C97304, 0x59D96A05, 0x59E97440, 0x59F97441,
    0x5A097442, 0x5A196F43, 0x5A296B44, 0x5A396545, 0x5A497480, 0x5A597481, 0x5A697482, 0x5A796F83,
    0x5A896B84, 0x5A996585, 0x5AA974C0, 0x5AB973C1, 0x5AC96EC2, 0x5AD96AC3, 0x5AE967C4, 0x5AF97410,
    0x5B097411, 0x5B197412, 0x5B297413, 0x5B397314, 0x5B496A15, 0x5B597450, 0x5B697451, 0x5B797452,
    0x5B896F53, 0x5B996B54, 0x5BA96555, 0x5BB97490, 0x5BC97491, 0x5BD97492, 0x5BE96F93, 0x5BF96B94,
    0x5C096595, 0x5C1974D0, 0x5C2973D1, 0x5C396ED2, 0x5C496AD3, 0x5C5967D4, 0x5C697420, 0x5C797421,
    0x5C897422, 0x5C997423, 0x5CA97324, 0x5CB96A25, 0x5CC97460, 0x5CD97461, 0x5CE97462, 0x5CF96F63,
    0x5D096B64, 0x5D196565, 0x5D2974A0, 0x5D3974A1, 0x5D4974A2, 0x5D596FA3, 0x5D696BA4, 0x5D7965A5,
    0x5D8974E0, 0x5D9973E1, 0x5DA96EE2, 0x5DB96AE3, 0x5DC967E4, 0x5DD974B0, 0x5DE974B1, 0x5DF974B2,
    0x5E096FB3, 0x5E196BB4, 0x5E2965B5, 0x5E3974F0, 0x5E4973F1, 0x5E596EF2, 0x5E696AF3, 0x5E7967F4,
    0x5E823402, 0x5E923403, 0x5EA23404, 0x5EB23405, 0x5EC23406, 0x5ED23407, 0x5EE23408, 0x5EF23409,
    0x5F02340A, 0x5F12340B, 0x5F223442, 0x5F323443, 0x5F423444, 0x5F523445, 0x5F623446, 0x5F723447,
    0x5F823448, 0x5F923449, 0x5FA2344A, 0x5FB2344B, 0x5FC23482, 0x5FD23483, 0x5FE23484, 0x5FF23485,
    0x60023486, 0x60123487, 0x60223488, 0x60323489, 0x6042348A, 0x6052348B, 0x606234C2, 0x607234C3,
    0x608234C4, 0x609234C5, 0x60A234C6, 0x60B234C7, 0x60C233C8, 0x60D232C9, 0x60E230CA, 0x60F22FCB,
    0x610A3400, 0x611A3401, 0x612A3402, 0x613A3403, 0x614A3404, 0x615A3405, 0x616A3106, 0x617A2C07,
    0x618A2508, 0x619A3440, 0x61AA3441, 0x61BA3442, 0x61CA3443, 0x61DA3344, 0x61EA2E45, 0x61FA2A46,
    0x620A2747, 0x621A3480, 0x622A3481, 0x623A3482, 0x624A3483, 0x625A3384, 0x626A2E85, 0x627A2A86,
    0x628A2787, 0x629A34C0, 0x62AA34C1, 0x62BA33C2, 0x62CA30C3, 0x62DA2DC4, 0x62EA2AC5, 0x62FA27C6,
    0x630A24C7, 0x631A3410, 0x632A3411, 0x633A3412, 0x634A3413, 0x635A3414, 0x636A3415, 0x637A3116,
    0x638A2C17, 0x639A2518, 0x63AA3450, 0x63BA3451, 0x63CA3452, 0x63DA3453, 0x63EA3354, 0x63FA2E55,
    0x640A2A56, 0x641A2757, 0x642A3490, 0x643A3491, 0x644A3492, 0x645A3493, 0x646A3394, 0x647A2E95,
    0x648A2A96, 0x649A2797, 0x64AA34D0, 0x64BA34D1, 0x64CA33D2, 0x64DA30D3, 0x64EA2DD4, 0x64FA2AD5,
    0x650A27D6, 0x651A24D7, 0x652A3420, 0x653A3421, 0x654A3422, 0x655A3423, 0x656A3424, 0x657A3425,
    0x658A3126, 0x659A2C27, 0x65AA2528, 0x65BA3460, 0x65CA3461, 0x65DA3462, 0x65EA3463, 0x65FA3364,
    0x660A2E65, 0x661A2A66, 0x662A2767, 0x663A34A0, 0x664A34A1, 0x665A34A2, 0x666A34A3, 0x667A33A4,
    0x668A2EA5, 0x669A2AA6, 0x66AA27A7, 0x66BA34E0, 0x66CA34E1, 0x66DA33E2, 0x66EA30E3, 0x66FA2DE4,
    0x670A2AE5, 0x671A27E6, 0x672A24E7, 0x673A34B0, 0x674A34B1, 0x675A34B2, 0x676A34B3, 0x677A33B4,
    0x678A2EB5, 0x679A2AB6, 0x67AA27B7, 0x67BA34F0, 0x67CA34F1, 0x67DA33F2, 0x67EA30F3, 0x67FA2DF4,
    0x680A2AF5, 0x681A27F6, 0x682A24F7, 0x68315402, 0x68415403, 0x68515404, 0x68615405, 0x68715406,
    0x68815407, 0x68915408, 0x68A15409, 0x68B1540A, 0x68C1540B, 0x68D15442, 0x68E15443, 0x68F15444,
    0x69015445, 0x69115446, 0x69215447, 0x69315448, 0x69415449, 0x6951544A, 0x6961544B, 0x69715482,
    0x69815483, 0x69915484, 0x69A15485, 0x69B15486, 0x69C15487, 0x69D15488, 0x69E15489, 0x69F1548A,
    0x6A01548B, 0x6A1154C2, 0x6A2154C3, 0x6A3154C4, 0x6A4154C5, 0x6A5154C6, 0x6A6154C7, 0x6A7153C8,
    0x6A8152C9, 0x6A9150CA, 0x6AA14FCB, 0x6AB95400, 0x6AC95401, 0x6AD95402, 0x6AE95403, 0x6AF95404,
    0x6B095405, 0x6B195106, 0x6B294C07, 0x6B394508, 0x6B495440, 0x6B595441, 0x6B695442, 0x6B795443,
    0x6B895344, 0x6B994E45, 0x6BA94A46, 0x6BB94747, 0x6BC95480, 0x6BD95481, 0x6BE95482, 0x6BF95483,
    0x6C095384, 0x6C194E85, 0x6C294A86, 0x6C394787, 0x6C4954C0, 0x6C5954C1, 0x6C6953C2, 0x6C7950C3,
    0x6C894DC4, 0x6C994AC5, 0x6CA947C6, 0x6CB944C7, 0x6CC95410, 0x6CD95411, 0x6CE95412, 0x6CF95413,
    0x6D095414, 0x6D195415, 0x6D295116, 0x6D394C17, 0x6D494518, 0x6D595450, 0x6D695451, 0x6D795452,
    0x6D895453, 0x6D995354, 0x6DA94E55, 0x6DB94A56, 0x6DC94757, 0x6DD95490, 0x6DE95491, 0x6DF95492,
    0x6E095493, 0x6E195394, 0x6E294E95, 0x6E394A96, 0x6E494797, 0x6E5954D0, 0x6E6954D1, 0x6E7953D2,
    0x6E8950D3, 0x6E994DD4, 0x6EA94AD5, 0x6EB947D6, 0x6EC944D7, 0x6ED95420, 0x6EE95421, 0x6EF95422,
    0x6F095423, 0x6F195424, 0x6F295425, 0x6F395126, 0x6F494C27, 0x6F594528, 0x6F695460, 0x6F795461,
    0x6F895462, 0x6F995463, 0x6FA95364, 0x6FB94E65, 0x6FC94A66, 0x6FD94767, 0x6FE954A0, 0x6FF954A1,
    0x700954A2, 0x701954A3, 0x702953A4, 0x70394EA5, 0x70494AA6, 0x705947A7, 0x706954E0, 0x707954E1,
    0x708953E2, 0x709950E3, 0x70A94DE4, 0x70B94AE5, 0x70C947E6, 0x70D944E7, 0x70E954B0, 0x70F954B1,
    0x710954B2, 0x711954B3, 0x712953B4, 0x71394EB5, 0x71494AB6, 0x715947B7, 0x716954F0, 0x717954F1,
    0x718953F2, 0x719950F3, 0x71A94DF4, 0x71B94AF5, 0x71C947F6, 0x71D944F7, 0x71E13404, 0x71F13405,
    0x72013406, 0x72113407, 0x72213408, 0x72313409, 0x7241340A, 0x7251340B, 0x72613444, 0x72713445,
    0x72813446, 0x72913447, 0x72A13448, 0x72B13449, 0x72C1344A, 0x72D1344B, 0x72E13484, 0x72F13485,
    0x73013486, 0x73113487, 0x73213488, 0x73313489, 0x7341348A, 0x7351348B, 0x736134C4, 0x737134C5,
    0x738134C6, 0x739134C7, 0x73A134C8, 0x73B134C9, 0x73C134CA, 0x73D134CB, 0x73E93401, 0x73F93402,
    0x74093403, 0x74193404, 0x74293405, 0x74393406, 0x74493407, 0x74593408, 0x74693309, 0x74792F0A,
    0x74892A0B, 0x74993441, 0x74A93442, 0x74B93443, 0x74C93444, 0x74D93445, 0x74E93446, 0x74F93247,
    0x75092E48, 0x75192B49, 0x7529294A, 0x7539254B, 0x75493481, 0x75593482, 0x75693483, 0x75793484,
    0x75893485, 0x75993486, 0x75A93287, 0x75B92E88, 0x75C92B89, 0x75D9298A, 0x75E9258B, 0x75F934C1,
    0x760934C2, 0x761934C3, 0x762933C4, 0x763930C5, 0x76492EC6, 0x76592CC7, 0x76692AC8, 0x767927C9,
    0x768925CA, 0x76993411, 0x76A93412, 0x76B93413, 0x76C93414, 0x76D93415, 0x76E93416, 0x76F93417,
    0x77093418, 0x77193319, 0x77292F1A, 0x77392A1B, 0x77493451, 0x77593452, 0x77693453, 0x77793454,
    0x77893455, 0x77993456, 0x77A93257, 0x77B92E58, 0x77C92B59, 0x77D9295A, 0x77E9255B, 0x77F93491,
    0x78093492, 0x78193493, 0x78293494, 0x78393495, 0x78493496, 0x78593297, 0x78692E98, 0x78792B99,
    0x7889299A, 0x7899259B, 0x78A934D1, 0x78B934D2, 0x78C934D3, 0x78D933D4, 0x78E930D5, 0x78F92ED6,
    0x79092CD7, 0x79192AD8, 0x792927D9, 0x793925DA, 0x79493421, 0x79593422, 0x79693423, 0x79793424,
    0x79893425, 0x79993426, 0x79A93427, 0x79B93428, 0x79C93329, 0x79D92F2A, 0x79E92A2B, 0x79F93461,
    0x7A093462, 0x7A193463, 0x7A293464, 0x7A393465, 0x7A493466, 0x7A593267, 0x7A692E68, 0x7A792B69,
    0x7A89296A, 0x7A99256B, 0x7AA934A1, 0x7AB934A2, 0x7AC934A3, 0x7AD934A4, 0x7AE934A5, 0x7AF934A6,
    0x7B0932A7, 0x7B192EA8, 0x7B292BA9, 0x7B3929AA, 0x7B4925AB, 0x7B5934E1, 0x7B6934E2, 0x7B7934E3,
    0x7B8933E4, 0x7B9930E5, 0x7BA92EE6, 0x7BB92CE7, 0x7BC92AE8, 0x7BD927E9, 0x7BE925EA, 0x7BF934B1,
    0x7C0934B2, 0x7C1934B3, 0x7C2934B4, 0x7C3934B5, 0x7C4934B6, 0x7C5932B7, 0x7C692EB8, 0x7C792BB9,
    0x7C8929BA, 0x7C9925BB, 0x7CA934F1, 0x7CB934F2, 0x7CC934F3, 0x7CD933F4, 0x7CE930F5, 0x7CF92EF6,
    0x7D092CF7, 0x7D192AF8, 0x7D2927F9, 0x7D3925FA, 0x7D441402, 0x7D541403, 0x7D641404, 0x7D741405,
    0x7D841406, 0x7D941407, 0x7DA41408, 0x7DB41409, 0x7DC4140A, 0x7DD4140B, 0x7DE41442, 0x7DF41443,
    0x7E041444, 0x7E141445, 0x7E241446, 0x7E341447, 0x7E441448, 0x7E541449, 0x7E64144A, 0x7E74144B,
    0x7E841482, 0x7E941483, 0x7EA41484, 0x7EB41485, 0x7EC41486, 0x7ED41487, 0x7EE41488, 0x7EF41489,
    0x7F04148A, 0x7F14148B, 0x7F2414C2, 0x7F3414C3, 0x7F4414C4, 0x7F5414C5, 0x7F6414C6, 0x7F7414C7,
    0x7F8413C8, 0x7F9412C9, 0x7FA410CA, 0x7FB40FCB, 0x7FCC1400, 0x7FDC1401, 0x7FEC1402, 0x7FFC1403,
    0x800C1404, 0x801C1405, 0x802C1106, 0x803C0C07, 0x804C0508, 0x805C1440, 0x806C1441, 0x807C1442,
    0x808C1443, 0x809C1344, 0x80AC0E45, 0x80BC0A46, 0x80CC0747, 0x80DC1480, 0x80EC1481, 0x80FC1482,
    0x810C1483, 0x811C1384, 0x812C0E85, 0x813C0A86, 0x814C0787, 0x815C14C0, 0x816C14C1, 0x817C13C2,
    0x818C10C3, 0x819C0DC4, 0x81AC0AC5, 0x81BC07C6, 0x81CC04C7, 0x81DC1410, 0x81EC1411, 0x81FC1412,
    0x820C1413, 0x821C1414, 0x822C1415, 0x823C1116, 0x824C0C17, 0x825C0518, 0x826C1450, 0x827C1451,
    0x828C1452, 0x829C1453, 0x82AC1354, 0x82BC0E55, 0x82CC0A56, 0x82DC0757, 0x82EC1490, 0x82FC1491,
    0x830C1492, 0x831C1493, 0x832C1394, 0x833C0E95, 0x834C0A96, 0x835C0797, 0x836C14D0, 0x837C14D1,
    0x838C13D2, 0x839C10D3, 0x83AC0DD4, 0x83BC0AD5, 0x83CC07D6, 0x83DC04D7, 0x83EC1420, 0x83FC1421,
    0x840C1422, 0x841C1423, 0x842C1424, 0x843C1425, 0x844C1126, 0x845C0C27, 0x846C0528, 0x847C1460,
    0x848C1461, 0x849C1462, 0x84AC1463, 0x84BC1364, 0x84CC0E65, 0x84DC0A66, 0x84EC0767, 0x84FC14A0,
    0x850C14A1, 0x851C14A2, 0x852C14A3, 0x853C13A4, 0x854C0EA5, 0x855C0AA6, 0x856C07A7, 0x857C14E0,
    0x858C14E1, 0x859C13E2, 0x85AC10E3, 0x85BC0DE4, 0x85CC0AE5, 0x85DC07E6, 0x85EC04E7, 0x85FC14B0,
    0x860C14B1, 0x861C14B2, 0x862C14B3, 0x863C13B4, 0x864C0EB5, 0x865C0AB6, 0x866C07B7, 0x867C14F0,
    0x868C14F1, 0x869C13F2, 0x86AC10F3, 0x86BC0DF4, 0x86CC0AF5, 0x86DC07F6, 0x86EC04F7, 0x86F09402,
    0x87009403, 0x87109404, 0x87209405, 0x87309406, 0x87409407, 0x87509408, 0x87609409, 0x8770940A,
    0x8780940B, 0x87909442, 0x87A09443, 0x87B09444, 0x87C09445, 0x87D09446, 0x87E09447, 0x87F09448,
    0x88009449, 0x8810944A, 0x8820944B, 0x88309482, 0x88409483, 0x88509484, 0x88609485, 0x88709486,
    0x88809487, 0x88909488, 0x88A09489, 0x88B0948A, 0x88C0948B, 0x88D094C2, 0x88E094C3, 0x88F094C4,
    0x890094C5, 0x891094C6, 0x892094C7, 0x893093C8, 0x894092C9, 0x895090CA, 0x89608FCB, 0x89789400,
    0x89889401, 0x89989402, 0x89A89403, 0x89B89404, 0x89C89405, 0x89D89106, 0x89E88C07, 0x89F88508,
    0x8A089440, 0x8A189441, 0x8A289442, 0x8A389443, 0x8A489344, 0x8A588E45, 0x8A688A46, 0x8A788747,
    0x8A889480, 0x8A989481, 0x8AA89482, 0x8AB89483, 0x8AC89384, 0x8AD88E85, 0x8AE88A86, 0x8AF88787,
    0x8B0894C0, 0x8B1894C1, 0x8B2893C2, 0x8B3890C3, 0x8B488DC4, 0x8B588AC5, 0x8B6887C6, 0x8B7884C7,
    0x8B889410, 0x8B989411, 0x8BA89412, 0x8BB89413, 0x8BC89414, 0x8BD89415, 0x8BE89116, 0x8BF88C17,
    0x8C088518, 0x8C189450, 0x8C289451, 0x8C389452, 0x8C489453, 0x8C589354, 0x8C688E55, 0x8C788A56,
    0x8C888757, 0x8C989490, 0x8CA89491, 0x8CB89492, 0x8CC89493, 0x8CD89394, 0x8CE88E95, 0x8CF88A96,
    0x8D088797, 0x8D1894D0, 0x8D2894D1, 0x8D3893D2, 0x8D4890D3, 0x8D588DD4, 0x8D688AD5, 0x8D7887D6,
    0x8D8884D7, 0x8D989420, 0x8DA89421, 0x8DB89422, 0x8DC89423, 0x8DD89424, 0x8DE89425, 0x8DF89126,
    0x8E088C27, 0x8E188528, 0x8E289460, 0x8E389461, 0x8E489462, 0x8E589463, 0x8E689364, 0x8E788E65,
    0x8E888A66, 0x8E988767, 0x8EA894A0, 0x8EB894A1, 0x8EC894A2, 0x8ED894A3, 0x8EE893A4, 0x8EF88EA5,
    0x8F088AA6, 0x8F1887A7, 0x8F2894E0, 0x8F3894E1, 0x8F4893E2, 0x8F5890E3, 0x8F688DE4, 0x8F788AE5,
    0x8F8887E6, 0x8F9884E7, 0x8FA894B0, 0x8FB894B1, 0x8FC894B2, 0x8FD894B3, 0x8FE893B4, 0x8FF88EB5,
    0x90088AB6, 0x901887B7, 0x902894F0, 0x903894F1, 0x904893F2, 0x905890F3, 0x90688DF4, 0x90788AF5,
    0x908887F6, 0x909884F7, 0x90A31403, 0x90B31404, 0x90C31405, 0x90D31406, 0x90E31407, 0x90F31408,
    0x91031409, 0x9113140A, 0x9123140B, 0x91331443, 0x91431444, 0x91531445, 0x91631446, 0x91731447,
    0x91831448, 0x91931449, 0x91A3144A, 0x91B3144B, 0x91C31483, 0x91D31484, 0x91E31485, 0x91F31486,
    0x92031487, 0x92131488, 0x92231489, 0x9233148A, 0x9243148B, 0x925314C3, 0x926314C4, 0x927314C5,
    0x928314C6, 0x929314C7, 0x92A314C8, 0x92B314C9, 0x92C314CA, 0x92D313CB, 0x92EB1401, 0x92FB1402,
    0x930B1403, 0x931B1404, 0x932B1405, 0x933B1406, 0x934B1407, 0x935B1108, 0x936B0C09, 0x937B080A,
    0x938B1441, 0x939B1442, 0x93AB1443, 0x93BB1444, 0x93CB1445, 0x93DB1146, 0x93EB0E47, 0x93FB0A48,
    0x940B0749, 0x941B044A, 0x942B1481, 0x943B1482, 0x944B1483, 0x945B1484, 0x946B1485, 0x947B1186,
    0x948B0E87, 0x949B0A88, 0x94AB0789, 0x94BB048A, 0x94CB14C1, 0x94DB14C2, 0x94EB13C3, 0x94FB11C4,
    0x950B0EC5, 0x951B0BC6, 0x952B0AC7, 0x953B07C8, 0x954B04C9, 0x955B1411, 0x956B1412, 0x957B1413,
    0x958B1414, 0x959B1415, 0x95AB1416, 0x95BB1417, 0x95CB1118, 0x95DB0C19, 0x95EB081A, 0x95FB1451,
    0x960B1452, 0x961B1453, 0x962B1454, 0x963B1455, 0x964B1156, 0x965B0E57, 0x966B0A58, 0x967B0759,
    0x968B045A, 0x969B1491, 0x96AB1492, 0x96BB1493, 0x96CB1494, 0x96DB1495, 0x96EB1196, 0x96FB0E97,
    0x970B0A98, 0x971B0799, 0x972B049A, 0x973B14D1, 0x974B14D2, 0x975B13D3, 0x976B11D4, 0x977B0ED5,
    0x978B0BD6, 0x979B0AD7, 0x97AB07D8, 0x97BB04D9, 0x97CB1421, 0x97DB1422, 0x97EB1423, 0x97FB1424,
    0x980B1425, 0x981B1426, 0x982B1427, 0x983B1128, 0x984B0C29, 0x985B082A, 0x986B1461, 0x987B1462,
    0x988B1463, 0x989B1464, 0x98AB1465, 0x98BB1166, 0x98CB0E67, 0x98DB0A68, 0x98EB0769, 0x98FB046A,
    0x990B14A1, 0x991B14A2, 0x992B14A3, 0x993B14A4, 0x994B14A5, 0x995B11A6, 0x996B0EA7, 0x997B0AA8,
    0x998B07A9, 0x999B04AA, 0x99AB14E1, 0x99BB14E2, 0x99CB13E3, 0x99DB11E4, 0x99EB0EE5, 0x99FB0BE6,
    0x9A0B0AE7, 0x9A1B07E8, 0x9A2B04E9, 0x9A3B14B1, 0x9A4B14B2, 0x9A5B14B3, 0x9A6B14B4, 0x9A7B14B5,
    0x9A8B11B6, 0x9A9B0EB7, 0x9AAB0AB8, 0x9ABB07B9, 0x9ACB04BA, 0x9ADB14F1, 0x9AEB14F2, 0x9AFB13F3,
    0x9B0B11F4, 0x9B1B0EF5, 0x9B2B0BF6, 0x9B3B0AF7, 0x9B4B07F8, 0x9B5B04F9, 0x9B607403, 0x9B707404,
    0x9B807405, 0x9B907406, 0x9BA07407, 0x9BB07408, 0x9BC07409, 0x9BD0740A, 0x9BE0740B, 0x9BF07443,
    0x9C007444, 0x9C107445, 0x9C207446, 0x9C307447, 0x9C407448, 0x9C507449, 0x9C60744A, 0x9C70744B,
    0x9C807483, 0x9C907484, 0x9CA07485, 0x9CB07486, 0x9CC07487, 0x9CD07488, 0x9CE07489, 0x9CF0748A,
    0x9D00748B, 0x9D1074C3, 0x9D2074C4, 0x9D3074C5, 0x9D4074C6, 0x9D5074C7, 0x9D6074C8, 0x9D7074C9,
    0x9D8074CA, 0x9D9073CB, 0x9DA87401, 0x9DB87402, 0x9DC87403, 0x9DD87404, 0x9DE87405, 0x9DF87406,
    0x9E087407, 0x9E187108, 0x9E286C09, 0x9E38680A, 0x9E487441, 0x9E587442, 0x9E687443, 0x9E787444,
    0x9E887445, 0x9E987146, 0x9EA86E47, 0x9EB86A48, 0x9EC86749, 0x9ED8644A, 0x9EE87481, 0x9EF87482,
    0x9F087483, 0x9F187484, 0x9F287485, 0x9F387186, 0x9F486E87, 0x9F586A88, 0x9F686789, 0x9F78648A,
    0x9F8874C1, 0x9F9874C2, 0x9FA873C3, 0x9FB871C4, 0x9FC86EC5, 0x9FD86BC6, 0x9FE86AC7, 0x9FF867C8,
    0xA00864C9, 0xA0187411, 0xA0287412, 0xA0387413, 0xA0487414, 0xA0587415, 0xA0687416, 0xA0787417,
    0xA0887118, 0xA0986C19, 0xA0A8681A, 0xA0B87451, 0xA0C87452, 0xA0D87453, 0xA0E87454, 0xA0F87455,
    0xA1087156, 0xA1186E57, 0xA1286A58, 0xA1386759, 0xA148645A, 0xA1587491, 0xA1687492, 0xA1787493,
    0xA1887494, 0xA1987495, 0xA1A87196, 0xA1B86E97, 0xA1C86A98, 0xA1D86799, 0xA1E8649A, 0xA1F874D1,
    0xA20874D2, 0xA21873D3, 0xA22871D4, 0xA2386ED5, 0xA2486BD6, 0xA2586AD7, 0xA26867D8, 0xA27864D9,
    0xA2887421, 0xA2987422, 0xA2A87423, 0xA2B87424, 0xA2C87425, 0xA2D87426, 0xA2E87427, 0xA2F87128,
    0xA3086C29, 0xA318682A, 0xA3287461, 0xA3387462, 0xA3487463, 0xA3587464, 0xA3687465, 0xA3787166,
    0xA3886E67, 0xA3986A68, 0xA3A86769, 0xA3B8646A, 0xA3C874A1, 0xA3D874A2, 0xA3E874A3, 0xA3F874A4,
    0xA40874A5, 0xA41871A6, 0xA4286EA7, 0xA4386AA8, 0xA44867A9, 0xA45864AA, 0xA46874E1, 0xA47874E2,
    0xA48873E3, 0xA49871E4, 0xA4A86EE5, 0xA4B86BE6, 0xA4C86AE7, 0xA4D867E8, 0xA4E864E9, 0xA4F874B1,
    0xA50874B2, 0xA51874B3, 0xA52874B4, 0xA53874B5, 0xA54871B6, 0xA5586EB7, 0xA5686AB8, 0xA57867B9,
    0xA58864BA, 0xA59874F1, 0xA5A874F2, 0xA5B873F3, 0xA5C871F4, 0xA5D86EF5, 0xA5E86BF6, 0xA5F86AF7,
    0xA60867F8, 0xA61864F9, 0xA6221405, 0xA6321406, 0xA6421407, 0xA6521408, 0xA6621409, 0xA672140A,
    0xA682140B, 0xA6921445, 0xA6A21446, 0xA6B21447, 0xA6C21448, 0xA6D21449, 0xA6E2144A, 0xA6F2144B,
    0xA7021485, 0xA7121486, 0xA7221487, 0xA7321488, 0xA7421489, 0xA752148A, 0xA762148B, 0xA77214C5,
    0xA78214C6, 0xA79214C7, 0xA7A214C8, 0xA7B214C9, 0xA7C214CA, 0xA7D214CB, 0xA7EA1401, 0xA7FA1402,
    0xA80A1403, 0xA81A1404, 0xA82A1405, 0xA83A1406, 0xA84A1407, 0xA85A1408, 0xA86A1409, 0xA87A140A,
    0xA88A110B, 0xA89A1441, 0xA8AA1442, 0xA8BA1443, 0xA8CA1444, 0xA8DA1445, 0xA8EA1446, 0xA8FA1447,
    0xA90A1248, 0xA91A0F49, 0xA92A0D4A, 0xA93A0A4B, 0xA94A1481, 0xA95A1482, 0xA96A1483, 0xA97A1484,
    0xA98A1485, 0xA99A1486, 0xA9AA1487, 0xA9BA1288, 0xA9CA0F89, 0xA9DA0D8A, 0xA9EA0A8B, 0xA9FA14C1,
    0xAA0A14C2, 0xAA1A14C3, 0xAA2A14C4, 0xAA3A13C5, 0xAA4A10C6, 0xAA5A0FC7, 0xAA6A0DC8, 0xAA7A0AC9,
    0xAA8A09CA, 0xAA9A07CB, 0xAAAA1411, 0xAABA1412, 0xAACA1413, 0xAADA1414, 0xAAEA1415, 0xAAFA1416,
    0xAB0A1417, 0xAB1A1418, 0xAB2A1419, 0xAB3A141A, 0xAB4A111B, 0xAB5A1451, 0xAB6A1452, 0xAB7A1453,
    0xAB8A1454, 0xAB9A1455, 0xABAA1456, 0xABBA1457, 0xABCA1258, 0xABDA0F59, 0xABEA0D5A, 0xABFA0A5B,
    0xAC0A1491, 0xAC1A1492, 0xAC2A1493, 0xAC3A1494, 0xAC4A1495, 0xAC5A1496, 0xAC6A1497, 0xAC7A1298,
    0xAC8A0F99, 0xAC9A0D9A, 0xACAA0A9B, 0xACBA14D1, 0xACCA14D2, 0xACDA14D3, 0xACEA14D4, 0xACFA13D5,
    0xAD0A10D6, 0xAD1A0FD7, 0xAD2A0DD8, 0xAD3A0AD9, 0xAD4A09DA, 0xAD5A07DB, 0xAD6A1421, 0xAD7A1422,
    0xAD8A1423, 0xAD9A1424, 0xADAA1425, 0xADBA1426, 0xADCA1427, 0xADDA1428, 0xADEA1429, 0xADFA142A,
    0xAE0A112B, 0xAE1A1461, 0xAE2A1462, 0xAE3A1463, 0xAE4A1464, 0xAE5A1465, 0xAE6A1466, 0xAE7A1467,
    0xAE8A1268, 0xAE9A0F69, 0xAEAA0D6A, 0xAEBA0A6B, 0xAECA14A1, 0xAEDA14A2, 0xAEEA14A3, 0xAEFA14A4,
    0xAF0A14A5, 0xAF1A14A6, 0xAF2A14A7, 0xAF3A12A8, 0xAF4A0FA9, 0xAF5A0DAA, 0xAF6A0AAB, 0xAF7A14E1,
    0xAF8A14E2, 0xAF9A14E3, 0xAFAA14E4, 0xAFBA13E5, 0xAFCA10E6, 0xAFDA0FE7, 0xAFEA0DE8, 0xAFFA0AE9,
    0xB00A09EA, 0xB01A07EB, 0xB02A14B1, 0xB03A14B2, 0xB04A14B3, 0xB05A14B4, 0xB06A14B5, 0xB07A14B6,
    0xB08A14B7, 0xB09A12B8, 0xB0AA0FB9, 0xB0BA0DBA, 0xB0CA0ABB, 0xB0DA14F1, 0xB0EA14F2, 0xB0FA14F3,
    0xB10A14F4, 0xB11A13F5, 0xB12A10F6, 0xB13A0FF7, 0xB14A0DF8, 0xB15A0AF9, 0xB16A09FA, 0xB17A07FB,
    0xB1805405, 0xB1905406, 0xB1A05407, 0xB1B05408, 0xB1C05409, 0xB1D0540A, 0xB1E0540B, 0xB1F05445,
    0xB2005446, 0xB2105447, 0xB2205448, 0xB2305449, 0xB240544A, 0xB250544B, 0xB2605485, 0xB2705486,
    0xB2805487, 0xB2905488, 0xB2A05489, 0xB2B0548A, 0xB2C0548B, 0xB2D054C5, 0xB2E054C6, 0xB2F054C7,
    0xB30054C8, 0xB31054C9, 0xB32054CA, 0xB33054CB, 0xB3485401, 0xB3585402, 0xB3685403, 0xB3785404,
    0xB3885405, 0xB3985406, 0xB3A85407, 0xB3B85408, 0xB3C85409, 0xB3D8540A, 0xB3E8510B, 0xB3F85441,
    0xB4085442, 0xB4185443, 0xB4285444, 0xB4385445, 0xB4485446, 0xB4585447, 0xB4685248, 0xB4784F49,
    0xB4884D4A, 0xB4984A4B, 0xB4A85481, 0xB4B85482, 0xB4C85483, 0xB4D85484, 0xB4E85485, 0xB4F85486,
    0xB5085487, 0xB5185288, 0xB5284F89, 0xB5384D8A, 0xB5484A8B, 0xB55854C1, 0xB56854C2, 0xB57854C3,
    0xB58854C4, 0xB59853C5, 0xB5A850C6, 0xB5B84FC7, 0xB5C84DC8, 0xB5D84AC9, 0xB5E849CA, 0xB5F847CB,
    0xB6085411, 0xB6185412, 0xB6285413, 0xB6385414, 0xB6485415, 0xB6585416, 0xB6685417, 0xB6785418,
    0xB6885419, 0xB698541A, 0xB6A8511B, 0xB6B85451, 0xB6C85452, 0xB6D85453, 0xB6E85454, 0xB6F85455,
    0xB7085456, 0xB7185457, 0xB7285258, 0xB7384F59, 0xB7484D5A, 0xB7584A5B, 0xB7685491, 0xB7785492,
    0xB7885493, 0xB7985494, 0xB7A85495, 0xB7B85496, 0xB7C85497, 0xB7D85298, 0xB7E84F99, 0xB7F84D9A,
    0xB8084A9B, 0xB81854D1, 0xB82854D2, 0xB83854D3, 0xB84854D4, 0xB85853D5, 0xB86850D6, 0xB8784FD7,
    0xB8884DD8, 0xB8984AD9, 0xB8A849DA, 0xB8B847DB, 0xB8C85421, 0xB8D85422, 0xB8E85423, 0xB8F85424,
    0xB9085425, 0xB9185426, 0xB9285427, 0xB9385428, 0xB9485429, 0xB958542A, 0xB968512B, 0xB9785461,
    0xB9885462, 0xB9985463, 0xB9A85464, 0xB9B85465, 0xB9C85466, 0xB9D85467, 0xB9E85268, 0xB9F84F69,
    0xBA084D6A, 0xBA184A6B, 0xBA2854A1, 0xBA3854A2, 0xBA4854A3, 0xBA5854A4, 0xBA6854A5, 0xBA7854A6,
    0xBA8854A7, 0xBA9852A8, 0xBAA84FA9, 0xBAB84DAA, 0xBAC84AAB, 0xBAD854E1, 0xBAE854E2, 0xBAF854E3,
    0xBB0854E4, 0xBB1853E5, 0xBB2850E6, 0xBB384FE7, 0xBB484DE8, 0xBB584AE9, 0xBB6849EA, 0xBB7847EB,
    0xBB8854B1, 0xBB9854B2, 0xBBA854B3, 0xBBB854B4, 0xBBC854B5, 0xBBD854B6, 0xBBE854B7, 0xBBF852B8,
    0xBC084FB9, 0xBC184DBA, 0xBC284ABB, 0xBC3854F1, 0xBC4854F2, 0xBC5854F3, 0xBC6854F4, 0xBC7853F5,
    0xBC8850F6, 0xBC984FF7, 0xBCA84DF8, 0xBCB84AF9, 0xBCC849FA, 0xBCD847FB, 0xBCE11408, 0xBCF11409,
    0xBD01140A, 0xBD11140B, 0xBD211448, 0xBD311449, 0xBD41144A, 0xBD51144B, 0xBD611488, 0xBD711489,
    0xBD81148A, 0xBD91148B, 0xBDA114C8, 0xBDB114C9, 0xBDC114CA, 0xBDD114CB, 0xBDE91402, 0xBDF91403,
    0xBE091404, 0xBE191405, 0xBE291406, 0xBE391407, 0xBE491408, 0xBE591409, 0xBE69140A, 0xBE79140B,
    0xBE891442, 0xBE991443, 0xBEA91444, 0xBEB91445, 0xBEC91446, 0xBED91447, 0xBEE91448, 0xBEF91449,
    0xBF09144A, 0xBF19144B, 0xBF291482, 0xBF391483, 0xBF491484, 0xBF591485, 0xBF691486, 0xBF791487,
    0xBF891488, 0xBF991489, 0xBFA9148A, 0xBFB9148B, 0xBFC914C2, 0xBFD914C3, 0xBFE914C4, 0xBFF914C5,
    0xC00914C6, 0xC01914C7, 0xC02913C8, 0xC03911C9, 0xC04910CA, 0xC0590ECB, 0xC0691412, 0xC0791413,
    0xC0891414, 0xC0991415, 0xC0A91416, 0xC0B91417, 0xC0C91418, 0xC0D91419, 0xC0E9141A, 0xC0F9141B,
    0xC1091452, 0xC1191453, 0xC1291454, 0xC1391455, 0xC1491456, 0xC1591457, 0xC1691458, 0xC1791459,
    0xC189145A, 0xC199145B, 0xC1A91492, 0xC1B91493, 0xC1C91494, 0xC1D91495, 0xC1E91496, 0xC1F91497,
    0xC2091498, 0xC2191499, 0xC229149A, 0xC239149B, 0xC24914D2, 0xC25914D3, 0xC26914D4, 0xC27914D5,
    0xC28914D6, 0xC29914D7, 0xC2A913D8, 0xC2B911D9, 0xC2C910DA, 0xC2D90EDB, 0xC2E91422, 0xC2F91423,
    0xC3091424, 0xC3191425, 0xC3291426, 0xC3391427, 0xC3491428, 0xC3591429, 0xC369142A, 0xC379142B,
    0xC3891462, 0xC3991463, 0xC3A91464, 0xC3B91465, 0xC3C91466, 0xC3D91467, 0xC3E91468, 0xC3F91469,
    0xC409146A, 0xC419146B, 0xC42914A2, 0xC43914A3, 0xC44914A4, 0xC45914A5, 0xC46914A6, 0xC47914A7,
    0xC48914A8, 0xC49914A9, 0xC4A914AA, 0xC4B914AB, 0xC4C914E2, 0xC4D914E3, 0xC4E914E4, 0xC4F914E5,
    0xC50914E6, 0xC51914E7, 0xC52913E8, 0xC53911E9, 0xC54910EA, 0xC5590EEB, 0xC56914B2, 0xC57914B3,
    0xC58914B4, 0xC59914B5, 0xC5A914B6, 0xC5B914B7, 0xC5C914B8, 0xC5D914B9, 0xC5E914BA, 0xC5F914BB,
    0xC60914F2, 0xC61914F3, 0xC62914F4, 0xC63914F5, 0xC64914F6, 0xC65914F7, 0xC66913F8, 0xC67911F9,
    0xC68910FA, 0xC6990EFB, 0xC6A03408, 0xC6B03409, 0xC6C0340A, 0xC6D0340B, 0xC6E03448, 0xC6F03449,
    0xC700344A, 0xC710344B, 0xC7203488, 0xC7303489, 0xC740348A, 0xC750348B, 0xC76034C8, 0xC77034C9,
    0xC78034CA, 0xC79034CB, 0xC7A83402, 0xC7B83403, 0xC7C83404, 0xC7D83405, 0xC7E83406, 0xC7F83407,
    0xC8083408, 0xC8183409, 0xC828340A, 0xC838340B, 0xC8483442, 0xC8583443, 0xC8683444, 0xC8783445,
    0xC8883446, 0xC8983447, 0xC8A83448, 0xC8B83449, 0xC8C8344A, 0xC8D8344B, 0xC8E83482, 0xC8F83483,
    0xC9083484, 0xC9183485, 0xC9283486, 0xC9383487, 0xC9483488, 0xC9583489, 0xC968348A, 0xC978348B,
    0xC98834C2, 0xC99834C3, 0xC9A834C4, 0xC9B834C5, 0xC9C834C6, 0xC9D834C7, 0xC9E833C8, 0xC9F831C9,
    0xCA0830CA, 0xCA182ECB, 0xCA283412, 0xCA383413, 0xCA483414, 0xCA583415, 0xCA683416, 0xCA783417,
    0xCA883418, 0xCA983419, 0xCAA8341A, 0xCAB8341B, 0xCAC83452, 0xCAD83453, 0xCAE83454, 0xCAF83455,
    0xCB083456, 0xCB183457, 0xCB283458, 0xCB383459, 0xCB48345A, 0xCB58345B, 0xCB683492, 0xCB783493,
    0xCB883494, 0xCB983495, 0xCBA83496, 0xCBB83497, 0xCBC83498, 0xCBD83499, 0xCBE8349A, 0xCBF8349B,
    0xCC0834D2, 0xCC1834D3, 0xCC2834D4, 0xCC3834D5, 0xCC4834D6, 0xCC5834D7, 0xCC6833D8, 0xCC7831D9,
    0xCC8830DA, 0xCC982EDB, 0xCCA83422, 0xCCB83423, 0xCCC83424, 0xCCD83425, 0xCCE83426, 0xCCF83427,
    0xCD083428, 0xCD183429, 0xCD28342A, 0xCD38342B, 0xCD483462, 0xCD583463, 0xCD683464, 0xCD783465,
    0xCD883466, 0xCD983467, 0xCDA83468, 0xCDB83469, 0xCDC8346A, 0xCDD8346B, 0xCDE834A2, 0xCDF834A3,
    0xCE0834A4, 0xCE1834A5, 0xCE2834A6, 0xCE3834A7, 0xCE4834A8, 0xCE5834A9, 0xCE6834AA, 0xCE7834AB,
    0xCE8834E2, 0xCE9834E3, 0xCEA834E4, 0xCEB834E5, 0xCEC834E6, 0xCED834E7, 0xCEE833E8, 0xCEF831E9,
    0xCF0830EA, 0xCF182EEB, 0xCF2834B2, 0xCF3834B3, 0xCF4834B4, 0xCF5834B5, 0xCF6834B6, 0xCF7834B7,
    0xCF8834B8, 0xCF9834B9, 0xCFA834BA, 0xCFB834BB, 0xCFC834F2, 0xCFD834F3, 0xCFE834F4, 0xCFF834F5,
    0xD00834F6, 0xD01834F7, 0xD02833F8, 0xD03831F9, 0xD04830FA, 0xD0582EFB,
};

uniform int get_bits(uniform uint32_t value, uniform int from, uniform int to)
{
    return (value >> from) & ((1 << (to + 1 - from)) - 1);
}

void load_mode_parameters(uniform astc_mode* uniform mode, uniform uint32_t packed_mode)
{    
    mode->width = 2 + get_bits(packed_mode, 13, 15); // 2..8 <= 2^3
    mode->height = 2 + get_bits(packed_mode, 16, 18); // 2..8 <= 2^3
    mode->dual_plane = get_bits(packed_mode, 19, 19); // 0 or 1
    mode->partitions = 1;

    mode->weight_range = get_bits(packed_mode, 0, 3);  // 0..11 <= 2^4
    mode->color_component_selector = get_bits(packed_mode, 4, 5);  // 0..2 <= 2^2
    mode->partition_id = 0;
    mode->color_endpoint_modes[0] = get_bits(packed_mode, 6, 7) * 2 + 6; // 6 or 8
    mode->color_endpoint_pairs = 1 + (mode->color_endpoint_modes[0] / 4);
    mode->endpoint_range = get_bits(packed_mode, 8, 12); // 0..20 <= 2^5
}

export void astc_rank_ispc(uniform rgba_surface src[], uniform int xx, uniform int yy, uniform uint32_t mode_buffer[], uniform astc_enc_settings settings[])
{
    int tex_width = src->width / settings->block_width;
    if (xx + programIndex >= tex_width) return;

    astc_rank_state _state;
    varying astc_rank_state* uniform state = &_state;

    state->block_width = settings->block_width;
    state->block_height = settings->block_height;
    state->fastSkipTreshold = settings->fastSkipTreshold;

    assert(state->fastSkipTreshold <= 64);

    load_block_interleaved(state->pixels, src, xx + programIndex, yy, state->block_width, state->block_height);
    if (settings->channels == 3) clear_alpha(state->pixels, state->block_width, state->block_height);

    compute_metrics(state);

    float threshold_error = 0;
    int count = -1;

    for (uniform int id = 0; id < packed_modes_count; id++)
    {
        uniform uint32_t packed_mode = packed_modes[id];

        uniform astc_mode _mode;
        uniform astc_mode* uniform mode = &_mode;
        load_mode_parameters(mode, packed_mode);

        if (mode->height > state->block_height) continue;
        if (mode->width > state->block_width) continue;

        if (settings->channels == 3 && mode->color_endpoint_modes[0] > 8) continue;

        float error = estimate_error(state, mode);
        count += 1;

        if (count < state->fastSkipTreshold)
        {
            state->best_modes[count] = packed_mode;
            state->best_scores[count] = error;

            threshold_error = max(threshold_error, error);
        }
        else if (error < threshold_error)
        {
            insert_element(state, error, packed_mode, &threshold_error);
        }
    }

    assert(count >= 0);

    for (uniform int i = 0; i < state->fastSkipTreshold; i++)
    {
        mode_buffer[programCount * i + programIndex] = state->best_modes[i];
    }
}

///////////////////////////////////////////////////////////
//				 ASTC candidate encoding

struct astc_block
{
    uniform int width;
    uniform int height;
    uniform bool dual_plane;
    int weight_range;
    uint8_t weights[64];
    int color_component_selector;

    uniform int partitions;
    int partition_id;
    uniform int color_endpoint_pairs;
    uniform int channels;
    int color_endpoint_modes[4];
    int endpoint_range;
    uint8_t endpoints[18];
};

struct astc_enc_state
{
    float pixels[256];
    float scaled_pixels[256];
    uint32_t data[4];

    // settings
    uniform int block_width;
    uniform int block_height;
    uniform int pitch;

    uniform int refineIterations;
};

struct astc_enc_context
{
    // uniform parameters
    int width;
    int height;
    int channels;
    bool dual_plane;
    int partitions;
    int color_endpoint_pairs;
};

uniform static const float filter_data[309] =
{
     0.688356,-0.188356, 0.414384, 0.085616, 0.085616, 0.414384,-0.188356, 0.688356,
     0.955516,-0.227273, 0.044484, 0.142349, 0.727273,-0.142349,-0.142349, 0.727273,
     0.142349, 0.044484,-0.227273, 0.955516, 0.600000,-0.200000, 0.400000, 0.000000,
     0.200000, 0.200000, 0.000000, 0.400000,-0.200000, 0.600000, 0.828571,-0.142857,
     0.028571, 0.342857, 0.285714,-0.057143,-0.142857, 0.714286,-0.142857,-0.057143,
     0.285714, 0.342857, 0.028571,-0.142857, 0.828571, 0.985714,-0.252381, 0.080952,
    -0.014286, 0.057143, 1.009524,-0.323810, 0.057143,-0.085714, 0.485714, 0.485714,
    -0.085714, 0.057143,-0.323810, 1.009524, 0.057143,-0.014286, 0.080952,-0.252381,
     0.985714, 0.510753,-0.177419, 0.381720,-0.048387, 0.252688, 0.080645, 0.080645,
     0.252688,-0.048387, 0.381720,-0.177419, 0.510753, 0.754228,-0.194882, 0.052858,
     0.398312, 0.147638,-0.040044,-0.016924, 0.547244,-0.148431,-0.148431, 0.547244,
    -0.016924,-0.040044, 0.147638, 0.398312, 0.052858,-0.194882, 0.754228, 0.921235,
    -0.216677, 0.063615,-0.013072, 0.210040, 0.577804,-0.169641, 0.034858,-0.164122,
     0.798726,-0.053828, 0.011061, 0.011061,-0.053828, 0.798726,-0.164122, 0.034858,
    -0.169641, 0.577804, 0.210040,-0.013072, 0.063615,-0.216677, 0.921235, 0.996932,
    -0.209923, 0.069231,-0.020846, 0.003068, 0.016362, 1.119589,-0.369231, 0.111180,
    -0.016362,-0.035452, 0.240891, 0.800000,-0.240891, 0.035452, 0.035452,-0.240891,
     0.800000, 0.240891,-0.035452,-0.016362, 0.111180,-0.369231, 1.119589, 0.016362,
     0.003068,-0.020846, 0.069231,-0.209923, 0.996932, 0.415909,-0.165909, 0.343182,
    -0.093182, 0.234091, 0.015909, 0.161364, 0.088636, 0.088636, 0.161364, 0.015909,
     0.234091,-0.093182, 0.343182,-0.165909, 0.415909, 0.653807,-0.172170, 0.058458,
     0.395689, 0.040094,-0.013613, 0.189195, 0.209906,-0.071270,-0.068923, 0.422170,
    -0.143341,-0.143341, 0.422170,-0.068923,-0.071270, 0.209906, 0.189195,-0.013613,
     0.040094, 0.395689, 0.058458,-0.172170, 0.653807, 0.805363,-0.204713, 0.061406,
    -0.016387, 0.363455, 0.220460,-0.066129, 0.017647,-0.078453, 0.645632,-0.193664,
     0.051682,-0.121551, 0.455481, 0.081527,-0.021756,-0.021756, 0.081527, 0.455481,
    -0.121551, 0.051682,-0.193664, 0.645632,-0.078453, 0.017647,-0.066129, 0.220460,
     0.363455,-0.016387, 0.061406,-0.204713, 0.805363, 0.881593,-0.204539, 0.075065,
    -0.021559, 0.004453, 0.270644, 0.467517,-0.171576, 0.049278,-0.010179,-0.169588,
     0.821023,-0.159819, 0.045902,-0.009481,-0.012311, 0.059603, 0.756331,-0.217226,
     0.044870, 0.044870,-0.217226, 0.756331, 0.059603,-0.012311,-0.009481, 0.045902,
    -0.159819, 0.821023,-0.169588,-0.010179, 0.049278,-0.171576, 0.467517, 0.270644,
     0.004453,-0.021559, 0.075065,-0.204539, 0.881593, 0.967275,-0.287351, 0.076902,
    -0.018670, 0.005432,-0.000959, 0.104719, 0.919524,-0.246087, 0.059743,-0.017382,
     0.003067,-0.127990, 0.653915, 0.300773,-0.073019, 0.021245,-0.003749, 0.064956,
    -0.331864, 1.007366,-0.105833, 0.030792,-0.005434,-0.006723, 0.034349,-0.104266,
     0.996397,-0.289905, 0.051160,-0.005112, 0.026120,-0.079287, 0.323158, 0.571013,
    -0.100767, 0.003834,-0.019590, 0.059465,-0.242368, 0.905074, 0.075575,-0.000959,
     0.004898,-0.014866, 0.060592,-0.226268, 0.981106,
};

uniform static const int filterbank[5][5] =
{
    {   0,   8,  -1,  -1,  -1 },
    {  20,  30,  45,  -1,  -1 },
    {  65,  77,  95, 119,  -1 },
    {  -1,  -1,  -1,  -1,  -1 },
    { 149, 165, 189, 221, 261 },
};

void scale_pixels(astc_enc_state state[], uniform astc_enc_context ctx[])
{
    uniform int channels = ctx->channels;
    uniform const float* uniform yfilter = &filter_data[filterbank[state->block_height - 4][ctx->height - 2]];
    uniform const float* uniform xfilter = &filter_data[filterbank[state->block_width - 4][ctx->width - 2]];

    for (uniform int y = 0; y < ctx->height; y++)
    {
        float line[8][4];
        
        if (state->block_height == ctx->height)
        {
            for (uniform int x = 0; x < state->block_width; x++)
            for (uniform int p = 0; p < channels; p++)
                line[x][p] = get_pixel(state->pixels, p, x, y);
        }
        else
        for (uniform int x = 0; x < state->block_width; x++)
        {
            uniform int n = ctx->height;

            for (uniform int p = 0; p < channels; p++) line[x][p] = 0;

            for (uniform int k = 0; k < state->block_height; k++)
            for (uniform int p = 0; p < channels; p++)
                line[x][p] += yfilter[k * n + y] * get_pixel(state->pixels, p, x, k);
        }
        
        if (state->block_width == ctx->width)
        {
            for (uniform int x = 0; x < ctx->width; x++)
            for (uniform int p = 0; p < channels; p++)
                set_pixel(state->scaled_pixels, p, x, y, clamp(line[x][p], 0, 255));
        }
        else
        for (uniform int x = 0; x < ctx->width; x++)
        {
            uniform int n = ctx->width;

            float value[4] = { 0, 0, 0, 0 };

            for (uniform int k = 0; k < state->block_width; k++)
            for (uniform int p = 0; p < channels; p++)
                value[p] += xfilter[k * n + x] * line[k][p];
            
            for (uniform int p = 0; p < channels; p++)
                set_pixel(state->scaled_pixels, p, x, y, clamp(value[p], 0, 255));
        }
    }
}

inline int clamp_unorm8(int value)
{
    if (value < 0) return 0;
    if (value > 255) return 255;
    return value;
}

inline void apply_blue_contract(int& r, int& g, int& b)
{
    r = (r + b) >> 1;
    g = (g + b) >> 1;
}

void decode_endpoints(float endpoints[8], uint8_t coded_endpoints[], int mode)
{    
    if ((mode % 4) == 2)
    {
        int v0 = coded_endpoints[0];
        int v1 = coded_endpoints[1];
        int v2 = coded_endpoints[2];
        int v3 = coded_endpoints[3];
        int v4 = coded_endpoints[4];
        int v5 = coded_endpoints[5];

        endpoints[0] = (v0 * v3) >> 8;
        endpoints[1] = (v1 * v3) >> 8;
        endpoints[2] = (v2 * v3) >> 8;
        endpoints[3] = 0xFF;

        endpoints[4] = v0;
        endpoints[5] = v1;
        endpoints[6] = v2;
        endpoints[7] = 0xFF;

        if (mode > 8)
        {
            endpoints[3] = clamp_unorm8(v4);
            endpoints[7] = clamp_unorm8(v5);
        }
    }

    if ((mode % 4) == 0)
    {
        int v0 = coded_endpoints[0];
        int v1 = coded_endpoints[1];
        int v2 = coded_endpoints[2];
        int v3 = coded_endpoints[3];
        int v4 = coded_endpoints[4];
        int v5 = coded_endpoints[5];
        int v6 = coded_endpoints[6];
        int v7 = coded_endpoints[7];

        bool swap_endpoints = v1 + v3 + v5 < v0 + v2 + v4;

        if (swap_endpoints)
        {
            swap(v0, v1);
            swap(v2, v3);
            swap(v4, v5);
            swap(v6, v7);

            apply_blue_contract(v0, v2, v4);
            apply_blue_contract(v1, v3, v5);
        }        

        endpoints[0] = clamp_unorm8(v0);
        endpoints[1] = clamp_unorm8(v2);
        endpoints[2] = clamp_unorm8(v4);
        endpoints[3] = 0xFF;

        endpoints[4] = clamp_unorm8(v1);
        endpoints[5] = clamp_unorm8(v3);
        endpoints[6] = clamp_unorm8(v5);
        endpoints[7] = 0xFF;

        if (mode > 8)
        {
            endpoints[3] = clamp_unorm8(v6);
            endpoints[7] = clamp_unorm8(v7);
        }
    }
}

void dequant_decode_endpoints(float endpoints[8], uint8_t block_endpoints[], int mode, int range)
{
    int levels = get_levels(range);
    int num_cem_pairs = 1 + mode / 4;

    uint8_t dequant_endpoints[8];
    for (uniform int k = 0; k < 2 * num_cem_pairs; k++)
    {
        dequant_endpoints[k] = (int)(((int)block_endpoints[k]) * 255.0f / (levels - 1) + 0.5);
    }

    decode_endpoints(endpoints, dequant_endpoints, mode);
}

bool compare_endpoints(uint8_t endpoints[8], astc_block block[])
{
    int sum = 0;
    for (uniform int p = 0; p < 3; p++)
    {
        sum += endpoints[p * 2 + 0];
        sum -= endpoints[p * 2 + 1];
    }
    
    if (-2 <= sum && sum <= 2)
    {
        // avoid being too close so we don't need proper rounding 
        for (uniform int p = 0; p < 3; p++)
        {
            if (sum<=0)
                endpoints[p * 2 + 0] = clamp(endpoints[p * 2 + 0] - 1, 0, get_levels(block->endpoint_range) - 1);
            if (sum>0)
                endpoints[p * 2 + 1] = clamp(endpoints[p * 2 + 1] - 1, 0, get_levels(block->endpoint_range) - 1);
        }

        sum = 0;
        for (uniform int p = 0; p < 3; p++)
        {
            sum += endpoints[p * 2 + 0];
            sum -= endpoints[p * 2 + 1];
        }
    }

    return sum > 0;
}

void reorder_endpoints(uint8_t endpoints[8], astc_block block[], bool blue_contract)
{    
    if (compare_endpoints(endpoints, block) == !blue_contract)
    for (uniform int p = 0; p < 4; p++) swap(endpoints[p * 2], endpoints[p * 2 + 1]);
}

inline int quant_endpoint(float value, int levels)
{
    return clamp(value / 255.0f * (levels - 1) + 0.5, 0, levels - 1);
}

void quantize_endpoints_scale(astc_block block[], float endpoints[4])
{
    int ep_levels = get_levels(block->endpoint_range);

    float near[3];
    float far[3];
    for (uniform int p = 0; p < 3; p++)
    {
        near[p] = endpoints[p * 2 + 0];
        far[p] = endpoints[p * 2 + 1];
    }

    for (uniform int p = 0; p < 3; p++)
        block->endpoints[p] = quant_endpoint(far[p], ep_levels);

    float sq_norm = dot3(far, far) + 0.00001;
    float scale = dot3(far, near) / sq_norm;

    block->endpoints[3] = quant_endpoint(scale * 256, ep_levels);

    if (block->color_endpoint_modes[0] > 8)
    {
        block->endpoints[4] = quant_endpoint(endpoints[3 * 2 + 0], ep_levels);
        block->endpoints[5] = quant_endpoint(endpoints[3 * 2 + 1], ep_levels);
    }
}

void quantize_endpoints_pair(astc_block block[], float endpoints[6])
{    
    int ep_levels = get_levels(block->endpoint_range);

    bool blue_contract = true;

    float blue_compressed[6];
    for (uniform int i = 0; i < 2; i++)
    {
        blue_compressed[i + 0] = endpoints[i + 0] * 2 - endpoints[i + 4];
        blue_compressed[i + 2] = endpoints[i + 2] * 2 - endpoints[i + 4];
        blue_compressed[i + 4] = endpoints[i + 4];

        if (blue_compressed[i + 0] < 0) blue_contract = false;
        if (blue_compressed[i + 0] > 255) blue_contract = false;
        if (blue_compressed[i + 2] < 0) blue_contract = false;
        if (blue_compressed[i + 2] > 255) blue_contract = false;
    }

    if (blue_contract)
    {
        for (uniform int p = 0; p < 3; p++)
        {
            block->endpoints[p * 2 + 0] = quant_endpoint(blue_compressed[p * 2 + 0], ep_levels);
            block->endpoints[p * 2 + 1] = quant_endpoint(blue_compressed[p * 2 + 1], ep_levels);
        }
    }
    else
    {
        for (uniform int p = 0; p < 3; p++)
        {
            block->endpoints[p * 2 + 0] = quant_endpoint(endpoints[p * 2 + 0], ep_levels);
            block->endpoints[p * 2 + 1] = quant_endpoint(endpoints[p * 2 + 1], ep_levels);
        }
    }

    if (block->color_endpoint_modes[0] > 8)
    {
        block->endpoints[6] = quant_endpoint(endpoints[3 * 2 + 0], ep_levels);
        block->endpoints[7] = quant_endpoint(endpoints[3 * 2 + 1], ep_levels);
    }

    reorder_endpoints(block->endpoints, block, blue_contract);
}

void quantize_endpoints(astc_block block[], float endpoints[])
{
    bool zero_based = (block->color_endpoint_modes[0] % 4) == 2;

    if (zero_based)
    {
        quantize_endpoints_scale(block, endpoints);
    }
    else
    {
        quantize_endpoints_pair(block, endpoints);
    }
}

void opt_weights(float scaled_pixels[], astc_block block[])
{
    uniform int channels = 4;
    if (block->dual_plane) channels = 3;

    float rec_endpoints[8];
    dequant_decode_endpoints(rec_endpoints, block->endpoints, block->color_endpoint_modes[0], block->endpoint_range);

    int w_levels = get_levels(block->weight_range);

    float dir[4]; dir[3] = 0;
    for (uniform int p = 0; p < channels; p++) dir[p] = rec_endpoints[4 + p] - rec_endpoints[0 + p];
    float sq_norm = dot4(dir, dir) + 0.00001;
    for (uniform int p = 0; p < channels; p++) dir[p] *= (w_levels - 1) / sq_norm;

    for (uniform int y = 0; y < block->height; y++)
    for (uniform int x = 0; x < block->width; x++)
    {
        float pixel[4]; pixel[3] = 0;
        for (uniform int p = 0; p < channels; p++) pixel[p] = get_pixel(scaled_pixels, p, x, y) - rec_endpoints[0 + p];
        
        int q = clamp(dot4(pixel, dir) + 0.5, 0, w_levels - 1);

        block->weights[y * block->width + x] = q;
    }
}

void sgesv2(float A[4], float x[2], float b[2])
{
    float inv_det = 1.0f / (A[0] * A[3] - A[1] * A[2]);
    x[0] = (b[0] * +A[3] + b[1] * -A[2])*inv_det;
    x[1] = (b[0] * -A[1] + b[1] * +A[0])*inv_det;
}

void ls_refine_scale(float endpoints[4], float scaled_pixels[], astc_block block[])
{
    int levels = get_levels(block->weight_range);
    float levels_rcp = 1.0f / (levels - 1);

    float sum_w = 0;
    float sum_ww = 0;
    float sum_d = 0;
    float sum_wd = 0;

    for (uniform int y = 0; y < block->height; y++)
    for (uniform int x = 0; x < block->width; x++)
    {
        float w = (int)block->weights[y * block->width + x] * levels_rcp;
        float d = 0;

        for (uniform int p = 0; p < 3; p++) d += sq(get_pixel(scaled_pixels, p, x, y));
        d = sqrt(d+0.01f);

        sum_w += w;
        sum_ww += w*w;
        sum_d += d;
        sum_wd += w*d;
    }

    float sum_1 = 1.0f * block->height * block->width;

    float C[4] = { sum_1, sum_w, sum_w, sum_ww };
    float b[2] = { sum_d, sum_wd };
    float xx[2];

    sgesv2(C, xx, b);

    float scale = xx[0] / (xx[1] + xx[0]);

    if (xx[1] + xx[0] < 1) scale = 1;
    if (scale > 0.9999) scale = 0.9999;
    if (scale < 0) scale = 0;
        
    float sum_zz = 0;
    float sum_zp[3] = { 0, 0, 0 };
        
    for (uniform int y = 0; y < block->height; y++)
    for (uniform int x = 0; x < block->width; x++)
    {
        float w = (int)block->weights[y * block->width + x] * levels_rcp;
        float z = scale + (1 - scale)*w;

        sum_zz += z * z;
        for (uniform int p = 0; p < 3; p++) sum_zp[p] += z * get_pixel(scaled_pixels, p, x, y);
    }

    for (uniform int p = 0; p < 3; p++) endpoints[2 * p + 0] = scale  * sum_zp[p] / sum_zz;
    for (uniform int p = 0; p < 3; p++) endpoints[2 * p + 1] = sum_zp[p] / sum_zz;

    if (block->channels == 4)
    {
        float Atb1 = 0;
        float sum_q = 0;
        float sum_qq = 0;
        float sum[2] = { 0, 0 };

        for (uniform int y = 0; y < block->height; y++)
        for (uniform int x = 0; x < block->width; x++)
        {
            int q = block->weights[y * block->width + x];
            int z = (levels - 1) - q;

            sum_q += q;
            sum_qq += q*q;

            sum[1] += 1;
            sum[0] += get_pixel(scaled_pixels, 3, x, y);
            Atb1 += z * get_pixel(scaled_pixels, 3, x, y);
        }

        float Atb2 = (levels - 1)*sum[0] - Atb1;

        float Cxx = sum[1] * sq(levels - 1) - 2 * (levels - 1)*sum_q + sum_qq;
        float Cyy = sum_qq;
        float Cxy = (levels - 1)*sum_q - sum_qq;
        float scale = 1.0f / (Cxx*Cyy - Cxy*Cxy);

        float ep[8];   
        ep[0 + 3] = (levels - 1)*(Atb1 * Cyy - Atb2 * Cxy)*scale;
        ep[4 + 3] = (levels - 1)*(Atb2 * Cxx - Atb1 * Cxy)*scale;
   
        if (abs(Cxx*Cyy - Cxy*Cxy) < 0.001)
        {
            ep[0 + 3] = sum[0] / sum[1];
            ep[4 + 3] = ep[0 + 3];
        }

        endpoints[6 + 0] = ep[0 + 3];
        endpoints[6 + 1] = ep[4 + 3];
    }
}

void ls_refine_pair(float endpoints[6], float scaled_pixels[], astc_block block[])
{
    uniform int channels = block->channels;
    int levels = get_levels(block->weight_range);
    
    float Atb1[4] = { 0, 0, 0, 0 };
    float sum_q = 0;
    float sum_qq = 0;
    float sum[5] = { 0, 0, 0, 0, 0 };

    for (uniform int y = 0; y < block->height; y++)
    for (uniform int x = 0; x < block->width; x++)
    {
        int q = block->weights[y * block->width + x];

        int z = (levels - 1) - q;

        sum_q += q;
        sum_qq += q*q;

        sum[4] += 1;
        for (uniform int p = 0; p < channels; p++) sum[p] += get_pixel(scaled_pixels, p, x, y);
        for (uniform int p = 0; p < channels; p++) Atb1[p] += z * get_pixel(scaled_pixels, p, x, y);
    }

    float Atb2[4];
    for (uniform int p = 0; p < channels; p++)
    {
        Atb2[p] = (levels - 1)*sum[p] - Atb1[p];
    }

    float Cxx = sum[4] * sq(levels - 1) - 2 * (levels - 1)*sum_q + sum_qq;
    float Cyy = sum_qq;
    float Cxy = (levels - 1)*sum_q - sum_qq;
    float scale = 1.0f / (Cxx*Cyy - Cxy*Cxy);

    float ep[8];
    for (uniform int p = 0; p < channels; p++)
    {
        ep[0 + p] = (levels - 1)*(Atb1[p] * Cyy - Atb2[p] * Cxy)*scale;
        ep[4 + p] = (levels - 1)*(Atb2[p] * Cxx - Atb1[p] * Cxy)*scale;
    }

    if (abs(Cxx*Cyy - Cxy*Cxy) < 0.001)
    {
        // flatten
        for (int p = 0; p < channels; p++)
        {
            ep[0 + p] = sum[p] / sum[4];
            ep[4 + p] = ep[0 + p];
        }
    }

    for (uniform int p = 0; p < channels; p++)
    {
        endpoints[2 * p + 0] = ep[0 + p];
        endpoints[2 * p + 1] = ep[4 + p];
    }
}

void ls_refine(float endpoints[], float scaled_pixels[], astc_block block[])
{
    if (block->color_endpoint_modes[0] % 4 == 2)
    {
        ls_refine_scale(endpoints, scaled_pixels, block);
    }
    else
    {
        ls_refine_pair(endpoints, scaled_pixels, block);
    }
}

float optimize_alt_plane(uint8_t alt_weights[], float scaled_pixels[], astc_block block[])
{
    int ccs = block->color_component_selector;

    float ext[2] = { 1000, -1000 };

    for (uniform int y = 0; y < block->height; y++)
    for (uniform int x = 0; x < block->width; x++)
    {
        float value = get_pixel(scaled_pixels, 3, x, y);
        ext[0] = min(ext[0], value);
        ext[1] = max(ext[1], value);
    }
    
    block->endpoints[3 * 2 + 0] = 0;
    block->endpoints[3 * 2 + 1] = 255;

    float _rec_endpoints[8];
    dequant_decode_endpoints(_rec_endpoints, block->endpoints, block->color_endpoint_modes[0], block->endpoint_range);
    
    float endpoints[8];
    for (int p = 0; p < 3; p++)
    {
        endpoints[p * 2 + 0] = _rec_endpoints[0 + p];
        endpoints[p * 2 + 1] = _rec_endpoints[4 + p];
    }

    endpoints[3 * 2 + 0] = gather_float(endpoints, ccs * 2 + 0);
    endpoints[3 * 2 + 1] = gather_float(endpoints, ccs * 2 + 1);

    scatter_float(endpoints, ccs * 2 + 0, ext[0]);
    scatter_float(endpoints, ccs * 2 + 1, ext[1]);
    
    quantize_endpoints(block, endpoints);

    float rec_endpoints[8];
    dequant_decode_endpoints(rec_endpoints, block->endpoints, block->color_endpoint_modes[0], block->endpoint_range);

    float base = gather_float(rec_endpoints, 0 + ccs);
    float dir = gather_float(rec_endpoints, 4 + ccs) - base;
    float sq_norm = sq(dir) + 0.00001;
    
    int w_levels = get_levels(block->weight_range);
    dir *= (w_levels - 1) / sq_norm;

    float err = 0;
    for (uniform int y = 0; y < block->height; y++)
    for (uniform int x = 0; x < block->width; x++)
    {
        float value = get_pixel(scaled_pixels, 3, x, y) - base;

        int q = clamp(value * dir + 0.5, 0, w_levels - 1);

        alt_weights[y * block->width + x] = q;
    }

    if (dir < 0)
    for (uniform int y = 0; y < block->height; y++)
    for (uniform int x = 0; x < block->width; x++)
    {
        int q = block->weights[y * block->width + x];

        block->weights[y * block->width + x] = w_levels - 1 - q;
    }

    return err;
}

void optimize_block(float scaled_pixels[], astc_block block[], astc_enc_state state[])
{
    pixel_set pset;
    pset.pixels = scaled_pixels;
    pset.width = block->width;
    pset.height = block->height;

    float ep[8];
    bool zero_based = (block->color_endpoint_modes[0] % 4) == 2;
    compute_pca_endpoints(ep, &pset, zero_based, 4);

    quantize_endpoints(block, ep);
    opt_weights(scaled_pixels, block);
    
    for (uniform int i = 0; i < state->refineIterations; i++)
    {
        ls_refine(ep, scaled_pixels, block);
        quantize_endpoints(block, ep);
        opt_weights(scaled_pixels, block);
    }
    
    if (block->dual_plane)
    {
        uint8_t alt_weights[64];
        optimize_alt_plane(alt_weights, scaled_pixels, block);

        uint8_t block_weights[64];
        for (uniform int i = 0; i < block->width * block->height; i++)
        {
            block_weights[i] = block->weights[i];
        }

        for (uniform int i = 0; i < block->width * block->height; i++)
        {
            block->weights[i * 2 + 0] = block_weights[i];
            block->weights[i * 2 + 1] = alt_weights[i];
        }
    }
}

float measure_error(astc_block block[], astc_enc_state state[])
{
    uniform int pitch = state->block_height * state->block_width;
    assert(pitch <= 64);

    // dequant values    
    uniform int num_weights = block->width * block->height * (block->dual_plane ? 2 : 1);

    range_values weight_range_values = get_range_values(block->weight_range);

    int block_weights[64];
    for (int i = 0; i < num_weights; i++)
    {
        block_weights[i] = ((int)block->weights[i] * 64.0f / (weight_range_values.levels - 1) + 0.5);
    }

    float rgba_endpoints[8];
    dequant_decode_endpoints(rgba_endpoints, block->endpoints, block->color_endpoint_modes[0], block->endpoint_range);
    
    uniform int stride = block->width;
    uniform int Ds = (1024 + state->block_width / 2) / (state->block_width - 1);
    uniform int Dt = (1024 + state->block_height / 2) / (state->block_height - 1);

    uint8_t main_weights[64];
    uint8_t alt_weights[64];

    for (uniform int i = 0; i < num_weights; i++) main_weights[i] = block_weights[i];

    if (block->dual_plane)
    for (uniform int i = 0; i < num_weights/2; i++)
    {
        main_weights[i] = block_weights[i * 2 + 0];
        alt_weights[i] = block_weights[i * 2 + 1];
    }

    float sq_error = 0;

    for (uniform int y = 0; y < state->block_height; y++)
    for (uniform int x = 0; x < state->block_width; x++)
    {
        uniform int gs = (x * Ds * (block->width  - 1) + 32) >> 6;
        uniform int gt = (y * Dt * (block->height - 1) + 32) >> 6;

        uniform int js = gs >> 4;
        uniform int jt = gt >> 4;

        uniform int fs = gs & 0x0F;
        uniform int ft = gt & 0x0F;
        uniform int w11 = ((fs*ft + 8) >> 4);

        int filled_weight = 0;
        int alt_filled_weight = 0;

        {
            int acc = 0;
            acc += main_weights[stride * (jt + 0) + js + 0] * (16 - ft - fs + w11);
            acc += main_weights[stride * (jt + 0) + js + 1] * (fs - w11);
            acc += main_weights[stride * (jt + 1) + js + 0] * (ft - w11);
            acc += main_weights[stride * (jt + 1) + js + 1] * w11;
            filled_weight = (acc + 8) >> 4;
        }
        
        if (block->dual_plane)
        {
            int acc = 0;
            acc += alt_weights[stride * (jt + 0) + js + 0] * (16 - ft - fs + w11);
            acc += alt_weights[stride * (jt + 0) + js + 1] * (fs - w11);
            acc += alt_weights[stride * (jt + 1) + js + 0] * (ft - w11);
            acc += alt_weights[stride * (jt + 1) + js + 1] * w11;
            alt_filled_weight = (acc + 8) >> 4;
        }

        for (uniform int p = 0; p < block->channels; p++)
        {
            int C0 = rgba_endpoints[0 + p] * 256 + 128;
            int C1 = rgba_endpoints[4 + p] * 256 + 128;
            int w = filled_weight;

            if (block->dual_plane && block->color_component_selector == p)
            {
                w = alt_filled_weight;
            }

            int C = (C0 * (64 - w) + C1 * w + 32) / 64;

            float diff = (C >> 8) - get_pixel(state->pixels, p, x, y);
            sq_error += diff * diff;
        }
    }

    return sq_error;
}

int code_value(int value, range_values range)
{
    int coded = value;

    if (range.levels_m != 2 && range.levels > 5)
    {
        int value2 = value;
        if (value >= range.levels / 2) value2 = (range.levels - 1) - value;
        int q = (value2 * range.levels_m_rcp) >> 16;
        int r = value2 - q * range.levels_m;
        coded = q + r * (1 << (range.levels_e - 1));
        coded = coded * 2 + ((value >= range.levels / 2) ? 1 : 0);
    }

    return coded;
}

void code_block(astc_block block[])
{
    uniform int num_weights = block->width * block->height * (block->dual_plane ? 2 : 1);

    range_values weight_range_values = get_range_values(block->weight_range);
    for (uniform int i = 0; i < num_weights; i++)
    {
        block->weights[i] = code_value(block->weights[i], weight_range_values);
    }

    range_values endpoint_range_values = get_range_values(block->endpoint_range);
    for (uniform int i = 0; i < 2 * block->color_endpoint_pairs; i++)
    {
        block->endpoints[i] = code_value(block->endpoints[i], endpoint_range_values);
    }
}

extern "C" void pack_block_c(uniform uint32_t data[4], uniform astc_block block[]);

void pack_block(astc_block block[], astc_enc_state state[])
{
    code_block(block);

    foreach_active (instance) 
    {
        uniform astc_block ublock;

        ublock.width = block->width;
        ublock.height = block->height;
        ublock.dual_plane = block->dual_plane;
        ublock.partitions = block->partitions;
        ublock.color_endpoint_pairs = block->color_endpoint_pairs;

        ublock.weight_range = extract(block->weight_range, instance);
        ublock.color_component_selector = extract(block->color_component_selector, instance);
        ublock.partition_id = extract(block->partition_id, instance);
        ublock.endpoint_range = extract(block->endpoint_range, instance);
        ublock.color_endpoint_modes[0] = extract(block->color_endpoint_modes[0], instance);

        uniform int num_weights = block->width * block->height * (block->dual_plane ? 2 : 1);
        for (uniform int i = 0; i < num_weights; i++)
            ublock.weights[i] = extract(block->weights[i], instance);

        for (uniform int i = 0; i < 8; i++)
            ublock.endpoints[i] = extract(block->endpoints[i], instance);

        uniform uint32_t data[4];
        pack_block_c(data, &ublock);
        
        for (uniform int i = 0; i < 4; i++) state->data[i] = insert(state->data[i], instance, data[i]);
    }
}

int get_bits(uint32_t value, uniform int from, uniform int to)
{
    return (value >> from) & ((1 << (to + 1 - from)) - 1);
}

void load_block_parameters(astc_block block[], uint32_t mode, uniform astc_enc_context ctx[])
{
    // uniform parameters
    block->width = ctx->width;
    block->height = ctx->height;
    block->dual_plane = ctx->dual_plane;
    block->partitions = ctx->partitions;
    block->color_endpoint_pairs = ctx->color_endpoint_pairs;
    block->channels = ctx->channels;

    // varying parameters
    block->weight_range = get_bits(mode, 0, 3);  // 0..11 <= 2^4
    block->color_component_selector = get_bits(mode, 4, 5);  // 0..2 <= 2^2 
    block->partition_id = 0;
    block->color_endpoint_modes[0] = get_bits(mode, 6, 7) * 2 + 6; // 6, 8, 10 or 12
    block->endpoint_range = get_bits(mode, 8, 12); // 0..20 <= 2^5
}

export void astc_encode_ispc(uniform rgba_surface src[], uniform float block_scores[], uniform uint8_t dst[], uniform uint64_t list[], uniform astc_enc_context list_context[], uniform astc_enc_settings settings[])
{
    uint64_t entry = list[programIndex];
    uint32_t offset = entry >> 32;
    uint32_t mode = (entry & 0xFFFFFFFF);
    if (mode == 0) return;
    int yy = offset >> 16;
    int xx = offset & 0xFFFF;

    int tex_width = src->width / settings->block_width;

    astc_enc_state _state;
    varying astc_enc_state* uniform state = &_state;

    state->block_width = settings->block_width;
    state->block_height = settings->block_height;
    state->refineIterations = settings->refineIterations;

    load_block_interleaved(state->pixels, src, xx, yy, state->block_width, state->block_height);

    astc_block _block;
    varying astc_block* uniform block = &_block;

    load_block_parameters(block, mode, list_context);
    
    scale_pixels(state, list_context);
    if (block->channels == 3) clear_alpha(state->scaled_pixels, block->width, block->height);

    if (block->dual_plane)
    {
        pixel_set pset;
        pset.pixels = state->scaled_pixels;
        pset.width = block->width;
        pset.height = block->height;

        rotate_plane(&pset, block->color_component_selector);
    }

    optimize_block(state->scaled_pixels, block, state);
    float error = measure_error(block, state);
    
    if (error < gather_float(block_scores, yy * tex_width + xx))
    {
        pack_block(block, state);

        scatter_float(block_scores, yy * tex_width + xx, error);

        for (uniform int i = 0; i < 4; i++)
            scatter_uint((uint32_t*)dst, (yy * tex_width + xx) * 4 + i, state->data[i]);
    }
}
