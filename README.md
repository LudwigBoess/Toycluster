# Toycluster

## SPH initial conditions for idealized Cluster Mergers using weighted Voronoi Tesselations (Donnert 2014, 2017)

	* Physical model from Donnert 2014
	* Simple OpenMP Tree for neighbour finding with WC6
	* DM distribution function using numerical solution of Eddingtons eq.
	* SPH density sampling & relaxation with Voronoi Tesselations (Diehl+)
	* Magnetic fields from vector potential, Model from Bonafede+ 2010
	* Merger using Comet like or parabular shapes
	* Velocities parametrized using zero-energy orbit
	* optional: substructure population from Giocoli 2010
	* optional: placement of a third halo anywhere in the box
	
## Known Issues: 
	* Magnetic field normalisation resolution dependent, due to low accuracy
		SPH rotation operator. (LMB: wrong, issue due to race condition in OMP parallel loop. Fixed!)
	* DivB=0 constraint pretty bad.


# Usage:

Makefile options:
```
	OPT += -DNFWC_DUFFY08				# alternate fit to concentr. param

	OPT += -DBETA=0.66					# Beta for gas halo

	OPT += -DPARABOLA      			    # merge in a parabola
	OPT	+= -DCOMET						# merge like a comet, ball+tail (recommended)

	OPT	+= -DDOUBLE_BETA_COOL_CORES		# Fits 2 beta profiles instead of one to resemble cool-core clusters

	OPT	+= -DGIVEPARAMS					# more merger parameters in .par file

	OPT	+= -DNO_RCUT_IN_T				# set Rcut very large

	OPT	+= -DSUBSTRUCTURE				# add substructure
	OPT += -DSUBHOST=1					# which halo the structure should be added to [ 0, 1 ]
	OPT	+= -DSLOW_SUBSTRUCTURE			# put subhalos on Hernquist orbits
	OPT += -DREPORTSUBHALOS				# print info about all subhaloes

	OPT += -DADD_THIRD_SUBHALO  		# manually set the first subhalo mass, pos, vel
	OPT += -DTHIRD_HALO_ONLY

	OPT += -DSPH_CUBIC_SPLINE 			# for use with Gadget2
```

Parameter file:

```
	% % Toycluster Parameter File %%

	Output_file ./IC_Cluster    % Base name

	Ntotal      2000000  		% Total Number of Particles in R200
	Mtotal      1.1e5   		% Total Mass in Code Units

	Mass_Ratio  0       		% set =0 for single cluster

	ImpactParam 0
	ZeroEOrbitFrac 0.5

	Cuspy       1      			% Use cuspy model (rc /= 10)

	Redshift	0.0				% Redshift of IC

	Bfld_Norm   5e-6    		% B(r) = B0 * normalised_density^eta
	Bfld_Eta    0.5     		% like Bonafede 2010. B0 /=2 if Mtotal<5d4
	Bfld_Scale  100

	bf          0.17 			% bf in r200, bf = 17% ~ 14% in r500
	h_100       1.0     		% HubbleConstant/100

	%Units
	UnitLength_in_cm 			3.085678e21        %  1.0 kpc
	UnitMass_in_g 				1.989e43           %  1.0e10 solar masses
	UnitVelocity_in_cm_per_s 	1e5                %  1 km/sec

	%% -DGIVEPARAMS Options
	%% here some more merger parameters can be set by hand

	% cluster 0
	c_nfw_0     3.18951
	v_com_0     0
	beta_0		0.66
	rc_0        215.588

	% cluster 1
	c_nfw_1     3.41133
	v_com_1     0
	beta_1		0.66
	rc_1        159.986

	%% -DADD_THIRD_SUBHALO Options

	SubFirstMass 1e12

	SubFirstPos0 10
	SubFirstPos1 120
	SubFirstPos2 980

	SubFirstVel0 100
	SubFirstVel1 100
	SubFirstVel2 100

	%% -DDOUBLE_BETA_COOL_CORES Options

	Rho0_Fac	50	% increase in Rho0
	Rc_Fac		40	% decrease in Rcore

```

To run:

```bash
	make
	export OMP_NUM_THREADS=<N>
	./Toycluster cluster.par
```

## Example

For a simple example of a 2:1 mass merger you can use the following Makefile options and parameter file.

Makefile options:

```
	OPT += -DNFWC_DUFFY08		# alternate fit to concentr. param

	OPT += -DBETA=0.66			# Beta for gas halo

	OPT	+= -DCOMET				# merge like a comet, ball+tail (recommended)

	OPT	+= -DNO_RCUT_IN_T		# set Rcut very large
```

Parameter file:

```
	% % Toycluster Parameter File %%

	Output_file ./IC_Cluster    % Base name

	Ntotal      2000000  		% Total Number of Particles in R200
	Mtotal      1.1e5   		% Total Mass in Code Units

	Mass_Ratio  0       		% set =0 for single cluster

	ImpactParam 0
	ZeroEOrbitFrac 0.5

	Cuspy       1      			% Use cuspy model (rc /= 10)

	Redshift	0.0				% Redshift of IC

	Bfld_Norm   5e-6    		% B(r) = B0 * normalised_density^eta
	Bfld_Eta    0.5     		% like Bonafede 2010. B0 /=2 if Mtotal<5d4
	Bfld_Scale  100

	bf          0.17 			% bf in r200, bf = 17% ~ 14% in r500
	h_100       1.0     		% HubbleConstant/100

	%Units
	UnitLength_in_cm 			3.085678e21        %  1.0 kpc
	UnitMass_in_g 				1.989e43           %  1.0e10 solar masses
	UnitVelocity_in_cm_per_s 	1e5                %  1 km/sec
```