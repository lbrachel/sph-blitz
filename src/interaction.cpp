///\file interaction.cpp
///\author Xiangyu Hu <Xiangyu.Hu@aer.mw.tum.de>
///\author changes by: Martin Bernreuther <Martin.Bernreuther@ipvs.uni-stuttgart.de>, 

//----------------------------------------------------------------------------------------
//		defines interaction between particles
//		interaction.cpp
//----------------------------------------------------------------------------------------
#include <glog/logging.h>
#include <assert.h>

// ***** localincludes *****
#include "Kernel/kernel.h"
#include "interaction.h"
#include "particle.h"
#include "initiation.h"
#include "glbfunc.h"

//----------------------------------------------------------------------------------------
//					constructor
//----------------------------------------------------------------------------------------
Interaction::Interaction(const spParticle prtl_org, const spParticle prtl_dest, 
			 spKernel weight_function, 
			 const double dstc,
			 const Initiation& ini):
  ini(ini),
  Org(prtl_org), Dest(prtl_dest),
  mi(Org->m), mj(Dest->m), 
  rmi(1.0/mi), rmj(1.0/mj),
  etai(Org->eta), etaj(Dest->eta),
  zetai(Org->zeta), zetaj(Dest->zeta),
  rij(dstc) {
  assert(prtl_dest != NULL);
  assert(prtl_org != NULL);
  /// particle distance should be in this range
  assert(rij>0.0);
  /// particle must not be that far 
  assert(rij<=2.0*ini.supportlength);
  rrij = 1.0/rij;
  Vi=Org->V;
  Vj=Dest->V;
  eij = (Org->R - Dest->R)*rrij;
  Wij = weight_function->w(rij);
  gradWij=weight_function->gradW(rij,/*Dest->R-Org->R*/eij);
  Fij = weight_function->F(rij)*rrij; //for Kernel weight fuction
   LOG_EVERY_N(INFO, 100000) << "Interaction created,interaction pair: "<<Org->ID<<"  "<<Dest->ID<<"org_x: "<<Org->R[0]<<"  Dest_x: "<<Dest->R[0]<<"  org_y: "<<Org->R[1]<<"Dest_y: "<<Dest->R[1];
}

//-------------------getter for origin-----------------
spParticle  Interaction::getOrigin() const {
  return Org;
}

//-------------------getter for distance-----------------
double Interaction::get_rij() const {
  return rij;
}
//--------------------getter for destination---------------------
spParticle  Interaction::getDest() const {
  return Dest;
}
//-----------getter for Wij
double Interaction::getWij() const {
  return Wij;
}
//----------getter for GradWij
Vec2d Interaction::getGradWij() const {
   return gradWij;
}

//----------------------------------------------------------------------------------------
					//renew pair parameters and changing pair values
//----------------------------------------------------------------------------------------
// Changes: Interaction object
// Depends on: Interaction Object, Org, Dest
void Interaction::RenewInteraction(spKernel weight_function) {
  LOG_EVERY_N(INFO, 10000) << "call Interaction::RenewInteraction()";
  ///- calculate pair parameters (weight functions, shear- and bulk-)
  rij = v_abs(Org->R - Dest->R);
  etai=Org->eta;
  zetai=Org->zeta;
  etaj=Dest->eta;
  zetaj=Dest->zeta;
  assert(rrij>0.0);
  rrij = 1.0/rij;
  Vi=Org->V;
  Vj=Dest->V;
  eij = (Org->R - Dest->R)*rrij;
  Wij = weight_function->w(rij);
  gradWij=weight_function->gradW(rij,/*Dest->R-Org->R*/eij);
  Fij = weight_function->F(rij)*rrij; //for Kernel fuction
}
//----------------------------------------------------------------------------------------
//					summation of the density
//					the idea is different from the original sph method
//----------------------------------------------------------------------------------------
// Changes: Org(rho:summation), Dest(rho:summation)
// Depends on: Interaction Object, Org(rho), Dest(rho)
void Interaction::SummationDensity() {
  //summation according to: rho_i=sum{m_j*W_ij} (here only the contribution of the pair in question)
  /// shell note be called with interaction of the particle
  /// with itself
  assert(Org->ID >= Dest->ID);
  Org->rho += mj*Wij;//changed from mi to mj (07/19/10)
  Dest->rho += mi*Wij; //changed from mj to mi (07/19/10)
}


//----------------------------------------------------------------------------------------
//					update pair forces
//----------------------------------------------------------------------------------------
void Interaction::UpdateForces() {
  LOG_EVERY_N(INFO, 100000) << "Interaction::UpdateForces()";
  //define pair values change in sub time steps
  const double rhoi = Org->rho; 
  const double rhoj = Dest->rho;
  
  /// make sure density is OK
  assert(rhoi>0.0);
  assert(rhoj>0.0);
  
  //const double rVi = 1.0/Vi; 
  //const double rVj = 1.0/Vj;
  
  const double pi = Org->p; 
  const double pj = Dest->p;
  
  /// make sure pressure is OK
  assert(pi>0.0);
  assert(pj>0.0);
  
  const Vec2d Ui = Org->U; 
  const Vec2d Uj = Dest->U;
  const Vec2d Uij = Ui - Uj;
  const double UijdotRij=dot(Uij,(Org->R - Dest->R));
  
  //pair focres or change rate
  //Vec2d dPdti, dUdti, dUdtj; //mometum&velocity change rate
  
  if(ini.simu_mode==2) {
    const double supportlength = ini.supportlength;
    assert(supportlength>0.0);
    assert(rij>0.0);
    /// particle must not be that far 
    assert(rij<=2.0*supportlength);
    
    //const double drhodti=mj*dot((Ui-Uj),gradWij);
    //const double drhodtj=mi*dot((Uj-Ui),((-1)*gradWij));
    
    const double hij=supportlength/2.0;//=0.5*(hi+hj)for later (variable smoothing length);
    const double cij=0.5*(Org->Cs+Dest->Cs);
    const double rhoij=0.5*(rhoi+rhoj);
    
    ///Monaghan artificial viscosity
    double piij = 0.0;
    
    //parameter for time step control
    double mue_ab=0.0;

    //control flag for artificial viscosity
    ///\brief control variable for artificial viscosity (to command if-loop):
    ///
    /// -1: arificial viscosity globally turned on
    ///  1: artificial viscosity globally turned off
    ///  UijdotRij: artificial viscosity turned on only for appraoching particles (compression)
    double artVisc_control;
    
    //assign value to art.visc. control flag depending on art. visc marker
    if (ini.artificial_viscosity_marker==0)//(art. visc. should be turned off)
      artVisc_control=1.0;//if-condition will always be false
    else if (ini.artificial_viscosity_marker==1)//(art. visc. for compression)
      artVisc_control=UijdotRij;
    else //this is the case if marker ==2 (as already tested for values 0,1,2)
      artVisc_control=-1;//always enter if loop below->art.visc. globally turned on
    // enter if loop to compute art. visc. depending on the artVisc_control value	
    if (artVisc_control<0) {
      //according to formula monaghan artificial viscosity
      const double phiij=(hij*UijdotRij)/(pow(rij,2)+ini.epsilon_artVis*pow(hij,2)); 
      //according to formula monaghan artificial viscosity
      piij=(-1*ini.alpha_artVis*cij*phiij+ini.beta_artVis*pow(phiij,2))/rhoij; 
      //parameter for automatic time control
      //mue_ab=-phiij;
      mue_ab=abs(phiij);
      //assert(mue_ab>=0);
    }
    else  {
      piij=0;
      mue_ab=0;//parameter for time control, in Monaghan1989
    };
        
    //assign value for mue_ab to mue_ab_max, if bigger than former max value.
    Org->mue_ab_max=AMAX1(Org->mue_ab_max,mue_ab);
    Dest->mue_ab_max=AMAX1(Dest->mue_ab_max,mue_ab);
    
    //initialize vector for velocity change rate due to physical viscosity
    Vec2d  dUdt_visc(0,0);
    if(Org->ID==2 && Dest->ID==1)
      LOG(INFO)<<"dUdt_visc before (should always be zero) : ("<<dUdt_visc[0]<<","<<dUdt_visc[1]<<")";
    if (ini.physical_viscosity_marker==1) {
      // physical viscosity implementation according to espagnol2003 (eq. 30)
      // where Fij is defined as >= 0 (gradW(r)=-F(r)*r) contary to Monaghan where Fij<=0 (gradW(r)=F(r)*r)
      
      const double d_i=1/Vi;// inverse of particle volume (partice number density)
      if(Org->ID==2 && Dest->ID==1)
	LOG(INFO)<<"d_i :"<<d_i;
      const double d_j=1/Vj;// inverse of particle volume (partice number density)
      if(Org->ID==2 && Dest->ID==1)
	LOG(INFO)<<"d_j :"<<d_j;
      //mean shear viscosity (in case of non-constant viscosity)
      const  double eta_ij=0.5*(etai+etaj); 
      if(Org->ID==2 && Dest->ID==1)
	LOG(INFO)<<" eta_ij :"<< eta_ij;
      //mean bulk viscosity (in case of non-constant viscosity)
      const  double zeta_ij=0.5*(zetai+zetaj);
      if(Org->ID==2 && Dest->ID==1)
	LOG(INFO)<<" zeta_ij :"<< zeta_ij;
      const double eijdotUij=dot(eij,Uij);// factor for calculation of phys. visc.
      if(Org->ID==2 && Dest->ID==1) {
	LOG(INFO)<<" eij :"<< eij;
	LOG(INFO)<<" Uij :"<< Uij;
	LOG(INFO)<<" Ui :"<< Org->U;
	LOG(INFO)<<" Uj :"<< Dest->U;
	LOG(INFO)<<" eijdotUij :"<< eijdotUij;
      }
      const double Fij_=abs(Fij);//as in Espagnol Fij defined >=0!
      if(Org->ID==2 && Dest->ID==1)
	LOG(INFO)<<" Fij_ :"<< Fij_;
      //velocity change rate due to physical viscosity
      dUdt_visc=1/mi*(-1*((5.0*eta_ij)/3.0-zeta_ij)*Fij_/(d_i*d_j)*Uij-5.0*(zeta_ij+eta_ij/3)*Fij_/(d_i*d_j)*eijdotUij*eij);
    } 
    if(Org->ID==2 && Dest->ID==1)
      LOG(INFO)<<"dUdt_visc after : ("<<dUdt_visc[0]<<","<<dUdt_visc[1]<<")";
    
    // calculate total velocity change rate (due to pressure, art. visc and physical visc.)
    const Vec2d dUdti=-mj*(pi/pow(rhoi,2)+pj/pow(rhoj,2)+piij)*gradWij+dUdt_visc;
    const Vec2d dUdtj=mi*(pi/pow(rhoi,2)+pj/pow(rhoj,2)+piij)*gradWij-dUdt_visc;
    
    //energy change rate    
    //(physical viscosity  not (yet) taken into account in energy equation)
    const double dedti=0.5*dot(dUdti,(Uj-Ui));
    const double dedtj=0.5*dot(dUdtj,(Ui-Uj));
    
    //add result to corresponding particle variable (so, an iteration over the interaction list corresponds to the required summation) 
    Org->dUdt += dUdti;
    Dest->dUdt += dUdtj;
    Org->dedt+=dedti;
    Dest->dedt+=dedtj;
  };
  
  if(ini.simu_mode==1)
    {
      LOG_EVERY_N(INFO, 10000) << "Interaction::UpdateForces(), simu_mode=1";
      const double Vi = mi/rhoi; 
      const double Vj = mj/rhoj;
      
      assert(Vi>0.0);
      assert(Vj>0.0);
      const double Vi2 = Vi*Vi; 
      const double Vj2 = Vj*Vj;
      
      ///- calculate density change rate
      const double shear_rij = 2.0*etai*etaj/(etai + etaj);
      LOG_EVERY_N(INFO, 10000) << "etai = " << etai;
      LOG_EVERY_N(INFO, 10000) << "shear_rij = " << shear_rij;
      
      /// viscous and pressure parts
      const Vec2d dPdti_visc = -shear_rij*Fij*(Vi2 + Vj2) * Uij;
      const Vec2d dPdti_pre = - eij*Fij*rij*(pi*Vi2 + pj*Vj2);
      
      const Vec2d dPdti = dPdti_visc  + dPdti_pre;
      LOG_EVERY_N(INFO, 10000) << "Ui = " << Ui;
      LOG_EVERY_N(INFO, 10000) << "Uj = " << Uj;
      LOG_EVERY_N(INFO, 10000) << "dPdti = " << dPdti;
      
      Org->dUdt += dPdti*rmi;
      Dest->dUdt -= dPdti*rmj;
    }
}

void Interaction::UpdateForcesAndRho() {
  LOG_EVERY_N(INFO, 1000) << "Interaction::UpdateForces()";
  
  //define pair values change in sub time steps
  const double rhoi = Org->rho; 
  const double rhoj = Dest->rho;
  
  /// make sure density is OK
  assert(rhoi>0.0);
  assert(rhoj>0.0);
  
  //const double rVi = 1.0/Vi; 
  //const double rVj = 1.0/Vj;
  
  const double pi = Org->p; 
  const double pj = Dest->p;
  
  /// make sure pressure is OK
  assert(pi>0.0);
  assert(pj>0.0);
  
  const Vec2d Ui = Org->U; 
  const Vec2d Uj = Dest->U;
  const Vec2d Uij = Ui - Uj;
  const double UijdotRij=dot(Uij,(Org->R - Dest->R));
  
  //pair focres or change rate
  //Vec2d dPdti, dUdti, dUdtj; //mometum&velocity change rate
  
  if(ini.simu_mode==2) {
    const double supportlength = ini.supportlength;
    assert(supportlength>0.0);
    assert(rij>0.0);
    /// particle must not be that far 
    assert(rij<=2.0*supportlength);
    
    const double drhodti=mj*dot((Ui-Uj),gradWij);
    const double drhodtj=mi*dot((Uj-Ui),((-1)*gradWij));
    
    const double hij=supportlength/2.0;//=0.5*(hi+hj)for later (variable smoothing length);
    const double cij=0.5*(Org->Cs+Dest->Cs);
    const double rhoij=0.5*(rhoi+rhoj);
    
    ///Monaghan artificial viscosity
    double piij = 0.0;
    
    //parameter for time step control
    double mue_ab=0.0;
    
    
    //control flag for artificial viscosity
    ///\brief control variable for artificial viscosity (to command if-loop):
    ///
    /// -1: arificial viscosity globally turned on
    ///  1: artificial viscosity globally turned off
    ///  UijdotRij: artificial viscosity turned on only for appraoching particles (compression)
    double artVisc_control;
    
    //assign value to art.visc. control flag depending on art. visc marker
    if (ini.artificial_viscosity_marker==0)//(art. visc. should be turned off)
      artVisc_control=1.0;//if loop condition will always be false
    else if (ini.artificial_viscosity_marker==1)//(art. visc. for compression)
      artVisc_control=UijdotRij;
    else //this is the case if marker ==2 (as already tested for values 0,1,2)
      artVisc_control=-1;//always enter if loop below->art.visc. globally turned on
    // enter if loop to compute art. visc. depending on the artVisc_control value	
    
    if (artVisc_control<0)  {
      //according to formula "monaghan artificial viscosity"
      const double phiij=(hij*UijdotRij)/(pow(rij,2)+ini.epsilon_artVis*pow(hij,2)); 
      //according to formula monaghan artificial viscosity
      piij=(-1*ini.alpha_artVis*cij*phiij+ini.beta_artVis*pow(phiij,2))/rhoij; 
      //parameter for time control
      mue_ab=-phiij;
      assert(mue_ab>=0);
    }
    else {
      piij=0;
      mue_ab=0;//parameter for time control, in Monaghan1989:=0, if no compression
    };
    LOG_EVERY_N(INFO, 100000)<<Org->ID<<"  "<<Dest->ID<<" artvis: "<<piij;
    const Vec2d dUdti=-mj*(pi/pow(rhoi,2)+pj/pow(rhoj,2)+piij)*gradWij;
    const Vec2d dUdtj=mi*(pi/pow(rhoi,2)+pj/pow(rhoj,2)+piij)*gradWij;
    
    const double dedti=0.5*dot(dUdti,(Uj-Ui));//could also be the other way round: (Ui-Uj)has to be tried out
    const double dedtj=0.5*dot(dUdtj,(Ui-Uj));
    
    //control output
    Org->dUdt += dUdti;
    Dest->dUdt += dUdtj;
    Org->dedt+=dedti;
    Dest->dedt+=dedtj;
    Org->drhodt += drhodti;
    Dest->drhodt += drhodtj;
  };
}

Interaction::~Interaction() {
  LOG_EVERY_N(INFO, 100000) << "Interaction destroyed" ;
}

