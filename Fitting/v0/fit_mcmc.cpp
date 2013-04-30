//****************************************************************************
// written by Anna Sajina 09/28/12
// updated by Noah Kurinsky 3/15/13
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
#include <stdio.h>

//For values to be passed in by widget
#define BANDS 3
#define NPAR 2
#define RUNS 10
#define DZ 0.1
#define ZMAX 4.0

using namespace std;

gsl_rng * r;  /* global generator */
bool metrop(double de,double t);

int main(int argc,char** argv){
  
  if(argc < 4){
    printf("%s","ERROR: Invalid number of arguments.\n");
    printf("%s","Calling sequence should be \"fit obsfile modfile sedfile [output]\"\n");
    return 1;}
  
  //File names passed in by Widget
  string outfile("output.fits");
  string obsfile(argv[1]);
  string modfile(argv[2]);
  string sedfile(argv[3]);
  //If outfile specified as argument, change from default
  if(argc > 4)
    outfile = argv[4];

  FILE *chain;
  chain = fopen("chain.txt","w");
  
  //temporary, of course in the end this will be much longer 
  //perhaps 100,000-500,000 
  unsigned long runs=RUNS;  
 
  const gsl_rng_type * T;

  // the initial guesses of the parameters, the width of the proposal distribution 
  // and the acceptable min/max range
  double p_o[NPAR],dp[NPAR],p_min[NPAR],p_max[NPAR]; 

  FITS *pInfile,*pInfile2;
  pInfile = new FITS(modfile,Read);
  pInfile2 = new FITS(obsfile,Read);
  
  //reading primary header from fits file
  HDU& params_table = pInfile->pHDU();
  HDU& obs_table = pInfile2->pHDU();

  double bs[BANDS],errs[BANDS],flims[BANDS];
  string pi[] = {"0","1","2","3","4","5"};

  printf("%s\n","Bands:");
  for(int i=0;i<BANDS;i++){
    obs_table.readKey("WAVE_"+pi[i+1],bs[i]); //should already be in microns
    obs_table.readKey("W"+pi[i+1]+"_FMIN",flims[i]); //should be in mJy
    obs_table.readKey("W"+pi[i+1]+"_FERR",errs[i]);
    printf("%fl\t%fl\t%fl\n",bs[i],flims[i],errs[i]);
  }

//=================================================================  
//Read-in Luminosity Function Parameters
//-----------------------------------------------------------------

//this array holds phi0,L0,alpha,beta,p, and q
  double lpars[6];
  params_table.readKey("PHI0",lpars[0]);
  params_table.readKey("L0",lpars[1]);
  params_table.readKey("ALPHA",lpars[2]);
  params_table.readKey("BETA",lpars[3]);
  params_table.readKey("P",lpars[4]);
  params_table.readKey("Q",lpars[5]);
  
  //note that the dp values here are the widths of the "proposal distribution"
  //the smaller they are the slower will converge onto the right answer
  //the bigger they are, the less well sampled the probability distribution 
  //will be and hence the less accurate the final answer
  //needs to find the "goldilocks" zone here but that's very problem specific 
  //so requires trial and error as we actually start fitting data.
  p_o[0]=lpars[4]; 
  dp[0]=0.3;
  p_min[0]= 0.0;
  p_max[0]= 10.0;
  
  p_o[1]=lpars[5];
  dp[1]=0.3;
  p_min[1]=0.0;
  p_max[1]=5.0;
 
  printf("Initial p: %5.3f, and q: %5.3f\n",p_o[0],p_o[1]);

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

  lumfunct lf;
  lf.set_phi0(lpars[0]);
  lf.set_L0(lpars[1]);
  lf.set_alpha(lpars[2]);
  lf.set_beta(lpars[3]);
  lf.set_p(lpars[4]);
  lf.set_q(lpars[5]);

  //initialize simulator
  simulator survey(bs,errs,flims,obsfile,sedfile); 
  survey.set_lumfunct(&lf);

  //the initial chi2_min value
  //this is iterated each time a better value is found
  double chi_min=1.0E+4; 
  double previous=chi_min;
  double trial,de;
  int minlink;
  long acceptot=0;
  bool ans;

  double p0_rng[runs];
  double q0_rng[runs];

  gsl_rng_default_seed=time(NULL);
  T=gsl_rng_default;
  r=gsl_rng_alloc(T);
  
  int nz,ns;
  double area, dz;
  products output;

  //note need to be able to pass the survey area down from the widget!!!
  //this is necessary for correct cosmological volume determinations
  //hence correct number count predictions
  //this value of area here is just a placeholder
  area=10.0*pow((M_PI/180.0),2.0); //in steradians from 10.0sq.deg.

  //we don't want to keep information on each simulated source, but ultimately need to know the distribution with redshift, flux (i.e. number counts) and SED type for all *accepted* runs. 
  // I think the best way to do so is to define the redshift, flux, and type arrays now, pass them on to survey.simulate from which we get not only chi2 but also these distributions. If accepted they are tagged onto the MC chain and saved with it. This increases the number of columns but not unmanageably so (currently, 21) 
  dz = DZ;
  nz = ZMAX/dz;
  printf("\nNZ: %i, DZ: %f\n\n",nz,dz);
 
  //the flux array is logarithmic in steps of 0.3dex 
  //for now lets just use one band (here 250um) although of course might be nice to keep the rest at some point, but one step at a time.
  ns=8;
  
  //note: chain is +1 as last column holds chi2 values for the particular "guess"
  int chainsize = NPAR+nz+ns+1;
  valarray<double> mcchain[chainsize];
  for(int i=0;i<chainsize;i++)
    mcchain[i].resize(runs);

  // the temperature, keep fixed for now, but can also try annealing 
  // in the future, this has similar effect as the width of the proposal
  // distribution, as determines how likely a far flung guess is of being 
  // accepted (see metrop function)
  double tmc=100.00; //to distinguish it from the random T
  double temp,bestp,bestq,ptemp,qtemp;

  fprintf(chain,"%s\n%s\n","Monte Carlo Parameter Chain","Iteration, P, Q, Chi-Square");

  for (unsigned long i=0;i<runs;i++){
    p0_rng[i]=gsl_ran_gaussian(r,dp[0]);
    q0_rng[i]=gsl_ran_gaussian(r,dp[1]);
    temp=lpars[4]+p0_rng[i];  //fix this (not just the initial guess)  
    if((temp >= p_min[0]) && (temp <= p_max[0])) ptemp=temp;
    temp=lpars[5]+q0_rng[i];
    if((temp >= p_min[1]) && (temp <= p_max[1])) qtemp=temp;
    //check to see if sensible guesses, need to also do some test 
    //the randomness at some point
    printf("\n\n%lu %lf %lf...",(i+1),lpars[4],lpars[5]);
    
    lf.set_p(ptemp);
    lf.set_q(qtemp);
    printf("Running...\n");
    output=survey.simulate(area,nz,dz,ns);
    trial=output.chisqr;
    printf("\nModel chi2: %lf\n",trial);

    de=trial-chi_min;
    if(trial < chi_min){
      chi_min=trial;
      minlink=i;
      bestp = ptemp;
      bestq = qtemp;
    }
    ans=metrop(de,tmc);
    if(ans=true){
      acceptot++;
      previous=trial;
      //update mcmc chain with accepted values
      lpars[4] = ptemp;
      lpars[5] = qtemp;
      fprintf(chain,"%lu %f %f %f \n",i,lpars[4],lpars[5],trial);
      mcchain[0][i]=lpars[4];
      mcchain[1][i]=lpars[5];
      for(int iz=0;iz<nz;iz++) mcchain[iz+NPAR][i]=output.dndz[iz];
      for(int is=0;is<ns;is++) mcchain[is+nz+NPAR][i] = 0;
      mcchain[chainsize-1][i]=trial;
    }
  }

  lf.set_p(bestp);
  lf.set_q(bestq);
  printf("Re-Running Best Fit...\n");
  output=survey.simulate(area,nz,dz,ns);
  printf("\nModel chi2: %lf\n",output.chisqr);
  survey.save(outfile);
  
  bool save = true;
  using namespace CCfits;
  std::auto_ptr<FITS> pFits(0);

  try{
    pFits.reset(new FITS(outfile,Write));
  }
  catch (CCfits::FITS::CantOpen){
    std::cerr << "Unknown Error Occurred, Can't save chain" << endl;
    save = false;
  }

  save=false;
  if(save){
    int vecsize = chainsize;
    std::vector<string> colnames(vecsize,"dnds");
    std::vector<string> colunits(vecsize,"");
    std::vector<string> colform(vecsize,"f");
    string hname("Chain");
    
    char temp[2];
    
    colnames[0] = "P";
    colnames[1] = "Q";
    for(int iz=0;iz<nz;iz++){
      sprintf(temp,"%i",iz);
      colnames[iz+NPAR] = "dndz"+std::string(temp);
    }
    for(int is=0;is<ns;is++){
      sprintf(temp,"%i",is);
      colnames[is+NPAR+nz] = "dnds"+std::string(temp);
    }
    colnames[vecsize-1] = "chisq";
    
    cerr << "writing" << endl;

    Table *newTable = pFits->addTable(hname,runs,colnames,colform,colunits,AsciiTbl);

    cerr << "table initialized" << endl;

    for (int i=0;i<(NPAR+nz+ns+1);i++){
      newTable->column(colnames[i]).write(mcchain[i],1);
    }

    cerr << "table written" << endl;
  }
  
  fclose(chain);
  gsl_rng_free (r);
  
  return 0;
}


bool metrop(double de,double tmc){
//here de is effectively Delta(Energy), and t is the temperature
//as in the boltzman factor exp(-de/t)
  
  static bool ans;
  static double itemp,tester;

  tester=de/tmc;
  //to avoid getting segmentation fault from taking e^x where x is very large
  if(tester <= -2) ans = (de<0.0) ? true : false;
  if(tester > -2) {
    static const gsl_rng_type * T;
    gsl_rng_env_setup();   
    gsl_rng_default_seed = time(NULL);
    T = gsl_rng_default;
    r = gsl_rng_alloc(T);
    itemp=gsl_rng_uniform(r);   //want uniform random number from 0-1
    ans = ((de < 0.0) or (itemp < exp(-de/tmc))) ? true : false;
  }
  return ans;
}

