# Contributing to SEN66-Dosimetry

Thank you for your interest in contributing to SEN66-Dosimetry! This document provides guidelines for contributing to the project.

## Code of Conduct

By participating in this project, you agree to maintain a respectful and collaborative environment for all contributors.

## How to Contribute

### Reporting Bugs

Before creating a bug report, please check existing issues to avoid duplicates.

When filing a bug report, include:
- **Clear title and description**
- **Steps to reproduce** the issue
- **Expected vs actual behavior**
- **Environment details** (ESP32 board, PlatformIO version, sensor model)
- **Code samples** or minimal reproducible examples
- **Serial output** or error messages

### Suggesting Enhancements

Enhancement suggestions are tracked as GitHub issues. When creating an enhancement suggestion:
- Use a clear and descriptive title
- Provide detailed description of the proposed functionality
- Explain why this enhancement would be useful
- Include code examples if applicable

### Pull Requests

1. **Fork the repository** and create your branch from `main`
2. **Follow the code style** (see below)
3. **Test your changes** thoroughly on actual hardware
4. **Update documentation** as needed
5. **Add tests** if applicable
6. **Update CHANGELOG.md** with your changes

#### Code Style Guidelines

- Use meaningful variable and function names
- Follow existing indentation (4 spaces)
- Comment complex logic and calculations
- Keep functions focused and modular
- Add Doxygen-style comments for public APIs

Example:
```cpp
/**
 * @brief Calculate dew point temperature
 * @param temp Temperature in Celsius
 * @param humidity Relative humidity in percent
 * @return Dew point in Celsius
 */
float calculateDewPoint(float temp, float humidity);
```

#### Commit Message Guidelines

- Use present tense ("Add feature" not "Added feature")
- Use imperative mood ("Move cursor to..." not "Moves cursor to...")
- Keep first line under 72 characters
- Reference issues and pull requests when relevant

Example:
```
Add CO2 threshold alerting feature

- Implement configurable CO2 threshold
- Add callback mechanism for alerts
- Update documentation

Fixes #42
```

### Development Setup

1. Install PlatformIO
2. Clone your fork:
   ```bash
   git clone https://github.com/YOUR_USERNAME/SEN66-Dosimetry.git
   cd SEN66-Dosimetry
   ```
3. Create a branch:
   ```bash
   git checkout -b feature/your-feature-name
   ```
4. Make changes and test
5. Commit and push:
   ```bash
   git add .
   git commit -m "Description of changes"
   git push origin feature/your-feature-name
   ```
6. Create Pull Request on GitHub

## Testing

- Test on actual ESP32-S3 hardware with SEN66 sensor
- Verify all features work as expected
- Check for memory leaks with long-running tests
- Validate CSV output format
- Ensure TWA calculations are accurate

## Documentation

When adding new features:
- Update README.md with usage examples
- Add API documentation in header files
- Update CHANGELOG.md
- Consider adding examples in `examples/` directory

## Questions?

Feel free to open an issue for questions or reach out to the maintainers.

## License

By contributing, you agree that your contributions will be licensed under the MIT License.

Thank you for contributing to SEN66-Dosimetry! 🎉
