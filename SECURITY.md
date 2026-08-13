# Security policy

## Supported versions

Until multiple maintained release lines exist, security fixes target current `main` and the latest stable `1.x` line. Older snapshots are not maintained independently.

## Reporting a vulnerability

Do not publish exploit details, proof-of-concept inputs, or sensitive crash traces in a public issue before coordinated disclosure.

Use GitHub private vulnerability reporting if the repository exposes it. If no private channel is available, open a minimal public issue requesting a private security contact channel and omit exploit details until one is established.

Useful reports include:

- affected commit/version, compiler/platform, algorithm, and input shape;
- minimal triggering comparator/projection/input sequence;
- sanitizer/crash diagnostics;
- whether the problem requires invalid comparator/domain preconditions;
- concrete impact and attacker-controlled prerequisites;
- proposed mitigation if available.

## Security-relevant scope

Examples include memory-safety defects, iterator/range misuse under documented valid preconditions, integer overflow in distribution algorithms, out-of-bounds network/index logic, undefined behavior reachable through public APIs, and algorithmic denial-of-service behavior that violates the documented complexity contract under valid inputs.

Ordinary performance regressions, benchmark-ranking disputes, and behavior caused solely by comparators that violate required ordering contracts are not security vulnerabilities unless a concrete security impact is demonstrated.

## Disclosure

Please allow time to reproduce, fix, run correctness/sanitizer coverage, and prepare a release before public disclosure. Release notes should identify the affected algorithm/API and any validation limitations.
