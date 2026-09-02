# Bootstrap Noise Bounds

See the main [README](https://github.com/rysolis/torqus#bootstrap-noise-bounds)
for the short summary and the table this page expands on.

[`gate_bootstrap_test.cpp`](https://github.com/rysolis/torqus/blob/main/test/runtime/tfhe/operation/bootstrap/gate_bootstrap_test.cpp)
exercises `GateBootstrap` under the 128-bit-security parameter set
(n=630, N=1024, on a 32-bit `Torus`) published by the reference TFHE
implementation's
[security and parameters page](https://tfhe.github.io/tfhe/security_and_params.html),
and is required to clear a 99% two-sided decryption-success threshold.
The TFHE paper's own Assumption 3.11 (Independence Heuristic) posits
that the error coefficients of the TLWE/TGSW samples in every linear
combination considered are independent and σ-subgaussian, where σ is
the square root of their variance. Given that heuristic premise, the
resulting tail bound (`P(|X| > t) <= 2*exp(-t^2/(2*sigma^2))`) is a
proven consequence -- not an assumption of exact normality, unlike the
Gaussian estimate below. `confidence_threshold`
(`tfhe/utility/analysis/tracker_if.hpp`) computes the 99% threshold
straight from that sub-Gaussian tail bound, and this is what the test
actually checks `norm` against (`EXPECT_LE`). A Gaussian estimate
(`gaussian_estimate_for_max_of`,
`tfhe/utility/analysis/variance_noise.hpp`) is also computed and printed
alongside it purely for comparison -- treating the error sum as exactly
Gaussian rather than merely subgaussian is a further modeling
simplification standard in the field, but it is *not* something the
central limit theorem rigorously establishes at a finite,
not-asymptotically-large sample size like this one, so it is never the
basis for a pass/fail check here.

Bootstrapping discards the input ciphertext's own noise entirely and
replaces it with fresh noise derived only from the bootstrap key
(RLWE-side alpha) and the gadget decomposition (`Bg`, `l`), regardless
of how much noise the ciphertext going in carried:

| Context | n | N | Bg | l | alpha (RLWE) | predicted σ | 99% threshold (sub-Gaussian) | 99% threshold (Gaussian est.) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 128-bit security | 630 | 1024 | 16 | 7 | 2⁻²⁵ | 7.181×10⁻⁴ | 2.338×10⁻³ | 1.850×10⁻³ |

Computed by `VarianceNoisePolicy<GateBootstrap>`
(`tfhe/utility/analysis/variance_noise.hpp`) and printed by the test
itself as `predicted stddev` / `99% threshold (subG)` / `99% threshold
(Gauss est.)` on every run (`gate_bootstrap_test.cpp`'s
`VerifyCorrectness`); confirmed by building and running
`GateBootstrapCorrectnessTest` directly. The sub-Gaussian bound is
looser than the Gaussian estimate (it has to be, since it's a proven
worst case rather than a convenient approximation); both are well
under the 0.25 decryption margin, the threshold `GateBootstrap` must
clear to decode its own output correctly at all -- staying under 0.25
is a hard requirement, not a target. How far under it you actually
need to be is a separate question with no universal answer: it's
whatever precision your own use case demands, so choose your own
`n`/`N`/`Bg`/`l`/`alpha` accordingly. Every gate here ends in its own
`GateBootstrap` call (see the README's Supported Operations section --
`BinaryExpansion` key-switches back to Lwe between gates rather than
chaining leveled ops across a single bootstrap's output), so this
requirement applies independently to each per-gate bootstrap.

This single-shot confidence check is cheaper than a many-trial
statistical suite, at the cost of less power (see the comment above
`VerifyCorrectness` in `gate_bootstrap_test.cpp`); a proper statistical
test (many trials, checking the empirical error distribution against
the predicted one) is planned as a future addition to the test suite.
