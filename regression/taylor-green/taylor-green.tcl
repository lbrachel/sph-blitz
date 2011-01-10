# 1D shock configuration file
# 1: liquids
# 2: gas dynamics
set SIMULATION_MODE 1

# possible values are 
# QuinticSpline, BSpline, CubicSpline
set KERNEL_TYPE QuinticSpline

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
#set initial_perturb 

#possible integration schemes
#1: leap frog
#2: predictor corrector
set INTEGRATION_SCHEME 2

# size of the domain
set L 1.0
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

set G_FORCE(0) 0.0
set G_FORCE(1) 0.0

set NUMBER_OF_MATERIALS 2
set Start_time 0.0
set End_time 0.1
# time between output
set D_time 0.01
set output_file_format_factor 100

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
set material_eta(0) 1.0
set material_gamma(0) 7.0
set material_b0(0) 1.0e2
set material_rho0(0) 1.0
set material_sound_speed(0) 1.0

set material_name(1) $material_name(0)
set material_type(1) $material_type(0)
set material_cv(1) $material_cv(0)
set material_eta(1) $material_eta(0)
set material_gamma(1) $material_gamma(0)
set material_b0(1) $material_b0(0)
set material_rho0(1) $material_rho0(0)
set material_sound_speed(1) $material_sound_speed(0)

set v0 1.0
proc getVelocity { } {
    # generate taylor-green initial conditions
    set pi 3.1415926
    set k0 [expr {2.0 * $pi/$L}]
    set vx  [expr {-$v0*cos($k0*$x)*sin($k0*$y)} ] 
    set vy  [expr {$v0*sin($k0*$x)*cos($k0*$y)} ]
}