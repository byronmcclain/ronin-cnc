# Hardware Requirements

## Minimum Requirements

| Component | Requirement |
|-----------|-------------|
| **Operating System** | macOS 11.0 (Big Sur) or later |
| **Processor** | Any 64-bit Intel or Apple Silicon Mac |
| **Memory** | 4 GB RAM |
| **Storage** | 500 MB available space |
| **Graphics** | Metal-compatible GPU |
| **Display** | 1280x720 resolution |

## Recommended Requirements

| Component | Requirement |
|-----------|-------------|
| **Operating System** | macOS 13.0 (Ventura) or later |
| **Processor** | Apple M1 or Intel Core i5 |
| **Memory** | 8 GB RAM |
| **Storage** | 1 GB available space (SSD recommended) |
| **Graphics** | Apple Silicon GPU or dedicated Intel GPU |
| **Display** | Retina display |

## Supported Hardware

### Apple Silicon Macs
- MacBook Air (M1, M2, 2020+)
- MacBook Pro (M1, M1 Pro, M1 Max, M2, M3, 2020+)
- Mac Mini (M1, M2, 2020+)
- iMac (M1, M3, 2021+)
- Mac Studio (M1 Max, M1 Ultra, M2, 2022+)
- Mac Pro (M2 Ultra, 2023+)

### Intel Macs
- MacBook Pro (2015 or later)
- MacBook Air (2015 or later)
- iMac (2015 or later)
- Mac Mini (2014 or later)
- Mac Pro (2013 or later)

## Performance Notes

### Apple Silicon Performance
- Native Apple Silicon binary
- 60 FPS typical gameplay
- Efficient power usage
- Instant wake support

### Intel Performance
- Universal binary includes Intel support
- 60 FPS on 2015+ hardware
- May throttle on battery power
- SSD recommended for fast loading

## Compatibility Notes

1. **macOS Version**: macOS 11.0 (Big Sur) is the minimum supported version. Earlier versions are not supported.

2. **32-bit Macs**: Not supported. The application requires a 64-bit processor.

3. **Graphics**: Requires Metal graphics API. Macs without Metal support cannot run this application.

4. **Display Scaling**: Retina displays fully supported with high-resolution graphics.

5. **External Displays**: Supported at native resolution.

## Troubleshooting

### Application Won't Launch
- Verify macOS version is 11.0 or later
- Check Gatekeeper settings in Security & Privacy
- Right-click and select "Open" for first launch

### Poor Performance
- Close other applications
- Check for thermal throttling
- Verify SSD has sufficient free space
- Update to latest macOS version

### Graphics Issues
- Update macOS to latest version
- Reset NVRAM/PRAM
- Check for GPU driver updates

## Getting Help

If you experience issues:
1. Check [Known Issues](KNOWN_ISSUES.md)
2. Search existing [GitHub Issues](https://github.com/example/redalert-macos/issues)
3. Create a new issue with:
   - Mac model and year
   - macOS version
   - Description of the problem
   - Steps to reproduce
