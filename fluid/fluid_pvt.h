/*
 *  fluid_pvt.h
 *  FluidApp
 */

#ifndef FLUID_PVT_H
#define FLUID_PVT_H

#include "fluid.h"
#include "field.h"


////////////////////////////////////////////////////////////////////////////////
//
//			Data passed around to the functions (so that we can use them
//			for various purposes!!)
//
//When just advecting velocity.  (Use this to create a repos matrix)
struct advection_stam_velocity
{
	// NOTE: All assume same stride and size
	field *srcVelX;
	field *srcVelY;
	field *dstVelX;
	field *dstVelY;
	
	//Use this to flip the timestep (from forward to backward, & vice-versa...
	float timestep;
};

//When doing predictor-corrector (correcting velocity + build repos)
struct mccormack_vel_repos
{
	field *srcVelX;		//Source velocities
	field *srcVelY;
	field *srcErrVelX;	//When we return back to the source
	field *srcErrVelY;
	field *srcAdvX;		//Original advected results
	field *srcAdvY;
	field *dstReposX;	//Used to advect everything else once....
	field *dstReposY;
	field *dstVelX;		//Most likely an initial velocity...
	field *dstVelY;
};

//Simple reposition of data...
struct repos
{
	field *reposX;		//Fields describing where to fetch data
	field *reposY;
	field *src;			//Source field
	field *dst;			//Destination field
};

//Ensure that parameters are passed around somewhat cleanly
typedef union
{
	//When just advecting velocity.  (Use this to create a repos matrix)
	struct advection_stam_velocity advection_stam_velocity;
	
	//When doing predictor-corrector (correcting velocity + build repos)
	struct mccormack_vel_repos mccormack_vel_repos;
	
	//When doing simple repositioning
	struct repos repos;
} pvt_fluidMode;

////////////////////////////////////////////////////////////////////////////////
//
//		Read-only set of data describing function pointers to the various
//		components...		(fluid object for pointers, rowID for dataset)
//
typedef void(*pvtFluidFn)(fluid *in_f, int rowID, pvt_fluidMode *mode);
typedef struct
{
	pvtFluidFn fn;
	pvt_fluidMode mode;
} pvtFluidFnS __attribute__ ((aligned(16)));

#define MAX_FNS				32

////////////////////////////////////////////////////////////////////////////////
//
//		Structure defining how we use the fluid
//
struct fluid
{
	//Function pointer bases
	pvtFluidFnS m_fns[MAX_FNS];
	
	//Pointers to current buffers
	field *r_velocityX;
	field *r_velocityY;
	field *r_density;
	field *r_density_swap;
	field *r_pressure;
	
	field *r_tmpVelX;		//Temporary velocity X (advection work as Stam)
	field *r_tmpVelY;		//Temporary velocity Y (advection work as Stam)
	
	field *r_reposX;		//Used to accelerate advection (noticeable now
	field *r_reposY;		//							that data is in cache!)
	
	//Used for working...
	mpCoherence *r_coherence;
	
	//Used to stall the system until an iteration completes
	mpQueue		*r_blocker;
	
	//Number of used functions
	int m_usedFunctions;
	
	//Temporary buffers used as needed (we want to minimize memory footprint
	//to maximize cache usage)
	
	int m_curField;
};

#define TIMESTEP	1.0f


////////////////////////////////////////////////////////////////////////////////
//
//			Useful methods (implemented elsewhere)
//

void fluid_advection_stam_velocity(fluid *in_f, int rowID, pvt_fluidMode *mode);

void fluid_advection_mccormack_repos(fluid *in_f, int rowID, pvt_fluidMode *mode);

void fluid_repos(fluid *in_f, int y, pvt_fluidMode *mode);

#endif
