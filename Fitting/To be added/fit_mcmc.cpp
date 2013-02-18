//****************************************************************************
// written by Anna Sajina 09/28/12
// The purpose of this program is to read-in the parameters set in the idl wrapper (simulation.pro) and to use MCMC to determine the best-fit set of parameters. // Specifically it runs through a number of iterations (runs) in each stage calling on simulator.cpp to compute the chi2 for a given set of parameters
// then the metrop algorithm is invoked to decide whether or not a given trial run is accepted. If so it is added to a chain of mcmc values.
//****************************************************************************

#include "simulator.h"
#include "functions.h"
#include "constants.h"

using namespace std;

gsl_rng * r;  /* global generator */

bool metrop(double de,double t){
  
  bool ans;
  
  const gsl_rng_type * T;
  gsl_rng_env_setup();   

  gsl_rng_default_seed = time(NULL);
  T = gsl_rng_default;
  r = gsl_rng_alloc(T);

  //want uniform random number from 0-1
  double itemp=gsl_rng_uniform(r);
  //cout<<itemp;

  if ((de < 0.0) or (itemp < exp(-de/t))){
      ans=true;
    } else ans=false;
  
  return (ans);
}

int main(int argc,char** argv){
  
  long int runs=30;
  const gsl_rng_type * T;
  /* for now only fit the two LF parameters: 
     p[0]="p" density evolution
     p[1]="q" luminosity evolution
  */  
  
  int npar=2; 
  double p_o[npar],dp[npar],p_min[npar],p_max[npar]; //the initial guesses of the parameters, the width of the proposal distribution and the acceptable min/max range
  //double p0_rng[runs],p1_rng[runs];
  double chain[npar][runs];

  string outfile("/Users/annie/students/noah_kurinsky/Fitting/output.fits");
  string modfile("/Users/annie/students/noah_kurinsky/Fitting/model.fits");
  string obsfile("/Users/annie/students/noah_kurinsky/Fitting/observation.fits");
  
  if(argc > 1)
    outfile = argv[1];
  
  int pnum; /*,znum;*/
  
  FITS *pInfile,*pInfile2;
  
  pInfile = new FITS(modfile,Read);
  pInfile2 = new FITS(obsfile,Read);
  
  //reading primary header from fits file
  HDU& params_table = pInfile->pHDU();
  HDU& obs_table = pInfile2->pHDU();
  
  //get number of parameters and initialize dynamic memory
  params_table.readKey("P_NUM",pnum);

  string pi[] = {"0","1","2","3","4","5"};

  double bs[3],errs[3],flims[3];

  for(int i=0;i<3;i++){
    obs_table.readKey("WAVE_"+pi[i+1],bs[i]);
    obs_table.readKey("W"+pi[i+1]+"_FMIN",flims[i]);
    obs_table.readKey("W"+pi[i+1]+"_FERR",errs[i]);
    flims[i]*=1e-3;
    errs[i]*=1e-3;
  }
  
//=================================================================  
//Read-in Luminosity Function Parameters
//-----------------------------------------------------------------
  lumfunct lums(flims[1]);

  double temp;
  params_table.readKey("PHI0",temp);
  lums.set_phi0(pow(10,temp));
  params_table.readKey("L0",temp);
  lums.set_L0(temp);
  params_table.readKey("ALPHA",temp);
  lums.set_alpha(temp);
  params_table.readKey("BETA",temp);
  lums.set_beta(temp);
  params_table.readKey("P",temp);
  lums.set_p(temp);
  p_o[0]=temp; 
  dp[0]=0.1;
  p_min[0]=0.0;
  p_max[0]=7.0;
  params_table.readKey("Q",temp);
  lums.set_q(temp);
  p_o[1]=temp;
  dp[1]=0.1;
  p_min[1]=0.0;
  p_max[1]=5.0;
 
  lums.initialize();

  printf("Initial p: %5.3f, and q: %5.3f\n",p_o[0],p_o[1]);
  fixed_params ps;
  ps.pnum = pnum;

  ps.p = new mparam[pnum];

  delete pInfile;
  delete pInfile2;
  
  //loop trough the mcmc runs
  // at each step, from some initial paramer value "p" call on the simulator.cpp //program to evaluate the chi2 for that particular color-color plot. Then use the metrop algorithm (top) to decide whether or not to keep a particular guess
  //the chain results are stored in ?
  
  double chi_min=1000.0; //the initial chi2_min value, this is iterated each time a better value is found
  double previous=chi_min;
  double trial;
  int minlink;
  //bool metrop;
  long acceptot=0;
  bool ans;
  static double nr[2];
  nr[0]=-1;
  nr[1]=1;
  double p0_rng[runs];
  double q0_rng[runs];
  //double ** p0_rng = new double*[runs];

  gsl_rng_default_seed=time(NULL);
    T=gsl_rng_default;
    r=gsl_rng_alloc(T);
  
    double my_dvdz;
    double area;

    area=4.0*M_PI;
 
  for (int i=0;i<runs;i++){
    p0_rng[i]=*gauss_random(r,nr,0.0,dp[0],1);
    q0_rng[i]=*gauss_random(r,nr,0.0,dp[1],1);
    temp=p_o[0]+p0_rng[i];  
    if((temp >= p_min[0]) && (temp <= p_max[0])) lums.set_p(temp);
    temp=p_o[1]+q0_rng[i];
    if((temp >= p_min[1]) && (temp <= p_max[1])) lums.set_q(temp);
    lums.initialize();
    //trial=simulator();

    my_dvdz=dvdz(area,1.0);
    cout<<my_dvdz<<endl;
    /*
   // call simulator
    //make chi2 from that particular run the trial chi2
    trial=10.0; //get_chi2();
    double r=trial-chi_min;
    if(trial < chi_min){
      chi_min=trial;
      minlink=i;
    }
    ans = metrop(r,temp);
    //if proposal move is accepted
    if (ans=true){
      acceptot++;
      previous=trial;
      //update mcmc chain with accepted values
      for (long l=0;l<=npar; l++){
	chain[l][i]=0.0;
      }
    }
    */
  }
  gsl_rng_free (r);
  return 0;
}


