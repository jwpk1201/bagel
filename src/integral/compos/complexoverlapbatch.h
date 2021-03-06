//
// BAGEL - Brilliantly Advanced General Electronic Structure Library
// Filename: complexoverlapbatch.h
// Copyright (C) 2014 Quantum Simulation Technologies, Inc.
//
// Author: Ryan D. Reynolds <RyanDReynolds@u.northwestern.edu>
// Maintainer: QSimulate
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


#ifndef __SRC_INTEGRAL_COMPOS_COMPLEXOVERLAPBATCH_H
#define __SRC_INTEGRAL_COMPOS_COMPLEXOVERLAPBATCH_H

#include <src/integral/os/osintegral.h>
#include <src/molecule/molecule.h>

namespace bagel {

class ComplexOverlapBatch : public OSIntegral<std::complex<double>,Int_t::London> {
  protected:
    void perform_VRR(std::complex<double>*) override;
    virtual std::complex<double> get_P(const double coord1, const double coord2, const double exp1, const double exp2, const double one12,
                                                const int dim, const bool swap) override;

    int nblocks() const override { return 1; }
    int nrank() const override { return 0; }

  public:
    ComplexOverlapBatch(const std::array<std::shared_ptr<const Shell>,2>& basis, std::shared_ptr<StackMem> stack = nullptr)
    : OSIntegral<std::complex<double>,Int_t::London>(basis, stack) { common_init(); }

    // TODO This redundancy should be avoidable
    constexpr static int Nblocks() { return 1; }

    void compute() override;
};

}

#endif
