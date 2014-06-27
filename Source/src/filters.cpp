/*
  Comment
*/

#include "filters.h"

filter::filter(){
  name="NULL";
  init=false;
  filter_size=0;
  lambda=NULL;
  response=NULL;
  acc=NULL;
  spline=NULL;
}

filter::filter(string filtername, vector<double> band, vector<double> transmission){
  name="NULL";
  init=false;
  filter_size=0;
  lambda=NULL;
  response=NULL;
  acc=NULL;
  spline=NULL;
  load(filtername,band,transmission);
}

bool filter::load(string filtername, vector<double> band, vector<double> transmission){
  
  if(band.size() < 2){
    printf("Error: Not enough valid lines in filter file, filter not initialized\n");
    return false;
  }
  
  if(band.size() != transmission.size()){
    printf("Error: Filter file improperly formatted (different number of wavelength and response values), filter not initialized\n");
    return false;
  }
  
  init=true;
  name = filtername;
  filter_size = band.size();
  filter_limits[0] = band.front();
  filter_limits[1] = band.back();

  if(lambda != NULL)
    delete [] lambda;
  if(response != NULL)
    delete [] response;
  
  //convert vectors to arrays for gsl functions and storage
  //NORMALIZE
  lambda = new double[filter_size];
  response = new double[filter_size];    
  for(unsigned long i=0;i<filter_size;i++){
    lambda[i] = band[i];
    response[i] = transmission[i];
  }
    
  if(init){
    gsl_spline_free(spline);
    gsl_interp_accel_free(acc);
  }   

  acc = gsl_interp_accel_alloc();
  spline = gsl_spline_alloc(gsl_interp_cspline,filter_size);
  gsl_spline_init(spline,lambda,response,filter_size);
  
  return true;
}

double filter::transmission(double wavelength){
  if (init)
    if((wavelength < low()) or (wavelength > high()))
      return 0;
    else
      return gsl_spline_eval(spline,wavelength,acc);
  else{
    printf("ERROR: Filter Not Initialized\n");
    return -1;
  }
}

double filter::low(){
  if(init)
    return filter_limits[0];
  else
    return 0;
}

double filter::high(){
  if(init)
    return filter_limits[1];
  else
    return -1;
}

void filter::print(bool all){
  if(init){
    printf("%s: Range = %g - %g, Points = %lu\n",name.c_str(),filter_limits[0],filter_limits[1],filter_size);
    if(all){
      printf("Printing all filter data points\n\tWavelength\tResponse\n");
      for(unsigned long i=0;i<filter_size;i++)
	printf("\t%g\t%f\n",lambda[i],response[i]);
    }
  }
  else
    printf("ERROR: Filter not initialized\n");
}

filter::~filter(){
  if(init){
    gsl_spline_free(spline);
    gsl_interp_accel_free(acc);
  }
  if (lambda != NULL){
    delete [] lambda;
    delete [] response;
  }
}

filter_lib::filter_lib(){
  initialized = false;
}

filter_lib::filter_lib(string ffilename){
  if(load_library(ffilename)){
    initialized = true;
  }
  else{
    printf("ERROR: file \"%s\" not read successfully, library not initialized\n",ffilename.c_str());
    initialized = false;
  }
}

bool filter_lib::load_library(string ffilename){
  
  if(initialized){
    library.clear();
    initialized = false;
  }

  bool invalid_name = false;
  char buffer[256], name[256], info[256];
  double scale_exp, dlambda, dresponse;
  filter_info temp;
  int matchnum;
  int bad = 0;

  long linenumber=0;
  
  FILE *infile;
  infile = fopen(ffilename.c_str(),"r");
  if(infile != NULL){
    while(!feof(infile)){
      //read operations, line by line
      if(fgets(buffer,255,infile) != NULL){
	linenumber++;
	switch(buffer[0]){
	case EOF:
	case '\n':
	case '#':
	  break;
	case '>':
	  //if vectors not empty, finalize previous filter
	  if(temp.band.size() != 0){
	    library.push_back(temp);
	    temp.band.clear();
	    temp.transmission.clear();
	  }
	  
	  //new filter
	  if(invalid_name){
	    printf("Discarded %i lines after bad filter initializer\n",bad);
	    invalid_name = false;
	    bad = 0;
	  }
	  
	  scale_exp = 0;
	  matchnum = sscanf(buffer," > %s %lf %s", name, &scale_exp, info);	  
	  
	  switch(matchnum){
	  case 0:
	    printf("Warning: Invalid value detected in filter library file \"%s\" at line %li, discarding line\n", ffilename.c_str(),linenumber);
	    invalid_name = true;
	    break;
	  case 1:
	    temp.info = "";
	    temp.scale = 1;
	    break;
	  case 2:
	    temp.name = (string) name;
	    if(scale_exp != 0){
	      temp.scale = pow(10,scale_exp);
	      temp.info = "";
	    }
	    else{
	      temp.info = (string) info;
	      temp.scale = 1;
	    }
	    break;
	  default: //3 or more (could never be more)
	    temp.name = (string) name;
	    temp.scale = pow(10,scale_exp);
	    temp.info = (string) info;
	    break;
	  }
	  break;
	default:
	  if(invalid_name){
	    bad++;
	    break;
	  }
	  
	  matchnum = sscanf(buffer," %lf %lf",&dlambda,&dresponse);
	  if((dlambda < 0) or (matchnum != 2))
	    printf("Warning: Invalid value detected in filter library file \"%s\" at line %li, discarding line\n", ffilename.c_str(),linenumber);
	  else{
	    if(dresponse < 0)
	      dresponse = 0;
	    temp.band.push_back(dlambda*temp.scale);
	    temp.transmission.push_back(dresponse);
	  }
	}
      }
    }

    //finalize previous filter
    if(temp.band.size() != 0)
      library.push_back(temp);

    fclose(infile);
    initialized = true;
    return true;
  }
  else{
    printf("ERROR: Could not read filter library file \"%s\"\n",ffilename.c_str());
    return false;
  }
}

bool filter_lib::load_filter(short num, string fname){
  int found=-1;
  
  if(initialized){
    if ((num >=0) and (num <3)){
      for(int i=0;i<library.size();i++){
	if(library[i].name == fname){
	  found = i;
	  i = library.size();
	}
      }
      if (found != -1)
	return filters[num].load(fname,library[found].band, library[found].transmission);
      else
	printf("Error: Filter \"%s\" not found in library\n",fname.c_str());
    }
    else
      printf("Error: Invalid filter number %i\n",num);
  }
  else printf("Error: library not initialized\n");

  return false;
}

filter& filter_lib::get(short num){
  if ((num >=0) and (num <3)){
    return filters[num];
  }
  else{
    printf("ERROR: Invalid filter number (%i)\n",num);
    return dummy;
  }
}

void filter_lib::print(){
  printf("Number of Filters in Library: %lu\nFilter List:\n",library.size());
  for(unsigned long i=0;i<library.size();i++)
    printf("\t%16s\t%6.3lf\t%s\n",library[i].name.c_str(),library[i].scale,library[i].info.c_str());
}
