//
//    MIT No Attribution
//
//    Copyright (C) 2010-2023 Joel Andersson, Joris Gillis, Moritz Diehl, KU Leuven.
//
//    Permission is hereby granted, free of charge, to any person obtaining a copy of this
//    software and associated documentation files (the "Software"), to deal in the Software
//    without restriction, including without limitation the rights to use, copy, modify,
//    merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
//    permit persons to whom the Software is furnished to do so.
//
//    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
//    INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
//    PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
//    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
//    OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

// SYMBOL "mv_dense"
template<typename T1>
void casadi_mv_dense(const T1* x, casadi_int nrow_x, casadi_int ncol_x,
    const T1* y, T1* z, casadi_int tr) {
  casadi_int i, j;
  if (!x || !y || !z) return;
  if (tr) {
    for (i=0; i<ncol_x; ++i) {
      for (j=0; j<nrow_x; ++j) {
        z[i] += *x++ * y[j];
      }
    }
  } else {
    for (i=0; i<ncol_x; ++i) {
      for (j=0; j<nrow_x; ++j) {
        z[j] += *x++ * y[i];
      }
    }
  }
}
