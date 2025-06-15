Cycles Renderer
===============

[Cycles](https://www.cycles-renderer.org) is a physically based production renderer developed by the Blender project. This repository is a fork that contains the changes to Cycles required to integrate it into [Geekbench](https://www.geekbench.com/), Primate Labs' cross-platform benchmark.

## Building

Cycles can be built as a standalone application or a Hydra render delegate. See [BUILDING.md](BUILDING.md) for instructions.

## Examples

The repository contains example xml scenes which could be used for testing.

Example usage:

    ./cycles scene_monkey.xml

You can also use optional parameters (see `./cycles --help`), like:

    ./cycles --samples 100 --output ./image.png scene_monkey.xml

For the OSL scene you need to enable the OSL shading system:

    ./cycles --shadingsys osl scene_osl_stripes.xml

## Contact

For help building or running Cycles, see the channels listed here:

https://www.cycles-renderer.org/development/
