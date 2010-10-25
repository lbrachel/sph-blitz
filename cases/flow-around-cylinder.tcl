# 1D shock configuration file
# 1: liquids
# 2: gas dynamics
set SIMULATION_MODE 1

# possible values are 
# QuinticSpline, BSpline, CubicSpline
set KERNEL_TYPE QuinticSpline
set OutputType Plain

#possible density treatments
#1: summation density (density obtained by smoothing)
#2: continuity density (density is integrated)
set DENSITY_MODE 1

set INITIAL_CONDITION	0

#possible density treatments
#1: summation density (density obtained by smoothing)
#2: continuity density (density is integrated)
set DENSITY_MODE 1

# number of cell
set ncell 8
set CELLS(0) $ncell
set CELLS(1) $ncell

#possible integration schemes
#1: leap frog
#2: predictor corrector
set INTEGRATION_SCHEME 2

# size of the domain
set L 0.1
set SUPPORT_LENGTH [expr {$L / $ncell}]
set CELL_SIZE $SUPPORT_LENGTH

# the number of particles in one support length
set CELL_RATIO  3

# initial velocity
set U0(0) 0.0
set U0(1) 0.0

set rho0 1e3
set p0 1.0
set T0 1.0

set G_FORCE(0) 1.5e-7
set G_FORCE(1) 0.0

set NUMBER_OF_MATERIALS 2
set Start_time 0.0
set End_time 1e6
# time between output
set D_time 1e2

# factor to create output file name 
set output_file_format_factor 1.0

# boundary conditions
set wall 0 
set periodic 1
set freeslip 2
set symmetry 3
# type and velocity

# disable boundary conditions:
# 1: boundary conditions disabled
# 0: boundary conditions enabled
set DISABLE_BOUNDARY 0
set xBl $periodic
set UxBl(0) 0.0
set UxBl(1) 0.0

set xBr $periodic
set UxBr(0) 0.0
set UxBr(1) 0.0

set yBd $periodic
set UyBd(0) 0.0
set UyBd(1) 0.0

set yBu $periodic
set UyBu(0) 0.0
set UyBu(1) 0.0

set NUMBER_OF_MATERIALS 2
set material_name(0) Air
set material_type(0) 1
set material_cv(0) 1.0e3
set material_eta(0) 1.0e-3
set material_gamma(0) 7.0
set material_b0(0) 1.0e2
set material_rho0(0) 1.0e3
set material_sound_speed(0) 5.77e-4

set material_name(1) $material_name(0)
set material_type(1) $material_type(0)
set material_cv(1) $material_cv(0)
set material_eta(1) $material_eta(0)
set material_gamma(1) $material_gamma(0)
set material_b0(1) $material_b0(0)
set material_rho0(1) $material_rho0(0)
set material_sound_speed(1) $material_sound_speed(0)

set R 2e-2
set x0 [expr {0.5*$L}]
set y0 [expr {0.5*$L}]

# make particles solid outside of the disc
proc getSolid { } {
    # x and y provided by the main program 
    set dx [expr {$x - $x0}]
    set dy [expr {$y - $y0}]
    
    if {[expr {pow($dx,2) + pow($dy,2) < pow($R,2)}]} { 
	# block 
	set issolid 1
    } else {
	set issolid 0
    }
}