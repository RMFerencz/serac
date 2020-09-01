// Copyright (c) 2019, Lawrence Livermore National Security, LLC and
// other Serac Project Developers. See the top-level LICENSE file for
// details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

#include "thermal_structural_solver.hpp"

#include "common/logger.hpp"
#include "common/serac_types.hpp"

namespace serac {

constexpr int NUM_FIELDS = 3;

ThermalStructuralSolver::ThermalStructuralSolver(int order, std::shared_ptr<mfem::ParMesh> mesh)
    : BaseSolver(mesh, NUM_FIELDS, order), therm_solver_(order, mesh), solid_solver_(order, mesh)
{
  temperature_  = therm_solver_.temperature();
  velocity_     = solid_solver_.velocity();
  displacement_ = solid_solver_.displacement();

  state_[0] = temperature_;
  state_[1] = velocity_;
  state_[2] = displacement_;

  coupling_ = serac::CouplingScheme::OperatorSplit;
}

void ThermalStructuralSolver::completeSetup()
{
  SLIC_ERROR_ROOT_IF(coupling_ != serac::CouplingScheme::OperatorSplit, mpi_rank_,
                     "Only operator split is currently implemented in the thermal structural solver.");

  therm_solver_.completeSetup();
  solid_solver_.completeSetup();
}

void ThermalStructuralSolver::setTimestepper(serac::TimestepMethod timestepper)
{
  timestepper_ = timestepper;
  therm_solver_.setTimestepper(timestepper);
  solid_solver_.setTimestepper(timestepper);
}

// Advance the timestep
void ThermalStructuralSolver::advanceTimestep(double& dt)
{
  if (coupling_ == serac::CouplingScheme::OperatorSplit) {
    double initial_dt = dt;
    therm_solver_.advanceTimestep(dt);
    solid_solver_.advanceTimestep(dt);
    SLIC_ERROR_ROOT_IF(std::abs(dt - initial_dt) > 1.0e-6, mpi_rank_,
                       "Operator split coupled solvers cannot adaptively change the timestep");
  } else {
    SLIC_ERROR_ROOT(mpi_rank_, "Only operator split coupling is currently implemented");
  }

  cycle_ += 1;
}

}  // namespace serac
