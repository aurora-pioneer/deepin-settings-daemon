#ifndef _GAUSSIANIIR2D_H_
#define _GAUSSIANIIR2D_H_

#include <glib.h>

void gaussianiir2d_c(unsigned char* image_c, 
		     long width, long height, 
		     double sigma, long numsteps);

void gaussianiir2d_f(double* image_f, 
		     long width, long height, 
		     double sigma, long numsteps);

#endif
