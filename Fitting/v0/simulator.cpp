#include "simulator.h"
#include "functions.h"
#include "constants.h"
#include "cosmo.h"

// find the location of the minimum value
int findMinloc(double vals[], int MAXELS)
{
  int i;
  double min = vals[0];
  int hit=0;

    for (i = 0; i<MAXELS; i++)
    if (min >= vals[i])
      {
	hit=i;
	min = vals[i];
      }
    return hit;
}

sprop::sprop(double z,double f[],double lum, double w){
  redshift = z;
  luminosity = lum;
  weight = w;

  bool do_colors(true);
  for (int i=0;i<3;i++){
    fluxes[i] = f[i];
    if(f[i] < 0.00000000E0)
      do_colors = false;
  }
  if(do_colors){
    c1 = get_color(f[2],f[0]);
    c2 = get_color(f[2],f[1]);
  }
  else{
    c1 = -99.0;
    c2 = -99.0;
  }
}

sprop::sprop(){
  redshift = -1;
  luminosity = -1;
  weight = 0;

  c1 = -99.0;
  c2 = -99.0;
}

products::products(){
  dndz.resize(10);
  dnds.resize(8);
}

products::products(int nz, int ns){
  dndz.resize(nz);
  dnds.resize(ns);
}

void simulator::init_rand(){
  gsl_rng * r;
  const gsl_rng_type * T;
  gsl_rng_default_seed = time(NULL);
  T = gsl_rng_default;
  r = gsl_rng_alloc(T);
}

simulator::simulator(double b[],double b_err[],double f_lims[],string obsfile,string sedfile){

  for (int i=0;i<3;i++){
    bands[i] = b[i];
    band_errs[i] = b_err[i];
    flux_limits[i]= f_lims[i];
  }
  
  observations = new obs_lib(obsfile);
  seds = new sed_lib(sedfile);

  //initialize observation portion of histograms
  //possibility here to integrate obs_lib into hist_lib
  diagnostic = new hist_lib();
  double *c1,*c2;
  int osize = observations->get_snum();
  observations->get_all_colors(c1,c2);
  diagnostic->init_obs(c1,c2,osize);
}

void simulator::set_bands(double b[],double b_err[],double f_lims[]){
  for (int i=0;i<3;i++){
    bands[i] = b[i];
    band_errs[i] = b_err[i];
    flux_limits[i]= f_lims[i];
  }
}

void simulator::set_lumfunct(lumfunct *lf){
  if(lf != NULL){
    this->lf = lf;
  }
  else
    cout << "ERROR: NULL Pointer Passed to Simulator" << endl;
}


void simulator::set_sed_lib(string sedfile){
  seds = new sed_lib(sedfile);
}


void simulator::set_obs(string obsfile){
  if(observations != NULL)
    delete observations;
  observations = new obs_lib(obsfile);
  double *c1,*c2;
  int osize = observations->get_snum();
  observations->get_all_colors(c1,c2);
  diagnostic->init_obs(c1,c2,osize);
}

products simulator::simulate(double area, int nz, double dz, double zmin, int ns){
  sources.clear();
  products output(nz,ns);

  if(seds != NULL){

    static int is,js,jsmin;
    static double tmpz,vol;

    int lnum = seds->get_lnum();
    double dl = seds->get_dl();
    static double sf;
    static long nsrcs;
    double lums[lnum];
    double zarray[nz],weights[nz];    
    
    seds->get_lums(lums);
    
    //=========================================================================
    // for each L-z depending on the number of sources, sample the SED and get the appropriate fluxes
    //*************************************************************************
    
    //previous approach was round to nearest template value.
    // We interpolate here, in future will convolve filter
 
    static double b_rest[3],flux_sim[3];
    static double noise[3];
    static sprop *temp_src;
    static int src_iter;
    static bool detected = true;

    //just temporary, complains if unused variables
    output.dnds[0]=0;

    gsl_rng * r;
    const gsl_rng_type * T;    
    gsl_rng_default_seed = time(NULL);
    T = gsl_rng_default;
    r = gsl_rng_alloc(T);    

    //NOTE templates are given in W/Hz
    for (is=0;is<nz;is++){
      zarray[is]=(is+1)*dz+zmin;
      output.dndz[is]=0;

      tmpz=zarray[is]+dz/2.0;
      vol=(dvdz(tmpz,area)*dz);
      weights[is] = 1.e0; //still here in case scaling must be done

      b_rest[0]=bands[0]/(1.0+zarray[is]);
      b_rest[1]=bands[1]/(1.0+zarray[is]);
      b_rest[2]=bands[2]/(1.0+zarray[is]);

      //denominator of conversion from luminosity to flux, precompute for speed
      sf = 4.0*M_PI*pow(lumdist(zarray[is])*MPC_TO_METER,2.0); 
      
      jsmin = 0;
      for (js=0;js<lnum;js++){
	flux_sim[0] = seds->get_flux(lums[js],b_rest[0]);
	flux_sim[0] *= (1+zarray[is])*Wm2Hz_TO_mJy/sf;
	if(flux_sim[0]>=flux_limits[0]){
	  jsmin = (js > 0) ? (js-1) : js; //js-1 to allow for noise
	  js = lnum; //break out of loop
	};
      };
      //printf("Z: %f, Lmin: %f\n",zarray[is],lums[jsmin]);
      
      for (js=jsmin;js<lnum;js++){
	//source number is dn/(dldv)*Dv*Dl
	nsrcs = long(dl*vol*lf->get_nsrcs(zarray[is],lums[js]));
	for (src_iter=0;src_iter<nsrcs;src_iter++){
	  detected = true;
	  for (int i=0;i<3;i++){
	    noise[i]=gsl_ran_gaussian(r,band_errs[i]);
	    flux_sim[i] = seds->get_flux(lums[js],b_rest[i]);
	    flux_sim[i] *= (1+zarray[is])*Wm2Hz_TO_mJy/sf;
	    flux_sim[i] += noise[i];
	    if (flux_sim[i] < flux_limits[i]) //reject sources below flux limit
	      detected = false;
	  }
	  
	  //check for detectability, if "Yes" add to list
	  if(detected){
	    temp_src = new sprop(zarray[is],flux_sim,lums[js],weights[is]);
	    sources.push_back(*temp_src);
	    output.dndz[is]+=1; 
	    delete temp_src;
	  }
	}
      }
    }
     
    //=========================================================================
    // generate diagnostic color-color plots
    //*************************************************************************
    
    double *c1,*c2,*w;
    
    int snum(sources.size());
    c1 = new double[snum];
    c2 = new double[snum];
    w = new double[snum];
    for (int i=0;i<snum;i++){
      c1[i] = sources[i].c1;
      c2[i] = sources[i].c2;
      w[i] = sources[i].weight;
    }
    
    diagnostic->init_model(c1,c2,w,snum);
    output.chisqr=diagnostic->get_chisq();

    delete[] c1;
    delete[] c2;
    delete[] w;
    
    return output;
  }
  else{
    cout << "ERROR: NULL Model Library" << endl;
    //doesn't work now as the output is of type "products" and "-1" is integer
    return output;
  }
}


bool simulator::save(string outfile){
  bool opened =  diagnostic->write_fits(outfile);
  
  if(not opened)
    return opened;
  
  using namespace CCfits;
  std::auto_ptr<FITS> pFits(0);

  try{
    pFits.reset(new FITS(outfile,Write));
  }
  catch (CCfits::FITS::CantOpen){
    cerr << "Can't open " << outfile << endl;
    opened = false;       
  }
  
  double lpars[6];
  lf->get_params(lpars);
  
  pFits->pHDU().addKey("PHI0",lpars[0],"Normalization"); 
  pFits->pHDU().addKey("L0",lpars[1],"Knee location z=0"); 
  pFits->pHDU().addKey("ALPHA",lpars[2],"upper slope"); 
  pFits->pHDU().addKey("BETA",lpars[3],"lower slope"); 
  pFits->pHDU().addKey("P",lpars[4],"Norm evolution"); 
  pFits->pHDU().addKey("Q",lpars[5],"Knee evolution"); 

  unsigned long size = sources.size();
  printf("%s %lu\n","Soures Being Saved: ",size);
  int pnum = 2;
  valarray<double> f1(size),f2(size),f3(size),luminosity(size);
  valarray<double> redshift(size);
  valarray<double> param(size);
  valarray<double> params[pnum];
  for (int i=0;i<pnum;i++)
    params[i].resize(size);
  
  for(unsigned long i=0;i<size;i++){
    f1[i] = sources[i].fluxes[0];
    f2[i] = sources[i].fluxes[1];
    f3[i] = sources[i].fluxes[2];
    param[i] = 1.0; //placeholder for now
    redshift[i] = sources[i].redshift;
    luminosity[i] = sources[i].luminosity;
    for(int j=0;j<pnum;j++)
      params[j][i] = 1.0; //also placeholder for now
  }
  
  static std::vector<string> colname((6+pnum),"");
  static std::vector<string> colunit((6+pnum),"");
  static std::vector<string> colform((6+pnum),"f13.8");
  
  static string pnames[] = {"P0","P1","P2","P3","P4"};
  
  colname[0] = "F1";
  colname[1] = "F2";
  colname[2] = "F3";
  colname[3] = "Z";
  colname[4] = "M";
  colname[5] = "Lum";
  
  colunit[0] = "Jy";
  colunit[1] = "Jy";
  colunit[2] = "Jy";
  colunit[3] = "-";
  colunit[4] = "-";
  colunit[5] = "W/Hz";
  
  colform[5] = "e13.5";
  
  for(int i=6;i<6+pnum;i++){
    colname[i] = pnames[i-6];
    colunit[i] = "-";
  }
  
  static string hname("Parameter Distributions");
  Table *newTable= pFits->addTable(hname,size,colname,colform,colunit,AsciiTbl);
  
  newTable->column(colname[0]).write(f1,1);
  newTable->column(colname[1]).write(f2,1);
  newTable->column(colname[2]).write(f3,1);
  newTable->column(colname[3]).write(redshift,1);
  newTable->column(colname[4]).write(param,1);
  newTable->column(colname[5]).write(luminosity,1);
  for (int i=6;i<6+pnum;i++)
    newTable->column(colname[i]).write(params[i-6],1);
  
  return true;
}

simulator::~simulator(){
  if(observations != NULL)
    delete observations;
  if(seds != NULL)
    delete seds;  
  if(diagnostic != NULL)
    delete diagnostic;
}

void simulator::reset(){
  sources.clear();
}
