# LEAP Simulation with DD4hep

This is a repository to translate the LEAP geometry and simulation from
https://github.com/JennPopp/leap_sims/ to DD4hep.

## Building

For building setup a Key4hep environment first via

``` bash
source /cvmfs/sw.hsf.org/key4hep/setup.sh
```

When building for the first time you have to run cmake via:

```bash
cmake -B build -S . -DCMAKE_INSTALL_PREFIX=$(pwd)/install -GNinja
```

Afterwards you can build the project via

```bash
cmake --build build
```

Whenever you change any of the c++ files and want to see if things still
compile, run this command again.

## Testing / running

For testing the geometry it is necessary to install it first via.

```bash
cmake --build build --target install
```

This step has to be repeated everytime any of the c++ files is changed. If
nothing changed, the command will simply report `nothing to do`, so it can
always be run in case you are unsure.

Once installed it is easiest to setup the rest of the environment via

``` bash
source install/bin/thisLEAP.sh
```

**This only has to be done once** (and everytime you enter a new shell).

Afterwards it should be possible to visualize the geometry via

```bash
ddsim --compactFile compact/LEAP_calorimeter.xml --runType vis
```
