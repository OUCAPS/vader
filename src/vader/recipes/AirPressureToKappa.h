/*
 * (C) Copyright 203 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <map>
#include <string>
#include <vector>

#include "atlas/field/FieldSet.h"
#include "atlas/functionspace/FunctionSpace.h"
#include "oops/util/parameters/Parameter.h"
#include "oops/util/parameters/RequiredParameter.h"
#include "vader/RecipeBase.h"

namespace vader
{

// -------------------------------------------------------------------------------------------------

class AirPressureToKappa_AParameters : public RecipeParametersBase {
  OOPS_CONCRETE_PARAMETERS(AirPressureToKappa_AParameters, RecipeParametersBase)

 public:
    oops::RequiredParameter<std::string> name{"recipe name", this};
    oops::Parameter<double> kappa{"kappa", "kappa", 0.28571428571428570, this};
};

/*! \brief AirPressureToKappa_A class defines a recipe for pressure levels from pressure
           thickness.
 *
 *  \details This recipe uses pressure at the interfaces, along with the Phillips method to
 *           compute pressure at the mid points. It does not provide TL/AD algorithms.
 */
class AirPressureToKappa_A : public RecipeBase
{
 public:
    static const char Name[];
    static const std::vector<std::string> Ingredients;

    typedef AirPressureToKappa_AParameters Parameters_;

    explicit AirPressureToKappa_A(const Parameters_ &);

    std::string name() const override;
    std::string product() const override;
    std::vector<std::string> ingredients() const override;
    size_t productLevels(const atlas::FieldSet &) const override;
    atlas::FunctionSpace productFunctionSpace(const atlas::FieldSet &) const override;
    bool executeNL(atlas::FieldSet &) override;

 private:
    const double kappa_;
};

// -------------------------------------------------------------------------------------------------

}  // namespace vader
