//
// BAGEL - Brilliantly Advanced General Electronic Structure Library
// Filename: test_scf.cc
// Copyright (C) 2012 Toru Shiozaki
//
// Author: Alexander Humeniuk
// Maintainer: Shiozaki group
//
// This file is part of the BAGEL package.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <sstream>
#include <src/wfn/reference.h>
#include <src/wfn/dyson.h>
#include <src/wfn/get_energy.h>

using namespace bagel;

VectorB dyson_norms(std::string filename) {
  auto ofs = std::make_shared<std::ofstream>(filename + ".testout", std::ios::trunc);
  std::streambuf* backup_stream = std::cout.rdbuf(ofs->rdbuf());

  // a bit ugly to hardwire an input file, but anyway...
  std::stringstream ss; ss << location__ << filename << ".json";
  auto idata = std::make_shared<const PTree>(ss.str());
  auto keys = idata->get_child("bagel");
  std::shared_ptr<Geometry> geom;
  std::shared_ptr<const Reference> ref;

  for (auto& itree : *keys) {

    const std::string title = to_lower(itree->get<std::string>("title", ""));

    if (title == "molecule") {
      geom = geom ? std::make_shared<Geometry>(*geom, itree) : std::make_shared<Geometry>(itree);
    } else if (title == "dyson") {
      std::shared_ptr<DysonOrbitals> dyson = std::make_shared<DysonOrbitals>(itree);
      dyson->compute();
      return dyson->norms();
      
#ifndef DISABLE_SERIALIZATION
    } else if (title == "save_ref") {
      const std::string name = itree->get<std::string>("file", "reference");
      OArchive archive(name);
      archive << ref;
#endif
    } else {
      // otherwise, they are considered single point energy calculation
      tie(std::ignore, ref) = get_energy(title, itree, geom, ref);
    }

  }
  assert(false);
  return VectorB(0);
}

VectorB reference_dyson_norms() {
  VectorB norms(6);
     
  norms[0] = 0.9636565;
  norms[1] = 0.9626057;
  norms[2] = 0.0752643;
  norms[3] = 0.0015795;
  norms[4] = 0.0192686;
  norms[5] = 0.0076390;

  return norms;
}

BOOST_AUTO_TEST_SUITE(TEST_DYSON)

BOOST_AUTO_TEST_CASE(NORMS) {
  BOOST_CHECK(compare(dyson_norms("h2o_svp_cas_dyson.json"), reference_dyson_norms(), 0.000001));
}

BOOST_AUTO_TEST_SUITE_END()