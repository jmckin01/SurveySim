// -*-c++-*-

#ifndef AGN_FRAC_H
#define AGN_FRAC_H

#include <math.h>
#include "lumfunct.h"
#include "functions.h"

class agn_frac{
 private:
  lumfunct *lf;
  int _types;
  bool generate;
  bool hasComposites;
  bool hasCold;
  RandomNumberGenerator rng;
  map <tuple<double,double>,double> values;
  map <double,double> lumPower;
  map <double,double> evolZ;
  double _t1;
  double _t2;
  double _fagn0;
  double _zbt;
  double _compFrac;
  double _coldFrac;
  double _agnPower;
 public:
  agn_frac(int agn_types);
  void set_lumfunct(lumfunct *lf);
  void set_params(double lpars[]);
  void set_t1(double t1);
  void set_t2(double t2);
  void set_fagn0(double fagn0);
  void set_zbt(double zbt);
  void set_fComp(double fComp);
  void set_fCold(double fCold);
  void set_agnPower(double agnPower);
  double get_t1(){
    return _t1;
  }
  double get_t2(){
    return _t2;
  }
  double get_fagn0(){
    return _fagn0;
  }
  double get_zbt(){
    return _zbt;
  }
  double get_fComp(){
    return _compFrac;
  }
  double get_fCold(){
    return _coldFrac;
  }
  double get_agnPower(){
    return _agnPower;
  }
  double get_agn_frac(double lum, double redshift);
  int get_sedtype(double lum, double redshift);
};


#endif
