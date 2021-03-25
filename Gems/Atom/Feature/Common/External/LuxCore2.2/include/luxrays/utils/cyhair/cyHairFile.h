// cyCodeBase by Cem Yuksel
// [www.cemyuksel.com]
//-------------------------------------------------------------------------------
///
/// \file       cyHairFile.h 
/// \author     Cem Yuksel
/// \version    1.0
/// \date       April 4, 2007
///
/// \brief class for HAIR file type
///
/// Extended with a couple of additional features for LuxRender project.
///
//-------------------------------------------------------------------------------

// NOTE: this file is included in LuxCore so any external dependency must be
// avoided here

#ifndef _CY_HAIR_FILE_H_INCLUDED_
#define _CY_HAIR_FILE_H_INCLUDED_

//-------------------------------------------------------------------------------

#include <stdio.h>
#include <math.h>
#include <string.h>

#include <luxrays/utils/exportdefs.h>

namespace luxrays {

//-------------------------------------------------------------------------------

#define CY_HAIR_FILE_SEGMENTS_BIT       1
#define CY_HAIR_FILE_POINTS_BIT         2
#define CY_HAIR_FILE_THICKNESS_BIT      4
#define CY_HAIR_FILE_TRANSPARENCY_BIT   8
#define CY_HAIR_FILE_COLORS_BIT         16
#define CY_HAIR_FILE_UVS_BIT            32

#define CY_HAIR_FILE_INFO_SIZE          88

// File read errors
#define CY_HAIR_FILE_ERROR_CANT_OPEN_FILE           -1
#define CY_HAIR_FILE_ERROR_CANT_READ_HEADER         -2
#define CY_HAIR_FILE_ERROR_WRONG_SIGNATURE          -3
#define CY_HAIR_FILE_ERROR_READING_SEGMENTS         -4
#define CY_HAIR_FILE_ERROR_READING_POINTS           -5
#define CY_HAIR_FILE_ERROR_READING_THICKNESS        -6
#define CY_HAIR_FILE_ERROR_READING_TRANSPARENCY     -7
#define CY_HAIR_FILE_ERROR_READING_COLORS           -8
#define CY_HAIR_FILE_ERROR_READING_UVS              -9

//-------------------------------------------------------------------------------

/// Hair file header
struct cyHairFileHeader
{
    char            signature[4];   ///< This should be "HAIR"
    unsigned int    hair_count;     ///< number of hair strands
    unsigned int    point_count;    ///< total number of points of all strands
    unsigned int    arrays;         ///< bit array of data in the file

    unsigned int    d_segments;     ///< default number of segments of each strand
    float           d_thickness;    ///< default thickness of hair strands
    float           d_transparency; ///< default transparency of hair strands
    float           d_color[3];     ///< default color of hair strands

    char            info[CY_HAIR_FILE_INFO_SIZE];   ///< information about the file
};

//-------------------------------------------------------------------------------

/// HAIR file class
CPP_EXPORT class CPP_API cyHairFile
{
public:
    cyHairFile() : segments(NULL), points(NULL), thickness(NULL), transparency(NULL), colors(NULL), uvs(NULL) { Initialize(); }
    ~cyHairFile() { Initialize(); }


    //////////////////////////////////////////////////////////////////////////
    ///@name Constant Data Access Methods
    
    const cyHairFileHeader& GetHeader() const { return header; }        ///< Use this method to access header data.
    const unsigned short* GetSegmentsArray() const { return segments; } ///< Returns segments array (segment count for each hair strand).
    const float* GetPointsArray() const { return points; }              ///< Returns points array (xyz coordinates of each hair point).
    const float* GetThicknessArray() const { return thickness; }        ///< Returns thickness array (thickness at each hair point}.
    const float* GetTransparencyArray() const { return transparency; }  ///< Returns transparency array (transparency at each hair point).
    const float* GetColorsArray() const { return colors; }              ///< Returns colors array (rgb color at each hair point).
    const float* GetUVsArray() const { return uvs; }                    ///< Returns uvs array (uv at each hair point).


    //////////////////////////////////////////////////////////////////////////
    ///@name Data Access Methods

    unsigned short* GetSegmentsArray() { return segments; } ///< Returns segments array (segment count for each hair strand).
    float* GetPointsArray() { return points; }              ///< Returns points array (xyz coordinates of each hair point).
    float* GetThicknessArray() { return thickness; }        ///< Returns thickness array (thickness at each hair point}.
    float* GetTransparencyArray() { return transparency; }  ///< Returns transparency array (transparency at each hair point).
    float* GetColorsArray() { return colors; }              ///< Returns colors array (rgb color at each hair point).
    float* GetUVsArray() { return uvs; }                    ///< Returns uvs array (uv at each hair point).


    //////////////////////////////////////////////////////////////////////////
    ///@name Methods for Setting Array Sizes
    
    /// Deletes all arrays and initializes the header data.
    void Initialize()
    {
        if ( segments ) delete [] segments;
        if ( points ) delete [] points;
        if ( colors ) delete [] colors;
        if ( thickness ) delete [] thickness;
        if ( transparency ) delete [] transparency;
        if ( uvs ) delete [] uvs;
        header.signature[0] = 'H';
        header.signature[1] = 'A';
        header.signature[2] = 'I';
        header.signature[3] = 'R';
        header.hair_count = 0;
        header.point_count = 0;
        header.arrays = 0;  // no arrays
        header.d_segments = 0;
        header.d_thickness = 1.0f;
        header.d_transparency = 0.0f;
        header.d_color[0] = 1.0f;
        header.d_color[1] = 1.0f;
        header.d_color[2] = 1.0f;
        memset( header.info, '\0', CY_HAIR_FILE_INFO_SIZE );
    }

    /// Sets the hair count, re-allocates segments array if necessary.
    void SetHairCount( int count )
    {
        header.hair_count = count;
        if ( segments ) {
            delete [] segments;
            segments = new unsigned short[ header.hair_count ];
        }
    }

    // Sets the point count, re-allocates points, thickness, transparency, and colors arrays if necessary.
    void SetPointCount( int count )
    {
        header.point_count = count;
        if ( points ) {
            delete [] points;
            points = new float[ header.point_count*3 ];
        }
        if ( thickness ) {
            delete [] thickness;
            thickness = new float[ header.point_count ];
        }
        if ( transparency ) {
            delete [] transparency;
            transparency = new float[ header.point_count ];
        }
        if ( colors ) {
            delete [] colors;
            colors = new float[ header.point_count*3 ];
        }
        if ( uvs ) {
            delete [] uvs;
            uvs = new float[ header.point_count*2 ];
        }
    }

    /// Use this function to allocate/delete arrays.
    /// Before you call this method set hair count and point count.
    /// Note that a valid HAIR file should always have points array.
    void SetArrays( int array_types )
    {
        header.arrays = array_types;
        if ( header.arrays & CY_HAIR_FILE_SEGMENTS_BIT && !segments ) segments = new unsigned short[header.hair_count];
        if ( ! (header.arrays & CY_HAIR_FILE_SEGMENTS_BIT) && segments ) { delete [] segments; segments=NULL; }
        if ( header.arrays & CY_HAIR_FILE_POINTS_BIT && !points ) points = new float[header.point_count*3];
        if ( ! (header.arrays & CY_HAIR_FILE_POINTS_BIT) && points ) { delete [] points; points=NULL; }
        if ( header.arrays & CY_HAIR_FILE_THICKNESS_BIT && !thickness ) thickness = new float[header.point_count];
        if ( ! (header.arrays & CY_HAIR_FILE_THICKNESS_BIT) && thickness ) { delete [] thickness; thickness=NULL; }
        if ( header.arrays & CY_HAIR_FILE_TRANSPARENCY_BIT && !transparency ) transparency = new float[header.point_count];
        if ( ! (header.arrays & CY_HAIR_FILE_TRANSPARENCY_BIT) && transparency ) { delete [] transparency; transparency=NULL; }
        if ( header.arrays & CY_HAIR_FILE_COLORS_BIT && !colors ) colors = new float[header.point_count*3];
        if ( ! (header.arrays & CY_HAIR_FILE_COLORS_BIT) && colors ) { delete [] colors; colors=NULL; }
        if ( header.arrays & CY_HAIR_FILE_UVS_BIT && !uvs ) uvs = new float[header.point_count*2];
        if ( ! (header.arrays & CY_HAIR_FILE_UVS_BIT) && uvs ) { delete [] uvs; uvs=NULL; }
    }

    /// Sets default number of segments for all hair strands, which is used if segments array does not exist.
    void SetDefaultSegmentCount( int s ) { header.d_segments = s; }

    /// Sets default hair strand thickness, which is used if thickness array does not exist.
    void SetDefaultThickness( float t ) { header.d_thickness = t; }

    /// Sets default hair strand transparency, which is used if transparency array does not exist.
    void SetDefaultTransparency( float t ) { header.d_transparency = t; }

    /// Sets default hair color, which is used if color array does not exist.
    void SetDefaultColor( float r, float g, float b ) { header.d_color[0]=r; header.d_color[1]=g; header.d_color[2]=b; }

    //////////////////////////////////////////////////////////////////////////
    ///@name Load and Save Methods

    /// Loads hair data from the given HAIR file.
    int LoadFromFile( const char *filename )
    {
        Initialize();

        FILE *fp;
        fopen_s(&fp, filename, "rb" );
        if ( fp == NULL ) return CY_HAIR_FILE_ERROR_CANT_OPEN_FILE;

        // read the header
        size_t headread = fread( &header, sizeof(cyHairFileHeader), 1, fp );

        #define _CY_FAILED_RETURN(errorno) { Initialize(); fclose( fp ); return errorno; }


        // Check if header is correctly read
        if ( headread < 1 ) _CY_FAILED_RETURN(CY_HAIR_FILE_ERROR_CANT_READ_HEADER);

        // Check if this is a hair file
        if ( strncmp( header.signature, "HAIR", 4) != 0 ) _CY_FAILED_RETURN(CY_HAIR_FILE_ERROR_WRONG_SIGNATURE);

        // Read segments array
        if ( header.arrays & CY_HAIR_FILE_SEGMENTS_BIT ) {
            segments = new unsigned short[ header.hair_count ];
            size_t readcount = fread( segments, sizeof(unsigned short), header.hair_count, fp );
            if ( readcount < header.hair_count ) _CY_FAILED_RETURN(CY_HAIR_FILE_ERROR_READING_SEGMENTS);
        }

        // Read points array
        if ( header.arrays & CY_HAIR_FILE_POINTS_BIT ) {
            points = new float[ header.point_count*3 ];
            size_t readcount = fread( points, sizeof(float), header.point_count*3, fp );
            if ( readcount < header.point_count*3 ) _CY_FAILED_RETURN(CY_HAIR_FILE_ERROR_READING_POINTS);
        }

        // Read thickness array
        if ( header.arrays & CY_HAIR_FILE_THICKNESS_BIT ) {
            thickness = new float[ header.point_count ];
            size_t readcount = fread( thickness, sizeof(float), header.point_count, fp );
            if ( readcount < header.point_count ) _CY_FAILED_RETURN(CY_HAIR_FILE_ERROR_READING_THICKNESS);
        }

        // Read transparency array
        if ( header.arrays & CY_HAIR_FILE_TRANSPARENCY_BIT ) {
            transparency = new float[ header.point_count ];
            size_t readcount = fread( transparency, sizeof(float), header.point_count, fp );
            if ( readcount < header.point_count ) _CY_FAILED_RETURN(CY_HAIR_FILE_ERROR_READING_TRANSPARENCY);
        }

        // Read colors array
        if ( header.arrays & CY_HAIR_FILE_COLORS_BIT ) {
            colors = new float[ header.point_count*3 ];
            size_t readcount = fread( colors, sizeof(float), header.point_count*3, fp );
            if ( readcount < header.point_count*3 ) _CY_FAILED_RETURN(CY_HAIR_FILE_ERROR_READING_COLORS);
        }

        // Read UVs array
        if ( header.arrays & CY_HAIR_FILE_UVS_BIT ) {
            uvs = new float[ header.point_count*2 ];
            size_t readcount = fread( uvs, sizeof(float), header.point_count*2, fp );
            if ( readcount < header.point_count*2 ) _CY_FAILED_RETURN(CY_HAIR_FILE_ERROR_READING_UVS);
        }

        fclose( fp );

        return header.hair_count;
    }

    /// Saves hair data to the given HAIR file.
    int SaveToFile( const char *filename ) const
    {
        FILE *fp;
        fopen_s( &fp, filename, "wb" );
        if ( fp == NULL ) return -1;

        // Write header
        fwrite( &header, sizeof(cyHairFileHeader), 1, fp );

        // Write arrays
        if ( header.arrays & CY_HAIR_FILE_SEGMENTS_BIT ) fwrite( segments, sizeof(unsigned short), header.hair_count, fp );
        if ( header.arrays & CY_HAIR_FILE_POINTS_BIT ) fwrite( points, sizeof(float), header.point_count*3, fp );
        if ( header.arrays & CY_HAIR_FILE_THICKNESS_BIT ) fwrite( thickness, sizeof(float), header.point_count, fp );
        if ( header.arrays & CY_HAIR_FILE_TRANSPARENCY_BIT ) fwrite( transparency, sizeof(float), header.point_count, fp );
        if ( header.arrays & CY_HAIR_FILE_COLORS_BIT ) fwrite( colors, sizeof(float), header.point_count*3, fp );
        if ( header.arrays & CY_HAIR_FILE_UVS_BIT ) fwrite( uvs, sizeof(float), header.point_count*2, fp );

        fclose( fp );

        return header.hair_count;
    }


    //////////////////////////////////////////////////////////////////////////
    ///@name Other Methods

    /// Fills the given direction array with normalized directions using the points array.
    /// Call this function if you need strand directions for shading.
    /// The given array dir should be allocated as an array of size 3 times point count.
    /// Returns point count, returns zero if fails.
    int FillDirectionArray( float *dir )
    {
        if ( dir==NULL || header.point_count<=0 || points==NULL ) return 0;

        int p = 0;  // point index
        for ( unsigned int i=0; i<header.hair_count; i++ ) {
            int s = (segments) ? segments[i] : header.d_segments;
            if ( s > 1 ) {
                // direction at point1
                float len0, len1;
                ComputeDirection( &dir[(p+1)*3], len0, len1, &points[p*3], &points[(p+1)*3], &points[(p+2)*3] );

                // direction at point0
                float d0[3];
                d0[0] = points[(p+1)*3]   - dir[(p+1)*3]  *len0*0.3333f - points[p*3];
                d0[1] = points[(p+1)*3+1] - dir[(p+1)*3+1]*len0*0.3333f - points[p*3+1];
                d0[2] = points[(p+1)*3+2] - dir[(p+1)*3+2]*len0*0.3333f - points[p*3+2];
                float d0lensq = d0[0]*d0[0] + d0[1]*d0[1] + d0[2]*d0[2];
                float d0len = ( d0lensq > 0 ) ? (float) sqrt(d0lensq) : 1.0f;
                dir[p*3]   = d0[0] / d0len;
                dir[p*3+1] = d0[1] / d0len;
                dir[p*3+2] = d0[2] / d0len;

                // We computed the first 2 points
                p += 2;

                // Compute the direction for the rest
                for ( int t=2; t<s; t++, p++ ) {
                    ComputeDirection( &dir[p*3], len0, len1, &points[(p-1)*3], &points[p*3], &points[(p+1)*3] );
                }

                // direction at the last point
                d0[0] = - points[(p-1)*3]   + dir[(p-1)*3]  *len1*0.3333f + points[p*3];
                d0[1] = - points[(p-1)*3+1] + dir[(p-1)*3+1]*len1*0.3333f + points[p*3+1];
                d0[2] = - points[(p-1)*3+2] + dir[(p-1)*3+2]*len1*0.3333f + points[p*3+2];
                d0lensq = d0[0]*d0[0] + d0[1]*d0[1] + d0[2]*d0[2];
                d0len = ( d0lensq > 0 ) ? (float) sqrt(d0lensq) : 1.0f;
                dir[p*3]   = d0[0] / d0len;
                dir[p*3+1] = d0[1] / d0len;
                dir[p*3+2] = d0[2] / d0len;
                p++;

            } else if ( s > 0 ) {
                // if it has a single segment
                float d0[3];
                d0[0] = points[(p+1)*3]   - points[p*3];
                d0[1] = points[(p+1)*3+1] - points[p*3+1];
                d0[2] = points[(p+1)*3+2] - points[p*3+2];
                float d0lensq = d0[0]*d0[0] + d0[1]*d0[1] + d0[2]*d0[2];
                float d0len = ( d0lensq > 0 ) ? (float) sqrt(d0lensq) : 1.0f;
                dir[p*3]   = d0[0] / d0len;
                dir[p*3+1] = d0[1] / d0len;
                dir[p*3+2] = d0[2] / d0len;
                dir[(p+1)*3]   = dir[p*3];
                dir[(p+1)*3+1] = dir[p*3+1];
                dir[(p+1)*3+2] = dir[p*3+2];
                p += 2;
            }
            //*/
        }
        return p;
    }


private:
    //////////////////////////////////////////////////////////////////////////
    ///@name Private Variables and Methods

    cyHairFileHeader header;
    unsigned short  *segments;
    float           *points;
    float           *thickness;
    float           *transparency;
    float           *colors;
    float           *uvs;

    // Given point before (p0) and after (p2), computes the direction (d) at p1.
    float ComputeDirection( float *d, float &d0len, float &d1len, const float *p0, const float *p1, const float *p2 )
    {
        // line from p0 to p1
        float d0[3];
        d0[0] = p1[0] - p0[0];
        d0[1] = p1[1] - p0[1];
        d0[2] = p1[2] - p0[2];
        float d0lensq = d0[0]*d0[0] + d0[1]*d0[1] + d0[2]*d0[2];
        d0len = ( d0lensq > 0 ) ? (float) sqrt(d0lensq) : 1.0f;

        // line from p1 to p2
        float d1[3];
        d1[0] = p2[0] - p1[0];
        d1[1] = p2[1] - p1[1];
        d1[2] = p2[2] - p1[2];
        float d1lensq = d1[0]*d1[0] + d1[1]*d1[1] + d1[2]*d1[2];
        d1len = ( d1lensq > 0 ) ? (float) sqrt(d1lensq) : 1.0f;

        // make sure that d0 and d1 has the same length
        d0[0] *= d1len / d0len;
        d0[1] *= d1len / d0len;
        d0[2] *= d1len / d0len;

        // direction at p1
        d[0] = d0[0] + d1[0];
        d[1] = d0[1] + d1[1];
        d[2] = d0[2] + d1[2];
        float dlensq = d[0]*d[0] + d[1]*d[1] + d[2]*d[2];
        float dlen = ( dlensq > 0 ) ? (float) sqrt(dlensq) : 1.0f;
        d[0] /= dlen;
        d[1] /= dlen;
        d[2] /= dlen;

        return d0len;
    }
};

//-------------------------------------------------------------------------------

namespace cy {
    typedef cyHairFileHeader HairFileHeader;
    typedef cyHairFile HairFile;
}

//-------------------------------------------------------------------------------

}

#endif
