#include <math.h>
#include "AVTmathLib.h"
#include <GL/freeglut.h>

extern float mCompMatrix[COUNT_COMPUTED_MATRICES][16];

/*-----------------------------------------------------------------
The objects motion is restricted to a rotation on a predefined axis
The function bellow does cylindrical billboarding on the Y axis, i.e.
the object will be able to rotate on the Y axis only.
-----------------------------------------------------------------*/
void l3dBillboardCylindricalBegin(float *cam, float *worldPos) {

	float lookAt[3]={0,0,1},objToCamProj[3],upAux[3],angleCosine;

// objToCamProj is the vector in world coordinates from the local origin to the camera
// projected in the XZ plane
	objToCamProj[0] = cam[0] - worldPos[0] ;
	objToCamProj[1] = 0;
	objToCamProj[2] = cam[2] - worldPos[2] ;


// normalize both vectors to get the cosine directly afterwards
	normalize(objToCamProj);

// easy fix to determine wether the angle is negative or positive
// for positive angles upAux will be a vector pointing in the 
// positive y direction, otherwise upAux will point downwards
// effectively reversing the rotation.

	crossProduct(lookAt,objToCamProj, upAux);

// compute the angle
	angleCosine = dotProduct(lookAt,objToCamProj);

// perform the rotation. The if statement is used for stability reasons
// if the lookAt and v vectors are too close together then |aux| could
// be bigger than 1 due to lack of precision
	if ((angleCosine < 0.99990) && (angleCosine > -0.9999))
		rotate(MODEL,acos(angleCosine)*180/3.14,upAux[0], upAux[1], upAux[2]);
}


/*----------------------------------------------------------------
True billboarding. With the spherical version the object will 
always face the camera. It requires more computational effort than
the cylindrical billboard though. The parameters camX,camY, and camZ,
are the target, i.e. a 3D point to which the object will point.
----------------------------------------------------------------*/

void l3dBillboardSphericalBegin(float *cam, float *worldPos) {

	float lookAt[3]={0,0,1},objToCamProj[3],objToCam[3],upAux[3],angleCosine;

// objToCamProj is the vector in world coordinates from the local origin to the camera
// projected in the XZ plane
	objToCamProj[0] = cam[0] - worldPos[0] ;
	objToCamProj[1] = 0;
	objToCamProj[2] = cam[2] - worldPos[2] ;

// normalize both vectors to get the cosine directly afterwards
	normalize(objToCamProj);

// easy fix to determine wether the angle is negative or positive
// for positive angles upAux will be a vector pointing in the 
// positive y direction, otherwise upAux will point downwards
// effectively reversing the rotation.

	crossProduct(lookAt, objToCamProj, upAux);

// compute the angle
	angleCosine = dotProduct(lookAt,objToCamProj);

// perform the rotation. The if statement is used for stability reasons
// if the lookAt and v vectors are too close together then |aux| could
// be bigger than 1 due to lack of precision
	if ((angleCosine < 0.99990) && (angleCosine > -0.9999))
		rotate(MODEL,acos(angleCosine)*180/3.14,upAux[0], upAux[1], upAux[2]);


// The second part tilts the object so that it faces the camera

// objToCam is the vector in world coordinates from the local origin to the camera
	objToCam[0] = cam[0] - worldPos[0] ;
	objToCam[1] = cam[1] - worldPos[1] ;
	objToCam[2] = cam[2] - worldPos[2] ;

// Normalize to get the cosine afterwards
	normalize(objToCam);

// Compute the angle between v and v2, i.e. compute the
// required angle for the lookup vector
	angleCosine = dotProduct(objToCamProj,objToCam);


// Tilt the object. The test is done to prevent instability when objToCam and objToCamProj have a very small
// angle between them
	if ((angleCosine < 0.99990) && (angleCosine > -0.9999))
		if (objToCam[1] < 0)
			rotate(MODEL,acos(angleCosine)*180/3.14,1,0,0);
		else
			rotate(MODEL,acos(angleCosine)*180/3.14,-1,0,0);

}

/* --------------------------------------------------------
Simplest form of Sperical Billboarding. It is not real billboarding 
since the object won't face the camera in the general case.
The object will face a plane perpendicular to the cameras
"look at" vector. It is the fastest of them all though.
---------------------------------------------------------*/

void BillboardCheatSphericalBegin() {
	
	
	int i,j;

	// undo all rotations
	// beware all scaling is lost as well 
	for( i=0; i<3; i++ ) 
		for( j=0; j<3; j++ ) {
			if ( i==j )
				mCompMatrix[VIEW_MODEL][i*4+j] = 1.0;
			else
				mCompMatrix[VIEW_MODEL][i*4+j] = 0.0;
		}
}


/* -------------------------------------------------------
The comments above apply in here as well but this is the
cylindrical version, i.e. the up vector is not changed
---------------------------------------------------------*/
void BillboardCheatCylindricalBegin() {

	int i,j;
 
	// Note that a row in the C convention is a column 
	// in OpenGL convention (see the red book, pg.106 in version 1.2)
	// right vector is [1,0,0]  (1st column)
	// lookAt vector is [0,0,1] (3d column)
	// leave the up vector unchanged (2nd column)
	// notice the increment in i in the first cycle (i+=2)
	for( i=0; i<3; i+=2 ) 
		for( j=0; j<3; j++ ) {
			if ( i==j )
				mCompMatrix[VIEW_MODEL][i*4+j] = 1.0;
			else
				mCompMatrix[VIEW_MODEL][i*4+j] = 0.0;
		}
}
