//#include "obs_lib.h"
#include "model_lib.h"
#include "lumfunct.h"
#include "hist_lib.h"

//Storage structure for each individual source
struct sprop{
  //source properties (as determined by distributions)
  int mod_num;
  double redshift;
  double luminosity;
  double bands[3];
  double fluxes[3];
  double c1,c2;
  sprop();
  sprop(int m,double z,double b[],double f[],double lum); //constructor initializes variables
  //as well as automatically computes colors if it is a valid operation
  friend ostream &operator<<(ostream &out,sprop c);
};

class simulator{
 private:
  double chisq;
  vector<sprop> sources;
  bool simulated;
  lumfunct *lf;
  sed_lib *seds;
  obs_lib *observations;
  hist_lib * diagnostic;
  double bands[3];
  double band_errs[3];
  double flux_limits[3];
  double distribution_size;
  void init_rand(); //Initializes GNU random number generator (see functions.h)
 public:
  simulator() {chisq=0;}
  simulator(double b[],double b_err[],double f_lims[],string obsfile,string sedfile);
  void set_bands(double b[],double b_err[],double f_lims[]);
  void set_lumfunct(lumfunct *lf, double area, int nz,double dz);
  void set_sed_lib(string sedfile);
  void set_obs(string obsfile);
  void reset();
  double simulate(double area, int nz, double dz);
  double model_chisq() { return chisq; }
  bool save(string outfile);
  ~simulator();
};

struct mparam{
  int p_id;
  double mean;
  double sigma;
  double min;
  double max;
};

struct fixed_params{
  simulator * sim;
  int npar;
  int znum;
  mparam *p;
};

double simulate(const gsl_vector *v,void *params);
