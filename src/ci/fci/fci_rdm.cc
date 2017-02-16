//
// BAGEL - Brilliantly Advanced General Electronic Structure Library
// Filename: fci_rdm.cc
// Copyright (C) 2011 Toru Shiozaki
//
// Author: Toru Shiozaki <shiozaki@northwestern.edu>
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


#include <src/ci/fci/fci.h>
#include <src/util/prim_op.h>
#include <src/util/math/algo.h>
#include <src/wfn/rdm.h>

using namespace std;
using namespace bagel;

FCI_bare::FCI_bare(shared_ptr<const CIWfn> ci) {
  print_thresh_ = 1.0e-8;
  nelea_ = ci->det()->nelea();
  neleb_ = ci->det()->neleb();
  ncore_ = ci->ncore();
  norb_  = ci->nact();
  nstate_ = ci->nstates();
  energy_ = ci->energies();
  cc_ = ci->civectors()->copy();
  det_ = ci->det();
  rdm1_ = make_shared<VecRDM<1>>();
  rdm2_ = make_shared<VecRDM<2>>();
}


tuple<shared_ptr<RDM<1>>, shared_ptr<RDM<2>>>
  FCI::compute_rdm12_from_civec(shared_ptr<const Civec> cbra, shared_ptr<const Civec> cket) const {

  // since we consider here number conserving operators...
  auto dbra = make_shared<Dvec>(cbra->det(), norb_*norb_);
  sigma_2a1(cbra, dbra);
  sigma_2a2(cbra, dbra);

  shared_ptr<Dvec> dket;
  // if bra and ket vectors are different, we need to form Sigma for ket as well.
  if (cbra != cket) {
    dket = make_shared<Dvec>(cket->det(), norb_*norb_);
    sigma_2a1(cket, dket);
    sigma_2a2(cket, dket);
  } else {
    dket = dbra;
  }

  return compute_rdm12_last_step(dbra, dket, cbra);
}


// computes 3 and 4RDM
tuple<shared_ptr<RDM<3>>, shared_ptr<RDM<4>>> FCI::rdm34(const int ist, const int jst) const {
  auto rdm3 = make_shared<RDM<3>>(norb_);
  auto rdm4 = make_shared<RDM<4>>(norb_);

  auto detex = make_shared<Determinants>(norb_, nelea_, neleb_, false, /*mute=*/true);
  cc_->set_det(detex);

  shared_ptr<Civec> cbra = cc_->data(ist);
  shared_ptr<Civec> cket = cc_->data(jst);

  // first make <I|E_ij|0>
  auto dbra = make_shared<Dvec>(cbra->det(), norb_*norb_);
  sigma_2a1(cbra, dbra);
  sigma_2a2(cbra, dbra);

  shared_ptr<Dvec> dket = dbra;
  if (cbra != cket) {
    dket = dbra->clone();
    sigma_2a1(cket, dket);
    sigma_2a2(cket, dket);
  }

  // second make <J|E_kl|I><I|E_ij|0> - delta_il <J|E_kj|0>
  auto make_evec_half = [this](shared_ptr<Dvec> d, shared_ptr<Matrix> e, const int dsize, const int offset) {
    const int norb2 = norb_ * norb_;
    const int lena = cc_->det()->lena();
    const int lenb = cc_->det()->lenb();
    int no = 0;

    for (int ij = 0; ij != norb2; ++ij) {
      if (ij % mpi__->size() != mpi__->rank()) {
        no += (norb2 - ij);
        continue;
      }
      const int j = ij/norb_;
      const int i = ij-j*norb_;

      for (int kl = ij; kl != norb2; ++kl) {
        const int l = kl/norb_;
        const int k = kl-l*norb_;

        for (auto& iter : cc_->det()->phia(k,l)) {
          size_t iaJ = iter.source;
          size_t iaI = iter.target;
          double sign = static_cast<double>(iter.sign);
          for (size_t ib = 0; ib != lenb; ++ib) {
            size_t iI = ib + iaI*lenb;
            size_t iJ = ib + iaJ*lenb;
            if ((iJ - offset) < dsize)
              e->element(iJ-offset, no) += sign * d->data(ij)->data(iI);
          }
        }

        for (size_t ia = 0; ia != lena; ++ia) {
          for (auto& iter : cc_->det()->phib(k,l)) {
            size_t ibJ = iter.source;
            size_t ibI = iter.target;
            double sign = static_cast<double>(iter.sign);
            size_t iI = ibI + ia*lenb;
            size_t iJ = ibJ + ia*lenb;
            if ((iJ - offset) < dsize)
              e->element(iJ-offset, no) += sign * d->data(ij)->data(iI);
          }
        }

        if (i == l) {
          const int kj = k+j*norb_;
          for (size_t iJ = offset; iJ != offset+dsize; ++iJ) {
            e->element(iJ-offset, no) -= d->data(kj)->data(iJ);
          }
        }
        ++no;
      }
    }

    e->allreduce();

  };

  // When the number of words in <I|E_ij,kl|0> is larger than (10,10) case (which is 635,040,000)
  const size_t ndet = cbra->det()->size();
  const size_t norb2 = norb_ * norb_;
  const size_t ijmax = 635040001;
  const size_t ijnum = ndet * norb2 * norb2;
  const size_t npass = (ijnum-1) / ijmax + 1;
  const size_t nsize = (ndet-1) / npass + 1;
  Timer timer;
  if (npass > 1) {
    cout << "    * Third and fourth order RDM (" << setw(2) << ist + 1 << "," << setw(2) << jst + 1 << ") evaluation" << endl;
    cout << "      will be done with " << npass << " passes" << endl;
  }

  rdm3->zero();
  rdm4->zero();

  // multipassing through {I}
  for (size_t ipass = 0; ipass != npass; ++ipass) {
    const size_t ioffset = ipass * nsize;
    const size_t isize = (ipass != (npass - 1)) ? nsize : ndet - ioffset;
    const size_t halfsize = norb2 * (norb2 + 1) / 2;
    auto eket_half = make_shared<Matrix>(isize, halfsize);
    make_evec_half(dket, eket_half, isize, ioffset);
 
    auto dbram = make_shared<Matrix>(isize, norb2);
    for (size_t ij = 0; ij != norb2; ++ij)
      copy_n(&(dbra->data(ij)->data(ioffset)), isize, dbram->element_ptr(0, ij));

    // put in third-order RDM: <0|E_mn|I><I|E_ij < kl|0>
    auto tmp3 = make_shared<Matrix>(*dbram % *eket_half);
    auto tmp3_full = make_shared<Matrix>(norb2, norb2 * norb2);

    for (size_t mn = 0; mn != norb2; ++mn) {
      int no = 0;
      for (size_t ij = 0; ij != norb2; ++ij) {
        for (size_t kl = ij; kl != norb2; ++kl) {
          int ijkl = ij + kl*norb2;
          int klij = kl + ij*norb2;
          tmp3_full->element(mn, ijkl) = tmp3->element(mn, no);
          tmp3_full->element(mn, klij) = tmp3->element(mn, no);
          ++no;
        }
      }
    }
    sort_indices<1,0,2,1,1,1,1>(tmp3_full->data(), rdm3->data(), norb_, norb_, norb2*norb2);
 
    // put in fourth-order RDM: <0|E_ij > kl|I><I|E_mn < op|0>
    shared_ptr<Matrix> ebra_half = eket_half;
    if (cbra != cket) {
      ebra_half = eket_half->clone();
      make_evec_half(dbra, ebra_half, isize, ioffset);
    }
    auto tmp4 = make_shared<Matrix>(*ebra_half % *eket_half);
    auto tmp4_full = make_shared<Matrix>(norb2 * norb2, norb2 * norb2);

    {
      int nklij = 0;
      for (size_t kl = 0; kl != norb2; ++kl) {
        for (size_t ij = kl; ij != norb2; ++ij) {
          int nmnop = 0;
          int klij = kl+ij*norb2;
          int ijkl = ij+kl*norb2;
          for (size_t mn = 0; mn != norb2; ++mn) {
            for (size_t op = mn; op != norb2; ++op) {
              int mnop = mn + op*norb2;
              int opmn = op + mn*norb2;
              tmp4_full->element(ijkl, mnop) = tmp4->element(nklij, nmnop);
              tmp4_full->element(klij, mnop) = tmp4->element(nklij, nmnop);
              tmp4_full->element(ijkl, opmn) = tmp4->element(nklij, nmnop);
              tmp4_full->element(klij, opmn) = tmp4->element(nklij, nmnop);
              ++nmnop;
            }
          }
          ++nklij;
        }
      }
    }
    sort_indices<1,0,3,2,4,1,1,1,1>(tmp4_full->data(), rdm4->data(), norb_, norb_, norb_, norb_, norb2*norb2);
    if (npass > 1) {
      stringstream ss; ss << "RDM evaluation (" << setw(2) << ipass + 1 << "/" << setw(2) << npass << ")";
      timer.tick_print(ss.str());
    }
  }

  // The remaining terms can be evaluated without multipassing
  {
    // then perform Eq. 49 of JCP 89 5803 (Werner's MRCI paper)
    // we assume that rdm2_[ist] is set
    for (int i0 = 0; i0 != norb_; ++i0)
      for (int i1 = 0; i1 != norb_; ++i1)
        for (int i2 = 0; i2 != norb_; ++i2)
          for (int i3 = 0; i3 != norb_; ++i3) {
            blas::ax_plus_y_n(-1.0, rdm2_->at(ist, jst)->element_ptr(0, i2, i1, i0), norb_, rdm3->element_ptr(0, i3, i3, i2, i1, i0));
            blas::ax_plus_y_n(-1.0, rdm2_->at(ist, jst)->element_ptr(0, i0, i3, i2), norb_, rdm3->element_ptr(0, i1, i3, i2, i1, i0));
          }
  }

  {
    for (int l = 0; l != norb_; ++l)
      for (int k = 0; k != norb_; ++k)
        for (int j = 0; j != norb_; ++j)
          for (int b = 0; b != norb_; ++b) {
            blas::ax_plus_y_n(-1.0, rdm3->element_ptr(0,0,0,k,b,l), norb_*norb_*norb_, rdm4->element_ptr(0,0,0,j,j,k,b,l));
            blas::ax_plus_y_n(-1.0, rdm3->element_ptr(0,0,0,l,b,k), norb_*norb_*norb_, rdm4->element_ptr(0,0,0,j,b,k,j,l));
            for (int i = 0; i != norb_; ++i) {
              blas::ax_plus_y_n(-1.0, rdm2_->at(ist, jst)->element_ptr(0,k,b,l), norb_, rdm4->element_ptr(0,i,b,j,i,k,j,l));
              blas::ax_plus_y_n(-1.0, rdm2_->at(ist, jst)->element_ptr(0,l,b,k), norb_, rdm4->element_ptr(0,i,b,j,j,k,i,l));
              for (int d = 0; d != norb_; ++d) {
                blas::ax_plus_y_n(-1.0, rdm3->element_ptr(0,k,b,j,d,l), norb_, rdm4->element_ptr(0,i,b,j,i,k,d,l));
                blas::ax_plus_y_n(-1.0, rdm3->element_ptr(0,l,b,j,d,k), norb_, rdm4->element_ptr(0,i,b,j,d,k,i,l));
              }
            }
          }
  }

  cc_->set_det(det_);

  return make_tuple(rdm3, rdm4);
}


#if 0
// computes 3 and 4RDM
// TODO duplicate code to be cleaned up
tuple<shared_ptr<RDM<3>>, shared_ptr<RDM<3>>> FCI::rdm34f(const int ist, const int jst, shared_ptr<const Matrix> fock) const {
  auto rdm3 = make_shared<RDM<3>>(norb_);
  auto frdm4 = make_shared<RDM<3>>(norb_);

  auto detex = make_shared<Determinants>(norb_, nelea_, neleb_, false, /*mute=*/true);
  cc_->set_det(detex);

  shared_ptr<Civec> cbra = cc_->data(ist);
  shared_ptr<Civec> cket = cc_->data(jst);

  // first make <I|E_ij|0>
  auto dbra = make_shared<Dvec>(cbra->det(), norb_*norb_);
  sigma_2a1(cbra, dbra);
  sigma_2a2(cbra, dbra);

  shared_ptr<Dvec> dket = dbra;
  if (cbra != cket) {
    dket = dbra->clone();
    sigma_2a1(cket, dket);
    sigma_2a2(cket, dket);
  }

  // second make <J|E_kl|I><I|E_ij|0> - delta_li <J|E_kj|0>
  auto make_evec = [this](shared_ptr<Dvec> d, shared_ptr<Dvec> e, shared_ptr<Dvec> tmp) {
    int ijkl = 0;
    int ij = 0;
    for (auto iter = d->dvec().begin(); iter != d->dvec().end(); ++iter, ++ij) {
      const int j = ij/norb_;
      const int i = ij-j*norb_;
      tmp->zero();
      sigma_2a1(*iter, tmp);
      sigma_2a2(*iter, tmp);
      int kl = 0;
      for (auto t = tmp->dvec().begin(); t != tmp->dvec().end(); ++t, ++ijkl, ++kl) {
        *e->data(ijkl) = **t;
        const int l = kl/norb_;
        const int k = kl-l*norb_;
        if (l == i) *e->data(ijkl) -= *d->data(k+j*norb_);
      }
    }
  };
  auto ebra = make_shared<Dvec>(cbra->det(), norb_*norb_*norb_*norb_);
  auto tmp = make_shared<Dvec>(cbra->det(), norb_*norb_);
  make_evec(dbra, ebra, tmp);

  shared_ptr<Dvec> eket = ebra;
  if (cbra != cket) {
    eket = ebra->clone();
    make_evec(dket, eket, tmp);
  }

  // size of the RI space
  const size_t nri = ebra->lena() * ebra->lenb();
  assert(nri == dbra->lena()*dbra->lenb());

  // first form <0|E_mn|I><I|E_ij,kl|0>
  {
    auto tmp3 = make_shared<RDM<3>>(norb_);
    auto tmp3v = group(group(*tmp3,2,6),0,2);
    contract(1.0, group(*dbra,0,2), {0,1}, group(*eket,0,2), {0,2}, 0.0, tmp3v, {1,2});
    sort_indices<1,0,2,0,1,1,1>(tmp3->data(), rdm3->data(), norb_, norb_, norb_*norb_*norb_*norb_);

    // then perform Eq. 49 of JCP 89 5803 (Werner's MRCI paper)
    // we assume that rdm2_[ist] is set
    for (int i0 = 0; i0 != norb_; ++i0)
      for (int i1 = 0; i1 != norb_; ++i1)
        for (int i2 = 0; i2 != norb_; ++i2)
          for (int i3 = 0; i3 != norb_; ++i3) {
            blas::ax_plus_y_n(-1.0, rdm2_->at(ist, jst)->element_ptr(0, i2, i1, i0), norb_, rdm3->element_ptr(0, i3, i3, i2, i1, i0));
            blas::ax_plus_y_n(-1.0, rdm2_->at(ist, jst)->element_ptr(0, i0, i3, i2), norb_, rdm3->element_ptr(0, i1, i3, i2, i1, i0));
          }
  }

  // 4RDM <0|E_ij,kl|I><I|E_mn,op|0>
  // now make target:  [0|E_ik,jl,op|0]  =  <0|E_ik,jl|K>[K|E_op|0] - f_nk(<0|E_in,jl,op|0>+d_ol<0|E_in,jp|0>)
  //                                                                - f_nl(<0|E_ik,jn,op|0>+d_ok<0|E_ip,jn|0>)
  //                                                                - d_ok [0|E_ip,jl|0]  - d_ol [0|E_ik,jp|0]
  {
    auto rdm2 = make_shared<RDM<2>>(norb_);
    dgemv_("T", nri, eket->ij(), 1.0, eket->data(), nri, cbra->data(), 1, 0.0, rdm2->data(), 1);

    // [0|E_ip,jl|0]
    auto frdm3 = make_shared<RDM<2>>(norb_);
    auto rdm3view = group(group(*rdm3, 4,6), 0,4);
    auto frdm3view = group(*frdm3, 0,4);
    contract(1.0, rdm3view, {0,1}, group(*fock, 0,2), {1}, 0.0, frdm3view, {0});

    // <0|E_ip,jn|0>f_nl (in this order)
    auto prdm2 = make_shared<RDM<2>>(norb_);
    auto prdm2v = group(*prdm2, 0,3);
    contract(1.0, group(*rdm2, 0,3), {0,1}, *fock, {1,2}, 0.0, prdm2v, {0,2});

    // <0|E_ik,jl,on|0>f_np (in this order)
    auto prdm3 = make_shared<RDM<3>>(norb_);
    auto prdm3v = group(*prdm3, 0,5);
    contract(1.0, group(*rdm3, 0,5), {0,1}, *fock, {1,2}, 0.0, prdm3v, {0,2});

    // <0|E_ik,jl|K>[K|E_op|0]
    auto tmp = make_shared<RDM<3>>(norb_);
    auto tmpv = group(group(*tmp, 4,6), 0,4);
    auto feket = dket->clone();
    dgemv_("N", nri*norb_*norb_, norb_*norb_, 1.0, eket->data(), nri*norb_*norb_, fock->data(), 1, 0.0, feket->data(), 1);
    contract(1.0, group(*ebra, 0,2), {0,1}, group(*feket, 0,2), {0,2}, 0.0, tmpv, {1,2});
    sort_indices<1,0,3,2,4,0,1,1,1>(tmp->data(), frdm4->data(), norb_, norb_, norb_, norb_, norb_*norb_);

    sort_indices<2,0,1,1,1,-1,1>(prdm3->data(), frdm4->data(), norb_*norb_, norb_*norb_, norb_*norb_);
    sort_indices<0,2,1,1,1,-1,1>(prdm3->data(), frdm4->data(), norb_*norb_, norb_*norb_, norb_*norb_);

    auto prdm2t = prdm2->clone();
    sort_indices<1,0,0,1,1,1>(prdm2->data(), prdm2t->data(), norb_*norb_, norb_*norb_);
    for (int p = 0; p != norb_; ++p)
      for (int l = 0; l != norb_; ++l) {
        blas::ax_plus_y_n(-1.0, prdm2t->element_ptr(0,0,0,p), norb_*norb_*norb_, frdm4->element_ptr(0,0,0,l,l,p));
        blas::ax_plus_y_n(-1.0, frdm3->element_ptr(0,0,0,p),  norb_*norb_*norb_, frdm4->element_ptr(0,0,0,l,l,p));
        for (int k = 0; k != norb_; ++k)
          for (int j = 0; j != norb_; ++j) {
            blas::ax_plus_y_n(-1.0, prdm2->element_ptr(0,p,j,l), norb_, frdm4->element_ptr(0,k,j,l,k,p));
            blas::ax_plus_y_n(-1.0, frdm3->element_ptr(0,p,j,l), norb_, frdm4->element_ptr(0,k,j,l,k,p));
          }
      }
  }

  cc_->set_det(det_);

  return make_tuple(rdm3, frdm4);
}
#endif
