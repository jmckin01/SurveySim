#include "obs_lib.h"
#include "sed_lib.h"
#include "lumfunct.h"
#include "hist_lib.h"

#define LUMPARS 7

//Storage structure for each individual source
struct sprop{
  //source properties (as determined by distributions)
  double redshift;
  double luminosity;
  double weight;
  double fluxes[3];
  double c1,c2;
  sprop();
  sprop(double z,double f[],double lum, double w); //constructor initializes variables
  //as well as automatically computes colors if it is a valid operation
  friend ostream &operator<<(ostream &out,sprop c);
};

struct products{
  double chisqr;
  valarray<double> dndz;
  valarray<double> dnds[3];
  products(int nz, int ns[]);
};

struct clementsBins{
  //temporary structure created to compute clements'10 style number counts
  int bnum[3]={16,13,10};
  //bin minima [mJy]
  valarray<double> b250 ({34.0,38.0,42.0,47.0,52.0,56.0,63.0,73.0,83.0,93.0,125.0,175.0,250.0,350.0,450.0,800.0},16);
  valarray<double> b350 ({38.0,41.0,46.0,50.0,55.0,61.0,71.0,82.0,91.0,125.0,175.0,250.0,700.0},13);
  valarray<double> b500 ({48.0,52.0,56.0,60.0,63.0,70.0,82.0,95.0,150.0,250.0},10);
  //bin widths [Jy]
  valarray<double> db250 ({0.004,0.004,0.005,0.005,0.004,0.007,0.01,0.01,0.01,0.032,0.05,0.075,0.1,0.1,0.35,0.5},16);
  valarray<double> db350 ({0.003,0.005,0.004,0.005,0.006,0.01,0.011,0.009,0.034,0.05,0.075,0.45,0.5},13);
  valarray<double> db500 ({0.004,0.004,0.004,0.003,0.007,0.012,0.013,0.055,0.1,0.25},10);
  //bin minima [Jy]^2.5
  valarray<double> S250 ({0.000213156,0.000281487,0.000361512,0.0004789,0.000616607,0.000742113,0.000996211,0.00143982,0.0019847,0.00263759,0.00552427,0.0128114,0.03125,0.072472,0.135841,0.572433},16);
  valarray<double> S350 ({0.000281487,0.000340377,0.000453831,0.000559017,0.000709425,0.000919019,0.00134322,0.00192546,0.00249806,0.00552427,0.0128114,0.03125,0.409963},13);
  valarray<double> S500 ({0.000504781,0.000616607,0.000742113,0.000881816,0.000996211,0.00129642,0.00192546,0.00278169,0.00871421,0.03125},10);
}

class simulator{
 private:
  products last_output;
  clementsBins dndsInfo;
  int binNum(int band, double flux);
  vector<sprop> sources;
  //bool simulated;
  lumfunct *lf;
  sed_lib *seds;
  obs_lib *observations;
  hist_lib * diagnostic;
  double bands[3];
  double band_errs[3];
  double flux_limits[3];
  double color_exp;
  double area;
  double dz;
  double zmin;
  int nz;
  int ns;
 public:
  simulator(){
    last_output.chisqr=0;
    color_exp=0; //default to no color evolution
    area = pow((M_PI/180.0),2.0); //default to 1sq degree
    zmin = 0.1; //default to 0.1-6.0, 0.1 steps
    nz = 59;
    dz = 0.1;}
  simulator(double b[],double b_err[],double f_lims[],string obsfile,string sedfile);
  void set_bands(double b[],double b_err[],double f_lims[]);
  void set_size(double area,double dz,double zmin,int nz,int ns);
  void set_color_exp(double val);
  void set_lumfunct(lumfunct *lf);
  void set_sed_lib(string sedfile);
  void set_obs(string obsfile);
  void reset();
  products simulate();
  double model_chisq() { return last_output.chisqr; }
  bool save(string outfile);
  ~simulator();
};
