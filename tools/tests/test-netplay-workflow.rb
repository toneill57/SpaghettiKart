# Asserts the structure of .github/workflows/netplay-macos.yml (FASE 0.6). Ruby stdlib only.
# Usage: ruby tools/tests/test-netplay-workflow.rb [WORKFLOW.yml]   Exit: 0 ok · 1 a named contract line failed.
require 'yaml'
d = YAML.load_file(ARGV[0] || File.expand_path('../../.github/workflows/netplay-macos.yml', __dir__))
on = d[true] || d['on'] # YAML 1.1 parses the key `on` as boolean true
job = d.fetch('jobs').fetch('build-macos-arm64'); steps = job.fetch('steps')
runs = steps.map { |s| s['run'].to_s }.join("\n")
fail_with = ->(m) { warn "FAIL: #{m}"; exit 1 }
fail_with.('push branches') unless on['push']['branches'] == ['netplay', 'np/**']
fail_with.('pull_request branches') unless on['pull_request']['branches'] == ['netplay']
fail_with.('runs-on') unless job['runs-on'] == 'macos-latest'
fail_with.('no needs: allowed (no generate-port-o2r job)') if job.key?('needs')
fail_with.('configure line') unless runs.include?('cmake -H. -Bbuild-cmake -GNinja -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release -DENABLE_VCPKG=ON 2>&1')
fail_with.('build line') unless runs.include?('cmake --build build-cmake --config Release -j3')
fail_with.('-DVCPKG_ROOT must not be passed in CI (OQ8)') if runs.include?('-DVCPKG_ROOT')
fail_with.('no unpinned controller db') if runs.include?('gamecontrollerdb')
fail_with.('uname -m step') unless runs.include?('uname -m')
fail_with.('identity must opt out of mk64.o2r (no ROM in CI)') unless runs.include?('tools/build-identity.sh') && runs.include?('--no-mk64')
co = steps.find { |s| s['uses'].to_s.start_with?('actions/checkout@') }
fail_with.('checkout options') unless co && co['with']['fetch-depth'] == 0 && co['with']['fetch-tags'] == true && co['with']['submodules'] == 'recursive'
up = steps.find { |s| s['uses'].to_s.start_with?('actions/upload-artifact@') }
fail_with.('retention 7') unless up && up['with']['retention-days'] == 7
fail_with.('cache key on vcpkg.json') unless steps.any? { |s| s['uses'].to_s.start_with?('actions/cache@') && s['with']['key'].to_s.include?("hashFiles('vcpkg.json')") }
puts 'workflow structure ok'
