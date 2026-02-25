# Contributing to SnapRHI

Thank you for your interest in SnapRHI We welcome community contributions and appreciate your time and effort in helping improve the project. Before getting started, please take a moment to review these guidelines.

## Code of Conduct

We want this project to be a welcoming space for everyone. By contributing, you agree to follow our Code of Conduct and help keep the community respectful and inclusive.

## How to Contribute

We welcome contributions in various forms, including:

Bug fixes
New features
Documentation improvements
Tests and performance optimizations

To contribute, follow these steps:

Fork the repository and create a new branch.
Make your changes, ensuring they align with our coding standards (§4 below).
Run tests to ensure your changes do not break existing functionality.
Submit a pull request (PR) with a clear description of the changes.
A maintainer will review your PR, suggest any necessary changes, and merge it once approved.

### Commit Messages

Use clear and descriptive commit messages. Follow the conventional commit format when possible:

feat: Add new authentication method
fix: Resolve issue with session timeout
docs: Update README with new installation steps

## 3.  Contributor License Agreement

We do not require a signed Contributor License Agreement (CLA) to be on file. However, by contributing, you agree to license your submission as follows:

By submitting a contribution, you:

Represent and warrant that it is your original work, or you have sufficient rights to submit it under these terms and conditions.
Grant the SnapRHI project maintainers and users the right to use, modify, and distribute it under the MIT license (see LICENSE file); and
To the extent your contribution is covered by patents, grant a perpetual, worldwide, non-exclusive, royalty-free, irrevocable license to the SnapRHI maintainers and users to make, use, sell, offer for sale, import, and otherwise transfer your contribution as part of the project.
Will ensure that all third-party code in your contribution is MIT-compatible and properly attributed.
Where permitted by law, waive any moral rights in your contribution (e.g., the right to object to modifications). If such rights cannot be waived, you agree not to assert them in a way that interferes with the project’s use of your contribution.

## 4.  Coding Standards

To maintain a consistent codebase, please follow these guidelines:

- Use the existing coding style and conventions.
- Ensure all code changes are well-documented.
- Write tests for new features and bug fixes.
- Avoid introducing unnecessary dependencies.

### Code Formatting

The repository uses **clang-format** for C/C++/Objective-C and **pre-commit**
hooks to enforce style automatically. Set up the hooks once:

```bash
pip install pre-commit && pre-commit install
```

After this, every `git commit` will auto-format staged files. You can also
format manually at any time:

```bash
pre-commit run --all-files     # format everything
pre-commit run clang-format    # only C/C++ on staged files
```

See the [Build Guide — Code Formatting](docs/build.md#code-formatting) for
full details and IDE setup.

## 5.  Reporting Issues

If you find a bug or have a feature request, please open an issue and provide as much detail as possible:

Steps to reproduce (if applicable)
Expected and actual behavior
Suggested solutions (if any)

## 6.  Recognition

We use the All Contributors specification to recognize community members. If your contribution is merged, you will be added to the project's list of contributors. This includes contributions of all kinds—code, documentation, design, testing, and more.

Thank you for helping make SnapRHI better!
