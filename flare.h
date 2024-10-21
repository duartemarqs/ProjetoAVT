#ifndef __FLARE_H
#define __FLARE_H

/* --- Defines --- */

#define FLARE_MAXELEMENTSPERFLARE         15
#define NTEXTURES 5

/* --- Types --- */

typedef struct FLARE_ELEMENT_DEF {
    
    float			fDistance;        // Distance along ray from source (0.0-1.0)
    float           fSize;            // Size relative to flare envelope (0.0-1.0)
	float			matDiffuse[4];  // color
	int				textureId;	
} FLARE_ELEMENT_DEF;

typedef struct FLARE_DEF {
    float           fScale;     // Scale factor for adjusting overall size of flare elements.
    float           fMaxSize;   // Max size of largest element, as proportion of screen width (0.0-1.0)
    int             nPieces;    // Number of elements in use
    FLARE_ELEMENT_DEF    element[FLARE_MAXELEMENTSPERFLARE];
} FLARE_DEF;

char * flareTextureNames[NTEXTURES] =  {"crcl", "flar", "hxgn", "ring", "sun"};

void    render_flare(FLARE_DEF *flare, int lx, int ly, int *m_viewport);
void    loadFlareFile(FLARE_DEF *flare, char *filename);

#endif
