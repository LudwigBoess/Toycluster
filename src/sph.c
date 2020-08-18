#include "globals.h"
#include "sph.h"
#include "tree.h"

#define JUMPTOLERANCE (0.05)

static inline double sph_kernel_M4(const double r, const double h);
static inline double sph_kernel_derivative_M4(const double r, const double h);

static inline double sph_kernel_WC6(const double r, const double h);
static inline double sph_kernel_derivative_WC6(const double r, const double h);

extern void Find_sph_quantities() 
{
	Sort_Particles_By_Peano_Key();	
	
	Build_Tree();	

	#pragma omp parallel for shared(SphP, P) \
        schedule(dynamic, Param.Npart[0]/Omp.NThreads/64)
    for (size_t ipart = 0; ipart<Param.Npart[0]; ipart++) {  
        
		double hsml = SphP[ipart].Hsml;

        if (hsml == 0)
            hsml = 2*Guess_hsml(ipart, DESNNGB); // always too large

		Assert(isfinite(hsml), "hsml not finite ipart=%d parent=%d \n", 
				ipart, P[ipart].Tree_Parent);

        double dRhodHsml = 0;
        double rho = 0;

		int it = 0;
        
		for (;;) {

        	int ngblist[NGBMAX] = { 0 };

           	int ngbcnt = Find_ngb_tree(ipart, hsml, ngblist); 

			if (ngbcnt == NGBMAX) { // prevent overflow of ngblist
	
				hsml /= 1.24;

				continue;
			}

			if (ngbcnt < DESNNGB) {
			
				hsml *= 1.23;

				continue;
			}

           	bool part_done = Find_hsml(ipart, ngblist, ngbcnt, &dRhodHsml, 
                    &hsml, &rho); 

			if (ngbcnt < DESNNGB && (!part_done))
				hsml *= 1.24;

           	if (part_done)
                break;
        }

        double varHsmlFac = 1.0 / ( 1 + hsml/(3*rho)* dRhodHsml );

        SphP[ipart].Hsml = hsml;
        SphP[ipart].Rho = rho;
        SphP[ipart].VarHsmlFac = varHsmlFac;

    }

    return;
}

/* 
 * solve SPH continuity eq via Newton-Raphson, bisection and tree search 
 */
extern bool Find_hsml(const int ipart, const int *ngblist, const int ngbcnt,
        double *dRhodHsml_out, double *hsml_out, double *rho_out)
{
    const double boxhalf = 0.5 * Param.Boxsize;
    const double boxsize = Param.Boxsize;

    double upper = *hsml_out*sqrt3;
    double lower = 0;
    
    double hsml = *hsml_out; //lower + 0.5*(upper-lower); 
    double rho = 0, dRhodHsml = 0;

    int it = 0;

    bool part_done = 0;

    for (;;) {  
    
		const double pos_i[3] = { P[ipart].Pos[0], P[ipart].Pos[1],
			P[ipart].Pos[2] };

        double wkNgb = 0; // kernel weight number of neighbours

        rho = dRhodHsml = 0;
        
        it++;

        for (int i = 0; i < ngbcnt; i++) {

	    	int jpart = ngblist[i];	

            double dx = pos_i[0] - P[jpart].Pos[0];
	    	double dy = pos_i[1] - P[jpart].Pos[1];
    		double dz = pos_i[2] - P[jpart].Pos[2];
			
		    if (dx > boxhalf)	// find closest image 
	    		dx -= boxsize;

    		if (dx < -boxhalf)
		    	dx += boxsize;

	    	if (dy > boxhalf)
    			dy -= boxsize;

		    if (dy < -boxhalf)
	    		dy += boxsize;

    		if (dz > boxhalf)
			    dz -= boxsize;

		    if (dz < -boxhalf)
	    		dz += boxsize;

            double r2 = dx*dx + dy*dy + dz*dz;

    		if (r2 > p2(hsml)) 
                continue ;
                    
    		double r = sqrt(r2);

#ifdef SPH_CUBIC_SPLINE
			double wk = sph_kernel_M4(r, hsml);
			double dwk = sph_kernel_derivative_M4(r, hsml);
#else
		    double wk = sph_kernel_WC6(r, hsml);
		    double dwk = sph_kernel_derivative_WC6(r, hsml);
#endif // SPH_CUBIC_SPLINE


            wkNgb += fourpithird*wk*p3(hsml);

            rho += Param.Mpart[0]*wk;

            dRhodHsml += -Param.Mpart[0] * ( 3/hsml*wk + r/hsml * dwk );
        }

       if (it > 128) // not enough neighbours ? -> hard exit 
            break;
        
       double ngbDev = fabs(wkNgb - DESNNGB);
        
	   if (ngbDev < NNGBDEV) {

            part_done = true;

            break;
		}

        if (fabs(upper-lower) < 1e-4) { // find more neighbours ! 
            
            hsml *= 1.26; // double volume
            
            break;
        }

		if (ngbDev < 0.5 * DESNNGB) { // Newton Raphson

			double omega =  (1 + dRhodHsml * hsml / (3*rho));

	     	double fac = 1 - (wkNgb - DESNNGB) / (3*wkNgb * omega);

			fac = fmin(1.24, fac); // handle overshoot
			fac = fmax(1/1.24, fac);

			hsml *= fac;    
 
		} else {  // bisection

			if (wkNgb > DESNNGB) 
    	        upper = hsml;

	        if (wkNgb < DESNNGB) 
            	lower = hsml;
        
        	hsml = pow( 0.5 * ( p3(lower) + p3(upper) ), 1.0/3.0 );
		}
    } // for(;;)
    
    *hsml_out = (double) hsml;
    *rho_out = (double) rho;

#ifndef SPH_CUBIC_SPLINE
    if (part_done) {
    
        *dRhodHsml_out = (double) dRhodHsml;

        double bias_corr = -0.0116 * pow(DESNNGB*0.01, -2.236) 
            * Param.Mpart[0] * sph_kernel_WC6(0, hsml); // WC6 (Dehnen+ 12)
    
        *rho_out += bias_corr;   
    }
#endif

    return part_done;
}

extern void Bfld_from_rotA_SPH()
{
	printf("Constructing B from rot(A)");fflush(stdout);

	const double mpart = Param.Mpart[0];
	const double boxhalf = Param.Boxsize / 2;
    const double boxsize = Param.Boxsize;

	#pragma omp parallel for shared(SphP, P) \
    	schedule(dynamic, Param.Npart[0]/Omp.NThreads/64)
    for (int ipart = 0; ipart < Param.Npart[0]; ipart++) {
        
		int ngblist[NGBMAX] = { 0 };
	    int ngbcnt = Find_ngb_tree(ipart, SphP[ipart].Hsml, ngblist);

        double varHsmlFac = SphP[ipart].VarHsmlFac;
        double hsml = SphP[ipart].Hsml;
        double rho_i = SphP[ipart].Rho;

        double pos_i[3] = {P[ipart].Pos[0], P[ipart].Pos[1], P[ipart].Pos[2]};

		double apot_i[3] = {SphP[ipart].Apot[0], SphP[ipart].Apot[1], 
							SphP[ipart].Apot[2]};

		double bfld[3] = { 0 };

         for (int i = 0; i < ngbcnt; i++) {

			int jpart = ngblist[i];	

            if (jpart == ipart)
                continue;

			double dx = pos_i[0] - P[jpart].Pos[0];
			double dy = pos_i[1] - P[jpart].Pos[1];
			double dz = pos_i[2] - P[jpart].Pos[2];
			
			if (dx > boxhalf)	// find closest image 
				dx -= boxsize;

			if (dx < -boxhalf)
				dx += boxsize;

			if (dy > boxhalf)
				dy -= boxsize;

			if (dy < -boxhalf)
				dy += boxsize;

			if (dz > boxhalf)
				dz -= boxsize;

			if (dz < -boxhalf)
				dz += boxsize;

            double r2 = p2(dx) + p2(dy) + p2(dz);

			if (r2 > hsml*hsml) 
                continue ;
                
			double r = sqrt(r2);

			double dwk = sph_kernel_derivative_WC6(r, hsml);

			double weight = -mpart/rho_i * dwk / r  * varHsmlFac;

            /* Price JCOP 2010, eq 79 */
		    double dAx = apot_i[0] - SphP[jpart].Apot[0];
			double dAy = apot_i[1] - SphP[jpart].Apot[1];
			double dAz = apot_i[2] - SphP[jpart].Apot[2];

			bfld[0] += weight * (dz*dAy - dy*dAz); // B = rot(A)
			bfld[1] += weight * (dx*dAz - dz*dAx);
			bfld[2] += weight * (dy*dAx - dx*dAy);
		}

        SphP[ipart].Bfld[0] = (double) bfld[0];
		SphP[ipart].Bfld[1] = (double) bfld[1];
		SphP[ipart].Bfld[2] = (double) bfld[2];
	}

    printf(" done \n\n");fflush(stdout);

	return ;
}

extern void Smooth_SPH_quantities()
{
	const double mpart = Param.Mpart[0];
	const double boxhalf = Param.Boxsize / 2;
    const double boxsize = Param.Boxsize;

	printf("Smoothing Sph Quantities,"); fflush(stdout);

	size_t nBytes = Param.Npart[0] * sizeof(double);
	
	double *uArr = Malloc(nBytes);
	memset(uArr, 0, nBytes);

	double *velArr[3] = { NULL }; 
	velArr[0] = Malloc(nBytes);
	velArr[1] = Malloc(nBytes);
	velArr[2] = Malloc(nBytes);

	memset(velArr[0], 0, nBytes);
	memset(velArr[1], 0, nBytes);
	memset(velArr[2], 0, nBytes);

#pragma omp parallel for shared(SphP, P) \
    schedule(dynamic, Param.Npart[0]/Omp.NThreads/128)
    for (int ipart = 0; ipart < Param.Npart[0]; ipart++) {
        
		int ngblist[NGBMAX] = { 0 };
	    int ngbcnt = Find_ngb_tree(ipart, SphP[ipart].Hsml, ngblist);

        double varHsmlFac = SphP[ipart].VarHsmlFac;
        double hsml = SphP[ipart].Hsml;
        double rho_i = SphP[ipart].Rho;
        double *pos_i = P[ipart].Pos;
		
		double wk_u = 0, wk_vel[3] = { 0 },  wk_total = 0;
		
		for (int i = 0; i < ngbcnt; i++) {

			int jpart = ngblist[i];	

            if (jpart == ipart)
                continue;

			double dx = pos_i[0] - P[jpart].Pos[0];
			double dy = pos_i[1] - P[jpart].Pos[1];
			double dz = pos_i[2] - P[jpart].Pos[2];
			
			if (dx > boxhalf)	// find closest image 
				dx -= boxsize;

			if (dx < -boxhalf)
				dx += boxsize;

			if (dy > boxhalf)
				dy -= boxsize;

			if (dy < -boxhalf)
				dy += boxsize;

			if (dz > boxhalf)
				dz -= boxsize;

			if (dz < -boxhalf)
				dz += boxsize;

            double r2 = p2(dx) + p2(dy) + p2(dz);

			if (r2 > hsml*hsml) 
                continue ;
                
			double r = sqrtf(r2);

#ifdef SPH_CUBIC_SPLINE
			double dwk = sph_kernel_derivative_M4(r, hsml);
#else
			double dwk = sph_kernel_derivative_WC6(r, hsml);
#endif // SPH_CUBIC_SPLINE

			double weight = -mpart/rho_i * dwk / r  * varHsmlFac;

			wk_total += weight;

            wk_u += SphP[jpart].U * weight;
            
			wk_vel[0] += P[jpart].Vel[0] * weight;
            wk_vel[1] += P[jpart].Vel[1] * weight;
            wk_vel[2] += P[jpart].Vel[2] * weight;
		}

		wk_u /= wk_total;
		wk_vel[0] /= wk_total;
		wk_vel[1] /= wk_total;
		wk_vel[2] /= wk_total;
		
		double u_err = fabs(wk_u - SphP[ipart].U);

		if ( u_err > JUMPTOLERANCE ) // replace with kernel weighted U
			uArr[ipart] = wk_u;

		velArr[0][ipart] = wk_vel[0];
		velArr[1][ipart] = wk_vel[1];
		velArr[2][ipart] = wk_vel[2];
	}

#pragma omp parallel for
    for (int ipart = 0; ipart < Param.Npart[0]; ipart++) {
		
		if (uArr[ipart])
			SphP[ipart].U = uArr[ipart];
	
		P[ipart].Vel[0] = velArr[0][ipart];
		P[ipart].Vel[1] = velArr[1][ipart];
		P[ipart].Vel[2] = velArr[2][ipart];
	}

	Free(uArr);
	Free(velArr[0]); Free(velArr[1]); Free(velArr[2]);

    printf(" done \n\n"); fflush(stdout);

	return ;
}


static inline double sph_kernel_WC6(const double r, const double h)
{   
	const double u= r/h;
    const double t = 1-u;

    return 1365.0/(64*pi)/p3(h) *t*t*t*t*t*t*t*t*(1+8*u + 25*u*u + 32*u*u*u);
}

static inline double sph_kernel_derivative_WC6(const double r, const double h)
{   
	const double u = r/h;
    const double t = 1-u;

    return 1365.0/(64*pi)/(h*h*h*h) * -22.0 *t*t*t*t*t*t*t*u*(16*u*u+7*u+1);
}

static inline double sph_kernel_M4(const double r, const double h) // cubic spline
{
	double wk = 0;
   	double u = r/h;
 
	if(u < 0.5) 
		wk = (2.546479089470 + 15.278874536822 * (u - 1) * u * u);
	else
		wk = 5.092958178941 * (1.0 - u) * (1.0 - u) * (1.0 - u);
	
	return wk/p3(h);
}

static inline double sph_kernel_derivative_M4(const double r, const double h)
{
	double dwk = 0;
	double u = r/h;
  	
   	if(u < 0.5) 
		dwk = u * (45.836623610466 * u - 30.557749073644);
	else
		dwk = (-15.278874536822) * (1.0 - u) * (1.0 - u);
   
   return dwk/(h*h*h*h);
}
