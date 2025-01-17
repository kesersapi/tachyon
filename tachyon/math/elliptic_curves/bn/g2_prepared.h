// Copyright 2022 arkworks contributors
// Use of this source code is governed by a MIT/Apache-2.0 style license that
// can be found in the LICENSE-MIT.arkworks and the LICENCE-APACHE.arkworks
// file.

#ifndef TACHYON_MATH_ELLIPTIC_CURVES_BN_G2_PREPARED_H_
#define TACHYON_MATH_ELLIPTIC_CURVES_BN_G2_PREPARED_H_

#include <utility>

#include "tachyon/math/elliptic_curves/pairing/g2_prepared_base.h"
#include "tachyon/math/elliptic_curves/pairing/g2_projective.h"

namespace tachyon::math::bn {

template <typename BNCurveConfig>
class G2Prepared : public G2PreparedBase<BNCurveConfig> {
 public:
  using Config = BNCurveConfig;
  using G2Curve = typename Config::G2Curve;
  using Fp2Ty = typename G2Curve::BaseField;
  using FpTy = typename Fp2Ty::BaseField;
  using G2AffinePointTy = typename G2Curve::AffinePointTy;

  G2Prepared() = default;
  explicit G2Prepared(const EllCoeffs<Fp2Ty>& ell_coeffs)
      : G2PreparedBase<BNCurveConfig>(ell_coeffs) {}
  explicit G2Prepared(EllCoeffs<Fp2Ty>&& ell_coeffs)
      : G2PreparedBase<BNCurveConfig>(std::move(ell_coeffs)) {}

  static G2Prepared From(const G2AffinePointTy& q) {
    if (q.infinity()) {
      return {};
    } else {
      G2Projective<Config> r(q.x(), q.y(), Fp2Ty::One());

      EllCoeffs<Fp2Ty> ell_coeffs;
      // NOTE(chokobole): |Config::kAteLoopCount| consists of elements
      // from [-1, 0, 1]. We reserve space in |ell_coeffs| assuming that these
      // elements are uniformly distributed.
      size_t size = std::size(Config::kAteLoopCount);
      ell_coeffs.reserve(/*double=*/size + /*add=*/size * 2 / 3);

      G2AffinePointTy neg_q = -q;

      FpTy two_inv = FpTy(2).Inverse();
      // NOTE(chokobole): skip the fist.
      for (size_t i = size - 2; i != SIZE_MAX; --i) {
        ell_coeffs.push_back(r.DoubleInPlace(two_inv));

        switch (Config::kAteLoopCount[i]) {
          case 1:
            ell_coeffs.push_back(r.AddInPlace(q));
            break;
          case -1:
            ell_coeffs.push_back(r.AddInPlace(neg_q));
            break;
          default:
            continue;
        }
      }

      G2AffinePointTy q1 = MulByCharacteristic(q);
      G2AffinePointTy q2 = MulByCharacteristic(q1);
      q2.NegInPlace();

      if constexpr (Config::kXIsNegative) {
        r.NegateInPlace();
      }

      ell_coeffs.push_back(r.AddInPlace(q1));
      ell_coeffs.push_back(r.AddInPlace(q2));

      return G2Prepared(std::move(ell_coeffs));
    }
  }

 private:
  static G2AffinePointTy MulByCharacteristic(const G2AffinePointTy& r) {
    Fp2Ty x = r.x();
    x.FrobeniusMapInPlace(1);
    x *= Config::kTwistMulByQX;
    Fp2Ty y = r.y();
    y.FrobeniusMapInPlace(1);
    y *= Config::kTwistMulByQY;
    return G2AffinePointTy(std::move(x), std::move(y));
  }
};

}  // namespace tachyon::math::bn

#endif  // TACHYON_MATH_ELLIPTIC_CURVES_BN_G2_PREPARED_H_
