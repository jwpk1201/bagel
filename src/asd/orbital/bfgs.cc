//
// BAGEL - Parallel electron correlation program.
// Filename: bfgs2.cc
// Copyright (C) 2015 Toru Shiozaki
//
// Author: Inkoo Kim <inkoo.kim@northwestern.edu>
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

#include <src/scf/hf/fock.h>
#include <src/asd/orbital/bfgs.h>
#include <src/util/math/davidson.h>
#include <src/util/math/step_restrict_bfgs.h>
#include <src/util/math/hpw_diis.h>

#include <src/asd/construct_asd.h>

using namespace std;
using namespace bagel;

void ASD_BFGS::compute() {

  // equation numbers refer to Chaban, Schmidt and Gordon 1997 TCA 97, 88.

  shared_ptr<SRBFGS<ASD_RotFile>> bfgs;
  pair<double,double> gpair;
  bool together = false;
  bool together_denom = false;
  double gthr = 5.0e-4;

  shared_ptr<SRBFGS<RotFile>> bfgs_large;
  shared_ptr<SRBFGS<ASD_ActiveRotFile>> bfgs_small;

  // ============================
  // macro iteration from here
  // ============================
  Timer timer;

  //allocate unitary rotation matrix
  auto x = make_shared<Matrix>(nbasis_, nbasis_);
  x->unit();
  auto x_large = make_shared<Matrix>(nbasis_, nbasis_);
  x_large->unit();
  auto x_small = make_shared<Matrix>(nbasis_, nbasis_);
  x_small->unit();

  vector<double> evals;

  auto asd = construct_ASD(asdinput_, dimer_);
  rdm1_ = make_shared<RDM<1>>(nact_);
  rdm2_ = make_shared<RDM<2>>(nact_);

  mute_stdcout();
  for (int iter = 0; iter != max_iter_; ++iter) {

    bool large = iter%2 == 0 ? true : false;

    //Perform ASD
    if (iter) {
      //update coeff_ & integrals..
      dimer_->update_coeff(coeff_);
      //build CI-space with updated coeff
      asd = construct_ASD(asdinput_, dimer_);
    }

    asd->compute();
    //get RDM
    rdm1_ = asd->rdm1_av();
    rdm2_ = asd->rdm2_av();
    //get energy
    energy_ = asd->energy();

    {
      // use state averaged energy to update trust radius
      double sa_en = 0.0;
      for (auto& i : asd->energy())
        sa_en += i;
      sa_en /= double((asd->energy()).size());
      evals.push_back(sa_en);
    }

//but get half_
    const MatView cdata = coeff_->slice(nclosed_, nclosed_+nact_);
    half_ = geom_->df()->compute_half_transform(cdata);

    shared_ptr<ASD_RotFile> sigma;
    shared_ptr<RotFile> sigma_large;
    shared_ptr<ASD_ActiveRotFile> sigma_small;
    if (together) {
      sigma = make_shared<ASD_RotFile>(nclosed_, nact_, nvirt_, nactA_, nactB_ );
      sigma->zero();
    } else {
      if (large) {
        sigma_large = make_shared<RotFile>(nclosed_, nact_, nvirt_);
        sigma_large->zero();
      } else {
        //small
        sigma_small = make_shared<ASD_ActiveRotFile>(nclosed_, nact_, nvirt_, nactA_, nactB_ );
        sigma_small->zero();
      }
    }

    // compute one-body operators
    // * preparation
    const MatView ccoeff = coeff_->slice(0, nclosed_);

    // * core Fock operator
    shared_ptr<const Matrix> cfockao = nclosed_ ? make_shared<const Fock<1>>(geom_, hcore_, nullptr, ccoeff, /*store*/false, /*rhf*/true) : hcore_;
    shared_ptr<const Matrix> cfock = make_shared<Matrix>(*coeff_ % *cfockao * *coeff_);

    // * active Fock operator
    // first make a weighted coefficient
    shared_ptr<Matrix> acoeff = coeff_->slice_copy(nclosed_, nocc_);
    shared_ptr<Matrix> rdm1_mat = rdm1_->rdm1_mat(/*nclose*/0);

    shared_ptr<Matrix> rdm1_scaled = rdm1_mat->copy();
    rdm1_scaled->sqrt();
    rdm1_scaled->delocalize();
    auto acoeffw = make_shared<Matrix>(*acoeff * (1.0/sqrt(2.0)) * *rdm1_scaled); // such that C' * (1/2 D) C will be obtained.

    // then make a AO density matrix
    shared_ptr<const Matrix> afockao = make_shared<Fock<1>>(geom_, hcore_->clone(), nullptr, acoeffw, /*store*/false, /*rhf*/true);
    shared_ptr<const Matrix> afock = make_shared<Matrix>(*coeff_ % *afockao * *coeff_);

    // * Q_xr = 2(xs|tu)P_rs,tu (x=general, mo)
//  auto qxr = make_shared<const Qvec>(coeff_->mdim(), nact_, coeff_, nclosed_, fci_, fci_->rdm2_av());
//ADDED
    auto qxr = Qvec(coeff_->mdim(), nact_, coeff_, nclosed_); //uses internal rdm1_ and rdm2_

    //mcfock(nact_,nact_)
    shared_ptr<Matrix> mcfock = make_shared<Matrix>(*cfock->get_submatrix(nclosed_, nclosed_, nact_, nact_) * *rdm1_mat
                                                    + *qxr->get_submatrix(nclosed_, 0, nact_, nact_) );
    cout << "MC Fock Matrix: symmetric check:" << check_symmetric(mcfock) << endl;

    if (together) {
    // grad(a/i) (eq.4.3a): 4(cfock_ai+afock_ai)
      grad_vc(cfock, afock, sigma);
      // grad(a/t) (eq.4.3b): 2cfock_au gamma_ut + q_at
      grad_va(cfock, qxr, rdm1_mat, sigma);
      // grad(r/i) (eq.4.3c): 4(cfock_ri+afock_ri) - 2cfock_iu gamma_ur - qxr_ir
      grad_ca(cfock, afock, qxr, rdm1_mat, sigma);
      grad_aa(mcfock, sigma);
    } else {
      if (large) {
        grad_vc_large(cfock, afock, sigma_large);
        grad_va_large(cfock, qxr, rdm1_mat, sigma_large);
        grad_ca_large(cfock, afock, qxr, rdm1_mat, sigma_large);
      } else {
        //small
        grad_aa_small(mcfock, sigma_small);
      }
    }

    // if this is the first time, set up the BFGS solver
    // BFGS and DIIS should start at the same time
    if (iter == 0) {
      shared_ptr<const RotFile> denom_large = compute_denom_large(cfock, afock, qxr, rdm1_mat);
      bfgs_large = make_shared<SRBFGS<RotFile>>(denom_large);
    } else if (iter == 1) {
      //small
      shared_ptr<const ASD_ActiveRotFile> denom_small = compute_denom_small(cfock, afock, rdm1_mat, mcfock);
      bfgs_small = make_shared<SRBFGS<ASD_ActiveRotFile>>(denom_small);
    }
    if (together && !together_denom) {
      shared_ptr<const ASD_RotFile> denom = compute_denom(cfock, afock, qxr, rdm1_mat, mcfock);
      bfgs = make_shared<SRBFGS<ASD_RotFile>>(denom);
      together_denom = true;
    }

    // extrapolation using BFGS
    Timer extrap(0);
    shared_ptr<RotFile> a;
    shared_ptr<ASD_ActiveRotFile> b;
    shared_ptr<ASD_RotFile> c;
    if (together) {
      cout << " " << endl;
      cout << " -------  Step Restricted BFGS Extrapolation  ------- " << endl;
      auto xcopy = x->log(8);
      auto xlog  = make_shared<ASD_RotFile>(xcopy, nclosed_, nact_, nvirt_, nactA_, nactB_ );
      bfgs->check_step(evals, sigma, xlog);
      c = bfgs->more_sorensen_extrapolate(sigma, xlog);
      cout << " ---------------------------------------------------- " << endl << endl;
    } else {
      if (large) {
        cout << " " << endl;
        cout << " -------  Step Restricted BFGS Extrapolation  ------- " << endl;
        auto xcopy = x_large->log(8);
        auto xlog  = make_shared<RotFile>(xcopy, nclosed_, nact_, nvirt_);
        bfgs_large->check_step(evals, sigma_large, xlog); //, /*tight*/false, limited_memory);
        a = bfgs_large->more_sorensen_extrapolate(sigma_large, xlog);
        cout << " ---------------------------------------------------- " << endl;
        extrap.tick_print("More-Sorensen/Hebden extrapolation");
        cout << " " << endl;
      } else {
        //small
        cout << " " << endl;
        cout << " -------  Step Restricted BFGS Extrapolation  ------- " << endl;
        auto xcopy = x_small->log(8);
        auto xlog  = make_shared<ASD_ActiveRotFile>(xcopy, nclosed_, nact_, nvirt_, nactA_, nactB_);
        bfgs_small->check_step(evals, sigma_small, xlog); //, /*tight*/false, limited_memory);
        b = bfgs_small->more_sorensen_extrapolate(sigma_small, xlog);
        cout << " ---------------------------------------------------- " << endl;
        extrap.tick_print("More-Sorensen/Hebden extrapolation");
        cout << " " << endl;
      }
    }

    // restore the matrix from ASD_RotFile
//  shared_ptr<const Matrix> amat = a->unpack<Matrix>();
//  shared_ptr<const Matrix> amat = iter%2 == 0 ? a->unpack<Matrix>() : b->unpack<Matrix>();
    shared_ptr<Matrix> amat;
    if (together) {
      amat = c->unpack<Matrix>();
    } else {
      amat = iter%2 == 0 ? a->unpack<Matrix>() : b->unpack<Matrix>();
    }

    shared_ptr<Matrix> expa = amat->exp(100);
    expa->purify_unitary();

    // updating coefficients
    coeff_ = make_shared<const Coeff>(*coeff_ * *expa);
    // for next BFGS e2xtrapolation
    if (together) {
      *x *= *expa;
    } else {
      if (large)
        *x_large *= *expa;
      else
        *x_small *= *expa;
    }
    // synchronization
    mpi__->broadcast(const_pointer_cast<Coeff>(coeff_)->data(), coeff_->size(), 0);

    // setting error of macro iteration
//  const double gradient = sigma->rms();
    double gradient;
    if (together) {
      gradient = sigma->rms();
    } else {
      gradient = iter%2 == 0 ? sigma_large->rms() : sigma_small->rms() ;
    }

    if (large)
      get<0>(gpair) = gradient;
    else
      get<1>(gpair) = gradient;

    if (!together) {
      if (get<0>(gpair) < gthr && get<1>(gpair) < gthr) {
        together = true;
        x->unit();
      }
    }

    resume_stdcout();
    print_iteration(iter, 0, 0, energy_, gradient, timer.tick());

//  if (gradient < thresh_) {
    if (max(get<0>(gpair),get<1>(gpair)) < thresh_) {
      rms_grad_ = gradient;
      cout << " " << endl;
      cout << "    * quasi-Newton optimization converged. *   " << endl << endl;
      mute_stdcout();
      break;
    }

    if (iter == max_iter_-1) {
      rms_grad_ = gradient;
      cout << " " << endl;
      if (rms_grad_ > thresh_) cout << "    * The calculation did NOT converge. *    " << endl;
      cout << "    * Max iteration reached during the quasi-Newton optimization. *     " << endl << endl;
    }
    mute_stdcout();
  }
  resume_stdcout();
  // ============================
  // macro iteration to here
  // ============================
}
