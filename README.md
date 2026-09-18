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

### Visualizing the geometry

Afterwards it should be possible to visualize the geometry via

```bash
ddsim --compactFile compact/LEAP_calorimeter.xml --runType qt --macroFile vis.mac
```

### Shooting electrons into the cell

The particle gun has to be enabled on the command line. Without any generator
`/run/beamOn` aborts with
`ObjectExtensions::extension: The object has no extension of type ...`:

```bash
ddsim --compactFile compact/LEAP_calorimeter.xml --runType qt --macroFile vis.mac \
    --enableGun --gun.particle e- --gun.energy "1*GeV" \
    --gun.direction "0 0 1" --gun.position "0 0 -10*cm"
```

The gun fires from the origin by default, which is exactly the front face of
the crystal. Starting 10 cm upstream instead makes the incoming track visible
as well.

Then, in the `Session:` prompt of the Qt window:

```
/run/beamOn 5
```

`vis.mac` puts the viewer into `accumulate` mode, so the five showers pile up
in the same picture. Use `/vis/scene/endOfEventAction refresh` to look at one
event at a time.

The gun can also be reconfigured without restarting ddsim. Note that these
properties are in Geant4 units, i.e. mm and MeV, and do not accept the
`1*GeV` syntax of the command line options:

```
/ddg4/Gun/particle mu-
/ddg4/Gun/energy 500
/ddg4/Gun/position (0,0,-200)
/ddg4/Gun/show
/run/beamOn 1
```
