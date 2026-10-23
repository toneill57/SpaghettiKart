# MK64 Online fork of SpaghettiKart
Base: HarbourMasters/SpaghettiKart `5073d2e3636cdb2794f4c845ee3bdec5ac4718c4` (2026-10-11, "Remove jitter fix slider (#767)"), tag
`base-5073d2e`. Submodules: libultraship `60c9bb71`, torch `2d474ddb`, doxygen-awesome-css `9760c300`, fast64 `d89593c8`.
## Remotes, branches, tags, pin
- `origin` = https://github.com/toneill57/SpaghettiKart (GitHub fork, public; Actions disabled on the fork until Fase 0.6.3); `upstream` =
  https://github.com/HarbourMasters/SpaghettiKart.git — fetch only, never pushed to (upstreaming is Won't for v1).
- `main` mirrors upstream, never committed to, stays the default branch unless decided (`schedule:` workflows run only there).
- `netplay` = integration branch; work on `np/<fase>-<slug>`, merged by PR. `base-<sha7>` = annotated tag per upstream base;
  `git describe --tags` feeds `SPAGHETTI_VERSION` (`cmake/GetVersion.cmake`).
- vcpkg tool pin: `cmake/automate-vcpkg.cmake` checks out `VCPKG_PIN_COMMIT` (default = `vcpkg.json` builtin-baseline) on clone and
  reuse; `-DVCPKG_PIN_COMMIT=` restores upstream behaviour. Test: `cmake -P tests/cmake/vcpkg_pin_test.cmake`.
## Rebasing onto newer upstream — a decision (it invalidates the exploration file:lines), never a chore
    git fetch upstream --tags && git switch -c rebase/try-<date> netplay
    git rebase --onto <new-sha> base-5073d2e      # <new-sha> chosen explicitly, never upstream/main blindly
    git submodule update --init --recursive && cmake -P tests/cmake/vcpkg_pin_test.cmake
    # rebuild (docs/BUILD-MAC.md), re-run the determinism harness, tag base-<sha7>, fast-forward netplay
