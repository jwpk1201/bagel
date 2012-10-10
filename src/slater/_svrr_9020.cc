//
// BAGEL - Parallel electron correlation program.
// Filename: _svrr_9020.cc
// Copyright (C) 2009 Toru Shiozaki
//
// Author: Toru Shiozaki <shiozaki@northwestern.edu>
// Maintainer: Shiozaki group
//
// This file is part of the BAGEL package.
//
// The BAGEL package is free software; you can redistribute it and/or modify
// it under the terms of the GNU Library General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// The BAGEL package is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with the BAGEL package; see COPYING.  If not, write to
// the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include <src/slater/svrrlist.h>

using namespace bagel;

// returns double array of length 210
void SVRRList::_svrr_9020(double* data_, const double* C00, const double* D00, const double* B00, const double* B01, const double* B10) {
#ifdef __GNUC__
  const double C00_[7]__attribute__((aligned(32))) = {C00[0], C00[1], C00[2], C00[3], C00[4], C00[5], C00[6]};
  const double D00_[7]__attribute__((aligned(32))) = {D00[0], D00[1], D00[2], D00[3], D00[4], D00[5], D00[6]};
  const double B00_[7]__attribute__((aligned(32))) = {B00[0], B00[1], B00[2], B00[3], B00[4], B00[5], B00[6]};
  const double B01_[7]__attribute__((aligned(32))) = {B01[0], B01[1], B01[2], B01[3], B01[4], B01[5], B01[6]};
  const double B10_[7]__attribute__((aligned(32))) = {B10[0], B10[1], B10[2], B10[3], B10[4], B10[5], B10[6]};
#else
  const double* C00_ = C00;
  const double* D00_ = D00;
  const double* B00_ = B00;
  const double* B01_ = B01;
  const double* B10_ = B10;
#endif

  for (int t = 0; t != 7; ++t)
    data_[0+t] = 1.0;

  for (int t = 0; t != 7; ++t)
    data_[7+t] = C00_[t];

#ifdef __GNUC__
  double B10_current[7]__attribute__((aligned(32)));
#else
  double B10_current[7];
#endif
  for (int t = 0; t != 7; ++t)
    B10_current[t] = B10_[t];

  for (int t = 0; t != 7; ++t)
    data_[14+t] = C00_[t] * data_[7+t] + B10_current[t];

  for (int t = 0; t != 7; ++t)
    B10_current[t] += B10_[t];

  for (int t = 0; t != 7; ++t)
    data_[21+t] = C00_[t] * data_[14+t] + B10_current[t] * data_[7+t];

  for (int t = 0; t != 7; ++t)
    B10_current[t] += B10_[t];

  for (int t = 0; t != 7; ++t)
    data_[28+t] = C00_[t] * data_[21+t] + B10_current[t] * data_[14+t];

  for (int t = 0; t != 7; ++t)
    B10_current[t] += B10_[t];

  for (int t = 0; t != 7; ++t)
    data_[35+t] = C00_[t] * data_[28+t] + B10_current[t] * data_[21+t];

  for (int t = 0; t != 7; ++t)
    B10_current[t] += B10_[t];

  for (int t = 0; t != 7; ++t)
    data_[42+t] = C00_[t] * data_[35+t] + B10_current[t] * data_[28+t];

  for (int t = 0; t != 7; ++t)
    B10_current[t] += B10_[t];

  for (int t = 0; t != 7; ++t)
    data_[49+t] = C00_[t] * data_[42+t] + B10_current[t] * data_[35+t];

  for (int t = 0; t != 7; ++t)
    B10_current[t] += B10_[t];

  for (int t = 0; t != 7; ++t)
    data_[56+t] = C00_[t] * data_[49+t] + B10_current[t] * data_[42+t];

  for (int t = 0; t != 7; ++t)
    B10_current[t] += B10_[t];

  for (int t = 0; t != 7; ++t)
    data_[63+t] = C00_[t] * data_[56+t] + B10_current[t] * data_[49+t];

  for (int t = 0; t != 7; ++t)
    data_[70+t] = D00_[t];

#ifdef __GNUC__
  double cB00_current[7]__attribute__((aligned(32)));
#else
  double cB00_current[7];
#endif
  for (int t = 0; t != 7; ++t)
    cB00_current[t] = B00_[t];

  for (int t = 0; t != 7; ++t)
    data_[77+t] = C00_[t] * data_[70+t] + cB00_current[t];

  for (int t = 0; t != 7; ++t)
    B10_current[t] = B10_[t];

  for (int t = 0; t != 7; ++t)
    data_[84+t] = C00_[t] * data_[77+t] + B10_current[t] * data_[70+t] + cB00_current[t] * data_[7+t];

  for (int t = 0; t != 7; ++t)
    B10_current[t] += B10_[t];

  for (int t = 0; t != 7; ++t)
    data_[91+t] = C00_[t] * data_[84+t] + B10_current[t] * data_[77+t] + cB00_current[t] * data_[14+t];

  for (int t = 0; t != 7; ++t)
    B10_current[t] += B10_[t];

  for (int t = 0; t != 7; ++t)
    data_[98+t] = C00_[t] * data_[91+t] + B10_current[t] * data_[84+t] + cB00_current[t] * data_[21+t];

  for (int t = 0; t != 7; ++t)
    B10_current[t] += B10_[t];

  for (int t = 0; t != 7; ++t)
    data_[105+t] = C00_[t] * data_[98+t] + B10_current[t] * data_[91+t] + cB00_current[t] * data_[28+t];

  for (int t = 0; t != 7; ++t)
    B10_current[t] += B10_[t];

  for (int t = 0; t != 7; ++t)
    data_[112+t] = C00_[t] * data_[105+t] + B10_current[t] * data_[98+t] + cB00_current[t] * data_[35+t];

  for (int t = 0; t != 7; ++t)
    B10_current[t] += B10_[t];

  for (int t = 0; t != 7; ++t)
    data_[119+t] = C00_[t] * data_[112+t] + B10_current[t] * data_[105+t] + cB00_current[t] * data_[42+t];

  for (int t = 0; t != 7; ++t)
    B10_current[t] += B10_[t];

  for (int t = 0; t != 7; ++t)
    data_[126+t] = C00_[t] * data_[119+t] + B10_current[t] * data_[112+t] + cB00_current[t] * data_[49+t];

  for (int t = 0; t != 7; ++t)
    B10_current[t] += B10_[t];

  for (int t = 0; t != 7; ++t)
    data_[133+t] = C00_[t] * data_[126+t] + B10_current[t] * data_[119+t] + cB00_current[t] * data_[56+t];


  for (int t = 0; t != 7; ++t)
    data_[140+t] = D00_[t] * data_[70+t] + B01_[t];

  for (int t = 0; t != 7; ++t)
    cB00_current[t] += B00_[t];

  for (int t = 0; t != 7; ++t)
    data_[147+t] = C00_[t] * data_[140+t] + cB00_current[t] * data_[70+t];

  for (int t = 0; t != 7; ++t)
    B10_current[t] = B10_[t];

  for (int t = 0; t != 7; ++t)
    data_[154+t] = C00_[t] * data_[147+t] + B10_current[t] * data_[140+t] + cB00_current[t] * data_[77+t];

  for (int t = 0; t != 7; ++t)
    B10_current[t] += B10_[t];

  for (int t = 0; t != 7; ++t)
    data_[161+t] = C00_[t] * data_[154+t] + B10_current[t] * data_[147+t] + cB00_current[t] * data_[84+t];

  for (int t = 0; t != 7; ++t)
    B10_current[t] += B10_[t];

  for (int t = 0; t != 7; ++t)
    data_[168+t] = C00_[t] * data_[161+t] + B10_current[t] * data_[154+t] + cB00_current[t] * data_[91+t];

  for (int t = 0; t != 7; ++t)
    B10_current[t] += B10_[t];

  for (int t = 0; t != 7; ++t)
    data_[175+t] = C00_[t] * data_[168+t] + B10_current[t] * data_[161+t] + cB00_current[t] * data_[98+t];

  for (int t = 0; t != 7; ++t)
    B10_current[t] += B10_[t];

  for (int t = 0; t != 7; ++t)
    data_[182+t] = C00_[t] * data_[175+t] + B10_current[t] * data_[168+t] + cB00_current[t] * data_[105+t];

  for (int t = 0; t != 7; ++t)
    B10_current[t] += B10_[t];

  for (int t = 0; t != 7; ++t)
    data_[189+t] = C00_[t] * data_[182+t] + B10_current[t] * data_[175+t] + cB00_current[t] * data_[112+t];

  for (int t = 0; t != 7; ++t)
    B10_current[t] += B10_[t];

  for (int t = 0; t != 7; ++t)
    data_[196+t] = C00_[t] * data_[189+t] + B10_current[t] * data_[182+t] + cB00_current[t] * data_[119+t];

  for (int t = 0; t != 7; ++t)
    B10_current[t] += B10_[t];

  for (int t = 0; t != 7; ++t)
    data_[203+t] = C00_[t] * data_[196+t] + B10_current[t] * data_[189+t] + cB00_current[t] * data_[126+t];
}

