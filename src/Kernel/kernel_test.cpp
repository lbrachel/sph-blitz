/// Kernels  test program 
#include "Kernel/quinticspline.h"
#include "Kernel/cubicspline.h"
#include "Kernel/harmonic.h"
#include <boost/test/minimal.hpp>
#include <blitz/vector.h>
#include <blitz/array.h>
#include <numeric>
#include <iostream>
#include <fstream>

void test_kernel(const Kernel& weight_function , const double supportlength) {

  const double eps  = 5e-3;
  const double pi = 3.141592653589793;
 
  // create an array with kernel values
 const int numSamples = 2000;  // Number of samples
 const double dx = supportlength / numSamples; // step 
 blitz::firstIndex i;
 blitz::Array<double, 1> x(numSamples);
 x = dx * i;
 /// output kernel and derivative
 blitz::Array<double, 1> w(weight_function.w(x));
 blitz::Array<double, 1> dw(weight_function.F(x));
 std::ofstream wOut;
 std::ofstream dwOut;
 wOut.open("w.dat", std::ios::out);
 dwOut.open("dw.dat", std::ios::out);
 for (int i=0; i<numSamples; i++) {
   wOut << w(i) << '\n';
   dwOut << dw(i) << '\n';
 }
 wOut.close();
 dwOut.close();
 std::cerr << "output kernel to w.dat\n" ;
 std::cerr << "output derivative  to dw.dat\n" ;

 // integrate kernel 
 w = blitz::Array<double, 1>(weight_function.w(x)*x);
 double s =  std::accumulate(w.begin(), w.end(), 0.0);
 s = s - 0.5*w(0) - 0.5*w(numSamples);
 s = 2.0 * pi * dx * s;
 BOOST_REQUIRE( abs(s - 1.0) < eps );

 // integrate the derivative of the kernel
 w = blitz::Array<double, 1>(weight_function.F(x)*x*x);
 s =  std::accumulate(w.begin(), w.end(), 0.0);
 s = s - 0.5*w(0) - 0.5*w(numSamples);
 s = pi * dx * s;
 std::cerr << "s = " << s << std::endl;
 BOOST_REQUIRE( abs(s - 1.0) < eps );

 // cumulative sum of the derivatives
 blitz::Array<double, 1> cumsum(numSamples);
 dw = blitz::Array<double, 1>(weight_function.F(x));
 w = blitz::Array<double, 1>(weight_function.w(x));
 cumsum = 0.0;
 s = 0.0;
 for (int idx=0; idx<numSamples; idx++) {
   cumsum(numSamples-idx-1) = s * dx; 
   s += dw(numSamples - idx - 1);
 }
 w = blitz::Array<double, 1>(weight_function.w(x));
 for (int idx=0; idx<numSamples; idx++) {
   BOOST_REQUIRE( abs(cumsum(idx) - w(idx)) < eps*w(0.0) );
 }
  /// check integral of the kernel function
  /// use Trapezoidal Rule
}

int test_main( int, char *[] )     {  
  const double supportlength = 0.8;
  Harmonic q_weight_function(supportlength, 5.0);
  test_kernel(q_weight_function, supportlength);

  //  CubicSpline c_weight_function(supportlength);
  //  std::cerr << "Testing CubicSpline kernel\n";
  //  test_kernel(c_weight_function, supportlength);

  return EXIT_SUCCESS;
};
