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

///////////////////////////
//   generic helpers

inline void swap_ints(int u[], int v[], uniform int n)
{
    for (uniform int i=0; i<n; i++)
	{
		int t = u[i];
		u[i] = v[i];
		v[i] = t;
	}
}

inline void swap_uints(uint32 u[], uint32 v[], uniform int n)
{
    for (uniform int i=0; i<n; i++)
	{
		uint32 t = u[i];
		u[i] = v[i];
		v[i] = t;
	}
}

inline float sq(float v)
{
	return v*v;
}

inline int pow2(int x) 
{
	return 1<<x; 
}

inline float clamp(float v, int a, int b)
{
    return clamp(v, (float)a, (float)b);
}

// the following helpers isolate performance warnings

inline unsigned int32 gather_uint(const uniform unsigned int32* const uniform ptr, int idx)
{
	return ptr[idx]; // (perf warning expected)
}

inline unsigned int32 gather_uint(const varying unsigned int32* const uniform ptr, int idx)
{
	return ptr[idx]; // (perf warning expected)
}

inline int32 gather_int(const uniform int32* const uniform ptr, int idx)
{
	return ptr[idx]; // (perf warning expected)
}

inline float gather_float(varying float* uniform ptr, int idx)
{
	return ptr[idx]; // (perf warning expected)
}

inline void scatter_uint(uniform unsigned int32* ptr, int idx, uint32 value)
{
	ptr[idx] = value; // (perf warning expected)
}

inline void scatter_int(varying int32* uniform ptr, int idx, uint32 value)
{
	ptr[idx] = value; // (perf warning expected)
}

inline uint32 shift_right(uint32 v, const uniform int bits)
{
	return v>>bits; // (perf warning expected)
}

///////////////////////////////////////////////////////////
//				    BC1/BC7 shared

struct rgba_surface
{
	uint8* ptr;
	int width, height, stride;
};

inline void load_block_interleaved(float block[48], uniform rgba_surface* uniform src, int xx, uniform int yy)
{
    for (uniform int y = 0; y<4; y++)
    for (uniform int x = 0; x<4; x++)
    {
        uniform unsigned int32* uniform src_ptr = (unsigned int32*)&src->ptr[(yy * 4 + y)*src->stride];
        unsigned int32 rgba = gather_uint(src_ptr, xx * 4 + x);

        block[16 * 0 + y * 4 + x] = (int)((rgba >> 0) & 255);
        block[16 * 1 + y * 4 + x] = (int)((rgba >> 8) & 255);
        block[16 * 2 + y * 4 + x] = (int)((rgba >> 16) & 255);
    }
}

inline void load_block_interleaved_rgba(float block[64], uniform rgba_surface* uniform src, int xx, uniform int yy)
{
	for (uniform int y=0; y<4; y++)
	for (uniform int x=0; x<4; x++)
	{
		uniform unsigned int32* uniform src_ptr = (unsigned int32*)&src->ptr[(yy*4+y)*src->stride];
		unsigned int32 rgba = gather_uint(src_ptr, xx*4+x);

		block[16*0+y*4+x] = (int)((rgba>> 0)&255);
		block[16*1+y*4+x] = (int)((rgba>> 8)&255);
		block[16*2+y*4+x] = (int)((rgba>>16)&255);
		block[16*3+y*4+x] = (int)((rgba>>24)&255);
	}
}

inline void load_block_interleaved_16bit(float block[48], uniform rgba_surface* uniform src, int xx, uniform int yy)
{
    for (uniform int y = 0; y<4; y++)
    for (uniform int x = 0; x<4; x++)
    {
        uniform unsigned int32* uniform src_ptr_r = (unsigned int32*)&src->ptr[(yy * 4 + y)*src->stride + 0];
        uniform unsigned int32* uniform src_ptr_g = (unsigned int32*)&src->ptr[(yy * 4 + y)*src->stride + 2];
        uniform unsigned int32* uniform src_ptr_b = (unsigned int32*)&src->ptr[(yy * 4 + y)*src->stride + 4];
        unsigned int32 xr = gather_uint(src_ptr_r, (xx * 4 + x) * 2);
        unsigned int32 xg = gather_uint(src_ptr_g, (xx * 4 + x) * 2);
        unsigned int32 xb = gather_uint(src_ptr_b, (xx * 4 + x) * 2);

        block[16 * 0 + y * 4 + x] = (int)(xr & 0xFFFF);
        block[16 * 1 + y * 4 + x] = (int)(xg & 0xFFFF);
        block[16 * 2 + y * 4 + x] = (int)(xb & 0xFFFF);
        block[16 * 3 + y * 4 + x] = 0;
    }
}

inline void store_data(uniform uint8 dst[], int width, int xx, uniform int yy, uint32 data[], int data_size)
{
	for (uniform int k=0; k<data_size; k++)
	{
		uniform uint32* dst_ptr = (uint32*)&dst[(yy)*width*data_size];
		scatter_uint(dst_ptr, xx*data_size+k, data[k]);
	}
}

inline void ssymv(float a[3], float covar[6], float b[3])
{
	a[0] = covar[0]*b[0]+covar[1]*b[1]+covar[2]*b[2];
	a[1] = covar[1]*b[0]+covar[3]*b[1]+covar[4]*b[2];
	a[2] = covar[2]*b[0]+covar[4]*b[1]+covar[5]*b[2];
}

inline void ssymv3(float a[4], float covar[10], float b[4])
{
	a[0] = covar[0]*b[0]+covar[1]*b[1]+covar[2]*b[2];
	a[1] = covar[1]*b[0]+covar[4]*b[1]+covar[5]*b[2];
	a[2] = covar[2]*b[0]+covar[5]*b[1]+covar[7]*b[2];
}

inline void ssymv4(float a[4], float covar[10], float b[4])
{
	a[0] = covar[0]*b[0]+covar[1]*b[1]+covar[2]*b[2]+covar[3]*b[3];
	a[1] = covar[1]*b[0]+covar[4]*b[1]+covar[5]*b[2]+covar[6]*b[3];
	a[2] = covar[2]*b[0]+covar[5]*b[1]+covar[7]*b[2]+covar[8]*b[3];
	a[3] = covar[3]*b[0]+covar[6]*b[1]+covar[8]*b[2]+covar[9]*b[3];
}

inline void compute_axis3(float axis[3], float covar[6], uniform const int powerIterations)
{
	float vec[3] = {1,1,1};

    for (uniform int i=0; i<powerIterations; i++)
	{
		ssymv(axis, covar, vec);
		for (uniform int p=0; p<3; p++) vec[p] = axis[p];

		if (i%2==1) // renormalize every other iteration
		{
			float norm_sq = 0;
			for (uniform int p=0; p<3; p++)
				norm_sq += axis[p]*axis[p];

			float rnorm = rsqrt(norm_sq);
			for (uniform int p=0; p<3; p++) vec[p] *= rnorm;
		}		
	}

	for (uniform int p=0; p<3; p++) axis[p] = vec[p];
}

inline void compute_axis(float axis[4], float covar[10], uniform const int powerIterations, uniform int channels)
{
	float vec[4] = {1,1,1,1};

    for (uniform int i=0; i<powerIterations; i++)
	{
		if (channels == 3) ssymv3(axis, covar, vec);
        if (channels == 4) ssymv4(axis, covar, vec);
		for (uniform int p=0; p<channels; p++) vec[p] = axis[p];

		if (i%2==1) // renormalize every other iteration
		{
			float norm_sq = 0;
			for (uniform int p=0; p<channels; p++)
				norm_sq += axis[p]*axis[p];

			float rnorm = rsqrt(norm_sq);
			for (uniform int p=0; p<channels; p++) vec[p] *= rnorm;
		}		
	}

	for (uniform int p=0; p<channels; p++) axis[p] = vec[p];
}

///////////////////////////////////////////////////////////
//					 BC1/BC3 encoding

inline int stb__Mul8Bit(int a, int b)
{
  int t = a*b + 128;
  return (t + (t >> 8)) >> 8;
}

inline unsigned int16 stb__As16Bit(int r, int g, int b)
{
   return (stb__Mul8Bit(r,31) << 11) + (stb__Mul8Bit(g,63) << 5) + stb__Mul8Bit(b,31);
}

inline unsigned int16 enc_rgb565(float c[3])
{
	return stb__As16Bit((int)c[0], (int)c[1], (int)c[2]);
}

inline void dec_rgb565(float c[3], int p)
{
	int c2 = (p>>0)&31;
	int c1 = (p>>5)&63;
	int c0 = (p>>11)&31;

	c[0] = (c0<<3)+(c0>>2);
	c[1] = (c1<<2)+(c1>>4);
	c[2] = (c2<<3)+(c2>>2);
}

inline void pick_endpoints_dc(int c0[3], int c1[3], int block[48], int iaxis[3])
{
	for (uniform int p=0; p<3; p++)
	for (uniform int y=0; y<4; y++)
	for (uniform int x=0; x<4; x++)
	{
		c0[p] += block[p*16+y*4+x];
	}

	for (uniform int p=0; p<3; p++)
		c0[p] >>= 4;
}

inline void pick_endpoints(float c0[3], float c1[3], float block[48], float axis[3], float dc[3])
{
	float min_dot = 256*256;
	float max_dot = 0;

	for (uniform int y=0; y<4; y++)
	for (uniform int x=0; x<4; x++)
	{
		float dot = 0;
		for (uniform int p=0; p<3; p++)
			dot += (block[p*16+y*4+x]-dc[p])*axis[p];
	
		min_dot = min(min_dot, dot);
		max_dot = max(max_dot, dot);
	}

	if (max_dot-min_dot < 1f)
	{
		min_dot -= 0.5f;
		max_dot += 0.5f;
	}

	float norm_sq = 0;
	for (uniform int p=0; p<3; p++)
		norm_sq += axis[p]*axis[p];

	float rnorm_sq = rcp(norm_sq);
	for (uniform int p=0; p<3; p++)
	{
		c0[p] = clamp(dc[p]+min_dot*rnorm_sq*axis[p], 0, 255);
		c1[p] = clamp(dc[p]+max_dot*rnorm_sq*axis[p], 0, 255);
	}
}

inline uint32 fast_quant(float block[48], int p0, int p1)
{
	float c0[3];
	float c1[3];
	dec_rgb565(c0, p0);
	dec_rgb565(c1, p1);

	float dir[3];
    for (uniform int p=0; p<3; p++) dir[p] = c1[p]-c0[p];
    
	float sq_norm = 0;
	for (uniform int p=0; p<3; p++) sq_norm += sq(dir[p]);

	float rsq_norm = rcp(sq_norm);

	for (uniform int p=0; p<3; p++) dir[p] *= rsq_norm*3;

	float bias = 0.5;
	for (uniform int p=0; p<3; p++) bias -= c0[p]*dir[p];

    uint32 bits = 0;    
	uint32 scaler = 1;
    for (uniform int k=0; k<16; k++)
    {
		float dot = 0;
        for (uniform int p=0; p<3; p++)
			dot += block[k+p*16]*dir[p];

		int q = clamp((int)(dot+bias), 0, 3);

		//bits += q<<(k*2);
		bits += q*scaler;
		scaler *= 4;
    }
	
    return bits;
}

inline void compute_covar_dc(float covar[6], float dc[3], float block[48])
{
	for (uniform int i=0; i<6; i++) covar[i] = 0;
	for (uniform int p=0; p<3; p++) dc[p] = 0;

	for (uniform int k=0; k<16; k++)
	{
		for (uniform int p=0; p<3; p++)
			dc[p] += block[k+p*16];
	}

	for (uniform int p=0; p<3; p++) dc[p] /= 16;
	
	for (uniform int k=0; k<16; k++)
	{
		float rgb[3];
		for (uniform int p=0; p<3; p++)
			rgb[p] = block[k+p*16]-dc[p];
		
		covar[0] += rgb[0]*rgb[0];
		covar[1] += rgb[0]*rgb[1];
		covar[2] += rgb[0]*rgb[2];
		
		covar[3] += rgb[1]*rgb[1];
		covar[4] += rgb[1]*rgb[2];

		covar[5] += rgb[2]*rgb[2];
	}
}

// ugly, but makes BC1 compression 20% faster overall
inline void compute_covar_dc_ugly(float covar[6], float dc[3], float block[48])
{
	for (uniform int p=0; p<3; p++)
	{
		float acc = 0;
		for (uniform int k=0; k<16; k++)
			acc += block[k+p*16];
		dc[p] = acc/16;
	}
	
	float covar0 = 0f;
	float covar1 = 0f;
	float covar2 = 0f;
	float covar3 = 0f;
	float covar4 = 0f;
	float covar5 = 0f;

	for (uniform int k=0; k<16; k++)
	{
		float rgb0, rgb1, rgb2;
		rgb0 = block[k+0*16]-dc[0];
		rgb1 = block[k+1*16]-dc[1];
		rgb2 = block[k+2*16]-dc[2];
		
		covar0 += rgb0*rgb0;
		covar1 += rgb0*rgb1;
		covar2 += rgb0*rgb2;
		
		covar3 += rgb1*rgb1;
		covar4 += rgb1*rgb2;

		covar5 += rgb2*rgb2;
	}

	covar[0] = covar0;
	covar[1] = covar1;
	covar[2] = covar2;
	covar[3] = covar3;
	covar[4] = covar4;
	covar[5] = covar5;
}

inline void bc1_refine(int pe[2], float block[48], unsigned int32 bits, float dc[3])
{
	float c0[3];
	float c1[3];

	if ((bits ^ (bits*4)) < 4)
    {
        // single color
        for (uniform int p=0; p<3; p++)
        {
            c0[p] = dc[p];
            c1[p] = dc[p];
        }
    }
    else
	{
		float Atb1[3] = {0,0,0};
		float sum_q = 0;
		float sum_qq = 0;
		unsigned int32 shifted_bits = bits;
               
        for (uniform int k=0; k<16; k++)
        {
            float q = (int)(shifted_bits&3);
			shifted_bits >>= 2;

            float x = 3-q;
            float y = q;
            
			sum_q += q;
			sum_qq += q*q;

            for (uniform int p=0; p<3; p++) Atb1[p] += x*block[k+p*16];
        }
        
		float sum[3];
		float Atb2[3];

		for (uniform int p=0; p<3; p++) 
		{
			sum[p] = dc[p]*16;
		    Atb2[p] = 3*sum[p]-Atb1[p];
		}
        
	    float Cxx = 16*sq(3)-2*3*sum_q+sum_qq;
	    float Cyy = sum_qq;
		float Cxy = 3*sum_q-sum_qq;
		float scale = 3f * rcp(Cxx*Cyy - Cxy*Cxy);

        for (uniform int p=0; p<3; p++)
        {
            c0[p] = (Atb1[p]*Cyy - Atb2[p]*Cxy)*scale;
            c1[p] = (Atb2[p]*Cxx - Atb1[p]*Cxy)*scale;
			
			c0[p] = clamp(c0[p], 0, 255);
			c1[p] = clamp(c1[p], 0, 255);
        }
    }

	pe[0] = enc_rgb565(c0);
    pe[1] = enc_rgb565(c1);
}

inline uint32 fix_qbits(uint32 qbits)
{
	uniform const uint32 mask_01b = 0x55555555;
	uniform const uint32 mask_10b = 0xAAAAAAAA;

	uint32 qbits0 = qbits&mask_01b;
	uint32 qbits1 = qbits&mask_10b;
	qbits = (qbits1>>1) + (qbits1 ^ (qbits0<<1));

	return qbits;
}

inline void CompressBlockBC1_core(float block[48], uint32 data[2])
{
	uniform const int powerIterations = 4;
    uniform const int refineIterations = 1;
    
	float covar[6];
	float dc[3];
	compute_covar_dc_ugly(covar, dc, block);
	
	float eps = 0.001;
	covar[0] += eps;
	covar[3] += eps;
	covar[5] += eps;
	
	float axis[3];
	compute_axis3(axis, covar, powerIterations);
		
    float c0[3];
    float c1[3];
    pick_endpoints(c0, c1, block, axis, dc);
	
	int p[2];
    p[0] = enc_rgb565(c0);
    p[1] = enc_rgb565(c1);
	if (p[0]<p[1]) swap_ints(&p[0], &p[1], 1);
	
	data[0] = (1<<16)*p[1]+p[0];
	data[1] = fast_quant(block, p[0], p[1]);
    	
    // refine
    for (uniform int i=0; i<refineIterations; i++)
    {
        bc1_refine(p, block, data[1], dc);
		if (p[0]<p[1]) swap_ints(&p[0], &p[1], 1);
        data[0] = (1<<16)*p[1]+p[0];
		data[1] = fast_quant(block, p[0], p[1]);
    }
	
	data[1] = fix_qbits(data[1]);
}

inline void CompressBlockBC3_alpha(float block[16], uint32 data[2])
{
    float ep[2] = { 255, 0 };
	
    for (uniform int k=0; k<16; k++)
	{
		ep[0] = min(ep[0], block[k]);
		ep[1] = max(ep[1], block[k]);
	}
    
    if (ep[0] == ep[1]) ep[1] = ep[0]+0.1f;
	    
    uint32 qblock[2] = { 0, 0 };
    float scale = 7f/(ep[1]-ep[0]);

    for (uniform int k=0; k<16; k++)
    {
        float v = block[k];
        float proj = (v-ep[0])*scale+0.5f;

        int q = clamp((int)proj, 0, 7);

		q = 7-q;

        if (q > 0) q++;
        if (q==8) q = 1;

        qblock[k/8] |= q << ((k%8)*3);
    }

	// (could be improved by refinement)
    
    data[0] = clamp((int)ep[0], 0, 255)*256+clamp((int)ep[1], 0, 255);
    data[0] |= qblock[0]<<16;
    data[1] = qblock[0]>>16;
    data[1] |= qblock[1]<<8;
}

inline void CompressBlockBC1(uniform rgba_surface src[], int xx, uniform int yy, uniform uint8 dst[])
{
	float block[48];
    uint32 data[2];

	load_block_interleaved(block, src, xx, yy);
	
    CompressBlockBC1_core(block, data);

	store_data(dst, src->width, xx, yy, data, 2);
}

inline void CompressBlockBC3(uniform rgba_surface src[], int xx, uniform int yy, uniform uint8 dst[])
{
	float block[64];
    uint32 data[4];

	load_block_interleaved_rgba(block, src, xx, yy);
	
    CompressBlockBC3_alpha(&block[48], &data[0]);
    CompressBlockBC1_core(block, &data[2]);

	store_data(dst, src->width, xx, yy, data, 4);
}

export void CompressBlocksBC1_ispc(uniform rgba_surface src[], uniform uint8 dst[])
{	
	for (uniform int yy = 0; yy<src->height/4; yy++)
	foreach (xx = 0 ... src->width/4)
	{
		CompressBlockBC1(src, xx, yy, dst);
	}
}

export void CompressBlocksBC3_ispc(uniform rgba_surface src[], uniform uint8 dst[])
{	
	for (uniform int yy = 0; yy<src->height/4; yy++)
	foreach (xx = 0 ... src->width/4)
	{
		CompressBlockBC3(src, xx, yy, dst);
	}
}

///////////////////////////////////////////////////////////
//					 BC7 encoding

struct bc7_enc_settings
{
	bool mode_selection[4];
	int refineIterations[8];

    bool skip_mode2;
	int fastSkipTreshold_mode1;
	int fastSkipTreshold_mode3;
	int fastSkipTreshold_mode7;

    int mode45_channel0;
	int refineIterations_channel;

    int channels;
};

struct bc7_enc_state
{
	float block[64];

    float opaque_err;       // error for coding alpha=255
	float best_err;
	uint32 best_data[5];	// 4, +1 margin for skips

	// settings
	uniform bool mode_selection[4];
	uniform int refineIterations[8];

    uniform bool skip_mode2;
	uniform int fastSkipTreshold_mode1;
	uniform int fastSkipTreshold_mode3;
	uniform int fastSkipTreshold_mode7;

    uniform int mode45_channel0;
	uniform int refineIterations_channel;

    uniform int channels;
};

struct mode45_parameters
{
	int qep[8];
	uint32 qblock[2];
	int aqep[2];
	uint32 aqblock[2];
	int rotation;
	int swap;
};

void bc7_code_mode01237(uint32 data[5], int qep[6], uint32 qblock[2], int part_id, uniform int mode);
void bc7_code_mode45(uint32 data[5], mode45_parameters params[], uniform int mode);
void bc7_code_mode6(uint32 data[5], int qep[8], uint32 qblock[2]);

///////////////////////////
//   BC7 format data

inline uniform const int* uniform get_unquant_table(uniform int bits)
{
    assert(bits>=2 && bits<=4); // invalid bit size

    static uniform const int unquant_table_2bits[] = { 0, 21, 43, 64 };
    static uniform const int unquant_table_3bits[] = { 0, 9, 18, 27, 37, 46, 55, 64 };
    static uniform const int unquant_table_4bits[] = { 0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64 };
    
	uniform const int* uniform unquant_tables[] = {unquant_table_2bits, unquant_table_3bits, unquant_table_4bits};

    return unquant_tables[bits-2];
}

inline uint32 get_pattern(int part_id)
{
	static uniform const uint32 pattern_table[] = {
        0x50505050u, 0x40404040u, 0x54545454u, 0x54505040u, 0x50404000u, 0x55545450u, 0x55545040u, 0x54504000u,
		0x50400000u, 0x55555450u, 0x55544000u, 0x54400000u, 0x55555440u, 0x55550000u, 0x55555500u, 0x55000000u,
		0x55150100u, 0x00004054u, 0x15010000u, 0x00405054u, 0x00004050u, 0x15050100u, 0x05010000u, 0x40505054u,
		0x00404050u, 0x05010100u, 0x14141414u, 0x05141450u, 0x01155440u, 0x00555500u, 0x15014054u, 0x05414150u,
		0x44444444u, 0x55005500u, 0x11441144u, 0x05055050u, 0x05500550u, 0x11114444u, 0x41144114u, 0x44111144u,
		0x15055054u, 0x01055040u, 0x05041050u, 0x05455150u, 0x14414114u, 0x50050550u, 0x41411414u, 0x00141400u,
		0x00041504u, 0x00105410u, 0x10541000u, 0x04150400u, 0x50410514u, 0x41051450u, 0x05415014u, 0x14054150u,
		0x41050514u, 0x41505014u, 0x40011554u, 0x54150140u, 0x50505500u, 0x00555050u, 0x15151010u, 0x54540404u,
		0xAA685050u, 0x6A5A5040u, 0x5A5A4200u, 0x5450A0A8u, 0xA5A50000u, 0xA0A05050u, 0x5555A0A0u, 0x5A5A5050u,
		0xAA550000u, 0xAA555500u, 0xAAAA5500u, 0x90909090u, 0x94949494u, 0xA4A4A4A4u, 0xA9A59450u, 0x2A0A4250u,
		0xA5945040u, 0x0A425054u, 0xA5A5A500u, 0x55A0A0A0u, 0xA8A85454u, 0x6A6A4040u, 0xA4A45000u, 0x1A1A0500u,
		0x0050A4A4u, 0xAAA59090u, 0x14696914u, 0x69691400u, 0xA08585A0u, 0xAA821414u, 0x50A4A450u, 0x6A5A0200u,
		0xA9A58000u, 0x5090A0A8u, 0xA8A09050u, 0x24242424u, 0x00AA5500u, 0x24924924u, 0x24499224u, 0x50A50A50u,
		0x500AA550u, 0xAAAA4444u, 0x66660000u, 0xA5A0A5A0u, 0x50A050A0u, 0x69286928u, 0x44AAAA44u, 0x66666600u,
		0xAA444444u, 0x54A854A8u, 0x95809580u, 0x96969600u, 0xA85454A8u, 0x80959580u, 0xAA141414u, 0x96960000u,
		0xAAAA1414u, 0xA05050A0u, 0xA0A5A5A0u, 0x96000000u, 0x40804080u, 0xA9A8A9A8u, 0xAAAAAA44u, 0x2A4A5254u
	};

	return gather_uint(pattern_table, part_id);
}

inline int get_pattern_mask(int part_id, int j)
{
    static uniform const uint32 pattern_mask_table[] = {
		0xCCCC3333u, 0x88887777u, 0xEEEE1111u, 0xECC81337u, 0xC880377Fu, 0xFEEC0113u, 0xFEC80137u, 0xEC80137Fu,
		0xC80037FFu, 0xFFEC0013u, 0xFE80017Fu, 0xE80017FFu, 0xFFE80017u, 0xFF0000FFu, 0xFFF0000Fu, 0xF0000FFFu,
		0xF71008EFu, 0x008EFF71u, 0x71008EFFu, 0x08CEF731u, 0x008CFF73u, 0x73108CEFu, 0x3100CEFFu, 0x8CCE7331u,
		0x088CF773u, 0x3110CEEFu, 0x66669999u, 0x366CC993u, 0x17E8E817u, 0x0FF0F00Fu, 0x718E8E71u, 0x399CC663u,
		0xAAAA5555u, 0xF0F00F0Fu, 0x5A5AA5A5u, 0x33CCCC33u, 0x3C3CC3C3u, 0x55AAAA55u, 0x96966969u, 0xA55A5AA5u,
		0x73CE8C31u, 0x13C8EC37u, 0x324CCDB3u, 0x3BDCC423u, 0x69969669u, 0xC33C3CC3u, 0x99666699u, 0x0660F99Fu,
		0x0272FD8Du, 0x04E4FB1Bu, 0x4E40B1BFu, 0x2720D8DFu, 0xC93636C9u, 0x936C6C93u, 0x39C6C639u, 0x639C9C63u,
		0x93366CC9u, 0x9CC66339u, 0x817E7E81u, 0xE71818E7u, 0xCCF0330Fu, 0x0FCCF033u, 0x774488BBu, 0xEE2211DDu,
		0x08CC0133u, 0x8CC80037u, 0xCC80006Fu, 0xEC001331u, 0x330000FFu, 0x00CC3333u, 0xFF000033u, 0xCCCC0033u,
		0x0F0000FFu, 0x0FF0000Fu, 0x00F0000Fu, 0x44443333u, 0x66661111u, 0x22221111u, 0x136C0013u, 0x008C8C63u,
		0x36C80137u, 0x08CEC631u, 0x3330000Fu, 0xF0000333u, 0x00EE1111u, 0x88880077u, 0x22C0113Fu, 0x443088CFu,
		0x0C22F311u, 0x03440033u, 0x69969009u, 0x9960009Fu, 0x03303443u, 0x00660699u, 0xC22C3113u, 0x8C0000EFu,
		0x1300007Fu, 0xC4003331u, 0x004C1333u, 0x22229999u, 0x00F0F00Fu, 0x24929249u, 0x29429429u, 0xC30C30C3u,
		0xC03C3C03u, 0x00AA0055u, 0xAA0000FFu, 0x30300303u, 0xC0C03333u, 0x90900909u, 0xA00A5005u, 0xAAA0000Fu,
		0x0AAA0555u, 0xE0E01111u, 0x70700707u, 0x6660000Fu, 0x0EE01111u, 0x07707007u, 0x06660999u, 0x660000FFu,
		0x00660099u, 0x0CC03333u, 0x03303003u, 0x60000FFFu, 0x80807777u, 0x10100101u, 0x000A0005u, 0x08CE8421u
	};

	uint32 mask_packed = gather_uint(pattern_mask_table, part_id);
	int mask0 = mask_packed&0xFFFF;
	int mask1 = mask_packed>>16;

	int mask = (j==2) ? (~mask0)&(~mask1) : ( (j==0) ? mask0 : mask1 );
	return mask;
}

inline void get_skips(int skips[3], int part_id)
{
	static uniform const int skip_table[] = {
        0xf0u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 
        0xf0u, 0x20u, 0x80u, 0x20u, 0x20u, 0x80u, 0x80u, 0xf0u, 0x20u, 0x80u, 0x20u, 0x20u, 0x80u, 0x80u, 0x20u, 0x20u,
        0xf0u, 0xf0u, 0x60u, 0x80u, 0x20u, 0x80u, 0xf0u, 0xf0u, 0x20u, 0x80u, 0x20u, 0x20u, 0x20u, 0xf0u, 0xf0u, 0x60u, 
        0x60u, 0x20u, 0x60u, 0x80u, 0xf0u, 0xf0u, 0x20u, 0x20u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 0x20u, 0x20u, 0xf0u,
        0x3fu, 0x38u, 0xf8u, 0xf3u, 0x8fu, 0x3fu, 0xf3u, 0xf8u, 0x8fu, 0x8fu, 0x6fu, 0x6fu, 0x6fu, 0x5fu, 0x3fu, 0x38u, 
        0x3fu, 0x38u, 0x8fu, 0xf3u, 0x3fu, 0x38u, 0x6fu, 0xa8u, 0x53u, 0x8fu, 0x86u, 0x6au, 0x8fu, 0x5fu, 0xfau, 0xf8u,
		0x8fu, 0xf3u, 0x3fu, 0x5au, 0x6au, 0xa8u, 0x89u, 0xfau, 0xf6u, 0x3fu, 0xf8u, 0x5fu, 0xf3u, 0xf6u, 0xf6u, 0xf8u, 
        0x3fu, 0xf3u, 0x5fu, 0x5fu, 0x5fu, 0x8fu, 0x5fu, 0xafu, 0x5fu, 0xafu, 0x8fu, 0xdfu, 0xf3u, 0xcfu, 0x3fu, 0x38u
	};

	int skip_packed = gather_int(skip_table, part_id);
	skips[0] = 0;
	skips[1] = skip_packed>>4;
	skips[2] = skip_packed&15;
}

///////////////////////////
//      PCA helpers

inline void compute_stats_masked(float stats[15], float block[64], int mask, uniform int channels)
{
	for (uniform int i=0; i<15; i++) stats[i] = 0;

	int mask_shifted = mask<<1;
	for (uniform int k=0; k<16; k++)
	{
		mask_shifted >>= 1;
		//if ((mask_shifted&1) == 0) continue;
		int flag = (mask_shifted&1);

		float rgba[4];
		for (uniform int p=0; p<channels; p++) rgba[p] = block[k+p*16];
		
		for (uniform int p=0; p<channels; p++) rgba[p] *= flag;
		stats[14] += flag;

		stats[10] += rgba[0];
		stats[11] += rgba[1];
		stats[12] += rgba[2];

		stats[0] += rgba[0]*rgba[0];
		stats[1] += rgba[0]*rgba[1];
		stats[2] += rgba[0]*rgba[2];

		stats[4] += rgba[1]*rgba[1];
		stats[5] += rgba[1]*rgba[2];

		stats[7] += rgba[2]*rgba[2];

        if (channels==4)
        {
		    stats[13] += rgba[3];

    		stats[3] += rgba[0]*rgba[3];
	    	stats[6] += rgba[1]*rgba[3];
		    stats[8] += rgba[2]*rgba[3];
		    stats[9] += rgba[3]*rgba[3];
        }
	}
}

inline void covar_from_stats(float covar[10], float stats[15], uniform int channels)
{
	covar[0] = stats[0] - stats[10+0]*stats[10+0]/stats[14];
	covar[1] = stats[1] - stats[10+0]*stats[10+1]/stats[14];
	covar[2] = stats[2] - stats[10+0]*stats[10+2]/stats[14];

	covar[4] = stats[4] - stats[10+1]*stats[10+1]/stats[14];
	covar[5] = stats[5] - stats[10+1]*stats[10+2]/stats[14];

	covar[7] = stats[7] - stats[10+2]*stats[10+2]/stats[14];

    if (channels == 4)
    {
        covar[3] = stats[3] - stats[10+0]*stats[10+3]/stats[14];
	    covar[6] = stats[6] - stats[10+1]*stats[10+3]/stats[14];
	    covar[8] = stats[8] - stats[10+2]*stats[10+3]/stats[14];
	    covar[9] = stats[9] - stats[10+3]*stats[10+3]/stats[14];
    }
}

inline void compute_covar_dc_masked(float covar[6], float dc[3], float block[64], int mask, uniform int channels)
{
	float stats[15];
	compute_stats_masked(stats, block, mask, channels);

	covar_from_stats(covar, stats, channels);
	for (uniform int p=0; p<channels; p++) dc[p] = stats[10+p]/stats[14];
}

void block_pca_axis(float axis[4], float dc[4], float block[64], int mask, uniform int channels)
{
	uniform const int powerIterations = 8; // 4 not enough for HQ

    float covar[10];
	compute_covar_dc_masked(covar, dc, block, mask, channels);

    //float var = covar[0] + covar[4] + covar[7] + covar[9] + 256;
    float inv_var = 1.0 / (256 * 256);
    for (uniform int k = 0; k < 10; k++)
    {
        covar[k] *= inv_var;
    }

    float eps = sq(0.001);
    covar[0] += eps;
	covar[4] += eps;
	covar[7] += eps;
	covar[9] += eps;

	compute_axis(axis, covar, powerIterations, channels);
}

void block_segment_core(float ep[], float block[64], int mask, uniform int channels)
{
	float axis[4];
	float dc[4];
	block_pca_axis(axis, dc, block, mask, channels);
	
	float ext[2];
	ext[0] = +1e99;
	ext[1] = -1e99;

	// find min/max
	int mask_shifted = mask<<1;
	for (uniform int k=0; k<16; k++)
	{
		mask_shifted >>= 1;
		if ((mask_shifted&1) == 0) continue;

		float dot = 0;
		for (uniform int p=0; p<channels; p++)
			dot += axis[p]*(block[16*p+k]-dc[p]);

		ext[0] = min(ext[0], dot);
        ext[1] = max(ext[1], dot);
	}

	// create some distance if the endpoints collapse
	if (ext[1]-ext[0] < 1f)
	{
		ext[0] -= 0.5f;
		ext[1] += 0.5f;
	}

    for (uniform int i=0; i<2; i++)
	for (uniform int p=0; p<channels; p++)
	{
        ep[4*i+p] = ext[i]*axis[p]+dc[p];
    }
}

void block_segment(float ep[], float block[64], int mask, uniform int channels)
{
    block_segment_core(ep, block, mask, channels);

	for (uniform int i=0; i<2; i++)
	for (uniform int p=0; p<channels; p++)
	{
		ep[4*i+p] = clamp(ep[4*i+p], 0, 255);
	}
}

float get_pca_bound(float covar[10], uniform int channels)
{
    uniform const int powerIterations = 4; // quite approximative, but enough for bounding

    float inv_var = 1.0 / (256 * 256);
    for (uniform int k = 0; k < 10; k++)
    {
        covar[k] *= inv_var;
    }

	float eps = sq(0.001);
	covar[0] += eps;
	covar[4] += eps;
	covar[7] += eps;

	float axis[4];
	compute_axis(axis, covar, powerIterations, channels);

	float vec[4];
    if (channels == 3) ssymv3(vec, covar, axis);
    if (channels == 4) ssymv4(vec, covar, axis);

	float sq_sum = 0f;
	for (uniform int p=0; p<channels; p++) sq_sum += sq(vec[p]);
	float lambda = sqrt(sq_sum);

	float bound = covar[0]+covar[4]+covar[7];
    if (channels == 4) bound += covar[9];
	bound -= lambda;
	bound = max(bound, 0.0);

	return bound;
}

float block_pca_bound(float block[64], int mask, uniform int channels)
{
	float stats[15];
	compute_stats_masked(stats, block, mask, channels);

	float covar[10];
	covar_from_stats(covar, stats, channels);

	return get_pca_bound(covar, channels);
}

float block_pca_bound_split(float block[64], int mask, float full_stats[15], uniform int channels)
{
    float stats[15];
	compute_stats_masked(stats, block, mask, channels);
    
	float covar1[10];
	covar_from_stats(covar1, stats, channels);
	
	for (uniform int i=0; i<15; i++)
		stats[i] = full_stats[i] - stats[i];

	float covar2[10];
	covar_from_stats(covar2, stats, channels);

	float bound = 0f;
	bound += get_pca_bound(covar1, channels);
	bound += get_pca_bound(covar2, channels);

	return sqrt(bound)*256;
}

///////////////////////////
// endpoint quantization

inline int unpack_to_byte(int v, uniform const int bits)
{
	assert(bits >= 4);
	int vv = v<<(8-bits);
	return vv + shift_right(vv, bits);
}

void ep_quant0367(int qep[], float ep[], uniform int mode, uniform int channels)
{
	uniform int bits = 7;
    if (mode == 0) bits = 4;
    if (mode == 7) bits = 5;

	uniform int levels = 1 << bits;
	uniform int levels2 = levels*2-1;
        
    for (uniform int i=0; i<2; i++)
	{
	    int qep_b[8];
    
		for (uniform int b=0; b<2; b++)
		for (uniform int p=0; p<4; p++)
		{
			int v = (int)((ep[i*4+p]/255f*levels2-b)/2+0.5)*2+b;
			qep_b[b*4+p] = clamp(v, b, levels2-1+b);
		}

		float ep_b[8];
		for (uniform int j=0; j<8; j++)
			ep_b[j] = qep_b[j];

		if (mode==0)
		for (uniform int j=0; j<8; j++)
			ep_b[j] = unpack_to_byte(qep_b[j], 5);
    
        float err0 = 0f;
        float err1 = 0f;
        for (uniform int p=0; p<channels; p++)
        {
            err0 += sq(ep[i*4+p]-ep_b[0+p]);
            err1 += sq(ep[i*4+p]-ep_b[4+p]);
        }

		for (uniform int p=0; p<4; p++)
			qep[i*4+p] = (err0<err1) ? qep_b[0+p] : qep_b[4+p];
    }
}

void ep_quant1(int qep[], float ep[], uniform int mode)
{
	int qep_b[16];
        
    for (uniform int b=0; b<2; b++)
	for (uniform int i=0; i<8; i++)
    {
        int v = ((int)((ep[i]/255f*127f-b)/2+0.5))*2+b;
		qep_b[b*8+i] = clamp(v, b, 126+b);
    }
    
	// dequant
	float ep_b[16];
	for (uniform int k=0; k<16; k++)
        ep_b[k] = unpack_to_byte(qep_b[k], 7);

	float err0 = 0f;
    float err1 = 0f;
    for (uniform int j = 0; j < 2; j++)
    for (uniform int p = 0; p < 3; p++)
    {
        err0 += sq(ep[j * 4 + p] - ep_b[0 + j * 4 + p]);
        err1 += sq(ep[j * 4 + p] - ep_b[8 + j * 4 + p]);
    }

	for (uniform int i=0; i<8; i++)
		qep[i] = (err0<err1) ? qep_b[0+i] : qep_b[8+i];

}

void ep_quant245(int qep[], float ep[], uniform int mode)
{
	uniform int bits = 5;
    if (mode == 5) bits = 7;
    uniform int levels = 1 << bits;
        
	for (uniform int i=0; i<8; i++)
	{
		int v = ((int)(ep[i]/255f*(levels-1)+0.5));
		qep[i] = clamp(v, 0, levels-1);
	}
}

void ep_quant(int qep[], float ep[], uniform int mode, uniform int channels)
{
	assert(mode <= 7);
	static uniform const int pairs_table[] = {3,2,3,2,1,1,1,2};
	uniform const int pairs = pairs_table[mode];

	if (mode == 0 || mode == 3 || mode == 6 || mode == 7)
	{
		for (uniform int i=0; i<pairs; i++)
			ep_quant0367(&qep[i*8], &ep[i*8], mode, channels);
	}
	else if (mode == 1)
	{
		for (uniform int i=0; i<pairs; i++)
			ep_quant1(&qep[i*8], &ep[i*8], mode);
	}
	else if (mode == 2 || mode == 4 || mode == 5)
	{
		for (uniform int i=0; i<pairs; i++)
			ep_quant245(&qep[i*8], &ep[i*8], mode);
	}
	else 
		assert(false);

}

void ep_dequant(float ep[], int qep[], uniform int mode)
{
    assert(mode <= 7);
	static uniform const int pairs_table[] = {3,2,3,2,1,1,1,2};
	uniform const int pairs = pairs_table[mode];
    
	// mode 3, 6 are 8-bit
	if (mode == 3 || mode == 6)
    {
	    for (uniform int i=0; i<8*pairs; i++)
		    ep[i] = qep[i];
    }
    else if (mode == 1 || mode == 5)
    {
	    for (uniform int i=0; i<8*pairs; i++)
            ep[i] = unpack_to_byte(qep[i], 7);
    }
    else if (mode == 0 || mode == 2 || mode == 4)
    {
	    for (uniform int i=0; i<8*pairs; i++)
            ep[i] = unpack_to_byte(qep[i], 5);
    }
    else if (mode == 7)
	{
        for (uniform int i=0; i<8*pairs; i++)
            ep[i] = unpack_to_byte(qep[i], 6);
    }
    else 
		assert(false);
}

void ep_quant_dequant(int qep[], float ep[], uniform int mode, uniform int channels)
{
	ep_quant(qep, ep, mode, channels);
	ep_dequant(ep, qep, mode);
}

///////////////////////////
//   pixel quantization

float block_quant(uint32 qblock[2], float block[64], uniform int bits, float ep[], uint32 pattern, uniform int channels)
{
	float total_err = 0;
	uniform const int* uniform unquant_table = get_unquant_table(bits);
    int levels = 1 << bits;

	// 64-bit qblock: 5% overhead in this function
	for (uniform int k=0; k<2; k++) qblock[k] = 0;

	int pattern_shifted = pattern;
	for (uniform int k=0; k<16; k++)
	{
		int j = pattern_shifted&3;
		pattern_shifted >>= 2;

		float proj = 0;
		float div = 0;
		for (uniform int p=0; p<channels; p++)
        {
			float ep_a = gather_float(ep, 8*j+0+p);
			float ep_b = gather_float(ep, 8*j+4+p);
            proj += (block[k+p*16]-ep_a)*(ep_b-ep_a);
            div += sq(ep_b-ep_a);
        }
        
        proj /= div;
        		
		int q1 = (int)(proj*levels+0.5);
		q1 = clamp(q1, 1, levels-1);
		
		float err0 = 0;
		float err1 = 0;
		int w0 = gather_int(unquant_table, q1-1);
		int w1 = gather_int(unquant_table, q1);

		for (uniform int p=0; p<channels; p++)
		{
			float ep_a = gather_float(ep, 8*j+0+p);
			float ep_b = gather_float(ep, 8*j+4+p);
			float dec_v0 = (int)(((64-w0)*ep_a + w0*ep_b + 32)/64);
			float dec_v1 = (int)(((64-w1)*ep_a + w1*ep_b + 32)/64);
			err0 += sq(dec_v0 - block[k+p*16]);
			err1 += sq(dec_v1 - block[k+p*16]);
		}
		
		int best_err = err1;
		int best_q = q1;
		if (err0<err1)
		{
			best_err = err0;
			best_q = q1-1;
		}

		assert(best_q>=0 && best_q<=levels-1);

		qblock[k/8] += ((uint32)best_q) << 4*(k%8);
		total_err += best_err;
    }

	return total_err;
}

///////////////////////////
// LS endpoint refinement

void opt_endpoints(float ep[], float block[64], uniform int bits, uint32 qblock[2], int mask, uniform int channels)
{
	uniform int levels = 1 << bits;
    
	float Atb1[4] = {0,0,0,0};
	float sum_q = 0;
	float sum_qq = 0;
	float sum[5] = {0,0,0,0,0};
                
	int mask_shifted = mask<<1;
	for (uniform int k1=0; k1<2; k1++)
	{
		uint32 qbits_shifted = qblock[k1];
		for (uniform int k2=0; k2<8; k2++)
		{
			uniform int k = k1*8+k2;
			float q = (int)(qbits_shifted&15);
			qbits_shifted >>= 4;

			mask_shifted >>= 1;
			if ((mask_shifted&1) == 0) continue;
		
			int x = (levels-1)-q;
			int y = q;
            
			sum_q += q;
			sum_qq += q*q;

			sum[4] += 1;
			for (uniform int p=0; p<channels; p++) sum[p] += block[k+p*16];
			for (uniform int p=0; p<channels; p++) Atb1[p] += x*block[k+p*16];
		}
	}
        
	float Atb2[4];
	for (uniform int p=0; p<channels; p++) 
	{
		//sum[p] = dc[p]*16;
		Atb2[p] = (levels-1)*sum[p]-Atb1[p];
	}
        
	float Cxx = sum[4]*sq(levels-1)-2*(levels-1)*sum_q+sum_qq;
	float Cyy = sum_qq;
	float Cxy = (levels-1)*sum_q-sum_qq;
	float scale = (levels-1) / (Cxx*Cyy - Cxy*Cxy);

    for (uniform int p=0; p<channels; p++)
    {
        ep[0+p] = (Atb1[p]*Cyy - Atb2[p]*Cxy)*scale;
        ep[4+p] = (Atb2[p]*Cxx - Atb1[p]*Cxy)*scale;
			
		//ep[0+p] = clamp(ep[0+p], 0, 255);
		//ep[4+p] = clamp(ep[4+p], 0, 255);
    }

	if (abs(Cxx*Cyy - Cxy*Cxy) < 0.001)
	{
		// flatten
		for (uniform int p=0; p<channels; p++)
		{
			ep[0+p] = sum[p]/sum[4];
			ep[4+p] = ep[0+p];
		}
	}
}

//////////////////////////
// parameter estimation

float compute_opaque_err(float block[64], uniform int channels)
{
    if (channels == 3) return 0;
    float err = 0f;
    for (uniform int k=0; k<16; k++)
    {
        err += sq(block[48+k]-255);
    }

    return err;
}

float bc7_enc_mode01237_part_fast(int qep[24], uint32 qblock[2], float block[64], int part_id, uniform int mode)
{
	uint32 pattern = get_pattern(part_id);
	uniform int bits = 2;  if (mode == 0 || mode == 1) bits = 3;
    uniform int pairs = 2; if (mode == 0 || mode == 2) pairs = 3;
    uniform int channels = 3; if (mode == 7) channels = 4;

	float ep[24];
	for (uniform int j=0; j<pairs; j++)
	{
		int mask = get_pattern_mask(part_id, j);
		block_segment(&ep[j*8], block, mask, channels);
	}

	ep_quant_dequant(qep, ep, mode, channels);

	float total_err = block_quant(qblock, block, bits, ep, pattern, channels);
	return total_err;
}

void bc7_enc_mode01237(bc7_enc_state state[], uniform int mode, int part_list[], uniform int part_count)
{
	if (part_count == 0) return;
	uniform int bits = 2;  if (mode == 0 || mode == 1) bits = 3;
    uniform int pairs = 2; if (mode == 0 || mode == 2) pairs = 3;
    uniform int channels = 3; if (mode == 7) channels = 4;

	int best_qep[24];
	uint32 best_qblock[2];
	int best_part_id = -1;
	float best_err = 1e99;

	for (uniform int part=0; part<part_count; part++)
	{
		int part_id = part_list[part]&63;
        if (pairs == 3) part_id += 64;

		int qep[24];
		uint32 qblock[2];
		float err = bc7_enc_mode01237_part_fast(qep, qblock, state->block, part_id, mode);
        
		if (err<best_err)
		{
			for (uniform int i=0; i<8*pairs; i++) best_qep[i] = qep[i];
			for (uniform int k=0; k<2; k++) best_qblock[k] = qblock[k];
			best_part_id = part_id;
			best_err = err;
		}
	}
    
	// refine
    uniform int refineIterations = state->refineIterations[mode];
	for (uniform int _=0; _<refineIterations; _++)
	{
		float ep[24];
		for (uniform int j=0; j<pairs; j++)
		{
			int mask = get_pattern_mask(best_part_id, j);
			opt_endpoints(&ep[j*8], state->block, bits, best_qblock, mask, channels);
		}

		int qep[24];
		uint32 qblock[2];

		ep_quant_dequant(qep, ep, mode, channels);
		
		uint32 pattern = get_pattern(best_part_id);
		float err = block_quant(qblock, state->block, bits, ep, pattern, channels);

		if (err<best_err)
		{
			for (uniform int i=0; i<8*pairs; i++) best_qep[i] = qep[i];
			for (uniform int k=0; k<2; k++) best_qblock[k] = qblock[k];
			best_err = err;
		}
	}
    
	if (mode != 7) best_err += state->opaque_err; // take into account alpha channel

	if (best_err<state->best_err)
    {
        state->best_err = best_err;
        bc7_code_mode01237(state->best_data, best_qep, best_qblock, best_part_id, mode);
    }
}

void partial_sort_list(int list[], uniform int length, uniform int partial_count)
{
	for (uniform int k=0; k<partial_count; k++)
	{
		int best_idx = k;
		int best_value = list[k];
		for (uniform int i=k+1; i<length; i++)
		{
			if (best_value > list[i])
			{
				best_value = list[i];
				best_idx = i;
			}
		}

		// swap
		scatter_int(list, best_idx, list[k]);
		list[k] = best_value;
	}
}

void bc7_enc_mode02(bc7_enc_state state[])
{
	int part_list[64];
	for (uniform int part=0; part<64; part++)
		part_list[part] = part;

	bc7_enc_mode01237(state, 0, part_list, 16); 
	if (!state->skip_mode2) bc7_enc_mode01237(state, 2, part_list, 64); // usually not worth the time
}

void bc7_enc_mode13(bc7_enc_state state[])
{
	if (state->fastSkipTreshold_mode1 == 0 && state->fastSkipTreshold_mode3 == 0) return;

	float full_stats[15];
	compute_stats_masked(full_stats, state->block, -1, 3);

	int part_list[64];
	for (uniform int part=0; part<64; part++)
	{
		int mask = get_pattern_mask(part+0, 0);
		float bound12 = block_pca_bound_split(state->block, mask, full_stats, 3);
		int bound = (int)(bound12);
		part_list[part] = part+bound*64;
	}

	partial_sort_list(part_list, 64, max(state->fastSkipTreshold_mode1, state->fastSkipTreshold_mode3));
	bc7_enc_mode01237(state, 1, part_list, state->fastSkipTreshold_mode1);
	bc7_enc_mode01237(state, 3, part_list, state->fastSkipTreshold_mode3);
}

void bc7_enc_mode7(bc7_enc_state state[])
{
    if (state->fastSkipTreshold_mode7 == 0) return;

	float full_stats[15];
	compute_stats_masked(full_stats, state->block, -1, state->channels);

	int part_list[64];
	for (uniform int part=0; part<64; part++)
	{
		int mask = get_pattern_mask(part+0, 0);
		float bound12 = block_pca_bound_split(state->block, mask, full_stats, state->channels);
		int bound = (int)(bound12);
		part_list[part] = part+bound*64;
	}

	partial_sort_list(part_list, 64, state->fastSkipTreshold_mode7);
	bc7_enc_mode01237(state, 7, part_list, state->fastSkipTreshold_mode7);
}

void channel_quant_dequant(int qep[2], float ep[2], uniform int epbits)
{
	int elevels = (1<<epbits);

	for (uniform int i=0; i<2; i++)
	{
		int v = ((int)(ep[i]/255f*(elevels-1)+0.5));
		qep[i] = clamp(v, 0, elevels-1);
		ep[i] = unpack_to_byte(qep[i], epbits);
	}
}

void channel_opt_endpoints(float ep[2], float block[16], uniform int bits, uint32 qblock[2])
{
	uniform int levels = 1 << bits;

	float Atb1 = 0;
	float sum_q = 0;
	float sum_qq = 0;
	float sum = 0;
                
	for (uniform int k1=0; k1<2; k1++)
	{
		uint32 qbits_shifted = qblock[k1];
		for (uniform int k2=0; k2<8; k2++)
		{
			uniform int k = k1*8+k2;
			float q = (int)(qbits_shifted&15);
			qbits_shifted >>= 4;

			int x = (levels-1)-q;
			int y = q;
            
			sum_q += q;
			sum_qq += q*q;

			sum += block[k];
			Atb1 += x*block[k];
		}
	}
        
	float Atb2 = (levels-1)*sum-Atb1;
        
	float Cxx = 16*sq(levels-1)-2*(levels-1)*sum_q+sum_qq;
	float Cyy = sum_qq;
	float Cxy = (levels-1)*sum_q-sum_qq;
	float scale = (levels-1) / (Cxx*Cyy - Cxy*Cxy);

    ep[0] = (Atb1*Cyy - Atb2*Cxy)*scale;
    ep[1] = (Atb2*Cxx - Atb1*Cxy)*scale;
			
	ep[0] = clamp(ep[0], 0, 255);
	ep[1] = clamp(ep[1], 0, 255);

	if (abs(Cxx*Cyy - Cxy*Cxy) < 0.001)
	{
		ep[0] = sum/16;
		ep[1] = ep[0];
	}
}

float channel_opt_quant(uint32 qblock[2], float block[16], uniform int bits, float ep[])
{
	uniform const int* uniform unquant_table = get_unquant_table(bits);
	int levels = (1<<bits);

	qblock[0] = 0;
	qblock[1] = 0;

	float total_err = 0;

	for (uniform int k=0; k<16; k++)
	{
		float proj = (block[k]-ep[0])/(ep[1]-ep[0]+0.001f);

		int q1 = (int)(proj*levels+0.5);
		q1 = clamp(q1, 1, levels-1);

		float err0 = 0;
		float err1 = 0;
		int w0 = gather_int(unquant_table, q1-1);
		int w1 = gather_int(unquant_table, q1);
		
		float dec_v0 = (int)(((64-w0)*ep[0] + w0*ep[1] + 32)/64);
		float dec_v1 = (int)(((64-w1)*ep[0] + w1*ep[1] + 32)/64);
		err0 += sq(dec_v0 - block[k]);
		err1 += sq(dec_v1 - block[k]);

		int best_err = err1;
		int best_q = q1;
		if (err0<err1)
		{
			best_err = err0;
			best_q = q1-1;
		}

		qblock[k/8] += ((uint32)best_q) << 4*(k%8);
		total_err += best_err;
	}

	return total_err;
}

float opt_channel(bc7_enc_state state[], uint32 qblock[2], int qep[2], float block[16], uniform int bits, uniform int epbits)
{
	float ep[2] = {255,0};

	for (uniform int k=0; k<16; k++)
	{
		ep[0] = min(ep[0], block[k]);
		ep[1] = max(ep[1], block[k]);
	}

	channel_quant_dequant(qep, ep, epbits);
	float err = channel_opt_quant(qblock, block, bits, ep);
		
	// refine
	uniform const int refineIterations = state->refineIterations_channel;
    for (uniform int i=0; i<refineIterations; i++)
	{
		channel_opt_endpoints(ep, block, bits, qblock);
		channel_quant_dequant(qep, ep, epbits);
		err = channel_opt_quant(qblock, block, bits, ep);
	}

	return err;
}

void bc7_enc_mode45_candidate(bc7_enc_state state[], mode45_parameters best_candidate[], 
	float best_err[], uniform int mode, uniform int rotation, uniform int swap)
{
	uniform int bits = 2; 
    uniform int abits = 2;   if (mode==4) abits = 3;
	uniform int aepbits = 8; if (mode==4) aepbits = 6;
	if (swap==1) { bits = 3; abits = 2; } // (mode 4)

	float block[48];
	for (uniform int k=0; k<16; k++)
	{
		for (uniform int p=0; p<3; p++)
			block[k+p*16] = state->block[k+p*16];

		if (rotation < 3)
		{
			// apply channel rotation
			if (state->channels == 4) block[k+rotation*16] = state->block[k+3*16];
			if (state->channels == 3) block[k+rotation*16] = 255;
		}
	}
	
	float ep[8];
	block_segment(ep, block, -1, 3);

	int qep[8];
	ep_quant_dequant(qep, ep, mode, 3);

	uint32 qblock[2];
	float err = block_quant(qblock, block, bits, ep, 0, 3);
	
	// refine
    uniform int refineIterations = state->refineIterations[mode];
	for (uniform int i=0; i<refineIterations; i++)
    {
        opt_endpoints(ep, block, bits, qblock, -1, 3);
        ep_quant_dequant(qep, ep, mode, 3);
		err = block_quant(qblock, block, bits, ep, 0, 3);
    }

	// encoding selected channel 
	int aqep[2];
	uint32 aqblock[2];
	err += opt_channel(state, aqblock, aqep, &state->block[rotation*16], abits, aepbits);

	if (err<*best_err)
	{
		
		swap_ints(best_candidate->qep, qep, 8);
		swap_uints(best_candidate->qblock, qblock, 2);
		swap_ints(best_candidate->aqep, aqep, 2);
		swap_uints(best_candidate->aqblock, aqblock, 2);
		best_candidate->rotation = rotation;
		best_candidate->swap = swap;
		*best_err = err;
	}	
}

void bc7_enc_mode45(bc7_enc_state state[])
{
	mode45_parameters best_candidate;
	float best_err = state->best_err;

	memset(&best_candidate, 0, sizeof(mode45_parameters));

    uniform int channel0 = state->mode45_channel0;
	for (uniform int p=channel0; p<state->channels; p++)
	{
    	bc7_enc_mode45_candidate(state, &best_candidate, &best_err, 4, p, 0);
		bc7_enc_mode45_candidate(state, &best_candidate, &best_err, 4, p, 1);
	}

	// mode 4
	if (best_err<state->best_err)
    {
        state->best_err = best_err;
        bc7_code_mode45(state->best_data, &best_candidate, 4);
    }
    
    for (uniform int p=channel0; p<state->channels; p++)
	{
		bc7_enc_mode45_candidate(state, &best_candidate, &best_err, 5, p, 0);
	}

	// mode 5
	if (best_err<state->best_err)
    {
        state->best_err = best_err;
        bc7_code_mode45(state->best_data, &best_candidate, 5);
    }
}

void bc7_enc_mode6(bc7_enc_state state[])
{
	uniform int mode = 6;
	uniform int bits = 4;
	float ep[8];
    block_segment(ep, state->block, -1, state->channels);
    
	if (state->channels == 3)
	{
		ep[3] = ep[7] = 255;
	}

	int qep[8];
	ep_quant_dequant(qep, ep, mode, state->channels);

	uint32 qblock[2];
	float err = block_quant(qblock, state->block, bits, ep, 0, state->channels);

	// refine
	uniform int refineIterations = state->refineIterations[mode];
    for (uniform int i=0; i<refineIterations; i++)
    {
        opt_endpoints(ep, state->block, bits, qblock, -1, state->channels);
        ep_quant_dequant(qep, ep, mode, state->channels);
		err = block_quant(qblock, state->block, bits, ep, 0, state->channels);
    }
        
    if (err<state->best_err)
    {
        state->best_err = err;
        bc7_code_mode6(state->best_data, qep, qblock);
    }
}

//////////////////////////
// BC7 bitstream coding

void bc7_code_apply_swap_mode456(int qep[], uniform int channels, uint32 qblock[2], uniform int bits)
{
	uniform int levels = 1 << bits;
	if ((qblock[0]&15)>=levels/2)
    {
		swap_ints(&qep[0], &qep[channels], channels);
            
		for (uniform int k=0; k<2; k++)
			qblock[k] = (uint32)(0x11111111*(levels-1)) - qblock[k];
    }

	assert((qblock[0]&15) < levels/2);
}

int bc7_code_apply_swap_mode01237(int qep[], uint32 qblock[2], uniform int mode, int part_id)
{
	uniform int bits = 2;  if (mode == 0 || mode == 1) bits = 3;
    uniform int pairs = 2; if (mode == 0 || mode == 2) pairs = 3;

	int flips = 0;
	uniform int levels = 1 << bits;
	int skips[3];
	get_skips(skips, part_id);

	for (uniform int j=0; j<pairs; j++)
	{
		int k0 = skips[j];
		//int q = (qblock[k0/8]>>((k0%8)*4))&15;
		int q = ((gather_uint(qblock, k0>>3)<<(28-(k0&7)*4))>>28);
		
		if (q>=levels/2)
		{
			swap_ints(&qep[8*j], &qep[8*j+4], 4);
			uint32 pmask = get_pattern_mask(part_id, j);
			flips |= pmask;
		}
    }

	return flips;
}

void put_bits(uint32 data[5], uniform int* uniform pos, uniform int bits, int v)
{
	assert(v<pow2(bits));
	data[*pos/32] |= ((uint32)v) << (*pos%32);
	if (*pos%32+bits>32)
	{
		data[*pos/32+1] |= shift_right(v, 32-*pos%32);
	}
	*pos += bits;
}

inline void data_shl_1bit_from(uint32 data[5], int from)
{
	if (from < 96)
	{
		assert(from > 64+10);

		uint32 shifted = (data[2]>>1) | (data[3]<<31); 
		uint32 mask = (pow2(from-64)-1)>>1;
		data[2] = (mask&data[2]) | (~mask&shifted);
		data[3] = (data[3]>>1) | (data[4]<<31);
		data[4] = data[4]>>1;
	}
	else if (from < 128)
	{
		uint32 shifted = (data[3]>>1) | (data[4]<<31); 
		uint32 mask = (pow2(from-96)-1)>>1;
		data[3] = (mask&data[3]) | (~mask&shifted);
		data[4] = data[4]>>1;
	}
}

void bc7_code_qblock(uint32 data[5], uniform int* uniform pPos, uint32 qblock[2], uniform int bits, int flips)
{
	uniform int levels = 1 << bits;
	int flips_shifted = flips;
	for (uniform int k1=0; k1<2; k1++)
	{
		uint32 qbits_shifted = qblock[k1];
		for (uniform int k2=0; k2<8; k2++)
		{
			int q = qbits_shifted&15;
			if ((flips_shifted&1)>0) q = (levels-1)-q;

			if (k1==0 && k2==0)	put_bits(data, pPos, bits-1, q);
			else				put_bits(data, pPos, bits  , q);
			qbits_shifted >>= 4;
			flips_shifted >>= 1;
		}
	}
}

void bc7_code_adjust_skip_mode01237(uint32 data[5], uniform int mode, int part_id)
{
	uniform int bits = 2;  if (mode == 0 || mode == 1) bits = 3;
    uniform int pairs = 2; if (mode == 0 || mode == 2) pairs = 3;
		
	int skips[3];
	get_skips(skips, part_id);

	if (pairs>2 && skips[1] < skips[2])
	{
		int t = skips[1]; skips[1] = skips[2]; skips[2] = t;
	}

	for (uniform int j=1; j<pairs; j++)
	{
		int k = skips[j];
		data_shl_1bit_from(data, 128+(pairs-1)-(15-k)*bits);
	}
}

void bc7_code_mode01237(uint32 data[5], int qep[], uint32 qblock[2], int part_id, uniform int mode)
{
	uniform int bits = 2;  if (mode == 0 || mode == 1) bits = 3;
    uniform int pairs = 2; if (mode == 0 || mode == 2) pairs = 3;
    uniform int channels = 3; if (mode == 7) channels = 4;

    int flips = bc7_code_apply_swap_mode01237(qep, qblock, mode, part_id);

	for (uniform int k=0; k<5; k++) data[k] = 0;
    uniform int pos = 0;

    // mode 0-3, 7
    put_bits(data, &pos, mode+1, 1<<mode);
    
    // partition
    if (mode==0)
    {
        put_bits(data, &pos, 4, part_id&15);
    }
    else
    {
        put_bits(data, &pos, 6, part_id&63);
    }
    
    // endpoints
    for (uniform int p=0; p<channels; p++)
	for (uniform int j=0; j<pairs*2; j++)
    {
        if (mode == 0)
        {
            put_bits(data, &pos, 4, qep[j*4+0+p]>>1);
        }
        else if (mode == 1)
        {
            put_bits(data, &pos, 6, qep[j*4+0+p]>>1);
        }
        else if (mode == 2)
        {
            put_bits(data, &pos, 5, qep[j*4+0+p]);
        }
        else if (mode == 3)
        {
			put_bits(data, &pos, 7, qep[j*4+0+p]>>1);
        }
        else if (mode == 7)
        {
            put_bits(data, &pos, 5, qep[j*4+0+p]>>1);
        }
        else
        {
            assert(false);
        }
    }
    
    // p bits
    if (mode == 1)
	for (uniform int j=0; j<2; j++)
    {
        put_bits(data, &pos, 1, qep[j*8]&1);
    }
    
    if (mode == 0 || mode == 3 || mode == 7)
    for (uniform int j=0; j<pairs*2; j++)
    {
        put_bits(data, &pos, 1, qep[j*4]&1);
    }
	
    // quantized values
    bc7_code_qblock(data, &pos, qblock, bits, flips);
	bc7_code_adjust_skip_mode01237(data, mode, part_id);
}

void bc7_code_mode45(uint32 data[5], varying mode45_parameters* uniform params, uniform int mode)
{
	int qep[8];
	uint32 qblock[2];
	int aqep[2];
	uint32 aqblock[2];

	swap_ints(params->qep, qep, 8);
	swap_uints(params->qblock, qblock, 2);
	swap_ints(params->aqep, aqep, 2);
	swap_uints(params->aqblock, aqblock, 2);
	int rotation = params->rotation;
	int swap = params->swap;	
	
	uniform int bits = 2; 
    uniform int abits = 2;   if (mode==4) abits = 3;
    uniform int epbits = 7;  if (mode==4) epbits = 5;
    uniform int aepbits = 8; if (mode==4) aepbits = 6;

	if (!swap)
	{
		bc7_code_apply_swap_mode456(qep, 4, qblock, bits);
		bc7_code_apply_swap_mode456(aqep, 1, aqblock, abits);
	}
	else
	{
		swap_uints(qblock, aqblock, 2);
		bc7_code_apply_swap_mode456(aqep, 1, qblock, bits);
		bc7_code_apply_swap_mode456(qep, 4, aqblock, abits);
	}

	for (uniform int k=0; k<5; k++) data[k] = 0;
    uniform int pos = 0;
	
    // mode 4-5
	put_bits(data, &pos, mode+1, 1<<mode);
	
	// rotation
	//put_bits(data, &pos, 2, (rotation+1)%4);
	put_bits(data, &pos, 2, (rotation+1)&3);
    
    if (mode==4)
    {
        put_bits(data, &pos, 1, swap);
    }
    
    // endpoints
    for (uniform int p=0; p<3; p++)
    {
        put_bits(data, &pos, epbits, qep[0+p]);
        put_bits(data, &pos, epbits, qep[4+p]);
    }
    
    // alpha endpoints
    put_bits(data, &pos, aepbits, aqep[0]);
    put_bits(data, &pos, aepbits, aqep[1]);
        
    // quantized values
    bc7_code_qblock(data, &pos, qblock, bits, 0);
    bc7_code_qblock(data, &pos, aqblock, abits, 0);
}

void bc7_code_mode6(uint32 data[5], int qep[8], uint32 qblock[2])
{
	bc7_code_apply_swap_mode456(qep, 4, qblock, 4);

    for (uniform int k=0; k<5; k++) data[k] = 0;
    uniform int pos = 0;

	// mode 6
    put_bits(data, &pos, 7, 64);
    
    // endpoints
    for (uniform int p=0; p<4; p++)
    {
        put_bits(data, &pos, 7, qep[0+p]>>1);
        put_bits(data, &pos, 7, qep[4+p]>>1);
    }
    
    // p bits
    put_bits(data, &pos, 1, qep[0]&1);
    put_bits(data, &pos, 1, qep[4]&1);
	
	// quantized values
    bc7_code_qblock(data, &pos, qblock, 4, 0);
}


//////////////////////////
//       BC7 core

inline void CompressBlockBC7_core(bc7_enc_state state[])
{
	if (state->mode_selection[0]) bc7_enc_mode02(state);
	if (state->mode_selection[1]) bc7_enc_mode13(state);
	if (state->mode_selection[1]) bc7_enc_mode7(state);
	if (state->mode_selection[2]) bc7_enc_mode45(state);
	if (state->mode_selection[3]) bc7_enc_mode6(state);
}

void bc7_enc_copy_settings(bc7_enc_state state[], uniform bc7_enc_settings settings[])
{
	state->channels = settings->channels;
	
	// mode02
	state->mode_selection[0] = settings->mode_selection[0];
	state->skip_mode2 = settings->skip_mode2;

	state->refineIterations[0] = settings->refineIterations[0];
	state->refineIterations[2] = settings->refineIterations[2];

	// mode137
	state->mode_selection[1] = settings->mode_selection[1];
	state->fastSkipTreshold_mode1 = settings->fastSkipTreshold_mode1;
	state->fastSkipTreshold_mode3 = settings->fastSkipTreshold_mode3;
    state->fastSkipTreshold_mode7 = settings->fastSkipTreshold_mode7;

	state->refineIterations[1] = settings->refineIterations[1];
	state->refineIterations[3] = settings->refineIterations[3];
    state->refineIterations[7] = settings->refineIterations[7];

	// mode45
	state->mode_selection[2] = settings->mode_selection[2];
    
    state->mode45_channel0 = settings->mode45_channel0;
	state->refineIterations_channel = settings->refineIterations_channel;
    state->refineIterations[4] = settings->refineIterations[4];
	state->refineIterations[5] = settings->refineIterations[5];

	// mode6
	state->mode_selection[3] = settings->mode_selection[3];

	state->refineIterations[6] = settings->refineIterations[6];
}

inline void CompressBlockBC7(uniform rgba_surface src[], int xx, uniform int yy, uniform uint8 dst[], 
							 uniform bc7_enc_settings settings[])
{
	bc7_enc_state _state;
	varying bc7_enc_state* uniform state = &_state;

    bc7_enc_copy_settings(state, settings);
	load_block_interleaved_rgba(state->block, src, xx, yy);
	state->best_err = 1e99;
	state->opaque_err = compute_opaque_err(state->block, state->channels);

	CompressBlockBC7_core(state);

	store_data(dst, src->width, xx, yy, state->best_data, 4);
}

export void CompressBlocksBC7_ispc(uniform rgba_surface src[], uniform uint8 dst[], uniform bc7_enc_settings settings[])
{
	for (uniform int yy = 0; yy<src->height/4; yy++)
	foreach (xx = 0 ... src->width/4)
	{
		CompressBlockBC7(src, xx, yy, dst, settings);
	}
}

///////////////////////////////////////////////////////////
//					 BC6H encoding

struct bc6h_enc_settings
{
    bool slow_mode;
    bool fast_mode;
    int refineIterations_1p;
    int refineIterations_2p;
    int fastSkipTreshold;
};

struct bc6h_enc_state
{
    float block[64];

    float best_err;
    uint32 best_data[5];	// 4, +1 margin for skips

    float rgb_bounds[6];
    float max_span;
    int max_span_idx;

    int mode;
    int epb;
    int qbounds[8];

    // settings
    uniform bool slow_mode;
    uniform bool fast_mode;
    uniform int refineIterations_1p;
    uniform int refineIterations_2p;
    uniform int fastSkipTreshold;
};

void bc6h_code_2p(uint32 data[5], int pqep[], uint32 qblock[2], int part_id, int mode);
void bc6h_code_1p(uint32 data[5], int qep[8], uint32 qblock[2], int mode);

///////////////////////////
//   BC6H format data

inline uniform int get_mode_prefix(uniform int mode)
{
    static uniform const int mode_prefix_table[] =
    {
        0, 1, 2, 6, 10, 14, 18, 22, 26, 30, 3, 7, 11, 15
    };

    return mode_prefix_table[mode];
}

inline uniform float get_span(uniform int mode)
{
    static uniform const float span_table[] =
    {
        0.9 * 0xFFFF /  64, //  (0) 4 / 10
        0.9 * 0xFFFF /   4, //  (1) 5 / 7
        0.8 * 0xFFFF / 256, //  (2) 3 / 11
        -1, -1,
        0.9 * 0xFFFF /  32, //  (5) 4 / 9
        0.9 * 0xFFFF /  16, //  (6) 4 / 8
        -1, -1,
        0xFFFF,             //  (9) absolute
        0xFFFF,             // (10) absolute
        0.95 * 0xFFFF / 8,  // (11) 8 / 11
        0.95 * 0xFFFF / 32, // (12) 7 / 12
        6,                  // (13) 3 / 16
    };

    uniform int span = span_table[mode];
    assert(span > 0);
    return span;
}

inline uniform int get_mode_bits(uniform int mode)
{
    static uniform const int mode_bits_table[] =
    {
        10,  7, 11, -1, -1,
         9,  8, -1, -1,  6,
        10, 11, 12, 16,
    };

    uniform int mode_bits = mode_bits_table[mode];
    assert(mode_bits > 0);
    return mode_bits;
}

///////////////////////////
// endpoint quantization

inline int unpack_to_uf16(uint32 v, int bits)
{
    if (bits >= 15) return v;
    if (v == 0) return 0;
    if (v == (1<<bits)-1) return 0xFFFF;

    return (v * 2 + 1) << (15-bits);
}

void ep_quant_bc6h(int qep[], float ep[], int bits, uniform int pairs)
{
    int levels = 1 << bits;

    for (uniform int i = 0; i < 8 * pairs; i++)
    {
        int v = ((int)(ep[i] / (256 * 256f - 1) * (levels - 1) + 0.5));
        qep[i] = clamp(v, 0, levels - 1);
    }
}

void ep_dequant_bc6h(float ep[], int qep[], int bits, uniform int pairs)
{
    for (uniform int i = 0; i < 8 * pairs; i++)
        ep[i] = unpack_to_uf16(qep[i], bits);
}

void ep_quant_dequant_bc6h(bc6h_enc_state state[], int qep[], float ep[], uniform int pairs)
{
    int bits = state->epb;
    ep_quant_bc6h(qep, ep, bits, pairs);

    for (uniform int i = 0; i < 2 * pairs; i++)
    for (uniform int p = 0; p < 3; p++)
    {
        qep[i * 4 + p] = clamp(qep[i * 4 + p], state->qbounds[p], state->qbounds[4 + p]);
    }

    ep_dequant_bc6h(ep, qep, bits, pairs);

}

//////////////////////////
// parameter estimation

float bc6h_enc_2p_part_fast(bc6h_enc_state state[], int qep[16], uint32 qblock[2], int part_id)
{
    uint32 pattern = get_pattern(part_id);
    uniform int bits = 3;
    uniform int pairs = 2;
    uniform int channels = 3;

    float ep[16];
    for (uniform int j = 0; j<pairs; j++)
    {
        int mask = get_pattern_mask(part_id, j);
        block_segment_core(&ep[j * 8], state->block, mask, channels);
    }

    ep_quant_dequant_bc6h(state, qep, ep, 2);

    float total_err = block_quant(qblock, state->block, bits, ep, pattern, channels);
    return total_err;

}

void bc6h_enc_2p_list(bc6h_enc_state state[], int part_list[], uniform int part_count)
{
    if (part_count == 0) return;
    uniform int bits = 3;
    uniform int pairs = 2;
    uniform int channels = 3;

    int best_qep[24];
    uint32 best_qblock[2];
    int best_part_id = -1;
    float best_err = 1e99;

    for (uniform int part = 0; part<part_count; part++)
    {
        int part_id = part_list[part] & 31;

        int qep[24];
        uint32 qblock[2];
        float err = bc6h_enc_2p_part_fast(state, qep, qblock, part_id);

        if (err<best_err)
        {
            for (uniform int i = 0; i<8 * pairs; i++) best_qep[i] = qep[i];
            for (uniform int k = 0; k<2; k++) best_qblock[k] = qblock[k];
            best_part_id = part_id;
            best_err = err;
        }
    }

    // refine
    uniform int refineIterations = state->refineIterations_2p;
    for (uniform int _ = 0; _<refineIterations; _++)
    {
        float ep[24];
        for (uniform int j = 0; j<pairs; j++)
        {
            int mask = get_pattern_mask(best_part_id, j);
            opt_endpoints(&ep[j * 8], state->block, bits, best_qblock, mask, channels);
        }

        int qep[24];
        uint32 qblock[2];
        ep_quant_dequant_bc6h(state, qep, ep, 2);

        uint32 pattern = get_pattern(best_part_id);
        float err = block_quant(qblock, state->block, bits, ep, pattern, channels);

        if (err<best_err)
        {
            for (uniform int i = 0; i<8 * pairs; i++) best_qep[i] = qep[i];
            for (uniform int k = 0; k<2; k++) best_qblock[k] = qblock[k];
            best_err = err;
        }
    }

    if (best_err<state->best_err)
    {
        state->best_err = best_err;
        bc6h_code_2p(state->best_data, best_qep, best_qblock, best_part_id, state->mode);
    }
}

void bc6h_enc_2p(bc6h_enc_state state[])
{
    float full_stats[15];
    compute_stats_masked(full_stats, state->block, -1, 3);

    int part_list[32];
    for (uniform int part = 0; part < 32; part++)
    {
        int mask = get_pattern_mask(part, 0);
        float bound12 = block_pca_bound_split(state->block, mask, full_stats, 3);
        int bound = (int)(bound12);
        part_list[part] = part + bound * 64;
    }
    
    partial_sort_list(part_list, 32, state->fastSkipTreshold);
    bc6h_enc_2p_list(state, part_list, state->fastSkipTreshold);
}

void bc6h_enc_1p(bc6h_enc_state state[])
{
    float ep[8];
    block_segment_core(ep, state->block, -1, 3);

    int qep[8];
    ep_quant_dequant_bc6h(state, qep, ep, 1);

    uint32 qblock[2];
    float err = block_quant(qblock, state->block, 4, ep, 0, 3);

    // refine
    uniform int refineIterations = state->refineIterations_1p;
    for (uniform int i = 0; i<refineIterations; i++)
    {
        opt_endpoints(ep, state->block, 4, qblock, -1, 3);
        ep_quant_dequant_bc6h(state, qep, ep, 1);
        err = block_quant(qblock, state->block, 4, ep, 0, 3);
    }

    if (err < state->best_err)
    {
        state->best_err = err;
        bc6h_code_1p(state->best_data, qep, qblock, state->mode);
    }
}

inline void compute_qbounds(bc6h_enc_state state[], float rgb_span[3])
{
    float bounds[8];
    for (uniform int p = 0; p < 3; p++)
    {
        float middle = (state->rgb_bounds[p] + state->rgb_bounds[3 + p]) / 2;

        bounds[  p] = middle - rgb_span[p] / 2;
        bounds[4+p] = middle + rgb_span[p] / 2;
    }

    ep_quant_bc6h(state->qbounds, bounds, state->epb, 1);
}

void compute_qbounds(bc6h_enc_state state[], float span)
{
    float rgb_span[3] = { span, span, span };
    compute_qbounds(state, rgb_span);
}

void compute_qbounds2(bc6h_enc_state state[], float span, int max_span_idx)
{
    float rgb_span[3] = { span, span, span };
    for (uniform int p = 0; p < 3; p++)
    {
        rgb_span[p] *= (p == max_span_idx) ? 2 : 1;
    }
    compute_qbounds(state, rgb_span);
}

void bc6h_test_mode(bc6h_enc_state state[], uniform int mode, uniform bool enc, uniform float margin)
{
    uniform int mode_bits = get_mode_bits(mode);
    uniform float span = get_span(mode);
    float max_span = state->max_span;
    int max_span_idx = state->max_span_idx;

    if (max_span * margin > span) return;

    if (mode >= 10)
    {
        state->epb = mode_bits;
        state->mode = mode;

        compute_qbounds(state, span);
        if (enc) bc6h_enc_1p(state);
    }
    else if (mode <= 1 || mode == 5 || mode == 9)
    {
        state->epb = mode_bits;
        state->mode = mode;

        compute_qbounds(state, span);
        if (enc) bc6h_enc_2p(state);
    }
    else
    {
        state->epb = mode_bits;
        state->mode = mode + max_span_idx;       
        
        compute_qbounds2(state, span, max_span_idx);
        if (enc) bc6h_enc_2p(state);
    }
}

//////////////////////////
// BC6H bitstream coding

int bit_at(int v, uniform int pos)
{
    return (v >> pos) & 1;
}

uint32 reverse_bits(uint32 v, uniform int bits)
{
    if (bits == 2)
    {
        return (v >> 1) + (v & 1) * 2;
    }
    if (bits == 6)
    {
        v = (v & 0x5555) * 2 + ((v >> 1) & 0x5555);
        return (v >> 4) + ((v >> 2) & 3) * 4 + (v & 3) * 16;
    }
    else
    {
        assert(false);
    }
}

void bc6h_pack(uint32 packed[], int qep[], int mode)
{
    if (mode == 0)
    {
        int pred_qep[16];
        for (uniform int p = 0; p < 3; p++)
        {
            pred_qep[     p] = qep[p];
            pred_qep[ 4 + p] = (qep[ 4 + p] - qep[p]) & 31;
            pred_qep[ 8 + p] = (qep[ 8 + p] - qep[p]) & 31;
            pred_qep[12 + p] = (qep[12 + p] - qep[p]) & 31;
        }

        for (uniform int i = 1; i < 4; i++)
        for (uniform int p = 0; p < 3; p++)
        {
            assert(       qep[i * 4 + p] - qep[p] <= 15);
            assert(-16 <= qep[i * 4 + p] - qep[p]);
        }
        
        /*
            g2[4], b2[4], b3[4], 
            r0[9:0], 
            g0[9:0], 
            b0[9:0], 
            r1[4:0], g3[4], g2[3:0],
            g1[4:0], b3[0], g3[3:0], 
            b1[4:0], b3[1], b2[3:0], 
            r2[4:0], b3[2], 
            r3[4:0], b3[3]
        */

        uint32 pqep[10];

        pqep[4] = pred_qep[4] + (pred_qep[ 8 + 1] & 15) * 64;
        pqep[5] = pred_qep[5] + (pred_qep[12 + 1] & 15) * 64;
        pqep[6] = pred_qep[6] + (pred_qep[ 8 + 2] & 15) * 64;

        pqep[4] += bit_at(pred_qep[12 + 1], 4) << 5;
        pqep[5] += bit_at(pred_qep[12 + 2], 0) << 5;
        pqep[6] += bit_at(pred_qep[12 + 2], 1) << 5;

        pqep[8] = pred_qep[ 8] + bit_at(pred_qep[12 + 2], 2) * 32;
        pqep[9] = pred_qep[12] + bit_at(pred_qep[12 + 2], 3) * 32;

        packed[0] = get_mode_prefix(0); 
        packed[0] += bit_at(pred_qep[ 8 + 1], 4) << 2;
        packed[0] += bit_at(pred_qep[ 8 + 2], 4) << 3;
        packed[0] += bit_at(pred_qep[12 + 2], 4) << 4;

        packed[1] = (pred_qep[2] << 20) + (pred_qep[1] << 10) + pred_qep[0];
        packed[2] = (pqep[6] << 20) + (pqep[5] << 10) + pqep[4];
        packed[3] = (pqep[9] << 6) + pqep[8];
    }
    else if (mode == 1)
    {
        int pred_qep[16];
        for (uniform int p = 0; p < 3; p++)
        {
            pred_qep[     p] = qep[p];
            pred_qep[ 4 + p] = (qep[ 4 + p] - qep[p]) & 63;
            pred_qep[ 8 + p] = (qep[ 8 + p] - qep[p]) & 63;
            pred_qep[12 + p] = (qep[12 + p] - qep[p]) & 63;
        }

        for (uniform int i = 1; i < 4; i++)
        for (uniform int p = 0; p < 3; p++)
        {
            assert(       qep[i * 4 + p] - qep[p] <= 31);
            assert(-32 <= qep[i * 4 + p] - qep[p]);
        }
        
        /*
            g2[5], g3[4], g3[5], 
            r0[6:0], b3[0], b3[1], b2[4], 
            g0[6:0], b2[5], b3[2], g2[4], 
            b0[6:0], b3[3], b3[5], b3[4], 
            r1[5:0], g2[3:0], 
            g1[5:0], g3[3:0], 
            b1[5:0], b2[3:0], 
            r2[5:0], 
            r3[5:0]
        */

        uint32 pqep[8];

        pqep[0] = pred_qep[0];
        pqep[0] += bit_at(pred_qep[12 + 2], 0) << 7;
        pqep[0] += bit_at(pred_qep[12 + 2], 1) << 8;
        pqep[0] += bit_at(pred_qep[ 8 + 2], 4) << 9;

        pqep[1] = pred_qep[1];
        pqep[1] += bit_at(pred_qep[ 8 + 2], 5) << 7;
        pqep[1] += bit_at(pred_qep[12 + 2], 2) << 8;
        pqep[1] += bit_at(pred_qep[ 8 + 1], 4) << 9;

        pqep[2] = pred_qep[2];
        pqep[2] += bit_at(pred_qep[12 + 2], 3) << 7;
        pqep[2] += bit_at(pred_qep[12 + 2], 5) << 8;
        pqep[2] += bit_at(pred_qep[12 + 2], 4) << 9;

        pqep[4] = pred_qep[4] + (pred_qep[ 8 + 1] & 15) * 64;
        pqep[5] = pred_qep[5] + (pred_qep[12 + 1] & 15) * 64;
        pqep[6] = pred_qep[6] + (pred_qep[ 8 + 2] & 15) * 64;

        packed[0] = get_mode_prefix(1); 
        packed[0] += bit_at(pred_qep[ 8 + 1], 5) << 2;
        packed[0] += bit_at(pred_qep[12 + 1], 4) << 3;
        packed[0] += bit_at(pred_qep[12 + 1], 5) << 4;

        packed[1] = (pqep[2] << 20) + (pqep[1] << 10) + pqep[0];
        packed[2] = (pqep[6] << 20) + (pqep[5] << 10) + pqep[4];
        packed[3] = (pred_qep[12] << 6) + pred_qep[8];
    }
    else if (mode == 2 || mode == 3 || mode == 4)
    {
        /*
            r0[9:0], g0[9:0], b0[9:0], 
            r1[3:0], xx[y], xx[y], g2[3:0], 
            g1[3:0], xx[y], xx[y], g3[3:0], 
            b1[3:0], xx[y], xx[y], b2[3:0], 
            r2[3:0], xx[y], xx[y], 
            r3[3:0], xx[y], xx[y]
        */

        int dqep[16];
        for (uniform int p = 0; p < 3; p++)
        {
            int mask = 15;
            if (p == mode - 2) mask = 31;
            dqep[p] = qep[p];
            dqep[ 4 + p] = (qep[ 4 + p] - qep[p]) & mask;
            dqep[ 8 + p] = (qep[ 8 + p] - qep[p]) & mask;
            dqep[12 + p] = (qep[12 + p] - qep[p]) & mask;
        }

        for (uniform int i = 1; i < 4; i++)
        for (uniform int p = 0; p < 3; p++)
        {
            int bits = 4;
            if (p == mode - 2) bits = 5;
            assert(                qep[i * 4 + p] - qep[p] <= (1<<bits)/2 - 1);
            assert(-(1<<bits)/2 <= qep[i * 4 + p] - qep[p]);
        }
        
        uint32 pqep[10];

        pqep[0] = dqep[0] & 1023;
        pqep[1] = dqep[1] & 1023;
        pqep[2] = dqep[2] & 1023;

        pqep[4] = dqep[4] + (dqep[ 8 + 1] & 15) * 64;
        pqep[5] = dqep[5] + (dqep[12 + 1] & 15) * 64;
        pqep[6] = dqep[6] + (dqep[ 8 + 2] & 15) * 64;

        pqep[8] = dqep[8];
        pqep[9] = dqep[12];

        if (mode == 2)
        {
            /*
                r0[9:0], g0[9:0], b0[9:0], 
                r1[3:0], r1[4],  r0[10], g2[3:0], 
                g1[3:0], g0[10], b3[0],  g3[3:0], 
                b1[3:0], b0[10], b3[1],  b2[3:0], 
                r2[3:0], r2[4],  b3[2], 
                r3[3:0], r3[4],  b3[3]
            */

            packed[0] = get_mode_prefix(2);

            //
            pqep[5] += bit_at(dqep[0 + 1], 10) << 4;
            pqep[6] += bit_at(dqep[0 + 2], 10) << 4;
            //
            //

            pqep[4] += bit_at(dqep[0 + 0], 10) << 5;
            pqep[5] += bit_at(dqep[12 + 2], 0) << 5;
            pqep[6] += bit_at(dqep[12 + 2], 1) << 5;
            pqep[8] += bit_at(dqep[12 + 2], 2) << 5;
            pqep[9] += bit_at(dqep[12 + 2], 3) << 5;
        }
        if (mode == 3)
        {
            /*
                r0[9:0], g0[9:0], b0[9:0], 
                r1[3:0], r0[10], g3[4],  g2[3:0], 
                g1[3:0], g1[4],  g0[10], g3[3:0], 
                b1[3:0], b0[10], b3[1],  b2[3:0], 
                r2[3:0], b3[0],  b3[2], 
                r3[3:0], g2[4],  b3[3]
            */

            packed[0] = get_mode_prefix(3);

            pqep[4] += bit_at(dqep[0 + 0], 10) << 4;
            //
            pqep[6] += bit_at(dqep[0 + 2], 10) << 4;
            pqep[8] += bit_at(dqep[12 + 2], 0) << 4;
            pqep[9] += bit_at(dqep[ 8 + 1], 4) << 4;

            pqep[4] += bit_at(dqep[12 + 1], 4) << 5;
            pqep[5] += bit_at(dqep[0 + 1], 10) << 5;
            pqep[6] += bit_at(dqep[12 + 2], 1) << 5;
            pqep[8] += bit_at(dqep[12 + 2], 2) << 5;
            pqep[9] += bit_at(dqep[12 + 2], 3) << 5;
        }
        if (mode == 4)
        {
            /*
                r0[9:0], g0[9:0], b0[9:0], 
                r1[3:0], r0[10], b2[4],  g2[3:0], 
                g1[3:0], g0[10], b3[0],  g3[3:0], 
                b1[3:0], b1[4],  b0[10], b2[3:0], 
                r2[3:0], b3[1],  b3[2], 
                r3[3:0], b3[4],  b3[3]
            */

            packed[0] = get_mode_prefix(4);

            pqep[4] += bit_at(dqep[0 + 0], 10) << 4;
            pqep[5] += bit_at(dqep[0 + 1], 10) << 4;
            //
            pqep[8] += bit_at(dqep[12 + 2], 1) << 4;
            pqep[9] += bit_at(dqep[12 + 2], 4) << 4;

            pqep[4] += bit_at(dqep[ 8 + 2], 4) << 5;
            pqep[5] += bit_at(dqep[12 + 2], 0) << 5;
            pqep[6] += bit_at(dqep[0 + 2], 10) << 5;
            pqep[8] += bit_at(dqep[12 + 2], 2) << 5;
            pqep[9] += bit_at(dqep[12 + 2], 3) << 5;
        }

        packed[1] = (pqep[2] << 20) + (pqep[1] << 10) + pqep[0];
        packed[2] = (pqep[6] << 20) + (pqep[5] << 10) + pqep[4];
        packed[3] = (pqep[9] << 6) + pqep[8];
    }
    else if (mode == 5)
    {
        int dqep[16];
        for (uniform int p = 0; p < 3; p++)
        {
            dqep[p] = qep[p];
            dqep[ 4 + p] = (qep[ 4 + p] - qep[p]) & 31;
            dqep[ 8 + p] = (qep[ 8 + p] - qep[p]) & 31;
            dqep[12 + p] = (qep[12 + p] - qep[p]) & 31;
        }

        for (uniform int i = 1; i < 4; i++)
        for (uniform int p = 0; p < 3; p++)
        {
            assert(       qep[i * 4 + p] - qep[p] <= 15);
            assert(-16 <= qep[i * 4 + p] - qep[p]);
        }
     
        /*
            r0[8:0], b2[4],
            g0[8:0], g2[4], 
            b0[8:0], b3[4], 
            r1[4:0], g3[4], g2[3:0],
            g1[4:0], b3[0], g3[3:0], 
            b1[4:0], b3[1], b2[3:0], 
            r2[4:0], b3[2], 
            r3[4:0], b3[3]
        */

        uint32 pqep[10];

        pqep[0] = dqep[0];
        pqep[1] = dqep[1];
        pqep[2] = dqep[2];
        pqep[4] = dqep[4] + (dqep[ 8 + 1] & 15) * 64;
        pqep[5] = dqep[5] + (dqep[12 + 1] & 15) * 64;
        pqep[6] = dqep[6] + (dqep[ 8 + 2] & 15) * 64;
        pqep[8] = dqep[8];
        pqep[9] = dqep[12];

        pqep[0] += bit_at(dqep[ 8 + 2], 4) << 9;
        pqep[1] += bit_at(dqep[ 8 + 1], 4) << 9;
        pqep[2] += bit_at(dqep[12 + 2], 4) << 9;
        
        pqep[4] += bit_at(dqep[12 + 1], 4) << 5;
        pqep[5] += bit_at(dqep[12 + 2], 0) << 5;
        pqep[6] += bit_at(dqep[12 + 2], 1) << 5;

        pqep[8] += bit_at(dqep[12 + 2], 2) << 5;
        pqep[9] += bit_at(dqep[12 + 2], 3) << 5;

        packed[0] = get_mode_prefix(5); 

        packed[1] = (pqep[2] << 20) + (pqep[1] << 10) + pqep[0];
        packed[2] = (pqep[6] << 20) + (pqep[5] << 10) + pqep[4];
        packed[3] = (pqep[9] << 6) + pqep[8];
    }
    else if (mode == 6 || mode == 7 || mode == 8)
    {
        /*
            r0[7:0], xx[y], b2[4],
            g0[7:0], xx[y], g2[4],
            b0[7:0], xx[y], b3[4],
            r1[4:0], xx[y], g2[3:0],
            g1[4:0], xx[y], g3[3:0],
            b1[4:0], xx[y], b2[3:0],
            r2[4:0], xx[y],
            r3[4:0], xx[y]
        */

        int dqep[16];
        for (uniform int p = 0; p < 3; p++)
        {
            int mask = 31;
            if (p == mode - 6) mask = 63;
            dqep[p] = qep[p];
            dqep[ 4 + p] = (qep[ 4 + p] - qep[p]) & mask;
            dqep[ 8 + p] = (qep[ 8 + p] - qep[p]) & mask;
            dqep[12 + p] = (qep[12 + p] - qep[p]) & mask;
        }

        for (uniform int i = 1; i < 4; i++)
        for (uniform int p = 0; p < 3; p++)
        {
            int bits = 5;
            if (p == mode - 6) bits = 6;
            assert(                qep[i * 4 + p] - qep[p] <= (1<<bits)/2 - 1);
            assert(-(1<<bits)/2 <= qep[i * 4 + p] - qep[p]);
        }
        
        uint32 pqep[10];

        pqep[0] = dqep[0];
        pqep[0] += bit_at(dqep[ 8 + 2], 4) << 9;

        pqep[1] = dqep[1];
        pqep[1] += bit_at(dqep[ 8 + 1], 4) << 9;

        pqep[2] = dqep[2];
        pqep[2] += bit_at(dqep[12 + 2], 4) << 9;

        pqep[4] = dqep[4] + (dqep[ 8 + 1] & 15) * 64;
        pqep[5] = dqep[5] + (dqep[12 + 1] & 15) * 64;
        pqep[6] = dqep[6] + (dqep[ 8 + 2] & 15) * 64;

        pqep[8] = dqep[8];
        pqep[9] = dqep[12];

        if (mode == 6)
        {
            /*
                r0[7:0], g3[4], b2[4],
                g0[7:0], b3[2], g2[4],
                b0[7:0], b3[3], b3[4],
                r1[4:0], r1[5], g2[3:0],
                g1[4:0], b3[0], g3[3:0],
                b1[4:0], b3[1], b2[3:0],
                r2[5:0],
                r3[5:0]
            */

            packed[0] = get_mode_prefix(6);

            pqep[0] += bit_at(dqep[12 + 1], 4) << 8;
            pqep[1] += bit_at(dqep[12 + 2], 2) << 8;
            pqep[2] += bit_at(dqep[12 + 2], 3) << 8;
            //
            pqep[5] += bit_at(dqep[12 + 2], 0) << 5;
            pqep[6] += bit_at(dqep[12 + 2], 1) << 5;
            //
            //
        }
        if (mode == 7)
        {
            /*
                r0[7:0], b3[0], b2[4],
                g0[7:0], g2[5], g2[4],
                b0[7:0], g3[5], b3[4],
                r1[4:0], g3[4], g2[3:0],
                g1[4:0], g1[5], g3[3:0],
                b1[4:0], b3[1], b2[3:0],
                r2[4:0], b3[2],
                r3[4:0], b3[3]
            */

            packed[0] = get_mode_prefix(7);

            pqep[0] += bit_at(dqep[12 + 2], 0) << 8;
            pqep[1] += bit_at(dqep[ 8 + 1], 5) << 8;
            pqep[2] += bit_at(dqep[12 + 1], 5) << 8;
            pqep[4] += bit_at(dqep[12 + 1], 4) << 5;
            //
            pqep[6] += bit_at(dqep[12 + 2], 1) << 5;
            pqep[8] += bit_at(dqep[12 + 2], 2) << 5;
            pqep[9] += bit_at(dqep[12 + 2], 3) << 5;
        }
        if (mode == 8)
        {
            /*
                r0[7:0], b3[1], b2[4],
                g0[7:0], b2[5], g2[4],
                b0[7:0], b3[5], b3[4],
                r1[4:0], g3[4], g2[3:0],
                g1[4:0], b3[0], g3[3:0],
                b1[4:0], b1[5], b2[3:0],
                r2[4:0], b3[2],
                r3[4:0], b3[3]
            */

            packed[0] = get_mode_prefix(8);

            pqep[0] += bit_at(dqep[12 + 2], 1) << 8;
            pqep[1] += bit_at(dqep[ 8 + 2], 5) << 8;
            pqep[2] += bit_at(dqep[12 + 2], 5) << 8;
            pqep[4] += bit_at(dqep[12 + 1], 4) << 5;
            pqep[5] += bit_at(dqep[12 + 2], 0) << 5;
            //
            pqep[8] += bit_at(dqep[12 + 2], 2) << 5;
            pqep[9] += bit_at(dqep[12 + 2], 3) << 5;
        }

        packed[1] = (pqep[2] << 20) + (pqep[1] << 10) + pqep[0];
        packed[2] = (pqep[6] << 20) + (pqep[5] << 10) + pqep[4];
        packed[3] = (pqep[9] << 6) + pqep[8];
    }
    else if (mode == 9)
    {
        /*
            r0[5:0], g3[4], b3[0], b3[1], b2[4], // 10
            g0[5:0], g2[5], b2[5], b3[2], g2[4], // 10
            b0[5:0], g3[5], b3[3], b3[5], b3[4], // 10
            r1[5:0], g2[3:0],  // 10
            g1[5:0], g3[3:0],  // 10
            b1[5:0], b2[3:0],  // 10
            r2[5:0],  // 6
            r3[5:0]   // 6
        */

        uint32 pqep[10];

        pqep[0] = qep[0];
        pqep[0] += bit_at(qep[12 + 1], 4) << 6;
        pqep[0] += bit_at(qep[12 + 2], 0) << 7;
        pqep[0] += bit_at(qep[12 + 2], 1) << 8;
        pqep[0] += bit_at(qep[ 8 + 2], 4) << 9;

        pqep[1] = qep[1];
        pqep[1] += bit_at(qep[ 8 + 1], 5) << 6;
        pqep[1] += bit_at(qep[ 8 + 2], 5) << 7;
        pqep[1] += bit_at(qep[12 + 2], 2) << 8;
        pqep[1] += bit_at(qep[ 8 + 1], 4) << 9;

        pqep[2] = qep[2];
        pqep[2] += bit_at(qep[12 + 1], 5) << 6;
        pqep[2] += bit_at(qep[12 + 2], 3) << 7;
        pqep[2] += bit_at(qep[12 + 2], 5) << 8;
        pqep[2] += bit_at(qep[12 + 2], 4) << 9;

        pqep[4] = qep[4] + (qep[ 8 + 1] & 15) * 64;
        pqep[5] = qep[5] + (qep[12 + 1] & 15) * 64;
        pqep[6] = qep[6] + (qep[ 8 + 2] & 15) * 64;

        packed[0] = get_mode_prefix(9);
        packed[1] = (pqep[2] << 20) + (pqep[1] << 10) + pqep[0];
        packed[2] = (pqep[6] << 20) + (pqep[5] << 10) + pqep[4];
        packed[3] = (qep[12] << 6) + qep[8];
    }
    else if (mode == 10)
    {
        // the only mode with nothing to do ~

        packed[0] = get_mode_prefix(10);
        packed[1] = (qep[2] << 20) + (qep[1] << 10) + qep[0];
        packed[2] = (qep[6] << 20) + (qep[5] << 10) + qep[4];
    }
    else if (mode == 11)
    {
        int dqep[8];
        for (uniform int p = 0; p < 3; p++)
        {
            dqep[p] = qep[p];
            dqep[4 + p] = (qep[4 + p] - qep[p]) & 511;
        }
            
        for (uniform int i = 1; i < 2; i++)
        for (uniform int p = 0; p < 3; p++)
        {
            assert(        qep[i * 4 + p] - qep[p] <= 255);
            assert(-256 <= qep[i * 4 + p] - qep[p]);
        }

        /*
            r0[9:0], g0[9:0], b0[9:0],
            r1[8:0], r0[10],
            g1[8:0], g0[10],
            b1[8:0], b0[10]
        */

        uint32 pqep[8];

        pqep[0] = dqep[0] & 1023;
        pqep[1] = dqep[1] & 1023;
        pqep[2] = dqep[2] & 1023;

        pqep[4] = dqep[4] + (dqep[0] >> 10) * 512;
        pqep[5] = dqep[5] + (dqep[1] >> 10) * 512;
        pqep[6] = dqep[6] + (dqep[2] >> 10) * 512;

        packed[0] = get_mode_prefix(11);
        packed[1] = (pqep[2] << 20) + (pqep[1] << 10) + pqep[0];
        packed[2] = (pqep[6] << 20) + (pqep[5] << 10) + pqep[4];
    }    
    else if (mode == 12)
    {
        int dqep[8];
        for (uniform int p = 0; p < 3; p++)
        {
            dqep[p] = qep[p];
            dqep[4 + p] = (qep[4 + p] - qep[p]) & 255;
        }
            
        for (uniform int i = 1; i < 2; i++)
        for (uniform int p = 0; p < 3; p++)
        {
            assert(        qep[i * 4 + p] - qep[p] <= 127);
            assert(-128 <= qep[i * 4 + p] - qep[p]);
        }

        /*
            r0[9:0], g0[9:0], b0[9:0], 
            r1[7:0], r0[10:11], 
            g1[7:0], g0[10:11],
            b1[7:0], b0[10:11]
        */

        uint32 pqep[8];

        pqep[0] = dqep[0] & 1023;
        pqep[1] = dqep[1] & 1023;
        pqep[2] = dqep[2] & 1023;

        pqep[4] = dqep[4] + reverse_bits(dqep[0] >> 10, 2) * 256;
        pqep[5] = dqep[5] + reverse_bits(dqep[1] >> 10, 2) * 256;
        pqep[6] = dqep[6] + reverse_bits(dqep[2] >> 10, 2) * 256;

        packed[0] = get_mode_prefix(12);
        packed[1] = (pqep[2] << 20) + (pqep[1] << 10) + pqep[0];
        packed[2] = (pqep[6] << 20) + (pqep[5] << 10) + pqep[4];
    }
    else if (mode == 13)
    {
        int dqep[8];
        for (uniform int p = 0; p < 3; p++)
        {
            dqep[p] = qep[p];
            dqep[4 + p] = (qep[4 + p] - qep[p]) & 15;
        }
            
        for (uniform int i = 1; i < 2; i++)
        for (uniform int p = 0; p < 3; p++)
        {
            assert(      qep[i * 4 + p] - qep[p] <= 7);
            assert(-8 <= qep[i * 4 + p] - qep[p]);
        }

        /*
            r0[9:0], g0[9:0], b0[9:0],
            r1[3:0], r0[10:15],
            g1[3:0], g0[10:15],
            b1[3:0], b0[10:15]
        */

        uint32 pqep[8];

        pqep[0] = dqep[0] & 1023;
        pqep[1] = dqep[1] & 1023;
        pqep[2] = dqep[2] & 1023;

        pqep[4] = dqep[4] + reverse_bits(dqep[0] >> 10, 6) * 16;
        pqep[5] = dqep[5] + reverse_bits(dqep[1] >> 10, 6) * 16;
        pqep[6] = dqep[6] + reverse_bits(dqep[2] >> 10, 6) * 16;

        packed[0] = get_mode_prefix(13);
        packed[1] = (pqep[2] << 20) + (pqep[1] << 10) + pqep[0];
        packed[2] = (pqep[6] << 20) + (pqep[5] << 10) + pqep[4];
    }
    else
    {
        assert(false);
    }
}

void bc6h_code_2p(uint32 data[5], int qep[], uint32 qblock[2], int part_id, int mode)
{
	uniform int bits = 3;
    uniform int pairs = 2;
    uniform int channels = 3;

    int flips = bc7_code_apply_swap_mode01237(qep, qblock, 1, part_id);

	for (uniform int k=0; k<5; k++) data[k] = 0;
    uniform int pos = 0;

    uint32 packed[4];
    bc6h_pack(packed, qep, mode);

    // mode
    put_bits(data, &pos, 5, packed[0]);

    // endpoints
    put_bits(data, &pos, 30, packed[1]);
    put_bits(data, &pos, 30, packed[2]);
    put_bits(data, &pos, 12, packed[3]);
    
    // partition
    put_bits(data, &pos, 5, part_id);

    // quantized values
    bc7_code_qblock(data, &pos, qblock, bits, flips);
	bc7_code_adjust_skip_mode01237(data, 1, part_id);
}

void bc6h_code_1p(uint32 data[5], int qep[8], uint32 qblock[2], int mode)
{
    bc7_code_apply_swap_mode456(qep, 4, qblock, 4);

    for (uniform int k = 0; k<5; k++) data[k] = 0;
    uniform int pos = 0;

    uint32 packed[4];
    bc6h_pack(packed, qep, mode);

    // mode
    put_bits(data, &pos, 5, packed[0]);

    // endpoints
    put_bits(data, &pos, 30, packed[1]);
    put_bits(data, &pos, 30, packed[2]);
    
    // quantized values
    bc7_code_qblock(data, &pos, qblock, 4, 0);
}

//////////////////////////
//       BC6H core

void bc6h_setup(bc6h_enc_state state[])
{
    for (uniform int p = 0; p < 3; p++)
    {
        state->rgb_bounds[p  ] = 0xFFFF;
        state->rgb_bounds[3+p] = 0;
    }

    // uf16 conversion, min/max
    for (uniform int p = 0; p < 3; p++)
    for (uniform int k = 0; k < 16; k++)
    {
        state->block[p * 16 + k] = (state->block[p * 16 + k] / 31) * 64;

        state->rgb_bounds[p  ] = min(state->rgb_bounds[p  ], state->block[p * 16 + k]);
        state->rgb_bounds[3+p] = max(state->rgb_bounds[3+p], state->block[p * 16 + k]);
    }

    state->max_span = 0;
    state->max_span_idx = 0;

    float rgb_span[0] = { 0, 0, 0 };
    for (uniform int p = 0; p < 3; p++)
    {
        rgb_span[p] = state->rgb_bounds[3+p] - state->rgb_bounds[p];
        if (rgb_span[p] > state->max_span)
        {
            state->max_span_idx = p;
            state->max_span = rgb_span[p];
        }
    }
}

inline void CompressBlockBC6H_core(bc6h_enc_state state[])
{
    bc6h_setup(state);

    if (state->slow_mode)
    {
        bc6h_test_mode(state, 0, true, 0);
        bc6h_test_mode(state, 1, true, 0);
        bc6h_test_mode(state, 2, true, 0);
        bc6h_test_mode(state, 5, true, 0);
        bc6h_test_mode(state, 6, true, 0);
        bc6h_test_mode(state, 9, true, 0);
        bc6h_test_mode(state, 10, true, 0);
        bc6h_test_mode(state, 11, true, 0);
        bc6h_test_mode(state, 12, true, 0);
        bc6h_test_mode(state, 13, true, 0);
    }
    else
    {        
        if (state->fastSkipTreshold > 0)
        {
            bc6h_test_mode(state, 9, false, 0);
            if (state->fast_mode) bc6h_test_mode(state, 1, false, 1);
            bc6h_test_mode(state, 6, false, 1 / 1.2);
            bc6h_test_mode(state, 5, false, 1 / 1.2);
            bc6h_test_mode(state, 0, false, 1 / 1.2);
            bc6h_test_mode(state, 2, false, 1);

            bc6h_enc_2p(state);
            if (!state->fast_mode) bc6h_test_mode(state, 1, true, 0);
        }

        bc6h_test_mode(state, 10, false, 0);
        bc6h_test_mode(state, 11, false, 1);
        bc6h_test_mode(state, 12, false, 1);
        bc6h_test_mode(state, 13, false, 1);
        bc6h_enc_1p(state);
    } 
}

void bc6h_enc_copy_settings(bc6h_enc_state state[], uniform bc6h_enc_settings settings[])
{
    state->slow_mode = settings->slow_mode;
    state->fast_mode = settings->fast_mode;
    state->fastSkipTreshold = settings->fastSkipTreshold;
    state->refineIterations_1p = settings->refineIterations_1p;
    state->refineIterations_2p = settings->refineIterations_2p;
}

inline void CompressBlockBC6H(uniform rgba_surface src[], int xx, uniform int yy, uniform uint8 dst[], uniform bc6h_enc_settings settings[])
{
    bc6h_enc_state _state;
    varying bc6h_enc_state* uniform state = &_state;

    bc6h_enc_copy_settings(state, settings);
    load_block_interleaved_16bit(state->block, src, xx, yy);
    state->best_err = 1e99;

    CompressBlockBC6H_core(state);

    store_data(dst, src->width, xx, yy, state->best_data, 4);
}

export void CompressBlocksBC6H_ispc(uniform rgba_surface src[], uniform uint8 dst[], uniform bc6h_enc_settings settings[])
{
    for (uniform int yy = 0; yy<src->height / 4; yy++)
    foreach(xx = 0 ... src->width / 4)
    {
        CompressBlockBC6H(src, xx, yy, dst, settings);
    }
}

///////////////////////////////////////////////////////////
//					 ETC encoding

struct etc_enc_settings
{
    int fastSkipTreshold;
};

struct etc_enc_state
{
    float block[64];
    int prev_qcenter[3];

    float best_err;
    uint32 best_data[2];

    uniform bool diff;

    // settings
    uniform int fastSkipTreshold;
};

inline uniform int get_etc1_dY(uniform int table, uniform int q)
{
    static uniform const int etc_codeword_table[8][4] =
    {
        { -8, -2, 2, 8 },
        { -17, -5, 5, 17 },
        { -29, -9, 9, 29 },
        { -42, -13, 13, 42 },
        { -60, -18, 18, 60 },
        { -80, -24, 24, 80 },
        { -106, -33, 33, 106 },
        { -183, -47, 47, 183 },
    };

    return etc_codeword_table[table][q];
}

uniform int remap_q[] = { 2, 3, 1, 0 };

int get_remap2_q(int x)
{
    x -= 2;
    if (x < 0) x = 1 - x;
    return x;
}

int extend_4to8bits(int value)
{
    return (value << 4) | value;
}

int extend_5to8bits(int value)
{
    return (value << 3) | (value >> 2);
}

int quantize_4bits(float value)
{
    return clamp((value / 255.0f) * 15 + 0.5, 0, 15);
}

int quantize_5bits(float value)
{
    return clamp((value / 255.0f) * 31 + 0.5, 0, 31);
}

void center_quant_dequant(int qcenter[3], float center[3], uniform bool diff, int prev_qcenter[3])
{
    if (diff)
    {
        for (uniform int p = 0; p < 3; p++)
        {
            qcenter[p] = quantize_5bits(center[p]);

            if (prev_qcenter[0] >= 0)
            {
                if (qcenter[p] - prev_qcenter[p] > 3) qcenter[p] = prev_qcenter[p] + 3;
                if (qcenter[p] - prev_qcenter[p] < -4) qcenter[p] = prev_qcenter[p] - 4;
            }

            center[p] = extend_5to8bits(qcenter[p]);
        }
    }
    else
    {
        for (uniform int p = 0; p < 3; p++)
        {
            qcenter[p] = quantize_4bits(center[p]);
            center[p] = extend_4to8bits(qcenter[p]);
        }
    }
}

float quantize_pixels_etc1_half(uint32 qblock[1], float block[48], float center[3], uniform int table)
{
    float total_err = 0;
    uint32 bits = 0;

    for (uniform int y = 0; y < 2; y++)
    for (uniform int x = 0; x < 4; x++)
    {
        float best_err = sq(255) * 3;
        int best_q = -1;

        for (uniform int q = 0; q < 4; q++)
        {
            int dY = get_etc1_dY(table, remap_q[q]);

            float err = 0;
            for (int p = 0; p < 3; p++)
                err += sq(block[16 * p + y*4+x] - clamp(center[p] + dY, 0, 255));

            if (err < best_err)
            {
                best_err = err;
                best_q = q;
            }
        }

        assert(best_q >= 0);

        bits |= (best_q  & 1) << (x * 4 + y);
        bits |= (best_q >> 1) << (x * 4 + y + 16);
        total_err += best_err;
    }

    qblock[0] = bits;
    return total_err;
}

float compress_etc1_half_1(uint32 out_qbits[1], int out_table[1], int out_qcenter[3], 
                           float half_pixels[], uniform bool diff, int prev_qcenter[3])
{
    float dc[3];

    for (uniform int p = 0; p<3; p++) dc[p] = 0;

    for (uniform int k = 0; k<8; k++)
    {
        for (uniform int p = 0; p<3; p++)
            dc[p] += half_pixels[k + p * 16];
    }

    float best_error = sq(255) * 3 * 8.0f;
    int best_table = -1;
    int best_qcenter[3];
    uint32 best_qbits;

    for (uniform int table_level = 0; table_level < 8; table_level++)
    {
        float center[3];
        int qcenter[3];
        uint32 qbits;

        for (uniform int p = 0; p < 3; p++) center[p] = dc[p] / 8 - get_etc1_dY(table_level, 2);
        center_quant_dequant(qcenter, center, diff, prev_qcenter);

        float err = quantize_pixels_etc1_half(&qbits, half_pixels, center, table_level);

        if (err < best_error)
        {
            best_error = err;
            best_table = table_level;
            best_qbits = qbits;
            for (uniform int p = 0; p < 3; p++) best_qcenter[p] = qcenter[p];
        }
    }
    
    out_table[0] = best_table;
    out_qbits[0] = best_qbits;
    for (uniform int p = 0; p < 3; p++) out_qcenter[p] = best_qcenter[p];
    return best_error;
}

float optimize_center(float colors[4][10], uniform int p, uniform int table_level)
{
    float best_center = 0;
    for (uniform int q = 0; q < 4; q++)
    {
        best_center += (colors[q][7 + p] - get_etc1_dY(table_level, q)) * colors[q][3];
    }
    best_center /= 8;

    float best_err = 0;
    for (uniform int q = 0; q < 4; q++)
    {
        float dY = get_etc1_dY(table_level, q);
        best_err += sq(clamp(best_center + dY, 0, 255) - colors[q][7 + p]) * colors[q][3];
    }

    for (uniform int branch = 0; branch < 4; branch++)
    {
        float new_center = 0;
        float sum = 0;
        for (uniform int q = 0; q < 4; q++)
        {
            if (branch <= 1 && q <= branch) continue;
            if (branch >= 2 && q >= branch) continue;
            new_center += (colors[q][7 + p] - get_etc1_dY(table_level, q)) * colors[q][3];
            sum += colors[q][3];
        }

        new_center /= sum;

        float err = 0;
        for (uniform int q = 0; q < 4; q++)
        {
            float dY = get_etc1_dY(table_level, q);
            err += sq(clamp(new_center + dY, 0, 255) - colors[q][7 + p]) * colors[q][3];
        }

        if (err < best_err)
        {
            best_err = err;
            best_center = new_center;
        }
    }

    return best_center;
}

float compress_etc1_half_7(uint32 out_qbits[1], int out_table[1], int out_qcenter[3],
                           float half_pixels[], etc_enc_state state[])
{
    int err_list[165];
    int y_sorted_inv[8];
    float y_sorted[8];

    {
        int y_sorted_idx[8];
        for (uniform int k = 0; k < 8; k++)
        {
            float value = 0;
            for (uniform int p = 0; p < 3; p++)
                value += half_pixels[k + p * 16];

            y_sorted_idx[k] = (((int)value) << 4) + k;
        }

        partial_sort_list(y_sorted_idx, 8, 8);

        for (uniform int k = 0; k < 8; k++)
            y_sorted_inv[k] = ((y_sorted_idx[k] & 0xF) << 4) + k;

        for (uniform int k = 0; k < 8; k++)
            y_sorted[k] = (y_sorted_idx[k] >> 4) / 3.0f;

        partial_sort_list(y_sorted_inv, 8, 8);
    }

    uniform int idx = -1;
    for (uniform int level1 = 0; level1 <= 8; level1++)
    for (uniform int level2 = level1; level2 <= 8; level2++)
    for (uniform int level3 = level2; level3 <= 8; level3++)
    {
        idx++;
        assert(idx < 165);
        
        float sum[4];
        float sum_sq[4];
        float count[4];
        float inv_count[4];

        for (uniform int q = 0; q < 4; q++)
        {
            sum[q] = 0;
            sum_sq[q] = 0;
            count[q] = 0;
            inv_count[q] = 0;
        }

        for (uniform int k = 0; k < 8; k++)
        {
            uniform int q = 0;
            if (k >= level1) q = 1;
            if (k >= level2) q = 2;
            if (k >= level3) q = 3;

            sum[q] += y_sorted[k];
            sum_sq[q] += sq(y_sorted[k]);
            count[q] += 1;
        }

        for (uniform int q = 0; q < 4; q++)
        {
            if (count[q] > 0) inv_count[q] = 1 / count[q];
        }

        float base_err = 0;
        for (uniform int q = 0; q < 4; q++) base_err += sum_sq[q] - sq(sum[q]) * inv_count[q];

        float t_err = sq(256) * 8;        
        for (uniform int table_level = 0; table_level < 8; table_level++)
        {
            float center = 0;
            for (uniform int q = 0; q < 4; q++) center += sum[q] - get_etc1_dY(table_level, q) * count[q];
            center /= 8;

            float err = base_err;
            for (uniform int q = 0; q < 4; q++)
            {
                err += sq(center + get_etc1_dY(table_level, q) - sum[q] * inv_count[q])*count[q];
            }

            t_err = min(t_err, err);
        }

        int packed = (level1 * 16 + level2) * 16 + level3;

        err_list[idx] = (((int)t_err) << 12) + packed;
    }

    partial_sort_list(err_list, 165, state->fastSkipTreshold);

    float best_error = sq(255) * 3 * 8.0f;
    int best_table = -1;
    int best_qcenter[3];
    uint32 best_qbits;

    for (uniform int i = 0; i < state->fastSkipTreshold; i++)
    {
        int packed = err_list[i] & 0xFFF;
        int level1 = (packed >> 8) & 0xF;
        int level2 = (packed >> 4) & 0xF;
        int level3 = (packed >> 0) & 0xF;
                
        float colors[4][10];

        for (uniform int p = 0; p < 7; p++)
        for (uniform int q = 0; q < 4; q++) colors[q][p] = 0;

        uint32 qbits = 0;
        for (uniform int kk = 0; kk < 8; kk++)
        {
            int k = y_sorted_inv[kk] & 0xF;

            int qq = 0;
            if (k >= level1) qq = 1;
            if (k >= level2) qq = 2;
            if (k >= level3) qq = 3;

            uniform int xx = kk & 3;
            uniform int yy = kk >> 2;

            int qqq = get_remap2_q(qq);
            qbits |= (qqq & 1) << (yy + xx * 4);
            qbits |= (qqq >> 1) << (16 + yy + xx * 4);

            float qvec[4];
            for (uniform int q = 0; q < 4; q++)
            {
                qvec[q] = q == qq ? 1.0 : 0.0;
                colors[q][3] += qvec[q];
            }

            for (uniform int p = 0; p < 3; p++)
            {
                float value = half_pixels[16 * p + kk];
                for (uniform int q = 0; q < 4; q++)
                {
                    colors[q][p] += value * qvec[q];
                    colors[q][4 + p] += sq(value) * qvec[q];
                }
            }
        }
        
        float base_err = 0;
        for (uniform int q = 0; q < 4; q++)
        {
            if (colors[q][3] > 0)
            for (uniform int p = 0; p < 3; p++)
            {
                colors[q][7 + p] = colors[q][p] / colors[q][3];
                base_err += colors[q][4 + p] - sq(colors[q][7 + p])*colors[q][3];
            }
        }

        for (uniform int table_level = 0; table_level < 8; table_level++)
        {
            float center[3];
            int qcenter[3];
            
            for (uniform int p = 0; p < 3; p++)
            {
                center[p] = optimize_center(colors, p, table_level);
            }
            
            center_quant_dequant(qcenter, center, state->diff, state->prev_qcenter);
            
            float err = base_err;
            for (uniform int q = 0; q < 4; q++)
            {
                int dY = get_etc1_dY(table_level, q);
                for (uniform int p = 0; p < 3; p++)
                    err += sq(clamp(center[p] + dY, 0, 255) - colors[q][7 + p])*colors[q][3];
            }
            
            if (err < best_error)
            {
                best_error = err;
                best_table = table_level;
                best_qbits = qbits;
                for (uniform int p = 0; p < 3; p++) best_qcenter[p] = qcenter[p];
            }
        }
    }

    out_table[0] = best_table;
    out_qbits[0] = best_qbits;
    for (uniform int p = 0; p < 3; p++) out_qcenter[p] = best_qcenter[p];
    return best_error;
}

float compress_etc1_half(uint32 qbits[1], int table[1], int qcenter[3], float half_pixels[], etc_enc_state state[])
{
    float err = compress_etc1_half_7(qbits, table, qcenter, half_pixels, state);

    for (uniform int p = 0; p < 3; p++)
        state->prev_qcenter[p] = qcenter[p];

    return err;
}

//////////////////////////
//       ETC1 core

inline uint32 bswap32(uint32 v)
{
    uint32 r = 0;
    r += ((v >> 24) & 255) << 0;
    r += ((v >> 16) & 255) << 8;
    r += ((v >> 8) & 255) << 16;
    r += ((v >> 0) & 255) << 24;
    return r;
}

void etc_pack(uint32 data[], uint32 qbits[2], int tables[2], int qcenters[2][3], uniform int diff, uniform int flip)
{
    for (uniform int k = 0; k < 2; k++) data[k] = 0;
    uniform int pos = 0;

    if (diff == 0)
    {
        put_bits(data, &pos, 4, qcenters[1][0]);
        put_bits(data, &pos, 4, qcenters[0][0]);

        put_bits(data, &pos, 4, qcenters[1][1]);
        put_bits(data, &pos, 4, qcenters[0][1]);

        put_bits(data, &pos, 4, qcenters[1][2]);
        put_bits(data, &pos, 4, qcenters[0][2]);
    }
    else
    {
        put_bits(data, &pos, 3, (qcenters[1][0] - qcenters[0][0]) & 7);
        put_bits(data, &pos, 5, qcenters[0][0]);

        put_bits(data, &pos, 3, (qcenters[1][1] - qcenters[0][1]) & 7);
        put_bits(data, &pos, 5, qcenters[0][1]);

        put_bits(data, &pos, 3, (qcenters[1][2] - qcenters[0][2]) & 7);
        put_bits(data, &pos, 5, qcenters[0][2]);
    }

    put_bits(data, &pos, 1, flip);
    put_bits(data, &pos, 1, diff);
    put_bits(data, &pos, 3, tables[1]);
    put_bits(data, &pos, 3, tables[0]);

    uint32 all_qbits_flipped = (qbits[1] << 2) | qbits[0];
    uint32 all_qbits = 0;

    if (flip != 0) all_qbits = all_qbits_flipped;

    if (flip == 0)
    for (uniform int k = 0; k < 2; k++)
    for (uniform int y = 0; y < 4; y++)
    for (uniform int x = 0; x < 4; x++)
    {
        int bit = (all_qbits_flipped >> (k * 16 + x * 4 + y)) & 1;
        all_qbits += bit << (k * 16 + y * 4 + x);
    }

    data[1] = bswap32(all_qbits);
}

inline void CompressBlockETC1_core(etc_enc_state state[])
{
    float flipped_block[48];

    for (uniform int y = 0; y < 4; y++)
    for (uniform int x = 0; x < 4; x++)
    for (uniform int p = 0; p < 3; p++)
    {
        flipped_block[16 * p + x * 4 + y] = state->block[16 * p + y * 4 + x];
    }

    for (uniform int flip = 0; flip < 2; flip++)
    for (uniform int diff = 1; diff >= 0; diff--)
    {
        state->diff = diff == 1;
        state->prev_qcenter[0] = -1;

        varying float * uniform pixels = state->block;
        if (flip == 0) pixels = flipped_block;

        uint32 qbits[2];
        int tables[2];
        int qcenters[2][3];

        float err = 0;
        err += compress_etc1_half(&qbits[0], &tables[0], qcenters[0], &pixels[0], state);
        err += compress_etc1_half(&qbits[1], &tables[1], qcenters[1], &pixels[8], state);

        if (err < state->best_err)
        {
            state->best_err = err;
            etc_pack(state->best_data, qbits, tables, qcenters, diff, flip);
        }
    }
}

void etc_enc_copy_settings(etc_enc_state state[], uniform etc_enc_settings settings[])
{
    state->fastSkipTreshold = settings->fastSkipTreshold;
}

inline void CompressBlockETC1(uniform rgba_surface src[], int xx, uniform int yy, uniform uint8 dst[], uniform etc_enc_settings settings[])
{
    etc_enc_state _state;
    varying etc_enc_state* uniform state = &_state;

    etc_enc_copy_settings(state, settings);
    load_block_interleaved(state->block, src, xx, yy);
    state->best_err = 1e99;

    CompressBlockETC1_core(state);

    store_data(dst, src->width, xx, yy, state->best_data, 2);
}

export void CompressBlocksETC1_ispc(uniform rgba_surface src[], uniform uint8 dst[], uniform etc_enc_settings settings[])
{
    for (uniform int yy = 0; yy<src->height / 4; yy++)
    foreach(xx = 0 ... src->width / 4)
    {
        CompressBlockETC1(src, xx, yy, dst, settings);
    }
}
