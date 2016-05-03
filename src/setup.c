#include "globals.h"

/* Set all relevant parameters for the collision */
void Setup()
{
    double mtot[2] = {0}, mDM, mGas;
    double d_clusters;
    char string[255];

    const double G = Grav/pow(Unit.Length,3)*Unit.Mass*pow(Unit.Time,2);
    const double bf = Cosmo.Baryon_Fraction;
    const double Xm = Param.Mass_Ratio;
	
	const double z = Param.Redshift;
	const double rho_crit =  Critical_Density(z);
	const double delta = Overdensity_Parameter();
    
    /* Halo Masses inside r200 */
    Halo[0].Mtotal200 = Param.Mtot200 / (1+Xm) ;
    Halo[1].Mtotal200 = Param.Mtot200 - Halo[0].Mtotal200;

	if (Xm == 0)
	    Param.Nhalos = 1;
	else
		Param.Nhalos = 2;

    for (int i = 0; i < Param.Nhalos; i++) { 

        Halo[i].Mass200[1] = Halo[i].Mtotal200 / (1+bf);
        Halo[i].Mass200[0] = Halo[i].Mtotal200 - Halo[i].Mass200[1];

		double c_nfw = Halo[i].C_nfw = Concentration_parameter(i);

        /* R200, Kitayama & Suto 99, Boehringer+ 2012 (1) */
        Halo[i].R200 = pow(Halo[i].Mtotal200*Unit.Mass
                / (delta*rho_crit*fourpithird),1./3.)/Unit.Length;
        
        Halo[i].Rs = Halo[i].R200 / Halo[i].C_nfw; // NFW scale radius

        /* (Springel & Farrar 07) */
        Halo[i].A_hernq = Halo[i].Rs*sqrt(2*(log(1+c_nfw)-c_nfw/(1+c_nfw)));
    }

    Param.Boxsize = floor(2*R200_TO_RMAX_RATIO * Halo[0].R200);

    for (int i = 0; i < Param.Nhalos; i++) { // Baryons and total mass 
	
    	Halo[i].R_Sample[0] = Halo[i].R200*1.5;
		Halo[i].R_Sample[1] = Halo[i].R200*1.5;
		Halo[i].Rcut = Halo[i].R200; 

		if (i == 0) { // 0 provides a background

			Halo[i].R_Sample[1] = Param.Boxsize/2;
			Halo[i].R_Sample[0] = sqrt(3)*Param.Boxsize/2;
		}
		double rs_gas = Halo[i].R_Sample[0];
		double rs_dm = Halo[i].R_Sample[1];
        double a = Halo[i].A_hernq;
		double m200_dm = Halo[i].Mass200[1];
		double m200_gas = Halo[i].Mass200[0];
        double r200 = Halo[i].R200;
        double rcut = Halo[i].Rcut;
	
		Halo[i].Rcore = Gas_core_radius(i, string);
		
		double rc = Halo[i].Rcore;

       	Halo[i].Rho0 = m200_gas / Mass_profile(r200, 1, rc, rcut);
		
		double rho0 = Halo[i].Rho0;

		Halo[i].MassCorrFac = 1/(1 + 2*a/rs_dm + p2(a/rs_dm));

	    Halo[i].Mass[0] = Mass_profile(rs_gas, rho0, rc, rcut);

        Halo[i].Mass[1] = m200_dm * (1 + 2*a/r200 + p2(a/r200)) 
			* Halo[i].MassCorrFac; // correct for finite R_sample != infty

        Halo[i].Mtotal = Halo[i].Mass[0] + Halo[i].Mass[1];

        if (!bf) {   // DM only
            Halo[i].Mass[1] += Halo[i].Mass[0];
            Halo[i].Mass[0] = 0;
        }
        
		printf("Halo Setup : <%d>\n"
               "   Model             = %s\n"
               "   Sample Radius Gas = %g kpc\n"
               "   Sample Radius DM  = %g kpc\n"
			   "   qmax              = %g \n"
               "   Mass              = %g 10^10 MSol\n"
               "   Mass in DM        = %g 10^10 MSol\n"
               "   Mass in Gas       = %g 10^10 MSol\n"
               "   Mass in R200      = %g 10^10 MSol\n"
               "   c_nfw             = %g \n"
               "   R200              = %g kpc\n"
               "   a_hernquist       = %g kpc\n"
               "   rho0_gas          = %g g/cm^3\n"
               "   rho0_gas          = %g [gadget]\n"
               "   beta              = 2/3 \n"
               "   rc                = %g kpc\n"
               ,i,string,Halo[i].R_Sample[0], Halo[i].R_Sample[1]
			   , Halo[i].MassCorrFac, Halo[i].Mtotal, Halo[i].Mass[1]
               ,Halo[i].Mass[0] ,Halo[i].Mtotal200
               , Halo[i].C_nfw, Halo[i].R200*Unit.Length/kpc2cgs
               , Halo[i].A_hernq*Unit.Length/kpc2cgs
               , Density(Halo[i].Rho0), Halo[i].Rho0,Halo[i].Rcore);

        Param.Mtotal += Halo[i].Mtotal;
        mtot[0] += Halo[i].Mass[0];
        mtot[1] += Halo[i].Mass[1];

		/* Calc & show effective Baryon Fraction in R500 */

		if (!bf) // DM only
			continue;

		if (!Halo[i].Mtotal200)
			continue;

		/* R500 */
		double rho_crit =  Critical_Density(z);
		Halo[i].R500 = pow(Halo[i].Mtotal200*Unit.Mass
                / (500*rho_crit*fourpithird),1./3.)/Unit.Length;

        double r500 = Halo[i].R500 * Unit.Length;
		double Mdm = Halo[i].Mass[1] * Unit.Mass;

		rho0 = Density(Halo[i].Rho0);
		a = Halo[i].A_hernq * Unit.Length;
		rc = Halo[i].Rcore * Unit.Length;

		Halo[i].Bf_eff = 4*pi*p3(rc)*rho0 * (r500/rc - atan(r500/rc))
			/ (Mdm * p2(r500) / p2(a+r500) );
		
		printf("   R500	           = %g kpc\n"
			   "   bf_200          = %g \n"
			   "   bf_500          = %g \n",
				r500/Unit.Length, bf, Halo[i].Bf_eff);

		printf("\n");
	}
    
    /* Particle numbers are calculated from the global
     * masses, not from masses in r200 */
    int nDM  = 0.5 * Param.Ntotal;
    int nGas = 0.5 * Param.Ntotal;

    Param.Mpart[0] = mGas = mtot[0]/nGas;
    Param.Mpart[1] = mDM = mtot[1]/nDM;

    for (int i = 0; i<Param.Nhalos; i++) {

        Halo[i].Npart[0] = round(Halo[i].Mass[0]/mGas);
        Halo[i].Npart[1] = round(Halo[i].Mass[1]/mDM);
       
        Halo[i].Ntotal = Halo[i].Npart[0] + Halo[i].Npart[1];
    }

    if (!bf) { // DM only, redo masses and numbers 
			
        Param.Mpart[0] = mGas = nGas = 0; 
        Param.Mpart[1] = mDM = Param.Mtotal/Param.Ntotal;

        for (int i = 0; i < Param.Nhalos; i++) {

            Halo[i].Npart[1] = round(Halo[i].Mtotal / mDM);
            Halo[i].Npart[0] = 0;

            Halo[i].Ntotal = Halo[i].Npart[1];
        }
    }
    
	for (int i = 0; i < 6; i++)
        Param.Npart[i] = Halo[0].Npart[i]+Halo[1].Npart[i];

    printf("\nSystem Setup : \n"
			"   Mass Ratio      = %g\n"
            "   Boxsize         = %g kpc\n"
            "   Total Mass      = %g 10^10 Msol\n"
            "   Mass in Gas     = %g 10^10 Msol\n"
            "   Mass in DM      = %g 10^10 Msol\n"
            "   Mass Ratio      = %g\n"
            "   given bf        = %g\n"
            "   boxwide bf      = %g\n"
            "   # of Particles  = %lld\n"
            "   Sph Part Mass   = %g 10^10 Msol\n"
            "   DM Part Mass    = %g 10^10 Msol\n"
			"   Npart Parent    = %8lld, %8lld \n"
			"   Npart Bullet    = %8lld, %8lld \n"
			"   Npart Total     = %8lld, %8lld\n\n"
            , Param.Mass_Ratio, Param.Boxsize 
            ,Param.Mtotal,mtot[0],mtot[1],Xm
            ,bf, mtot[0]/mtot[1]
            ,Param.Ntotal,Param.Mpart[0],Param.Mpart[1],
			Halo[0].Npart[0], Halo[0].Npart[1],
			Halo[1].Npart[0], Halo[1].Npart[1],
			Param.Npart[0], Param.Npart[1]);

    /* allocate particles */
    size_t nBytes = Param.Ntotal * sizeof(*P);
    P = Malloc(nBytes);
    memset(P, 0, nBytes);

    nBytes = Param.Npart[0] * sizeof(*SphP);
    SphP = Malloc(nBytes);
    memset(SphP, 0, nBytes);

    /* set access pointers */
    Halo[0].Gas = &(P[0]);
    Halo[0].DM = &(P[nGas]);
    Halo[0].SphP = &(SphP[0]);

    if (Xm) {

        Halo[1].Gas = &(P[Halo[0].Npart[0]]);

        Halo[1].DM = &(P[nGas + Halo[0].Npart[1]]);
        
		Halo[1].SphP = &(SphP[Halo[0].Npart[0]]);
    }

    /* grav softening  from larger cluster */
    Param.GravSofteningLength = pow( 
			p3(Halo[0].R_Sample[1])/Param.Ntotal, 1/3.) / 7;

    printf("\nGrav. Softening ~ %g kpc\n\n",
			 Param.GravSofteningLength*Unit.Length/kpc2cgs);

    /* kinematics */
    if (Xm) { // two clusters only

        d_clusters = 0.9 * (Halo[0].R200 + Halo[1].R200);
    
        Halo[0].D_CoM[0] = -1 * Halo[1].Mtotal200
            *d_clusters/Param.Mtot200;
        Halo[1].D_CoM[0] = d_clusters + Halo[0].D_CoM[0];
        
  /*      if (Xm > 1) {

            Halo[1].D_CoM[0] = -1 * Halo[1].Mtotal200
            *d_clusters/Param.Mtot200;
            
			Halo[0].D_CoM[0] = d_clusters + Halo[0].D_CoM[0];
        }*/

        Halo[0].D_CoM[1] = -1 * Halo[1].Mtotal200 
            * Param.Impact_Param/Param.Mtot200;
		
        Halo[1].D_CoM[1] = Param.Impact_Param + Halo[0].D_CoM[1];

#ifndef GIVEPARAMS
        Param.VelMerger[0] = sqrt(2*G*Halo[1].Mtotal200
                /(d_clusters*(1+1/Xm)));
        Param.VelMerger[1] = -Param.Mtot200/Halo[1].Mtotal200
            * Param.VelMerger[0];
		
		Param.VelMerger[0] *= ZERO_ENERGY_ORBIT_FRACTION;
		Param.VelMerger[1] *= ZERO_ENERGY_ORBIT_FRACTION;
#endif

        Halo[0].BulkVel[0] = Halo[0].BulkVel[1] = Halo[0].BulkVel[2] = 0;
        Halo[1].BulkVel[0] = Halo[1].BulkVel[1] = Halo[1].BulkVel[2] = 0;

#if !defined(PARABOLA) && !defined(COMET) // apply merger kinematics directly
        Halo[0].BulkVel[0] = Param.VelMerger[0];
        Halo[1].BulkVel[0] = Param.VelMerger[1];
#endif

        printf("Kinematics of Collision : \n"
			"   Zero-E fraction     = %g \n"
            "   Initial Distance    = %g kpc\n"
            "   CoM Distance of <0> = %g kpc\n"
            "   CoM Distance of <1> = %g kpc\n"
            "   CoM Velocity of <0> = %g km/s\n"
            "   CoM Velocity of <1> = %g km/s\n\n"
            "   Impact Parameter    = %g kpc\n"
            "   CoM Impact of <0>   = %g kpc\n"
            "   CoM Impact of <1>   = %g kpc\n\n"
			,ZERO_ENERGY_ORBIT_FRACTION
            ,d_clusters, Halo[0].D_CoM[0], Halo[1].D_CoM[0]
            , Param.VelMerger[0], Param.VelMerger[1]
            ,Param.Impact_Param, Halo[0].D_CoM[1]
            ,Halo[1].D_CoM[1]);

    } else { // just one cluster 

        d_clusters = 0;
        
		Halo[0].D_CoM[0] = 0;
        Halo[0].D_CoM[1] = 0;
        Halo[0].D_CoM[2] = 0;
        Param.VelMerger[0] = Param.VelMerger[1] = 0;
    }

#ifdef SUBSTRUCTURE
	printf("\nSubhalos hosted by cluster <%d> \n\n",SUBHOST );
#endif

	return;
}

static double sph_kernel_wc2(const float r, const float h)
{
	double u = r/h;
	double t = fmax(1 - u, 0);
	
	return 21/2/pi/p3(h)*t*t*t*t * (1+4*u);
}

/* set orbit */
void Apply_kinematics()
{
    const float vx_host = Param.VelMerger[0];
    const float vx_infa = Param.VelMerger[1];

#ifdef PARABOLA // move origin to R200 touch point 
    float dx =  - Halo[1].D_CoM[0] + Param.Boxsize/2 +Halo[1].R200;
    float dy =  - Halo[1].D_CoM[1] + Param.Boxsize/2;
    float dz =  - Halo[1].D_CoM[2] + Param.Boxsize/2;

#pragma omp parallel for 
    for (size_t ipart = 0; ipart < Param.Ntotal; ipart++) {

        float x = P[ipart].Pos[0] +dx; 
        float y = P[ipart].Pos[1] +dy; 
        float z = P[ipart].Pos[2] +dz; 

        if ( y*y + z*z < x*x && (x > 0) )  // infalling cluster 
            P[ipart].Vel[0] += vx_infa;
        else                              // host cluster 
            P[ipart].Vel[0] += vx_host;
    }
#endif // PARABOLA

#if defined(COMET)   
	
	const double boxhalf = Param.Boxsize/2;
    
	const float x0 = Halo[1].D_CoM[0] + boxhalf;
    const float y0 = Halo[1].D_CoM[1] + boxhalf;
    const float z0 = Halo[1].D_CoM[2] + boxhalf;

    const float rVir2 = p2(Halo[1].R200);

	const double h = Halo[1].R_Sample[0];
	const double norm = sph_kernel_wc2(0, h);
	
	#pragma omp parallel for 
    for (size_t ipart = 0; ipart < Param.Ntotal; ipart++) {

        float dx = P[ipart].Pos[0] - x0;
        float dy = P[ipart].Pos[1] - y0;
        float dz = P[ipart].Pos[2] - z0;

        float r2_cyl = p2(dy) + p2(dz);
        float r2 = p2(dx) + p2(dy) + p2(dz);

        if( ((dx > 0) && (r2_cyl < rVir2)) || (r2 < rVir2) ) {

			if (dx < 0)	{ // slow down front part
	
				double r = sqrt(dx*dx + dy*dy + dz*dz);
				
				double wk = 1; // sph_kernel_wc2(r,  h)/norm;

				P[ipart].Vel[0] += vx_infa * wk;
			
			} else { // std velocity
			
				P[ipart].Vel[0] += vx_infa;
			}

		} else {
            P[ipart].Vel[0] += vx_host;
		}
    }
#endif // COMET

    return;
}

/* center the simulation in the periodic box */
void Shift_Origin()
{
    const float boxsize = Param.Boxsize;
    const float boxHalf = boxsize / 2;

    for (int i = 0; i < Param.Nhalos; i++) { // shift clusters away from 0
        
        float dx = Halo[i].D_CoM[0];
        float dy = Halo[i].D_CoM[1];
        float dz = Halo[i].D_CoM[2];

        float vx = Halo[i].BulkVel[0];
        float vy = Halo[i].BulkVel[1];
        float vz = Halo[i].BulkVel[2];

#pragma omp parallel for // DM
        for (size_t ipart=0; ipart<Halo[i].Npart[1]; ipart++) {

            Halo[i].DM[ipart].Pos[0] += dx;
            Halo[i].DM[ipart].Pos[1] += dy;
            Halo[i].DM[ipart].Pos[2] += dz;

            Halo[i].DM[ipart].Vel[0] += vx;
            Halo[i].DM[ipart].Vel[1] += vy;
            Halo[i].DM[ipart].Vel[2] += vz;
        }
        
#pragma omp parallel for  // Gas
        for (size_t ipart=0; ipart<Halo[i].Npart[0]; ipart++) {

            Halo[i].Gas[ipart].Pos[0] += dx;
            Halo[i].Gas[ipart].Pos[1] += dy;
            Halo[i].Gas[ipart].Pos[2] += dz;

            Halo[i].Gas[ipart].Vel[0] += vx;
            Halo[i].Gas[ipart].Vel[1] += vy;
            Halo[i].Gas[ipart].Vel[2] += vz;
        }
    }
    
#pragma omp parallel for // shift 0 to bottom left corner, wrap around box
	for (size_t ipart=0; ipart<Param.Ntotal; ipart++) {
    
        P[ipart].Pos[0] += boxHalf;
        P[ipart].Pos[1] += boxHalf;
        P[ipart].Pos[2] += boxHalf;

        while (P[ipart].Pos[0] > boxsize)
            P[ipart].Pos[0] -= boxsize;

        while (P[ipart].Pos[0] < 0)
            P[ipart].Pos[0] += boxsize;
        
        while (P[ipart].Pos[1] > boxsize)
            P[ipart].Pos[1] -= boxsize;

        while (P[ipart].Pos[1] < 0)
            P[ipart].Pos[1] += boxsize;
        
        while (P[ipart].Pos[2] > boxsize)
            P[ipart].Pos[2] -= boxsize;

        while (P[ipart].Pos[2] < 0)
            P[ipart].Pos[2] += boxsize;
    }

    return ;
}

/* NFW concentration parameter for main and subhalos */ 
double Concentration_parameter(const int i)
{
#ifdef GIVEPARAMS
	if (i < Sub.First)
		return Halo[i].C_nfw;
#endif
	
	const double mass = Halo[i].Mtotal200 * Unit.Mass/Msol2cgs;

#ifdef NFWC_DUFFY08   // simulation fit in WMAP5 cosmology

	const double A = 5.74;
	const double B = -0.097;
	const double C = -0.47;
	const double Mpivot = 2e12 / Cosmo.h_100;
	
	double c_NFW = A * pow( mass/Mpivot, B) * pow( 1+Param.Redshift, C);

#endif // NFWC_DUFFY08

#ifdef NFWC_BUOTE07   // from observations in conc.cosm. (Buote+ 07) 
    
	double c_NFW = 9*pow( mass / (1e14*Msol2cgs), -0.172);

#endif // NFWC_BUOTE07

#ifdef SUBSTRUCTURE

	if (i >= Sub.First) { // Giocoli 2010 eq 16 
		
		double mass_sub = Halo[i].Mass[1] * Unit.Mass/Msol2cgs;

		const double aR = 0.237, c1 = 232.15, c2 = -181.74, 
			  a1 = 0.0146, a2 = 0.008;  // Pieri 2009, fit to Aquarius

		double dx = Halo[SUBHOST].D_CoM[0] - Halo[i].D_CoM[0];
		double dy = Halo[SUBHOST].D_CoM[1] - Halo[i].D_CoM[1];
		double dz = Halo[SUBHOST].D_CoM[2] - Halo[i].D_CoM[2];
		
		double d_vir = sqrt(dx*dx + dy*dy + dz*dz) / Halo[0].R200; 

		c_NFW = pow(d_vir, -aR) * // Pieri 2009
			( c1*pow(mass_sub, -a1) + c2*pow(mass_sub,-a2) );
		c_NFW /= 1+Param.Redshift;
	}

#endif

	return c_NFW ; // * sqrt(Omega_M(Param.Redshift))
}

/* r_core - beta model core radius model */
double Gas_core_radius(const int i, char *string)
{
	double rc = 0;

#ifdef GIVEPARAMS  
    sprintf(string, "rc from parameter file ");    

	if (i < Sub.First)
		return Halo[i].Rcore;
#endif // GIVEPARAMS

    
	if (Param.Cuspy & (1 << i) ) {

		sprintf(string, "Cool Core ");
            
		rc = Halo[i].Rs / 10;
	} else {
            
		sprintf(string, "Disturbed ");
            
		rc = Halo[i].Rs / 3;
    }

	return rc;
}

double 
Hernquist_density_profile(const double m, const double a, const double r)
{
	return m / (2*pi) * a/(r*p3(r+a));
}

/* return M(<= R) of a double beta profile with beta=2/3 */
double Mass_profile(const double r, const double rho0, const double rc, 
		const double rcut)
{	
//	double Mr = p2(rc) * p2(rcut) / (p2(rcut) - p2(rc))  // 2nd order cut
//		* (rcut*atan(r/rcut) - rc*atan(r/rc)); 
	
	const double r2 = p2(r);
	const double rc2 = p2(rc);
	const double rcut2 = p2(rcut);

	/*double Mr = rc2*rcut2*rcut/6/(rc2*rc2*rc2 + rcut2*rcut2*rcut2) * 
		(2*p2(rc2)*log(p3(rcut) + p3(r)) + rc2*rcut2 * log(rcut2 - rcut*r + r2)
		 - 2*rc2*rcut2 * log(rcut + r) 
		 + 2*sqrt(3)*rcut2*(rc2 + rcut2) * atan((2*r-rcut)/sqrt(3)/rcut)
		 - 3 * rc2*rc2*log(rc2 + r2) - 6*rc*rcut2*rcut * atan(r/rc)
		 + 2*rcut2*rcut2 * log(rcut + r) 
		 - rcut2*rcut2*log(rcut2 - rcut*r + r2));*/ // 3rd order

	double Mr = rc2*rcut2*rcut/(8*(p2(rcut2)+p2(rc2))) * // fourth order
		( sqrt2 *( (rc2-rcut2) *( log(rcut2 - sqrt2*rcut*r+r2) 
								- log(rcut2 + sqrt2*rcut*r+r2)) 
				   - 2 * (rc2 + rcut2) * atan(1 - sqrt2*r/rcut) 
				   + 2 * (rc2 + rcut2) * atan(sqrt2 * r/rcut + 1))
		  - 8 * rc * rcut * atan(r/rc)); 

	
	return 4 * pi * rho0 * Mr;
}

/* Double beta profile at rc and rcut */
double Gas_density_profile(const double r, const double rho0, 
		const double rc, const double rcut)
{
	return rho0 / (1 + p2(r/rc)) / (1 + p3(r/rcut) * (r/rcut));
}

