//==========================================================================
//  LEAP calorimeter -- DD4hep translation of leap_sims/src/Calorimeter.cc
//--------------------------------------------------------------------------
//
//  *** PARTIAL TRANSLATION -- STARTING POINT ONLY ***
//
//  Only the calorimeter mother volume and a single calorimeter cell are
//  built here.  One cell consists of the following nested volumes (outside
//  in), exactly as in Calorimeter::ConstructCalo():
//
//    cell envelope (air)
//      |- aluminium wrapping
//      |    `- thin air gap
//      |         `- lead glass crystal          <- sensitive
//      |- front virtual detector (vacuum)
//      `- back  virtual detector (vacuum)
//
//  The wrapping and the air gap are *closed at the front and open at the
//  back*: their z half length is only half a wrapping/gap thickness larger
//  than what they contain, and the daughter is shifted towards +z so that
//  the extra material ends up on the -z face only.  This reproduces the
//  placements of the original Geant4 code.
//
//  Still missing with respect to the original Geant4 geometry:
//    * the 3x3 crystal array.  The code below already loops over an
//      ncell_x * ncell_y grid, so setting ncell_x="3" ncell_y="3" in the
//      compact file is enough to get the array -- but the resulting id
//      scheme and the mother volume size should be reviewed.
//    * everything that the original builds for `[Calorimeter] type = full`:
//      the PEEK front plate with its nine holes and the two aluminium
//      top/bottom plates (a G4SubtractionSolid).
//    * the front/back virtual detectors are currently *passive*.  See the
//      comment at `place_cell()` below for why, and what to do about it.
//
//==========================================================================

#include <DD4hep/DetFactoryHelper.h>
#include <DD4hep/Printout.h>
#include <XML/Utilities.h>

#include <string>

using namespace dd4hep;

namespace {

/// Values for the `slice` field of the readout id descriptor.
enum CellSlice { SLICE_FRONT_DETECTOR = 0, SLICE_CRYSTAL = 1, SLICE_BACK_DETECTOR = 2 };

/// All dimensions of one calorimeter cell, derived from the primitive
/// parameters in the compact file the same way Calorimeter.cc does it.
struct CellDimensions {
  double crystal_xy{}, crystal_z{};
  double gap_xy{}, gap_z{}, gap_thickness{};
  double wrap_xy{}, wrap_z{}, wrap_thickness{};
  double front_thickness{}, back_thickness{};
  double cell_xy{}, cell_z{};
};

} // namespace

static Ref_t create_detector(Detector& description, xml_h handle, SensitiveDetector sens) {
  xml_det_t x_det = handle;
  const std::string det_name = x_det.nameStr();
  const int det_id = x_det.id();

  // The pieces one cell is built from. All of them are mandatory, so that a
  // typo in the compact file is reported instead of silently ignored.
  const xml_comp_t x_crystal = x_det.child(_Unicode(crystal));
  const xml_comp_t x_gap = x_det.child(_Unicode(airgap));
  const xml_comp_t x_wrap = x_det.child(_Unicode(wrapping));
  const xml_comp_t x_front = x_det.child(_Unicode(front_detector));
  const xml_comp_t x_back = x_det.child(_Unicode(back_detector));
  const xml_dim_t x_grid = x_det.child(_U(dimensions));

  //---------------------------------------------------------------------
  // dimensions, cf. the parameter block of Calorimeter::ConstructCalo()
  //---------------------------------------------------------------------
  CellDimensions dim;
  dim.crystal_xy = x_crystal.x();
  dim.crystal_z = x_crystal.z();
  dim.gap_thickness = x_gap.thickness();
  dim.wrap_thickness = x_wrap.thickness();
  dim.front_thickness = x_front.thickness();
  dim.back_thickness = x_back.thickness();

  dim.gap_xy = dim.crystal_xy + 2 * dim.gap_thickness;
  dim.gap_z = dim.crystal_z + dim.gap_thickness;
  dim.wrap_xy = dim.gap_xy + 2 * dim.wrap_thickness;
  dim.wrap_z = dim.gap_z + dim.wrap_thickness;
  dim.cell_xy = dim.wrap_xy;
  dim.cell_z = dim.wrap_z + dim.front_thickness + dim.back_thickness;

  // NOTE: xml_det_t::materialStr() expects a <material name="..."/> *child*
  // element, while all the sub-components below use a material *attribute*.
  // Read the attribute here as well to keep the compact file consistent.
  const std::string envelope_material = x_det.attr<std::string>(_U(material));

  const int n_cell_x = x_grid.attr<int>(_Unicode(ncell_x));
  const int n_cell_y = x_grid.attr<int>(_Unicode(ncell_y));
  if (n_cell_x < 1 || n_cell_y < 1) {
    except(det_name, "Need at least one cell in x and y, got ncell_x=%d, ncell_y=%d", n_cell_x, n_cell_y);
  }

  //---------------------------------------------------------------------
  // mother volume of the whole calorimeter
  //---------------------------------------------------------------------
  DetElement calo_det(det_name, det_id);

  // Selects the DDG4 sensitive action used for the sensitive volumes below
  // (`Geant4CalorimeterAction` with the ddsim defaults).
  if (sens.isValid()) {
    sens.setType("calorimeter");
  }

  Box mother_box(n_cell_x * dim.cell_xy / 2., n_cell_y * dim.cell_xy / 2., dim.cell_z / 2.);
  Volume mother_vol(det_name, mother_box, description.material(envelope_material));
  mother_vol.setVisAttributes(description, x_det.visStr());

  //---------------------------------------------------------------------
  // the cell(s). Each cell gets its own set of volumes, so that the
  // DetElement placement path of every sensitive volume stays unique.
  //---------------------------------------------------------------------
  for (int ix = 0; ix < n_cell_x; ++ix) {
    for (int iy = 0; iy < n_cell_y; ++iy) {
      const std::string cell_name = det_name + _toString(ix, "_cell%d") + _toString(iy, "_%d");

      Box cell_box(dim.cell_xy / 2., dim.cell_xy / 2., dim.cell_z / 2.);
      Volume cell_vol(cell_name, cell_box, description.material(envelope_material));
      cell_vol.setVisAttributes(description, x_det.visStr());

      //-----------------------------------------------------------------
      // reflective aluminium wrapping, closed towards -z, open towards +z
      //-----------------------------------------------------------------
      Box wrap_box(dim.wrap_xy / 2., dim.wrap_xy / 2., dim.wrap_z / 2.);
      Volume wrap_vol(cell_name + "_wrapping", wrap_box, description.material(x_wrap.materialStr()));
      wrap_vol.setVisAttributes(description, x_wrap.visStr());
      // The wrapping fills the cell envelope except for the two virtual
      // detector planes. It sits at z = 0 as long as both are equally
      // thick, which is the case in the original.
      cell_vol.placeVolume(wrap_vol, Position(0, 0, (dim.front_thickness - dim.back_thickness) / 2.));

      //-----------------------------------------------------------------
      // air gap between the wrapping and the crystal
      //-----------------------------------------------------------------
      Box gap_box(dim.gap_xy / 2., dim.gap_xy / 2., dim.gap_z / 2.);
      Volume gap_vol(cell_name + "_airgap", gap_box, description.material(x_gap.materialStr()));
      gap_vol.setVisAttributes(description, x_gap.visStr());
      wrap_vol.placeVolume(gap_vol, Position(0, 0, dim.wrap_thickness / 2.));

      //-----------------------------------------------------------------
      // the crystal itself (lead glass)
      //-----------------------------------------------------------------
      Box crystal_box(dim.crystal_xy / 2., dim.crystal_xy / 2., dim.crystal_z / 2.);
      Volume crystal_vol(cell_name + "_crystal", crystal_box, description.material(x_crystal.materialStr()));
      crystal_vol.setVisAttributes(description, x_crystal.visStr());
      if (x_crystal.isSensitive()) {
        crystal_vol.setSensitiveDetector(sens);
      }
      PlacedVolume crystal_pv = gap_vol.placeVolume(crystal_vol, Position(0, 0, dim.gap_thickness / 2.));
      crystal_pv.addPhysVolID("slice", SLICE_CRYSTAL);

      //-----------------------------------------------------------------
      // virtual detectors in front of and behind the crystal.
      //
      // In the original these are scoring planes in vacuum that record
      // the particles entering/leaving a cell, i.e. tracker-like
      // sensitive detectors. A DD4hep subdetector has exactly *one*
      // SensitiveDetector (and hence one readout and one DDG4 sensitive
      // action), which here is of type `calorimeter`. A calorimeter
      // action only sees energy deposits, of which there are none in
      // vacuum, so marking these volumes sensitive as-is would not give
      // any hits. They are therefore built as passive volumes;
      // `sensitive="true"` in the compact file is honoured should the
      // readout be changed. To get the original behaviour they need to
      // become a separate subdetector of type `tracker` with its own
      // readout.
      //-----------------------------------------------------------------
      Box front_box(dim.crystal_xy / 2., dim.crystal_xy / 2., dim.front_thickness / 2.);
      Volume front_vol(cell_name + "_front_det", front_box, description.material(x_front.materialStr()));
      front_vol.setVisAttributes(description, x_front.visStr());
      if (x_front.isSensitive()) {
        front_vol.setSensitiveDetector(sens);
      }
      PlacedVolume front_pv =
          cell_vol.placeVolume(front_vol, Position(0, 0, -dim.cell_z / 2. + dim.front_thickness / 2.));
      front_pv.addPhysVolID("slice", SLICE_FRONT_DETECTOR);

      Box back_box(dim.crystal_xy / 2., dim.crystal_xy / 2., dim.back_thickness / 2.);
      Volume back_vol(cell_name + "_back_det", back_box, description.material(x_back.materialStr()));
      back_vol.setVisAttributes(description, x_back.visStr());
      if (x_back.isSensitive()) {
        back_vol.setSensitiveDetector(sens);
      }
      PlacedVolume back_pv = cell_vol.placeVolume(back_vol, Position(0, 0, dim.cell_z / 2. - dim.back_thickness / 2.));
      back_pv.addPhysVolID("slice", SLICE_BACK_DETECTOR);

      //-----------------------------------------------------------------
      // place the cell into the calorimeter mother volume
      //-----------------------------------------------------------------
      const double x_pos = (ix - (n_cell_x - 1) / 2.) * dim.cell_xy;
      const double y_pos = (iy - (n_cell_y - 1) / 2.) * dim.cell_xy;
      PlacedVolume cell_pv = mother_vol.placeVolume(cell_vol, Position(x_pos, y_pos, 0));
      cell_pv.addPhysVolID("cell_x", ix).addPhysVolID("cell_y", iy);

      DetElement cell_det(calo_det, cell_name, ix * n_cell_y + iy);
      cell_det.setPlacement(cell_pv);
    }
  }

  //---------------------------------------------------------------------
  // place the calorimeter into the world
  //---------------------------------------------------------------------
  Volume mother = description.pickMotherVolume(calo_det);

  Position det_pos(0, 0, 0);
  if (x_det.hasChild(_U(position))) {
    const xml_dim_t x_det_pos = x_det.child(_U(position));
    det_pos = Position(x_det_pos.x(0.), x_det_pos.y(0.), x_det_pos.z(0.));
  }
  // The original rotates the whole calorimeter by [Calorimeter] xRot / yRot
  RotationZYX det_rot(0, 0, 0);
  if (x_det.hasChild(_U(rotation))) {
    const xml_dim_t x_det_rot = x_det.child(_U(rotation));
    det_rot = RotationZYX(x_det_rot.z(0.), x_det_rot.y(0.), x_det_rot.x(0.));
  }

  PlacedVolume calo_pv = mother.placeVolume(mother_vol, Transform3D(det_rot, det_pos));
  calo_pv.addPhysVolID("system", det_id);
  calo_det.setPlacement(calo_pv);

  xml::setDetectorTypeFlag(handle, calo_det);

  printout(INFO, det_name,
           "Built %d x %d calorimeter cell(s): cell %.3f x %.3f x %.3f cm, "
           "crystal %.3f x %.3f x "
           "%.3f cm",
           n_cell_x, n_cell_y, dim.cell_xy / cm, dim.cell_xy / cm, dim.cell_z / cm, dim.crystal_xy / cm,
           dim.crystal_xy / cm, dim.crystal_z / cm);

  return calo_det;
}

DECLARE_DETELEMENT(LEAP_Calorimeter, create_detector)
