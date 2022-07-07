
/*
 * (C) Crown Copyright 2022 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */



#include <cmath>
#include <string>
#include <vector>

#include "atlas/array.h"
#include "atlas/functionspace.h"

#include "mo/model2geovals_varchange.h"
#include "mo/utils.h"

#include "oops/util/Logger.h"

using atlas::array::make_view;
using atlas::idx_t;
using atlas::util::Config;

namespace mo {


void initField_rank2(atlas::Field & field, const double value_init)
{
  setUniformValue_rank2(field, value_init);
}


void setUniformValue_rank2(atlas::Field & field, const double value)
{
  auto ds_view = make_view<double, 2>(field);

  ds_view.assign(value);
}


bool evalTotalMassMoistAir(atlas::FieldSet & fields)
{
  oops::Log::trace() << "[evalTotalMassMoistAir()] starting ..." << std::endl;

  std::vector<std::string> fnames {"m_v", "m_ci", "m_cl", "m_r", "m_t"};
  checkFieldSetContent(fields, fnames);

  auto ds_m_v  = make_view<const double, 2>(fields["m_v"]);
  auto ds_m_ci = make_view<const double, 2>(fields["m_ci"]);
  auto ds_m_cl = make_view<const double, 2>(fields["m_cl"]);
  auto ds_m_r  = make_view<const double, 2>(fields["m_r"]);
  auto ds_m_t  = make_view<double, 2>(fields["m_t"]);

  auto fspace = fields["m_t"].functionspace();

  auto evaluateMt = [&] (idx_t i, idx_t j) {
    ds_m_t(i, j) = 1 + ds_m_v(i, j) + ds_m_ci(i, j) + ds_m_cl(i, j) + ds_m_r(i, j); };

  auto conf = Config("levels", fields["m_t"].levels()) |
              Config("include_halo", true);

  parallelFor(fspace, evaluateMt, conf);

  oops::Log::trace() << "[evalTotalMassMoistAir()] ... exit" << std::endl;

  return true;
}

/// \brief function to evaluate the quantity:
///   qx = m_x/m_t
/// where ...
///   m_x = [ mv | mci | mcl | m_r ]
///   m_t  = total mass of moist air
///
bool evalRatioToMt(atlas::FieldSet & fields, const std::vector<std::string> & vars)
{
  oops::Log::trace() << "[evalRatioToMt()] starting ..." << std::endl;

  // fields[0] = m_x = [ mv | mci | mcl | m_r ]
  auto ds_m_x  = make_view<const double, 2>(fields[vars[0]]);
  auto ds_m_t  = make_view<const double, 2>(fields[vars[1]]);
  auto ds_tfield  = make_view<double, 2>(fields[vars[2]]);

  auto fspace = fields[vars[1]].functionspace();

  auto evaluateRatioToMt = [&] (idx_t i, idx_t j) {
    ds_tfield(i, j) = ds_m_x(i, j) / ds_m_t(i, j); };

  auto conf = Config("levels", fields[1].levels()) |
              Config("include_halo", true);

  parallelFor(fspace, evaluateRatioToMt, conf);

  oops::Log::trace() << "[evalRatioToMt()] ... exit" << std::endl;

  return true;
}


bool evalSpecificHumidity(atlas::FieldSet & fields)
{
  oops::Log::trace() << "[evalSpecificHumidity()] starting ..." << std::endl;

  std::vector<std::string> fnames {"m_v", "m_t", "specific_humidity"};
  checkFieldSetContent(fields, fnames);

  bool rvalue = evalRatioToMt(fields, fnames);

  oops::Log::trace() << "[evalSpecificHumidity()] ... exit" << std::endl;

  return rvalue;
}

bool evalRelativeHumidity(atlas::FieldSet & fields)
{
  oops::Log::trace() << "[evalRelativeHumidity()] starting ..." << std::endl;

  std::vector<std::string> fnames {"specific_humidity",
                                   "qsat", "relative_humidity"};
  checkFieldSetContent(fields, fnames);

  bool cap_super_sat(false);

  if (fields["relative_humidity"].metadata().has("cap_super_sat")) {
    fields["relative_humidity"].metadata().get("cap_super_sat", cap_super_sat);
  }

  auto qView = make_view<const double, 2>(fields["specific_humidity"]);
  auto qsatView = make_view<const double, 2>(fields["qsat"]);
  auto rhView = make_view<double, 2>(fields["relative_humidity"]);

  auto conf = Config("levels", fields["relative_humidity"].levels()) |
              Config("include_halo", true);

  auto evaluateRH = [&] (idx_t i, idx_t j) {
    rhView(i, j) = fmax(qView(i, j)/qsatView(i, j) * 100.0, 0.0);
    rhView(i, j) = (cap_super_sat && (rhView(i, j) > 100.0)) ? 100.0 : rhView(i, j);
  };

  auto fspace = fields["relative_humidity"].functionspace();

  parallelFor(fspace, evaluateRH, conf);

  oops::Log::trace() << "[evalRelativeHumidity()] ... exit" << std::endl;

  return true;
}

bool evalTotalRelativeHumidity(atlas::FieldSet & fields)
{
  oops::Log::trace() << "[evalTotalRelativeHumidity()] starting ..." << std::endl;

  std::vector<std::string> fnames {"specific_humidity",
                                   "mass_content_of_cloud_liquid_water_in_atmosphere_layer",
                                   "mass_content_of_cloud_ice_in_atmosphere_layer", "qrain",
                                   "qsat", "rht"};
  checkFieldSetContent(fields, fnames);

  auto qView = make_view<const double, 2>(fields["specific_humidity"]);
  auto qclView = make_view<const double, 2>
                 (fields["mass_content_of_cloud_liquid_water_in_atmosphere_layer"]);
  auto qciView = make_view<const double, 2>
                 (fields["mass_content_of_cloud_ice_in_atmosphere_layer"]);
  auto qrainView = make_view<const double, 2>(fields["qrain"]);
  auto qsatView = make_view<const double, 2>(fields["qsat"]);
  auto rhtView = make_view<double, 2>(fields["rht"]);

  auto conf = Config("levels", fields["rht"].levels()) |
              Config("include_halo", true);

  auto evaluateRHT = [&] (idx_t i, idx_t j) {
    rhtView(i, j) = (qView(i, j)+qclView(i, j)+qciView(i, j)+qrainView(i, j))/qsatView(i, j)
                 * 100.0;
    if (rhtView(i, j) < 0.0) {rhtView(i, j) = 0.0;} };

  auto fspace = fields["rht"].functionspace();

  parallelFor(fspace, evaluateRHT, conf);

  oops::Log::trace() << "[evalTotalRelativeHumidity()] ... exit" << std::endl;

  return true;
}

bool evalMassCloudIce(atlas::FieldSet & fields)
{
  oops::Log::trace() << "[evalMassCloudIce()] starting ..." << std::endl;

  std::vector<std::string> fnames {"m_ci", "m_t",
                                   "mass_content_of_cloud_ice_in_atmosphere_layer"};
  checkFieldSetContent(fields, fnames);

  bool rvalue = evalRatioToMt(fields, fnames);

  oops::Log::trace() << "[evalMassCloudIce()] ... exit" << std::endl;

  return rvalue;
}


bool evalMassCloudLiquid(atlas::FieldSet & fields)
{
  oops::Log::trace() << "[evalMassCloudLiquid()] starting ..." << std::endl;

  std::vector<std::string> fnames {"m_cl", "m_t",
                                   "mass_content_of_cloud_liquid_water_in_atmosphere_layer"};
  checkFieldSetContent(fields, fnames);

  bool rvalue = evalRatioToMt(fields, fnames);

  oops::Log::trace() << "[evalMassCloudLiquid()] ... exit" << std::endl;

  return rvalue;
}


bool evalMassRain(atlas::FieldSet & fields)
{
  oops::Log::trace() << "[evalMassRain()] starting ..." << std::endl;

  std::vector<std::string> fnames {"m_r", "m_t", "qrain"};
  checkFieldSetContent(fields, fnames);

  bool rvalue = evalRatioToMt(fields, fnames);

  oops::Log::trace() << "[evalMassRain()] ... exit" << std::endl;

  return rvalue;
}


bool evalAirTemperature(atlas::FieldSet & fields)
{
  oops::Log::trace() << "[evalAirTemperature()] starting ..." << std::endl;

  std::vector<std::string> fnames {"theta", "exner", "air_temperature"};
  checkFieldSetContent(fields, fnames);

  auto ds_theta  = make_view<const double, 2>(fields["theta"]);
  auto ds_exner = make_view<const double, 2>(fields["exner"]);
  auto ds_atemp = make_view<double, 2>(fields["air_temperature"]);

  auto fspace = fields["air_temperature"].functionspace();

  auto evaluateAirTemp = [&] (idx_t i, idx_t j) {
    ds_atemp(i, j) = ds_theta(i, j) * ds_exner(i, j); };

  auto conf = Config("levels", fields["air_temperature"].levels()) |
              Config("include_halo", true);

  parallelFor(fspace, evaluateAirTemp, conf);

  oops::Log::trace() << "[evalAirTemperature()] ... exit" << std::endl;

  return true;
}


bool evalAirPressureLevels(atlas::FieldSet & fields)
{
  oops::Log::trace() << "[evalAirPressureLevels()] starting ..." << std::endl;

  auto ds_elmo = make_view<const double, 2>(fields["exner_levels_minus_one"]);
  auto ds_plmo = make_view<const double, 2>(fields["air_pressure_levels_minus_one"]);
  auto ds_t = make_view<const double, 2>(fields["theta"]);
  auto ds_hl = make_view<const double, 2>(fields["height_levels"]);
  auto ds_pl = make_view<double, 2>(fields["air_pressure_levels"]);

  idx_t levels(fields["air_pressure_levels"].levels());
  for (idx_t jn = 0; jn < fields["air_pressure_levels"].shape(0); ++jn) {
    for (idx_t jl = 1; jl < levels - 1; ++jl) {
      ds_pl(jn, jl) = ds_plmo(jn, jl);
    }

    ds_pl(jn, levels-1) =  Constants::p_zero * pow(
      ds_elmo(jn, levels-2) - (mo::Constants::grav * (ds_hl(jn, levels-1) - ds_hl(jn, levels-2))) /
      (mo::Constants::cp * ds_t(jn, levels-2)), (1.0 / Constants::rd_over_cp));

    ds_pl(jn, levels-1) = ds_pl(jn, levels-1) > 0.0 ? ds_pl(jn, levels-1) : mo::Constants::deps;
  }

  oops::Log::trace() << "[evalAirPressureLevels()] ... exit" << std::endl;

  return true;
}


bool evalSpecificHumidityFromRH_2m(atlas::FieldSet & fields)
{
  oops::Log::trace() << "[evalSpecificHumidityFromRH_2m()] starting ..." << std::endl;

  std::vector<std::string> fnames {"qsat",
                                   "relative_humidity_2m",
                                   "specific_humidity_at_two_meters_above_surface"};
  checkFieldSetContent(fields, fnames);

  auto ds_qsat = make_view<const double, 2>(fields["qsat"]);
  auto ds_rh = make_view<const double, 2>(fields["relative_humidity_2m"]);
  auto ds_q2m = make_view<double, 2>(
    fields["specific_humidity_at_two_meters_above_surface"]);

  auto fspace = fields["specific_humidity_at_two_meters_above_surface"].functionspace();

  auto evaluateSpecificHumidity_2m = [&] (idx_t i, idx_t j) {
    ds_q2m(i, j) = ds_rh(i, j) * ds_qsat(i, j); };

  auto conf = Config("levels",
    fields["specific_humidity_at_two_meters_above_surface"].levels()) |
              Config("include_halo", true);

  parallelFor(fspace, evaluateSpecificHumidity_2m, conf);

  oops::Log::trace() << "[evalSpecificHumidityFromRH_2m()] ... exit" << std::endl;

  return true;
}


bool evalParamAParamB(atlas::FieldSet & fields)
{
  oops::Log::trace() << "[evalParamAParamB2()] starting ..." << std::endl;

  std::vector<std::string> fnames {"height", "height_levels",
                                   "air_pressure_levels_minus_one",
                                   "specific_humidity",
                                   "param_a", "param_b"};
  checkFieldSetContent(fields, fnames);

  std::size_t blindex;
  if (!fields["height"].metadata().has("boundary_layer_index")) {
    oops::Log::error() << "ERROR - data validation failed "
                          "we expect boundary_layer_index value "
                          "in the meta data of the height field" << std::endl;
  }
  fields["height"].metadata().get("boundary_layer_index", blindex);

  auto heightView = make_view<const double, 2>(fields["height"]);
  auto heightLevelsView = make_view<const double, 2>(fields["height_levels"]);
  auto pressureLevelsView = make_view<const double, 2>(fields["air_pressure_levels_minus_one"]);
  auto specificHumidityView = make_view<const double, 2>(fields["specific_humidity"]);
  auto param_aView = make_view<double, 2>(fields["param_a"]);
  auto param_bView = make_view<double, 2>(fields["param_b"]);

  // temperature at level above boundary layer
  double t_bl;
  // temperature at model surface height
  double t_msh;

  double exp_pmsh = mo::Constants::Lclr * mo::Constants::rd / mo::Constants::grav;

  for (idx_t jn = 0; jn < param_aView.shape(0); ++jn) {
    t_bl = (-mo::Constants::grav / mo::Constants::rd) *
           (heightLevelsView(jn, blindex + 1) - heightLevelsView(jn, blindex)) /
           log(pressureLevelsView(jn, blindex + 1) / pressureLevelsView(jn, blindex));

    t_bl = t_bl / (1.0 + mo::Constants::c_virtual * specificHumidityView(jn, blindex));

    t_msh = t_bl + mo::Constants::Lclr * (heightView(jn, blindex) - heightLevelsView(jn, 0));

    param_aView(jn, 0) = heightLevelsView(jn, 0) + t_msh / mo::Constants::Lclr;
    param_bView(jn, 0) = t_msh / (pow(pressureLevelsView(jn, 0), exp_pmsh) * mo::Constants::Lclr);
  }

  oops::Log::trace() << "[evalParamAParamB()] ... exit" << std::endl;

  return true;
}

}  // namespace mo
