/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <map>
#include <string>
#include <vector>

#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/Parameter.h"
#include "RecipeBase.h"

// Recipe headers
#include "recipes/AirTemperature.h"
#include "recipes/PressureToDelP.h"
#include "recipes/TempToPTemp.h"
#include "recipes/TempToVTemp.h"

namespace vader {

class VaderParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(VaderParameters, Parameters)

 public:
  /// Vader's design is that it should not have any RequiredParameters,
  /// and neither should the individual Recipes.
  ///
  /// 'cookbook` defines the recipes that Vader's Recipe search algorithm will consider,
  /// and the order it will consider them.
  oops::Parameter<std::map<std::string, std::vector<std::string>>> cookbook{
    "cookbook",
    // Default VADER cookbook definition
    {{"potential_temperature",  {TempToPTemp::Name}},
     {"virtual_temperature",    {TempToVTemp::Name}},
     {"air_temperature",        {AirTemperature_A::Name}}
    //  {"air_pressure_thickness", {PressureToDelP::Name}}
    },
    this
  };
  /// 'recipeParams' is an optional Vader parameter which contains a
  /// a vector of RecipeParametersWrapper objects. Because Recipes should
  /// be designed without RequiredParameters, these Wrappers are needed
  /// only for Recipes for which the caller wishes to alter the Recipe's default
  /// configuration and/or behavior.
  oops::OptionalParameter<std::vector<RecipeParametersWrapper>> recipeParams{
     "recipe parameters",
     "Parameters to configure individual recipe functionality",
     this};
};

}  // namespace vader
