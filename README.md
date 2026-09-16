# Calcium3d

## Getting started

### Cloning

This repository makes use of submodules, so make sure to use `git clone --recurse-submodules` to clone.

### Building

Make sure you are using :

* Visual Studio 2026, up-to-date ;
* Vulkan SDK, up-to-date.

Other dependencies are already included directly in the external/ folder.

**Always make sure to keep your graphics drivers updated !**

The build process outputs executables in the bin/ folder at the root of this repository. To keep things clean, make sure your game project (or the example app) is configured to use `$(SolutionDir)bin` as the working directory (Project properties, `Debugging` panel, `Working directory` option or, in french, `Débogage` -> `Répertoire de travail`)

## Running


## Documentation

See [docs/README.md](docs/README.md) to get started.

Those who want to browse the documentation more comfortably can use [MkDocs](https://www.mkdocs.org/) to build a static website.
