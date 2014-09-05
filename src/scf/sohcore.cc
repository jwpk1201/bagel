//
// BAGEL - Parallel electron correlation program.
// Filename: sohcore.cc
// Copyright (C) 2014 Toru Shiozaki
//
// Author: Hai-Anh Le <anh@u.northwestern.edu>
// Maintainer: Shiozaki group
//
// This file is part of the BAGEL package.
//
// The BAGEL package is free software; you can redistribute it and/or modify
// it under the terms of the GNU Library General Public License as published by
// the Free Software Foundation; either version 3, or (at your option)
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


#include <src/scf/sohcore.h>

using namespace std;
using namespace bagel;

BOOST_CLASS_EXPORT_IMPLEMENT(SOHcore)

SOHcore::SOHcore(const shared_ptr<const Geometry> geom, const shared_ptr<const Hcore> h)
            : ZMatrix(2 * geom->nbasis(), 2 * geom->nbasis()), geom_(geom), hcore_(h) {
  form_sohcore();
}

void SOHcore::form_sohcore() {

  const int nbasis = geom_->nbasis();
  const complex<double> real(1.0, 0.0);
  const complex<double> imag(0.0, 1.0);

  hcore_->antisymmetrize_hso();

  add_real_block(real, 0, 0, nbasis, nbasis, *hcore_);
  add_real_block(imag, 0, 0, nbasis, nbasis, *hcore_->hso()->soiaa());

  add_real_block(real, nbasis, nbasis, nbasis, nbasis, *hcore_);
  add_real_block(-imag, nbasis, nbasis, nbasis, nbasis, *hcore_->hso()->soiaa());

  add_real_block(real, 0, nbasis, nbasis, nbasis, *hcore_->hso()->sorab());
  add_real_block(imag, 0, nbasis, nbasis, nbasis, *hcore_->hso()->soiab());

  add_real_block(-real, nbasis, 0, nbasis, nbasis, *hcore_->hso()->sorab());
  add_real_block( imag, nbasis, 0, nbasis, nbasis, *hcore_->hso()->soiab());

}
