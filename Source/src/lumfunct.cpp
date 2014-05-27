#include "lumfunct.h" //See this header for more information on the following funcitons

// void lumfunct::set_phi0(double val){
//   phi0=val;
// }

// void lumfunct::set_L0(double val){
//   L0=val;
// }

// void lumfunct::set_alpha(double val){
//   alpha=val;
// }

// void lumfunct::set_beta(double val){
//   beta=val;
// }

// void lumfunct::set_p(double val){
//   p=val;
// }

// void lumfunct::set_q(double val){
//   q=val;
// }

// void lumfunct::set_zmax(double val){
//   zmax=val;
// }

void lumfunct::get_params(double lpars[]){
  lpars[0] = phi0;
  lpars[1] = L0;
  lpars[2] = alpha;
  lpars[3] = beta;
  lpars[4] = p;
  lpars[5] = q;
  lpars[6] = zmax;
}

void lumfunct::set_params(double lpars[]){
  phi0 = lpars[0];
  L0 = lpars[1];
  alpha = lpars[2];
  beta = lpars[3];
  p = lpars[4];
  q = lpars[5];
  zmax = lpars[6];
}

//this function returns the number of sources (per Mpc^3) for a given L-z pair
//this is the main function which needs to be changed to change the lumfunct
double lumfunct::get_nsrcs(double redshift,double lum){
  double nsrcs;
  double t1,t2;
  if(redshift <= zmax) {
    t1=pow(10,phi0)*pow((1.+redshift),p);
    t2=pow(10,L0)*pow((1.+redshift),q);
  }
    if(redshift > zmax) {
    t1=pow(10,phi0)*pow((1.+zmax),p);
    t2=pow(10,L0)*pow((1.+zmax),q);
  }
  double ratio;
  ratio=pow(10,lum)/t2;
  nsrcs=t1/(pow(ratio,alpha)+pow(ratio,beta));
  return nsrcs;
}
