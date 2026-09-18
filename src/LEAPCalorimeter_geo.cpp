//==========================================================================
//  LEAP calorimeter -- DD4hep translation of leap_sims/src/Calorimeter.cc
//--------------------------------------------------------------------------
//
//  *** MINIMAL STARTING POINT ***
//
//  A single calorimeter cell, i.e. only the innermost part of
//  Calorimeter::ConstructCalo():
//
//    aluminium wrapping
//      `- thin air gap
//           `- lead glass crystal              <- sensitive
//
//  The wrapping and the air gap are *closed at the front and open at the
//  back*: their z half length is only half a wrapping/gap thickness larger
//  than what they contain, and the daughter is shifted towards +z so that
//  the extra material ends up on the -z face only.  This reproduces the
//  placements of the original Geant4 code.
//
//  The original wraps this into two more air volumes (logicCaloCell inside
//  logicCaloMother).  With a single cell and no virtual detectors both of
//  them have exactly the size of the wrapping, so they are left out here and
//  the wrapping is placed into the world directly.
//
//  Everything else is deliberately missing and meant to be added one step at
//  a time:
//    * the 3x3 crystal array ([Calorimeter] nCrystals = 9), which needs the
//      two air volumes mentioned above back
//    * the housing built for `[Calorimeter] type = full`: the PEEK front
//      plate with its nine holes and the two aluminium top/bottom plates
//    * the position relative to the solenoid ([Calorimeter] dist2Pol) and
//      the [Calorimeter] xRot / yRot rotation
//
//==========================================================================

#include <DD4hep/DetFactoryHelper.h>
#include <DD4hep/Printout.h>
#include <XML/Utilities.h>

#include <string>

using namespace dd4hep;

static Ref_t create_detector(Detector& description, xml_h handle, SensitiveDetector sens) {
  xml_det_t x_det = handle;
  const std::string det_name = x_det.nameStr();
  const int det_id = x_det.id();

  // The three volumes the cell is built from. All of them are mandatory, so
  // that a typo in the compact file is reported instead of silently ignored.
  const xml_comp_t x_wrap = x_det.child(_Unicode(wrapping));
  const xml_comp_t x_gap = x_det.child(_Unicode(airgap));
  const xml_comp_t x_crystal = x_det.child(_Unicode(crystal));

  //---------------------------------------------------------------------
  // dimensions, cf. the parameter block of Calorimeter::ConstructCalo()
  //---------------------------------------------------------------------
  const double crystal_xy = x_crystal.x();
  const double crystal_z = x_crystal.z();
  const double gap_thickness = x_gap.thickness();
  const double wrap_thickness = x_wrap.thickness();

  const double gap_xy = crystal_xy + 2 * gap_thickness;
  const double gap_z = crystal_z + gap_thickness;
  const double wrap_xy = gap_xy + 2 * wrap_thickness;
  const double wrap_z = gap_z + wrap_thickness;

  DetElement calo_det(det_name, det_id);

  // Selects the DDG4 sensitive action used for the crystal below
  // (`Geant4ScintillatorCalorimeterAction` with the ddsim defaults).
  if (sens.isValid()) {
    sens.setType("calorimeter");
  }

  //---------------------------------------------------------------------
  // reflective aluminium wrapping, closed towards -z, open towards +z.
  // This is the outermost volume of the cell, i.e. the one that gets
  // placed into the world below.
  //---------------------------------------------------------------------
  Box wrap_box(wrap_xy / 2., wrap_xy / 2., wrap_z / 2.);
  Volume wrap_vol(det_name, wrap_box, description.material(x_wrap.materialStr()));
  wrap_vol.setVisAttributes(description, x_wrap.visStr());

  //---------------------------------------------------------------------
  // air gap between the wrapping and the crystal
  //---------------------------------------------------------------------
  Box gap_box(gap_xy / 2., gap_xy / 2., gap_z / 2.);
  Volume gap_vol(det_name + "_airgap", gap_box, description.material(x_gap.materialStr()));
  gap_vol.setVisAttributes(description, x_gap.visStr());
  wrap_vol.placeVolume(gap_vol, Position(0, 0, wrap_thickness / 2.));

  //---------------------------------------------------------------------
  // the crystal itself (lead glass)
  //---------------------------------------------------------------------
  Box crystal_box(crystal_xy / 2., crystal_xy / 2., crystal_z / 2.);
  Volume crystal_vol(det_name + "_crystal", crystal_box, description.material(x_crystal.materialStr()));
  crystal_vol.setVisAttributes(description, x_crystal.visStr());
  if (x_crystal.isSensitive()) {
    crystal_vol.setSensitiveDetector(sens);
  }
  gap_vol.placeVolume(crystal_vol, Position(0, 0, gap_thickness / 2.));

  //---------------------------------------------------------------------
  // place the cell into the world
  //---------------------------------------------------------------------
  Volume mother = description.pickMotherVolume(calo_det);

  Position det_pos(0, 0, 0);
  if (x_det.hasChild(_U(position))) {
    const xml_dim_t x_det_pos = x_det.child(_U(position));
    det_pos = Position(x_det_pos.x(0.), x_det_pos.y(0.), x_det_pos.z(0.));
  }

  PlacedVolume calo_pv = mother.placeVolume(wrap_vol, det_pos);
  calo_pv.addPhysVolID("system", det_id);
  calo_det.setPlacement(calo_pv);

  xml::setDetectorTypeFlag(handle, calo_det);

  printout(INFO, det_name, "Built one calorimeter cell: %.3f x %.3f x %.3f cm, crystal %.3f x %.3f x %.3f cm",
           wrap_xy / cm, wrap_xy / cm, wrap_z / cm, crystal_xy / cm, crystal_xy / cm, crystal_z / cm);

  return calo_det;
}

DECLARE_DETELEMENT(LEAP_Calorimeter, create_detector)
