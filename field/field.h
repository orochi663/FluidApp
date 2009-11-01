//
//	Field exports functionality for large matrices.  These matrices are
//		normally quite large, and are limited to powers of two.
//

#ifndef FIELD_H
#define FIELD_H

#include "error.h"
#include "mpx.h"

//Dealing with a single field...
typedef struct field field;

//Create a new field with new data
field *fieldCreate(int in_width, int in_height, int in_components);

//Create a field with existing data
//	A copy of the data is not made; rather we just store the pointer and
//	pass it along.  For compatibility with other API's and frameworks,
//	it is assumed that the data is not reference counted.
field *fieldFromFloatData(float *in_data, int in_width, int in_height,
						  int in_strideX, int in_strideY, int in_components);

int fieldWidth(field *in_f);
int fieldHeight(field *in_f);
int fieldComponents(field *in_f);

//These two are in bytes.  (distance between each Y and each X)
int fieldStrideX(field *in_f);
int fieldStrideY(field *in_f);

//Be careful with thread safety!!!
float *fieldData(field *in_f);


//Utility macro to calculate offsets given strides
#define fieldComputOffsetFromStride(x,y,sx,sy)	(x*sx+y*sy)

//Get a pointer to data given a byte offset
#define fieldPointerFromOffset(base,offset)		\
					((float*)(((char*)base) + (offset)))

//Get a pointer given a base, stride, and value
#define fieldPointer(base, x, y, sx, sy)	\
			fieldPointerFromOffset(base,fieldComputeOffsetFromStride(x,y,sx,sy))

#endif
