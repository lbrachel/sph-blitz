
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <math.h>


using namespace std;

int main (){

  // values for exact solution------------------------------------
  const double d=0.5; // L  the canal width
  const double eta=1; // (dynamical) viscosity
  const double rho=1; //density
  const double g=1.0;// value of gravity force (acts in positive x-direction) 
  const double n_max=1000;// number of summation terms taken into account for the series solution
  const double PI=3.141592653589793238462643;
  // END values for exact solution------------------------------------
  
  
  int n_col=11;// number of columns in inputfile (11 for SPLASH optimized output)
  double dy_exact=0.005; // dx for exact solution (exactxxxxxxxxxxxx.dat) for plotting
 
  //read filestamp from user
  cout<<"Please enter the file stamp (12 digits) for the file of which the exact result should be calculated:\n ";
  string timestamp;//intermediate variable for timestamp user input
  cin>>timestamp;

  //read timestamp resolution from user
  double timestamResolution;
  int timestampCode=0;
  while(timestampCode!=1&&timestampCode!=2) { 
    cout<<"Please enter the selected timestamp resolution for the simulation:\n ";
    cout<<"Enter 1 for standard resolution (1e-6)\n ";
    cout<<"Enter 2 for high resolution (1e-10)\n ";
    cin>>timestampCode;
    if(timestampCode!=1&&timestampCode!=2)
      cout<<"you should enter eitehr 1 or 2!"<<endl;
  }
  double timestampResolution;
  if(timestampCode==1)
    timestampResolution=1e-6;
  else
    timestampResolution=1e-10;
  // read centerline coordinate from user
  // is needed to shift domain in y direction in a way that centerline is at zero
  double y_centerline;
  cout<<"enter y coordinate of channel center line. Channel width has to be 1!"<<endl;
  cin>>y_centerline;
   
  // string containing input-file name
  const string inputfile = "../../src/outdata/prtl" + timestamp + ".dat";

  // convert timestamp-string into an integer using a stringstream
    stringstream ss(timestamp); //define stringstream
  int time;// time in timestamp unit
  double t;// time in s
  ss >> time;// stream stringstream into int variable 
  cout << "time is (int) in timestamp unit " << time << "\n";
  t=time*timestampResolution;//convert time from ms to s
  cout << "time is  in s " << t << "\n";


  // ------------read input file----------------------

  string garbage; // string for first 2 lines of input file

  // data structure for an image of file content
  vector <vector <double> > simulationData;//data structure for an image of file content
  
  
  // read inputfile and save content
  // (without boundary particles, their ID is 0) in simulationData
  // (maintaining the file-structure) 
  ifstream fin(inputfile.c_str(), ios::in);
  if (!fin){
    cout<<"Initialtion: Cannot open "<< inputfile <<"! Restart program and try different file name \n";
    exit(1);
  }
  else cout<<"Starting the extraction of the file content "<< inputfile <<" \n"; 

  vector<double> row(n_col,0);// vector to save a whole row
  // throw away first 2 lines of input file (contain no data, only text)
  getline(fin,garbage);
  getline(fin,garbage);
  // first real particle row indicator 
  // only purpose is to identify 1st row of real particles (NO boundary particles) 
   // read rest of file
  while(!fin.eof()){
    // read one row (problem reads one row more than eof)
    for (int i=0; i<n_col;i++){
      fin>> row[i];
    }
       
    // append every row to 2D array (matrix) simulationData
    // if this row does not correspond to a boundary particle for which ID=0
    // AND if this row's x is the same as first particles( rows) x.
    // To be sure the last row (which is already eof and does
    // not correspond to a physical row of the file any more)
    // is not added: set last row's ID=0;
    // file structure: x, y, Ux, Uy, rho, e, p, h, m, V, ID 
    if(fin.eof()) {
      row[n_col-1]=0;
    }
    // if the row does not correspond to a boundary particle
    // (for which ID=0)
    // AND if no particle with the same y-value has been saved yet*
    // add it to the matrix vector
    //*: the particle quantities are perfectly symmetrical in x direction,
    // therefore only one particle per y value needs to be taken into account
    
    if(row[n_col-1]!=0) {
      //test if same y value has already been stored before and if no,
      // add it to simulationData-structure
      int y_already_stored=0;
      for (int i=0; i<simulationData.size();i++) {
	if(simulationData[i][1]==row[1]) {
	  y_already_stored=1;
	  i= simulationData.size();//end loop
	}
      }
      if(y_already_stored==0)
	simulationData.push_back(row);
    }
  }  
  fin.close();// close file
  cout<<endl<<"file read!"<<endl;
  
  // calculate exact solution at all particle positions (needed for norm calculation)
  // exact solution is: analytical solution for 2 semi infinite bodies
  // (taken from Cleary1999, NO from )
  //NOTE: v_x is denoted U in the following

  vector <double> one_y_U_couple(2,0);
  // vector for all (y,U) couples
  vector <vector <double> > all_y_U_couples;

  //calculate steady state velocity on centerline according to Sigalotti2003
  const double U_0=g*rho/(2*eta)*pow(d,2);

  for (int i=0;i<simulationData.size();i++) {
    const double y= simulationData[i][1]-y_centerline;// extract y (is more handy for the rest)AND shift it by y_centerline, as for theroetical solution y_centerline=0;!

    //calculate solution at desired time
    double U;

    //first sum up all terms of the series
    double U_sum=0;
    for (int n=0;n<=n_max;n++) {
      U_sum+=16*pow(-1,n)*pow(d,2)*g/(eta/rho*pow(PI,3)* pow(2*n+1,3))*cos(((2*n+1)*PI*y)/(2*d))
     	*exp(-(pow(2*n+1,2)*pow(PI,2)*eta*t)/(4*pow(d,2)*rho));
    }
    U=-1*(g*rho/(2*eta)*(pow(y,2)-pow(d,2))+U_sum);
    one_y_U_couple[0]=y;
    one_y_U_couple[1]=U;
    // append current y, U couple to all_y_U_couples 2D array
    all_y_U_couples.push_back(one_y_U_couple);
  }
  cout<<endl<<"exact results calculated"<<endl;
  // L norm calculation
  
  // variables for norms
  double L_1Norm=0;
  double L_2Norm=0;
  double L_infNorm=0;
  
  int N_part=0;//number of particles
  
  //actual norm calculation
  
  // iterate all rows of simulation data matrix(=file)
  for(int i=0;i<simulationData.size();i++) {
    // calculate the point's contribution to the norm
    N_part++;// increment number of particles in the domain
    // calculate delta between exact and simu
    const double delta_U=fabs( all_y_U_couples[i][1]-simulationData[i][2]);
    // calculate L_1 norm contribution (add contribution to norm)
    L_1Norm=L_1Norm+delta_U;
    // calculate L2 norm contributions (add contribution to norm)
    L_2Norm=L_2Norm+pow(delta_U,2);
    
    // calculate Linf norm (=maximum value)
    if(delta_U>L_infNorm)
      L_infNorm=delta_U;
  }
  //divide final results (except for L_inf norm) by the number of particles in the domain
  L_1Norm=L_1Norm/N_part;
  L_2Norm=L_2Norm/N_part;
  
  //for L_2 norm: sqrt (according to definition)
  L_2Norm=sqrt(L_2Norm);
  
  // and finally normalize them with characteristic value which is 
  // U_0 (steady state velocity at centerline)
  L_1Norm=L_1Norm/U_0;
  L_2Norm=L_2Norm/U_0;
  L_infNorm= L_infNorm/U_0;
  
  cout<<endl<<"norms calculated"<<endl;  
  
  // output the results in files
  
  // output file: global errors (norms)
  
  // assemble filename
  const string filename2 = "../../results/ResultsInProgress/ErrorNorms" + timestamp + ".dat";
  
  // create and open outputfile
  ofstream out4(filename2.c_str());
  if (!out4){
    cout<<"Initialtion: Cannot create "<<filename2 <<"! \n";
    exit(1);
  }
  else cout<<"\n writing norm error data into "<< filename2 <<" \n"; 
  
  // write file header
  out4<<"norm errors\n\n";
  out4<<" L1  |  L2  |  L_oo"<<endl<<endl;
  
  out4<<setprecision(6)
      << L_1Norm<<" "<< L_2Norm<<" "<< L_infNorm <<endl<<endl;
    
  // close output file
  out4.close();
  
  cout<<"\n norm error file successfully written \n";
  
 const string filename3 = "../../results/ResultsInProgress/simuForPlot" +timestamp + ".dat";
 // besides output simulation data (only one particle row) in a file for plotting
  ofstream out3(filename3.c_str());
  if (!out3){
    cout<<"Initialtion: Cannot create "<<filename3 <<"! \n";
    exit(1);
  }
  else cout<<"\n writing simu (y,v_x) data into "<< filename3 <<" \n"; 
  
  // write file header
  out3<<"extracted simu data (only one row of particles\n";
  out3<<"file structure:\n";
  out3<<"  y  |  U (is v_x)  "<<endl<<endl;
  for (int k=0;k< simulationData.size();k++) {
  out3<<setprecision(6)
      << simulationData[k][1]-y_centerline <<" "<< simulationData[k][2]<<" "<<"\n";
  }
  // close output file
  out3.close();
  
  cout<<"\n file with extracted simu data (y,v_x) for plotting successfully written \n";
  
  
  // besides output file with exact solution which can be used for plotting purposes
  // assemble output file name 
  const string outputfile = "../../results/ResultsInProgress/exact" +timestamp + ".dat";
  
  // create and open outputfile
  ofstream out(outputfile.c_str());
  if (!out){
    cout<<"Initialtion: Cannot create "<<outputfile <<"! \n";
    exit(1);
  }
  else cout<<"\n writing the exact solution into "<< outputfile <<" \n"; 
  
  //write data into file (2 columns)
  // file structur:  y, U_exact
  for(double y_exact=-0.5;y_exact<0.5+dy_exact;y_exact=y_exact+dy_exact) {
    //again calculate exact solution
    double U_exact;  
    //first sum up all terms of the series
    double U_sum=0;
    for (int n=0;n<=n_max;n++) {
      U_sum+=16*pow(-1,n)*pow(d,2)*g/(eta/rho*pow(PI,3)* pow(2*n+1,3))*cos(((2*n+1)*PI*y_exact)/(2*d))
     	*exp(-(pow(2*n+1,2)*pow(PI,2)*eta*t)/(4*pow(d,2)*rho));
    }
    U_exact=-1*(g*rho/(2*eta)*(pow(y_exact,2)-pow(d,2))+U_sum);
    
    out<<setprecision(9)
       << ::setw(17)<<y_exact// output x value 
       << ::setw(17)<<U_exact //output corresponding T-value
       <<"\n";
  }
  out.close();// close outputfile
  cout<<"\n file successfully written \n";// user output
  
  return 0;
}