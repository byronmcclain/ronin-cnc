# Release Checklist

Version: __________ Date: __________

## Pre-Release

### Build Verification
- [ ] Clean build succeeds on macOS
- [ ] No critical compiler warnings
- [ ] Universal binary includes both Intel and Apple Silicon
- [ ] All assets bundled correctly
- [ ] Version number updated in Info.plist

### Test Verification
- [ ] Unit tests pass (100%)
- [ ] Integration tests pass (100%)
- [ ] Visual tests pass (manual review)
- [ ] Performance tests meet targets
- [ ] Gameplay tests pass
- [ ] Regression tests pass (no regressions)

### Quality Verification
- [ ] No P0 (Critical) bugs open
- [ ] No P1 (High) bugs blocking release
- [ ] All P2+ bugs documented in KNOWN_ISSUES.md
- [ ] Code review completed for all changes
- [ ] No security vulnerabilities identified

### Hardware Testing
- [ ] Tested on Apple Silicon Mac
- [ ] Tested on Intel Mac
- [ ] Tested on minimum spec hardware
- [ ] Tested on macOS 11.0 (Big Sur)
- [ ] Tested on latest macOS

### Documentation
- [ ] README.md updated
- [ ] CHANGELOG.md updated with version
- [ ] KNOWN_ISSUES.md current
- [ ] HARDWARE_REQUIREMENTS.md accurate
- [ ] In-game help/credits updated

## Release Build

### Code Signing
- [ ] Developer ID certificate valid
- [ ] Entitlements file correct
- [ ] Bundle signed with hardened runtime
- [ ] Signature verified with codesign

### Notarization
- [ ] Submitted to Apple notary service
- [ ] Notarization approved
- [ ] Ticket stapled to bundle
- [ ] Gatekeeper accepts bundle

### Distribution
- [ ] DMG created
- [ ] DMG signed
- [ ] DMG tested on clean system
- [ ] SHA256 checksum recorded
- [ ] Upload to distribution server

## Post-Release

### Verification
- [ ] Download and install from distribution
- [ ] Verify Gatekeeper allows launch
- [ ] Basic functionality test
- [ ] Update checker works (if applicable)

### Announcement
- [ ] Release notes published
- [ ] GitHub release created
- [ ] Announcement posted

### Monitoring
- [ ] Crash reporting configured
- [ ] Analytics tracking (if applicable)
- [ ] Support channels prepared

---

## Sign-off

| Role | Name | Date | Signature |
|------|------|------|-----------|
| Developer | | | |
| QA | | | |
| Release Manager | | | |
