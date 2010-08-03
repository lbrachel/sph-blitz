

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

  const int n_col=8;// number of columns in smulation output file
  double buffer;
  int counter=0;
  vector<double> row(8,0);
  vector< vector<double> > matrix;

  // in the first part the particles ensembles with same x coordinate
  // are formed and their IDs stored

  // reading the sorted initial file sorted00000000.dat
  // create and open inputfile
  ifstream fin("../../results/ResultsInProgress/sorted00000000.dat", ios::in);
  // test if inputfile opended
  if (!fin){
    cout<<"Cannot open sorted00000000.dat \n" ;
    exit(1);
  }
  else cout<<"\n reading sorted00000000.dat and forming particle emsembles for later averaging \n"; 
  
  // while reading directls sort out the boundary particles
  while(!fin.eof()){
    for (int i=0; i<n_col&&!fin.eof();i++){
      // problem: reads always one more than eof
      fin>>buffer;
      row[i]=buffer; 
    }
    // to be sure the last row (which is already eof and does
    // not correspond to a physical row of the file any more)
    // is not added make ID=0;
    if(fin.eof()){
      row[6]=0;
    }
    // if the row does not correspond to a boundary particle
    // (for which ID=0) add it to the matrix vector
    if(row[n_col-2]!=0){
      matrix.push_back(row);
    }
  }  
  fin.close();// close inputfile
  
  // build the ID ensembles of all particles having the same x 
  // (the allgorithm takes advantage of the fact that the x-values are
  // allready sorted (by the shell script))
  
  // vector containing all ID couples (here, couple can also be more than 2)
  vector < vector <int> > allID_Couples;
  int i=0;
  // loop over all rows
  while(i<matrix.size()-1){
    // local vector variable of type int which will contain all IDs of
    // particles with the same x (already containing the ID of the
    // current particle
    vector<int>ID_Couple(1, matrix[i][6]);
    // as long as the next x coordinate (first value of next row) is the
    // same, add ID of the corresponding particle to ID_Couple
    while(matrix[i+1][0]==matrix[i][0]){
      ID_Couple.push_back(matrix[i+1][6]);
      // increment iterator if not yet at the end of file
      if(i<matrix.size()-1)
	i++;
      // to avoid segmentation fault when verifying condition
      // in while loop (matrix[i+1]....)
      if(i==matrix.size()-1)
	break;
    }
    // add the ID_Couple to teh variable allID_Couples
    allID_Couples.push_back(ID_Couple);
    // increment iterator if not yet at the end of file
    if(i<matrix.size()-1)
      i++;
  }

  // now read the actual simulation results file which has to be averaged

  // initial number of real particles (corresponds to number of rows in file(=matrix))
  const int initPartNr=matrix.size(); 
  // content of matrix not needed any more, 
  // matrix prepared for reading of actual simulation results file
  matrix.clear();
  // variables for file name
  string inputfile;
  string path("../../src/outdata/");
  string input;
  // read file name from user
  cout<<"Please enter the name of the input file to process (prtlxxxxxxxx.dat) (without path): ";
  cin>> inputfile;  
  // assemble file name (including path)
  input=path + inputfile;
  // reading the file which has to be post processed
  // create and open file
  ifstream fin2(input.c_str(), ios::in);
  
  // test if file opened
  if (!fin2){
    cout<<"Cannot open "<<input<<" \n" ;
    exit(1);
  }
  else cout<<"\n reading "<<input<<" and deleting boundary particle data \n"; 

  // while reading directls sort out the boundary particles

  // very "elegant" way of cutting off first to lines of file
  string garbage;
  getline(fin2,garbage);
  getline(fin2,garbage);
  // read rest of file (actual data)
  while(!fin2.eof()){
    for (int i=0; i<n_col&&!fin2.eof();i++){ 
      //same problem about reading one line too much as above
      fin2>>buffer;
      row[i]=buffer; 
    }
    // to be sure the last row (which is already eof and does not correspond
    // to a physical row of the file any more) is not added make ID=0;
    if(fin2.eof()){
      row[6]=0;
    }
    // if the row does not correspond to a boundary particle 
    // (for which ID=0) add it to the matrix vector
    if(row[n_col-2]!=0){
      matrix.push_back(row);
    }
  }  
  fin2.close();// close file
  
  // test if number of real particles is the same for data read from 
  // file at t=0 and at t>0
  cout<<"Number of real particles in initial file (at t=0): "<<initPartNr<<"      Number of real particles in current file: "<<matrix.size();
  // test if real particle number is the same as initially
  if(matrix.size()!=initPartNr){
    cout<<"\n \n there is something wrong with the data files: the number of real particles in the file at t=0 is different from the number of real particles in the file"<<inputfile<<"\n \n";
      exit(1);
  }
  else cout<<"  ... OK\n";
  
  // now: averaging all particle rows which have matching IDs
  
  // matrix (lines, rows) in which final results are stored for output to file
  vector < vector <double> > final_matrix;
  // loop over all ID couples
  for (int i=0;i<allID_Couples.size();i++){
    // matrix to temporarily store all rows with matching iDs
    vector <vector<double> > rowsWithMatchingIDs;
    // loop over all IDs of one ID couple
    for (int j=0;j<allID_Couples[i].size();j++){
      // save each ID in temporary variable
      int ID=allID_Couples[i][j];
      // search for row in matrix (which is our input file) which fits ID
      for(int k=0;k<matrix.size();k++){
	if(matrix[k][6]==ID){
	  // if ID fits add row to temporary matrix
	  rowsWithMatchingIDs.push_back(matrix[k]);
	  break;// no need to continue search as only one row (one particle)for each ID
	}
      }
    }
    // once all rows for one ID couple are found:
    // add the corresponding components and finally divide them by 
    // the number of rows to obtain the mean value of each component
    for(int l=0;l<rowsWithMatchingIDs.size()-1;l++){
      for(int m=0;m<n_col;m++){
	rowsWithMatchingIDs[0][m]=rowsWithMatchingIDs[0][m]+rowsWithMatchingIDs[l+1][m];
      }
    }
    // divide summed values by number of particles to finally obtain mean value
    for(int m=0;m<n_col;m++){
      rowsWithMatchingIDs[0][m]=rowsWithMatchingIDs[0][m]/rowsWithMatchingIDs.size();
    }
    // add averaged row to final matrix
    final_matrix.push_back(rowsWithMatchingIDs[0]);
  }
  
  // assemble output file name ...averagedxxxxxxxx.dat (where xxxxxxxx is the time)
  size_t length;
  char timeString[20];
  char  outputfile[51];
  length=inputfile.copy(timeString,8,inputfile.size()-12);
  timeString[length]='\0';
  strcat(outputfile,"../../results/ResultsInProgress/");
  strcat(outputfile, "averaged");
  strcat(outputfile, timeString);
  strcat(outputfile, ".dat");
  // create and open output file
  ofstream out(outputfile);
  // test if file opened
  if (!out){
    cout<<"Cannot create file "<<outputfile<<"\n" ;
    exit(1);
  }
  else cout<<"\n writing in file\n"; 
  // write file header
  out<<"averaged values\n";
  out<<"variables=x, y, rho, p, U, e, '-1'(just as a placeholder to maintain file structure(was ID)), m ";
  // write data into file
  for(int i=0;i<final_matrix.size();i++){
    out<<setprecision(9)
       << ::setw(17)<<final_matrix[i][0] //x
       << ::setw(17)<<final_matrix[i][1] //y
       << ::setw(17)<<final_matrix[i][2] //rho
       << ::setw(17)<<final_matrix[i][3] //p
       << ::setw(17)<<final_matrix[i][4] //U
       << ::setw(17)<<final_matrix[i][5] //e
       << ::setw(17)<<"-1"//(an averaged ID would not make any sense (entry "-1" just to maintain the file structure with 8 columns) 
       << ::setw(17)<<final_matrix[i][7] //m 
       <<"\n";
  }
  out.close();// close output file
  cout<<"\n averaged values written to file:\n";
  cout<<"   "<<outputfile<<"\n";
  cout<<"\n program ended\n";
  return 0;
}
