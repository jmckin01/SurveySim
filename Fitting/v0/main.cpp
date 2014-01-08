//****************************************************************************
// written by Anna Sajina and Noah Kurinsky, 2012
// updated by Noah Kurinsky 10/13
// The purpose of this program is to read-in the parameters set in the idl 
// wrapper (simulation.pro) and to use MCMC to determine the best-fit set of 
// parameters. 
// Specifically it runs through a number of iterations (runs) in each stage 
// calling on simulator.cpp to compute the chi2 for a given set of parameters
// then the metrop algorithm is invoked to decide whether or not a given trial 
// run is accepted. If so it is added to a chain of mcmc values.
//****************************************************************************

#include "simulator.h"
#include "functions.h"
#include "mc_util.h"
#include <stdio.h>

//Array size definitions 
#define BANDS 3

using namespace std; 

gsl_rng * r;  // global generator

int main(int argc,char** argv){
  
  if(argc < 4){
    printf("%s","ERROR: Invalid number of arguments.\n");
    printf("%s","Calling sequence should be \"fit obsfile modfile sedfile [output]\"\n");
    return 1;}

  printf("\nMCMC Fitting Code Compiled: %s\n\n",__DATE__);
  
  //File names passed in by Widget
  string outfile("output.fits");
  string obsfile(argv[1]);
  string modfile(argv[2]);
  string sedfile(argv[3]);
  //If outfile specified as argument, change from default
  if(argc > 4)
    outfile = argv[4];

  string pnames[] = {"PHI0","L0","ALPHA","BETA","P","Q","ZCUT"};
  unsigned long runs; 
  int nz,ns;
  double area, dz, zmax, zmin, rtemp, rmax, a_ci;
  double TMAX,IDEALPCT;
  unsigned long NCHAIN,BURN_STEP,CONV_STEP,BURN_RATIO;
  products output;
  double bs[BANDS],errs[BANDS],flims[BANDS];
  //these arrays holds phi0,L0,alpha,beta,p, and q as well as range and fixed
  double lpars[LUMPARS],lfix[LUMPARS],lmax[LUMPARS],lmin[LUMPARS],ldp[LUMPARS],cexp[5];
  vector<int> param_inds;
  const gsl_rng_type * T;

  FITS *pInfile,*pInfile2;
  pInfile = new FITS(modfile,Read);
  pInfile2 = new FITS(obsfile,Read);
  
  //reading primary header from fits file
  HDU& params_table = pInfile->pHDU();
  HDU& obs_table = pInfile2->pHDU();

  params_table.readKey("RUNS",rtemp);
  runs = (unsigned long) rtemp;
  params_table.readKey("ZMIN",zmin);
  params_table.readKey("ZMAX",zmax);
  params_table.readKey("DZ",dz);
  params_table.readKey("AREA",area);

  params_table.readKey("NCHAIN",rtemp);
  NCHAIN = (unsigned long) rtemp;
  params_table.readKey("TMAX",TMAX);
  params_table.readKey("ANN_PCT",IDEALPCT);
  params_table.readKey("BURN_STE",rtemp);
  BURN_STEP = (unsigned long) rtemp;
  params_table.readKey("CONV_STE",rtemp);
  CONV_STEP = (unsigned long) rtemp;
  params_table.readKey("BURNVRUN",rtemp);
  BURN_RATIO = (unsigned long) rtemp;
  params_table.readKey("CONV_RMA",rmax);
  params_table.readKey("CONV_CON",a_ci);

  printf("MC Settings\n");
  printf("Chain Number    : %lu\n",NCHAIN);
  printf("Starting Temp   : %f\n",TMAX);
  printf("Ideal Accept Pct: %f\n",IDEALPCT);
  printf("Burn-in Step    : %lu\n",BURN_STEP);
  printf("Convergence Step: %lu\n",CONV_STEP);
  printf("Run:Burn-In     : %lu\n",BURN_RATIO);
  printf("Convergence Criterion: %f\n",rmax);
  printf("Confidence Interval  : %f\n",a_ci);

  //this is necessary for correct cosmological volume determinations
  //hence correct number count predictions
  area*=pow((M_PI/180.0),2.0); //in steradians from sq.deg.

  //compute redshift bin number from FITS values
  nz = (zmax-zmin)/dz;
  printf("Simulation Settings:\n");
  printf("Run Number Max: %lu\nNumber Redshift Bins: %i\nRedshift Bin Width: %f\n\n",runs,nz,dz);

  char wtemp[2];
  string wnum;
  for(int i=0;i<BANDS;i++){
    sprintf(wtemp,"%i",i+1);
    wnum = string(wtemp);
    obs_table.readKey("WAVE_"+wnum,bs[i]); //should already be in microns
    obs_table.readKey("W"+wnum+"_FMIN",flims[i]); //should be in mJy
    obs_table.readKey("W"+wnum+"_FERR",errs[i]);
    printf("Band %s:\t%8.3e %7.3e %7.3e\n",wnum.c_str(),bs[i],flims[i],errs[i]);
  }
  
  //=================================================================  
  //Read-in Luminosity Function Parameters
  //-----------------------------------------------------------------

  //read parameter values
  params_table.readKey("PHI0",lpars[0]);
  params_table.readKey("L0",lpars[1]);
  params_table.readKey("ALPHA",lpars[2]);
  params_table.readKey("BETA",lpars[3]);
  params_table.readKey("P",lpars[4]);
  params_table.readKey("Q",lpars[5]);
  params_table.readKey("ZCUT",lpars[6]);
  //read parameter value fitting boolean
  params_table.readKey("PHI0_FIX",lfix[0]);
  params_table.readKey("L0_FIX",lfix[1]);
  params_table.readKey("ALPHA_FI",lfix[2]);
  params_table.readKey("BETA_FIX",lfix[3]);
  params_table.readKey("P_FIX",lfix[4]);
  params_table.readKey("Q_FIX",lfix[5]);
  params_table.readKey("ZCUT_FIX",lfix[6]);
  //read parameter minimum values
  params_table.readKey("PHI0_MIN",lmin[0]);
  params_table.readKey("L0_MIN",lmin[1]);
  params_table.readKey("ALPHA_MI",lmin[2]);
  params_table.readKey("BETA_MIN",lmin[3]);
  params_table.readKey("P_MIN",lmin[4]);
  params_table.readKey("Q_MIN",lmin[5]);
  params_table.readKey("ZCUT_MIN",lmin[6]);
  //read parameter maximum values
  params_table.readKey("PHI0_MAX",lmax[0]);
  params_table.readKey("L0_MAX",lmax[1]);
  params_table.readKey("ALPHA_MA",lmax[2]);
  params_table.readKey("BETA_MAX",lmax[3]);
  params_table.readKey("P_MAX",lmax[4]);
  params_table.readKey("Q_MAX",lmax[5]);
  params_table.readKey("ZCUT_MAX",lmax[6]);
  //read parameter sigma values
  params_table.readKey("PHI0_DP",ldp[0]);
  params_table.readKey("L0_DP",ldp[1]);
  params_table.readKey("ALPHA_DP",ldp[2]);
  params_table.readKey("BETA_DP",ldp[3]);
  params_table.readKey("P_DP",ldp[4]);
  params_table.readKey("Q_DP",ldp[5]);
  params_table.readKey("ZCUT_DP",ldp[6]);
  
  for (int i=0;i<LUMPARS;i++)
    if(lfix[i] == 0)
      param_inds.push_back(i);
  
  printf("\nNumber Unfixed Parameters: %lu\n",param_inds.size());

  //read color_exp values
  params_table.readKey("CEXP",cexp[0]);
  params_table.readKey("CEXP_FIX",cexp[1]);
  params_table.readKey("CEXP_MIN",cexp[2]);
  params_table.readKey("CEXP_MAX",cexp[3]);
  params_table.readKey("CEXP_DP",cexp[4]);
  
  /*
    note that the dp values here are the widths of the "proposal distribution"
    the smaller they are the slower will converge onto the right answer
    the bigger they are, the less well sampled the probability distribution 
    will be and hence the less accurate the final answer
    needs to find the "goldilocks" zone here but that's very problem specific 
    so requires trial and error as we actually start fitting data. 
  */

  lumfunct lf;
  lf.set_params(lpars);

  printf("Initial p: %5.3f, and q: %5.3f\n",lpars[4],lpars[5]);
  printf("p range: %5.3f - %5.3f, sigma: %5.3f\n",lmin[4],lmax[4],ldp[4]);
  printf("q range: %5.3f - %5.3f, sigma: %5.3f\n",lmin[5],lmax[5],ldp[5]);
  printf("Color Evolution: %5.3f\n",cexp[0]);
  printf("Redshift Cutoff: %5.3f\n",lpars[6]);

  delete pInfile;
  delete pInfile2;

  /*
    Loop trough the mcmc runs
    At each step, from some initial paramer value "p" call on the simulator.cpp 
    program to evaluate the chi2 for that particular color-color plot. 
    Then use the metrop algorithm (below) to decide whether or not to keep a particular guess
    The chain results (including all accepted guesses) will provide us with the posterior 
    probability distributions for the fitted parameters.
  */

  //initialize simulator
  simulator survey(bs,errs,flims,obsfile,sedfile); 
  survey.set_color_exp(cexp[0]); //color evolution
  survey.set_lumfunct(&lf);

  //the initial chi2_min value
  //this is iterated each time a better value is found
  double chi_min=1.0E+4; 
  double trial;
  double prng[param_inds.size()][runs];
  unsigned long i,m,p;

  // the temperature, keep fixed for now, but can also try annealing 
  // in the future, this has similar effect as the width of the proposal
  // distribution, as determines how likely a far flung guess is of being 
  // accepted (see metrop function)
  //double tmc=10.00; //to distinguish it from the random T
  double temp;
  double ptemp[NCHAIN][param_inds.size()];
  double pbest[param_inds.size()];
  double pcurrent[NCHAIN][param_inds.size()];

  gsl_rng_default_seed=time(NULL);
  T=gsl_rng_default;
  r=gsl_rng_alloc(T);

  //the flux array is logarithmic in steps of 0.3dex 
  //for now lets just use one band (here 250um) although of course might be nice to keep the rest at some point, but one step at a time.
  ns=8;
  survey.set_size(area,dz,zmin,nz,ns);
  MCChains mcchain(NCHAIN,param_inds.size(),runs,BURN_RATIO);
  mcchain.set_constraints(rmax,a_ci);
  MetropSampler metrop(NCHAIN,TMAX,IDEALPCT,r);

  int pqind[2];
  int itemp;
  pqind[0] = pqind[1] = 0;
  for(m=0;m<param_inds.size();m++){
    if (param_inds[m] == 4)
      pqind[0] = m;
    if (param_inds[m] == 5)
      pqind[1] = m;
  } 
  if ((pqind[0] == pqind[1]) or (pqind[0] < 0) or (pqind[1] < 0))
    printf("Warning: p and/or q fixed");
  
  for(m=0;m<NCHAIN;m++){ 
    for(unsigned long i=0;i<param_inds.size();i++){
      ptemp[m][i] = pcurrent[m][i] = lpars[param_inds[i]];
    }
    if (m > 0){
      itemp = ((m-1)/2)%2;
      if (m%2 == 0){
	pcurrent[m][pqind[itemp]] = ptemp[m][pqind[itemp]] = ptemp[m][pqind[itemp]] + (lmin[param_inds[pqind[itemp]]] - ptemp[m][pqind[itemp]])/2.0;
      }
      else{
	pcurrent[m][pqind[itemp]] = ptemp[m][pqind[itemp]] = ptemp[m][pqind[itemp]] + (lmax[param_inds[pqind[itemp]]] - ptemp[m][pqind[itemp]])/2.0;
      }
    }
  }
  
  printf("\n ---------- Beginning MC Burn-in Phase ---------- \n");
  for (i=0;i<(runs/BURN_RATIO);i++){
    for (m=0;m<NCHAIN;m++){
      for(p=0;p<param_inds.size();p++){
	prng[p][i]=gsl_ran_gaussian(r,ldp[param_inds[p]]);
	temp=pcurrent[m][p]+prng[p][i];
	if((temp >= lmin[param_inds[p]]) && (temp <= lmax[param_inds[p]])) {
	  ptemp[m][p]=temp;
	  lpars[param_inds[p]] = temp;
	}
      }
      
      printf("%lu %lu - %lf %lf : ",(i+1),(m+1),ptemp[m][pqind[0]],ptemp[m][pqind[1]]);
      lf.set_params(lpars);
      output=survey.simulate();
      trial=output.chisqr;
      printf("Iteration Chi-Square: %lf",trial);
      
      if(trial < chi_min){
	printf(" -- Current Best Trial");
	chi_min=trial;
	for(p=0;p<param_inds.size();p++)
	  pbest[p]=ptemp[m][p];
      }
      printf("\n");

      if(metrop.accept(m,trial)){
	for(p=0;p<param_inds.size();p++)
	  pcurrent[m][p]=ptemp[m][p];
      }      
    }
    if(((i+1) % BURN_STEP) == 0)
      if(not metrop.anneal())
	i = runs;
  }
  
  for(m=0;m<NCHAIN;m++)
    for(p=0;p<param_inds.size();p++)
      ptemp[m][p] = pcurrent[m][p] = pbest[p];
  
  metrop.reset();
  
  printf("\n\n ---------------- Fitting Start ---------------- \n Total Run Number: %ld\n\n",runs);
  for (i=0;i<runs;i++){
    for (m=0;m<NCHAIN;m++){
      for(p=0;p<param_inds.size();p++){
	prng[p][i]=gsl_ran_gaussian(r,ldp[param_inds[p]]);
	temp=pcurrent[m][p]+prng[p][i];
	if((temp >= lmin[param_inds[p]]) && (temp <= lmax[param_inds[p]])){
	  ptemp[m][p]=temp;
	  lpars[param_inds[p]] = temp;
	}
      }
      
      printf("%lu %lu - %lf %lf : ",(i+1),(m+1),ptemp[m][pqind[0]],ptemp[m][pqind[1]]);
      lf.set_params(lpars);
      output=survey.simulate();
      trial=output.chisqr;
      printf("Model chi2: %lf",trial);
      
      mcchain.add_link(m,ptemp[m],trial);
      
      if(metrop.accept(m,trial)){
	printf(" -- Accepted\n");
	for(p=0;p<param_inds.size();p++)
	  pcurrent[m][p]=ptemp[m][p];
      }
      else
	printf(" -- Rejected\n");
    }
    
    if(((i+1) % CONV_STEP) == 0){
      printf("Checking Convergence\n");
      metrop.anneal();
      if(mcchain.converged()){
	printf("Chains Converged!\n");
	i = runs;}
      else
	printf("Chains Have Not Converged\n");
    } 
  }
  
  mcchain.get_best_link(pbest,chi_min);
  for(p=0;p<param_inds.size();p++)
    lpars[param_inds[p]] = pbest[p];
  lf.set_params(lpars);
  printf("\nRe-Running Best Fit...\n");
  output=survey.simulate();
  printf("Model chi2: %lf\n",output.chisqr);
  printf("Acceptance Rate: %lf%%\n",metrop.mean_acceptance());
  
  string *parnames = new string[param_inds.size()];
  for(p=0;p<param_inds.size();p++)
    parnames[p] = pnames[param_inds[p]];
  
  bool saved;
  saved = survey.save(outfile);
  saved &= mcchain.save(outfile,parnames);
  if(saved)
    printf("Save Successful\n");
  else
    printf("Save Failed\n");
  
  delete[] parnames;
  gsl_rng_free (r);
  
  printf("\nReturning from fitting routine\n\n");
  
  return 0;
}
