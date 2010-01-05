/*
 *  fluid_pvt.c
 *  FluidApp
 */

#include "fluid_pvt.h"
#include "fluid_macros_2.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "memory.h"



////////////////////////////////////////////////////////////////////////////////
//
//	Private data structure and methods to operate on it
//
void fluidFree(void *in_o)
{
	fluid *o = (fluid*)in_o;
	
	if (o->r_blocker)		x_free(o->r_blocker);
	
	if (o->r_coherence)		x_free(o->r_coherence);
	
	if (o->r_pressure)		x_free(o->r_pressure);
	
	if (o->r_density)		x_free(o->r_density);
	if (o->r_density_swap)	x_free(o->r_density_swap);
	
	if (o->r_velocityX)		x_free(o->r_velocityX);
	if (o->r_velocityY)		x_free(o->r_velocityY);
	
	if (o->r_tmpVelX)		x_free(o->r_tmpVelX);
	if (o->r_tmpVelY)		x_free(o->r_tmpVelY);
	
	if (o->r_reposX)		x_free(o->r_reposX);
	if (o->r_reposY)		x_free(o->r_reposY);
	
	if (o->r_vidOutput)		x_free(o->r_vidOutput);
	
	if (o->gpu_velX_in)		x_free(o->gpu_velX_in);
	if (o->gpu_velY_in)		x_free(o->gpu_velY_in);
	
	if (o->gpu_velocityX)	x_free(o->gpu_velocityX);
	if (o->gpu_velocityY)	x_free(o->gpu_velocityY);
	
	if (o->gpu_dens_in)		x_free(o->gpu_dens_in);
	if (o->gpu_density)		x_free(o->gpu_density);
	if (o->gpu_dens_tmp)	x_free(o->gpu_dens_tmp);
	
	if (o->gpu_fn_viscosity)	x_free(o->gpu_fn_viscosity);
	if (o->gpu_fn_pressure)		x_free(o->gpu_fn_pressure);
	
	int i;
	for (i=0; i<GPU_BUFFERS; i++)
	{
		if (o->gpu_tmp1[i])		x_free(o->gpu_tmp1[i]);
	}
}


fluid *fluidCreate(int in_width, int in_height)
{
	fluid *toRet = x_malloc(sizeof(fluid), fluidFree);
	memset(toRet, 0, sizeof(fluid));
	
	//CPU Buffers
	toRet->r_velocityX = fieldCreate(in_width, in_height, 1);
	toRet->r_velocityY = fieldCreate(in_width, in_height, 1);
	toRet->r_tmpVelX = fieldCreate(in_width, in_height, 1);
	toRet->r_tmpVelY = fieldCreate(in_width, in_height, 1);
	toRet->r_reposX = fieldCreate(in_width, in_height, 1);
	toRet->r_reposY = fieldCreate(in_width, in_height, 1);
	toRet->r_pressure = fieldCreate(in_width, in_height, 1);
	toRet->r_density = fieldCreate(in_width, in_height, 4);
	toRet->r_density_swap = fieldCreate(in_width, in_height, 4);
	toRet->r_vidOutput = fieldCreateChar(in_width, in_height, 4);
	
	//GPU Buffers
	toRet->gpu_velX_in = GPUFieldCreate(in_width, in_height, 1);
	toRet->gpu_velY_in = GPUFieldCreate(in_width, in_height, 1);
	toRet->gpu_velocityX = GPUFieldCreate(in_width, in_height, 1);
	toRet->gpu_velocityY = GPUFieldCreate(in_width, in_height, 1);
	toRet->gpu_dens_in = GPUFieldCreate(in_width, in_height, 4);
	toRet->gpu_density = GPUFieldCreate(in_width, in_height, 4);
	toRet->gpu_dens_tmp = GPUFieldCreate(in_width, in_height, 4);
	int k;
	toRet->gpu_curBuffer = 0;
	for (k=0; k<GPU_BUFFERS; k++)
	{
		toRet->gpu_tmp1[k] = GPUFieldCreate(in_width, in_height, 1);
	}
	
	//GPU Programs...
	toRet->gpu_fn_viscosity = GPUProgramCreate("viscosity.cl", GPUPROGRAM_FROM_FILE);
	toRet->gpu_fn_pressure = GPUProgramCreate("pressure.cl", GPUPROGRAM_FROM_FILE);
	
	toRet->r_blocker = mpQueueCreate(2);
	
	//NOTE: Make this configurable????
	toRet->r_coherence = mpCCreate(in_height, 128, 512);
	
	toRet->m_curField = 0;
	
	toRet->m_usedFunctions = 0;
	
	toRet->m_viscosity = 1.0f;
	toRet->m_vorticity = 1.0f;
	toRet->m_timestep = 1.0f;
	
	toRet->m_fadeVel = 1.0f;
	toRet->m_fadeDens = 1.0f;
	
	toRet->flags = 0;
	
	int i;
	for (i=0; i<TIME_TOTAL; i++)
		toRet->m_times[i] = 0;
	
	return toRet;
}


void fluidSetViscosity(fluid *f, float in_v)
{
	f->m_viscosity = in_v;
}


void fluidSetVorticity(fluid *f, float in_v)
{
	f->m_vorticity = in_v;
}

void fluidSetTimestep(fluid *f, float in_v)
{
	f->m_timestep = in_v;
}

void fluidSetDensityFade(fluid *f, float in_v)
{
	f->m_fadeDens = in_v;
}

void fluidSetVelocityFade(fluid *f, float in_v)
{
	f->m_fadeVel = in_v;
}

void fluidFreeSurfaceNone(fluid *f)
{
	f->flags = (f->flags & (~FLUID_SIMPLEFREE));
}

void fluidFreeSurfaceSimple(fluid *f)
{
	f->flags = (f->flags | FLUID_SIMPLEFREE);
}


void fluidVorticityQuick(fluid *f)
{
	f->flags = (f->flags | FLUID_QUICK_VORTICTY);
}

void fluidVorticityAccurate(fluid *f)
{
	f->flags = (f->flags & (~FLUID_QUICK_VORTICTY));
}

void fluidEnableTimers(fluid *f)
{
	f->flags = (f->flags | FLUID_TIMERS);
}

void fluidDisableTimers(fluid *f)
{
	f->flags = (f->flags & (~FLUID_TIMERS));
}


field *fluidDensity(fluid *in_f)
{
	return in_f->r_density;
}

field *fluidMovedDensity(fluid *in_f)
{
	return in_f->r_density_swap;
}

field *fluidVelocityX(fluid *in_f)
{
	return in_f->r_velocityX;
}

field *fluidVelocityY(fluid *in_f)
{
	return in_f->r_velocityY;
}

float fluidAdvectionTime(fluid *f)
{
	return (float)f->m_times[TIME_ADVECTION]/1000000;
}

float fluidPressureTime(fluid *f)
{
	return (float)f->m_times[TIME_PRESSURE]/1000000;
}

float fluidViscosityTime(fluid *f)
{
	return (float)f->m_times[TIME_VISCOSITY]/1000000;
}

float fluidVorticityTime(fluid *f)
{
	return (float)f->m_times[TIME_VORTICITY]/1000000;
}

float fluidThreadSchedulerTime(fluid *f)
{
	return (float)f->m_times[TIME_TASKSCHED]/1000000;
}

field *fluidVideoOut(fluid *in_f)
{
	return in_f->r_vidOutput;
}
