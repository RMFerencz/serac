// Copyright (c) 2019-2021, Lawrence Livermore National Security, LLC and
// other Serac Project Developers. See the top-level LICENSE file for
// details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

/**
 * @file displacement_hyperelastic_integrator.hpp
 *
 * @brief The MFEM integrators for the displacement hyperelastic formulation
 */

#pragma once

#include "mfem.hpp"

#include <functional>

namespace serac::thermal::mfem_ext {

/**
 * @brief Integrator describing a nonlinear scalar reaction in the thermal conduction equation
 *
 */
class NonlinearReactionIntegrator : public mfem::NonlinearFormIntegrator {
public:
  /**
   * @brief The constructor for the Nonlinear Reaction Integrator
   *
   */
  explicit NonlinearReactionIntegrator(std::function<double(double)> reaction, std::function<double(double)> d_reaction)
      : reaction_(reaction), d_reaction_(d_reaction)
  {
  }
  NonlinearReactionIntegrator() = delete;

  /**
   * @brief The residual evaluation for the nonlinear reaction integrator
   *
   * @param[in] element The finite element to integrate
   * @param[in] basis_to_reference_transformation The element transformation operators
   * @param[in] state_vector The state vector to evaluate the residual
   * @param[out] residual The output residual
   */
  virtual void AssembleElementVector(const mfem::FiniteElement&   element,
                                     mfem::ElementTransformation& basis_to_reference_transformation,
                                     const mfem::Vector& state_vector, mfem::Vector& residual);

  /**
   * @brief Assemble the local gradient
   *
   * @param[in] element The finite element to integrate
   * @param[in] basis_to_reference_transformation The element transformation operators
   * @param[in] state_vector The state vector to evaluate the gradient
   * @param[out] elmat The output local gradient
   */
  virtual void AssembleElementGrad(const mfem::FiniteElement&   element,
                                   mfem::ElementTransformation& basis_to_reference_transformation,
                                   const mfem::Vector& state_vector, mfem::DenseMatrix& stiffness_matrix);

private:
  /**
   * @brief the reaction function q = q(T)
   *
   */
  std::function<double(double)> reaction_;

  /**
   * @brief the derivative of the reaction function dq = dq(T)/dT
   *
   */
  std::function<double(double)> d_reaction_;

  /**
   * @brief a working vector containing shape function evaluations
   *
   */
  mutable mfem::Vector shape_;
};

}  // namespace serac::thermal::mfem_ext