/// \file gastimesolver.cpp
/// \author Xiangyu Hu <Xiangyu.Hu@aer.mw.tum.de>
/// \author changes by: Martin Bernreuther <Martin.Bernreuther@ipvs.uni-stuttgart.de>, 

//-----------------------------------------------------------------------
//			Time solver class
//			timesolver.cpp
//-----------------------------------------------------------------------
// ***** system includes *****
#include <iostream>
#include <fstream>
#include <string>
#include <boost/foreach.hpp>
#include <iomanip>

#include <cstdio>
#include <cstdlib>
#include <cmath>

#include <glog/logging.h>

// ***** localincludes *****
#include "glbfunc.h"
#include "hydrodynamics.h"
#include "particlemanager.h"
#include "TimeSolver/gastimesolver.h"
#include "initiation.h"
#include "boundary.h"

using namespace std;

//----------------------------------------------------------------------------------------
//							constructor
//----------------------------------------------------------------------------------------
GasTimeSolver::GasTimeSolver():
  ite(0)
{
  ///- initialize the iteration
  std::cerr << "\n initiation of hydrotimesolver succeeded\n ";
}

void GasTimeSolver::show_information() const {

}

//----------------------------------------------------------------------------------------
//					advance time interval D_time with summation for density
//					predictor and corrector method used
//----------------------------------------------------------------------------------------
///time integration without density (summation density approach)
void GasTimeSolver::TimeIntegral_summation(Hydrodynamics &hydro, ParticleManager &particles, 
                                           Boundary &boundary,
                                           double &Time, double D_time,
                                           const Initiation &ini, spKernel weight_function)
{
  double integeral_time = 0.0;
	
  while(integeral_time < D_time) {

    ///\todo{ move into Initiation?...and/or make time step calculation automatically (constant time step was only for testing purposes)}
    const double dt = 0.0025;
    //control output
    LOG(INFO)<<"\n current timestep:"<<dt;
    LOG(INFO)<<"\n current absolute integraltime:"<<Time;
    LOG(INFO)<<"\n current (relative) integraltime:"<<integeral_time;
    LOG(INFO)<<"\n current (absolute) iterations:"<<ite;
    ite ++;
    integeral_time =integeral_time+ dt;
    Time += dt;
	  
       ///<ul><li>screen information for the iteration
    if(ite % 10 == 0) cout<<"N="<<ite<<" Time: "
			  <<Time<<"	dt: "<<dt<<"\n";

    //below is new code
    if(ite!=1)
      {
	hydro.UpdateUe2Half(dt);

    ///hiernach (noch in deer if schleife) würde ich das BuildBoundaryParticles machen, da es ja für Schritt 1 schon in sph gemacht wird
	boundary.BuildBoundaryParticle(particles,hydro);
      }

    //this block corresponds to calcDerivatives
    hydro.BuildInteractions(particles, weight_function, ini);
    hydro.UpdateDensity(ini, weight_function);

    //hierzwischen muss glaub ich ein boundaryCondition aufruf, um die daten zu aktualisieren, da in updateChangeRate die Particles  benutzt werden (die ja schon auf der interaktion list stehen)
    boundary.BoundaryCondition(particles);

    hydro.UpdateChangeRate(ini);

    if(ite==1)  
      hydro.AdvanceFirstStep(dt);
    else 
      hydro.AdvanceStandardStep(dt);

    //hier würde ich den runaway check (für real particles) einfügen, damit wenn die Zellliste aktualisiert wird, es keine probleme mit den indizes gibt, wenn partikel negatives R kriegen, bzw, damit sie immer in domain bleiben...
    boundary.RunAwayCheck(hydro);  

    particles.UpdateCellLinkedLists();

    //end new code

    /*

    //below is good old code:
 
    //control output
    // cout<<"\n just before build pair\n";
    //predictor and corrector method used
    ///<li>the prediction step
    hydro.BuildInteractions(particles, weight_function, ini);///<ol><li> rebuild interactions
    hydro.UpdateDensity(ini, weight_function);///<li> hydro.UpdateDensity
    //control output
    //	cout<<"\n     --- change rate for predictor:";	
    hydro.UpdateChangeRate(ini);///<li> hydro.UpdateChangeRate
	  
    hydro.Predictor_summation(dt);///<li>hydro.Predictor_summation</ol>
	  
    ///<li> the correction step without update the interaction list
  
 hydro.BuildInteractions(particles, weight_function, ini);///<ol><li> rebuild interactions   

 //hydro.UpdateInteractions(weight_function);///<li> update interactions
    hydro.UpdateDensity(ini, weight_function);///<li>hydro.UpdateDensity

    //control output
    LOG(INFO)<<"change rate for corrector:";
    hydro.UpdateChangeRate(ini); ///<li>hydro.UpdateChangeRate
    hydro.Corrector_summation(dt);///<li>hydro.Corrector_summation</ol>
    particles.UpdateCellLinkedLists();///<li>particles.UpdateCellLinkedLists

    //above is good old code

    */
 }
}
///time integration including density (continuity density approach)
void GasTimeSolver::TimeIntegral(Hydrodynamics &hydro, ParticleManager &particles, 
                                           Boundary &boundary,
                                           double &Time, double D_time,
                                           const Initiation &ini, spKernel weight_function)
{
  double integeral_time = 0.0;
	
  while(integeral_time < D_time) {

    ///\todo{ move into Initiation?...and/or make time step calculation automatically (constant time step was only for testing purposes)}
    const double dt = 0.0025;
    //control output
    LOG(INFO)<<"\n current timestep:"<<dt;
    LOG(INFO)<<"\n current absolute integraltime:"<<Time;
    LOG(INFO)<<"\n current (relative) integraltime:"<<integeral_time;
    LOG(INFO)<<"\n current (absolute) iterations:"<<ite;
    ite ++;
    integeral_time =integeral_time+ dt;
    Time += dt;
	  
       ///<ul><li>screen information for the iteration
    if(ite % 10 == 0) cout<<"N="<<ite<<" Time: "
			  <<Time<<"	dt: "<<dt<<"\n";

    //below is new code
    if(ite!=1)
      hydro.UpdateUeRho2Half(dt);

    //this block corresponds to calcDerivatives
    hydro.BuildInteractions(particles, weight_function, ini);
    if(ite==1) //smooth density (only) at first time step  
      hydro.UpdateDensity(ini, weight_function);
    else
      hydro.UpdateState(ini);
    hydro.UpdateChangeRateInclRho(ini);


    if(ite==1)  
      hydro.AdvanceFirstStepInclRho(dt);
    else 
      hydro.AdvanceStandardStepInclRho(dt);

    particles.UpdateCellLinkedLists();
  }
}
