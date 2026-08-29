# Security Policy

torqus is a from-scratch TFHE implementation that has **not** undergone an
independent third-party security audit (see [Security Status](README.md#security-status)
in the README). Treat it accordingly before relying on it for anything
security-critical, and please report suspected vulnerabilities responsibly.

## Supported Versions

torqus is pre-1.0. Only the latest [release](https://github.com/rysolis/torqus/releases)
is supported; fixes are not backported to older tags.

## Scope

In scope:

- Implementation bugs that weaken security guarantees the code claims to
  provide -- e.g. memory-safety issues, incorrect noise/parameter handling,
  key-management flaws.

Out of scope (but still welcome as a regular GitHub issue, not a private
report):

- Cryptanalytic weaknesses in the TFHE scheme itself, as opposed to this
  implementation of it -- that requires cryptographic review beyond what
  this repository can provide.
- Missing features or behavior that is documented as a known limitation
  (e.g. lack of independent audit itself).

## Reporting a Vulnerability

Please report suspected vulnerabilities privately using GitHub's
[Private Vulnerability Reporting](https://github.com/rysolis/torqus/security/advisories/new)
(Security tab → "Report a vulnerability") rather than a public issue or pull
request.

Include, if possible:

- The affected file(s)/function(s) and torqus version or commit.
- A minimal reproduction or proof of concept.
- The potential impact as you understand it.

## Response

This project is maintained on a best-effort basis without a dedicated
security team or a formal SLA. We aim to acknowledge reports within a
reasonable time and will coordinate on a disclosure timeline once a report
is triaged.
