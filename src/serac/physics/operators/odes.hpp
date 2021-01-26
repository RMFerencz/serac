// Copyright (c) 2019-2020, Lawrence Livermore National Security, LLC and
// other Serac Project Developers. See the top-level LICENSE file for
// details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

#pragma once

#include <functional>

#include "mfem.hpp"

#include "serac/physics/utilities/boundary_condition_manager.hpp"
#include "serac/physics/utilities/equation_solver.hpp"

namespace serac::mfem_ext {

/**
 * @brief SecondOrderODE is a class wrapping mfem::SecondOrderTimeDependentOperator
 *   so that the user can use std::function to define the implementations of
 *   mfem::SecondOrderTimeDependentOperator::Mult and
 *   mfem::SecondOrderTimeDependentOperator::ImplicitSolve
 *
 * The main benefit of this approach is that lambda capture lists allow
 * for a flexible inline representation of the overloaded functions,
 * without having to manually define a separate functor class.
 */
class SecondOrderODE : public mfem::SecondOrderTimeDependentOperator {
public:
  /**
   * @brief A set of references to physics-module-owned variables
   * used by the residual operator
   */
  struct State {
    /**
     * @brief Current time step
     */
    double& c0;

    /**
     * @brief Previous value of dt
     */
    double& c1;

    /**
     * @brief Predicted true DOFs
     */
    mfem::Vector& u;

    /**
     * @brief  Predicted du_dt
     */
    mfem::Vector& du_dt;

    /**
     * @brief Previous value of d^2u_dt^2
     */
    mfem::Vector& d2u_dt2;
  };

  /**
   * @brief Constructor defining the size and specific system of ordinary differential equations to be solved
   *
   * @param[in] n The number of components in each vector of the ODE
   * @param[in] state The collection of references to input/output variables from the physics module
   * @param[in] solver The solver that operates on the residual
   * @param[in] bcs The set of Dirichlet conditions to enforce
   *
   * Implements mfem::SecondOrderTimeDependentOperator::Mult and mfem::SecondOrderTimeDependentOperator::ImplicitSolve
   *      (described in more detail here:
   * https://mfem.github.io/doxygen/html/classmfem_1_1SecondOrderTimeDependentOperator.html)
   *
   * where
   *
   * mfem::SecondOrderTimeDependentOperator::Mult corresponds to the case where fac0, fac1 are both zero
   * mfem::SecondOrderTimeDependentOperator::ImplicitSolve corresponds to the case where either of fac0, fac1 are
   * nonzero
   *
   */
  SecondOrderODE(int n, State&& state, const EquationSolver& solver, const BoundaryConditionManager& bcs);

  /**
   * @brief Solves the equation d2u_dt2 = f(u, du_dt, t)
   *
   * @param[in] u The true DOFs
   * @param[in] du_dt The first time derivative of u
   * @param[out] d2u_dt2 The second time derivative of u
   */
  void Mult(const mfem::Vector& u, const mfem::Vector& du_dt, mfem::Vector& d2u_dt2) const
  {
    Solve(t, 0.0, 0.0, u, du_dt, d2u_dt2);
  }

  /**
   * @brief Solves the equation d2u_dt2 = f(u + 1/2 c1^2 * d2u_dt2, du_dt + c1 * d2u_dt2, t)
   *
   * @param[in] c0 The current time step
   * @param[in] c1 The previous time step
   * @param[in] u The true DOFs
   * @param[in] du_dt The first time derivative of u
   * @param[out] d2u_dt2 The second time derivative of u
   */
  void ImplicitSolve(const double c0, const double c1, const mfem::Vector& u, const mfem::Vector& du_dt,
                     mfem::Vector& d2u_dt2)
  {
    Solve(t, c0, c1, u, du_dt, d2u_dt2);
  }

  /**
     The FirstOrder recast that can be used by a first order ode solver
   */
  void ImplicitSolve(const double dt, const mfem::Vector& u, mfem::Vector& du_dt);

  /**
   * @brief Configures the Dirichlet enforcement method to use
   * @param[in] method The selected method
   */
  void SetEnforcementMethod(const DirichletEnforcementMethod method) { enforcement_method_ = method; }

  /**
   * @brief Set the time integration method
   *
   * @param[in] timestepper The timestepping method for the solver
   */
  void SetTimestepper(const serac::TimestepMethod timestepper);

  /**
   * @brief Performs a time step
   *
   * @param[inout] x The predicted solution
   * @param[inout] dxdt The predicted rate
   * @param[inout] t The current time
   * @param[inout] dt The desired time step
   *
   * @see mfem::SecondOrderODESolver::Step
   */
  void Step(mfem::Vector& x, mfem::Vector& dxdt, double& t, double& dt);

private:
  /**
   * @brief Internal implementation used for mfem::SOTDO::Mult and mfem::SOTDO::ImplicitSolve
   * @param[in] t The current time
   * @param[in] c0 The current time step
   * @param[in] c1 The previous time step
   * @param[in] u The true DOFs
   * @param[in] du_dt The first time derivative of u
   * @param[out] d2u_dt2 The second time derivative of u
   */
  void Solve(const double t, const double c0, const double c1, const mfem::Vector& u, const mfem::Vector& du_dt,
             mfem::Vector& d2u_dt2) const;

  /**
   * @brief Set of references to external variables used by residual operator
   */
  State state_;
  /**
   * @brief The method of enforcing time-varying dirichlet boundary conditions
   */
  DirichletEnforcementMethod enforcement_method_ = serac::DirichletEnforcementMethod::RateControl;
  /**
   * @brief Reference to the equationsolver used to solve for d2u_dt2
   */
  const EquationSolver& solver_;
  /**
   * @brief MFEM solver object for second-order ODEs
   */
  std::unique_ptr<mfem::SecondOrderODESolver> second_order_ode_solver_;

  /**
   * @brief MFEM solver object for second-order ODEs recast as first order
   */
  std::unique_ptr<mfem::ODESolver> first_order_system_ode_solver_;

  /**
   * @brief Reference to boundary conditions used to constrain the solution
   */
  const BoundaryConditionManager& bcs_;
  mfem::Vector                    zero_;

  /**
   * @brief Working vectors for ODE outputs prior to constraint enforcement
   */
  mutable mfem::Vector U_minus_;
  mutable mfem::Vector U_;
  mutable mfem::Vector U_plus_;
  mutable mfem::Vector dU_dt_;
  mutable mfem::Vector d2U_dt2_;
};

/**
 * @brief FirstOrderODE is a class wrapping mfem::TimeDependentOperator
 *   so that the user can use std::function to define the implementations of
 *   mfem::TimeDependentOperator::Mult and
 *   mfem::TimeDependentOperator::ImplicitSolve
 *
 * The main benefit of this approach is that lambda capture lists allow
 * for a flexible inline representation of the overloaded functions,
 * without having to manually define a separate functor class.
 */
class FirstOrderODE : public mfem::TimeDependentOperator {
public:
  /**
   * @brief A set of references to physics-module-owned variables
   * used by the residual operator
   */
  struct State {
    /**
     * @brief Predicted true DOFs
     */
    mfem::Vector& u;

    /**
     * @brief Current time step
     */
    double& dt;

    /**
     * @brief Previous value of du_dt
     */
    mfem::Vector& du_dt;

    /**
     * @brief Previous value of dt
     */
    double& previous_dt;
  };

  /**
   * @brief Constructor defining the size and specific system of ordinary differential equations to be solved
   *
   * @param[in] n The number of components in each vector of the ODE
   * @param[in] f The function that describing how to solve for the first derivative, given the current state.
   *    The two functions
   *
   * Implements mfem::TimeDependentOperator::Mult and mfem::TimeDependentOperator::ImplicitSolve
   *      (described in more detail here: https://mfem.github.io/doxygen/html/classmfem_1_1TimeDependentOperator.html)
   *
   * where
   *
   * mfem::TimeDependentOperator::Mult corresponds to the case where dt is zero
   * mfem::TimeDependentOperator::ImplicitSolve corresponds to the case where dt is nonzero
   *
   */
  FirstOrderODE(int n, FirstOrderODE::State&& state, const EquationSolver& solver, const BoundaryConditionManager& bcs);

  /**
   * @brief Recasts a second order equation as a set of first order equations
   *
   * @param[in] n The number of components in each vector of the ODE
   * @param[in] state The SecondOrderODE state
   * @param[in] solver The equation solver for the residual equation that is solved at the next time step
   * @param[in] bcs boundary conditions for the ode
   *
   */
  FirstOrderODE(int n, SecondOrderODE& ode2, const EquationSolver& solver, const BoundaryConditionManager& bcs);

  /**
   * @brief Solves the equation du_dt = f(u, t)
   *
   * @param[in] u The true DOFs
   * @param[in] du_dt The first time derivative of u
   */
  void Mult(const mfem::Vector& u, mfem::Vector& du_dt) const { Solve(0.0, u, du_dt); }

  /**
   * @brief Solves the equation du_dt = f(u + dt * du_dt, t)
   *
   * @param[in] dt The time step
   * @param[in] u The true DOFs
   * @param[in] du_dt The first time derivative of u
   */
  void ImplicitSolve(const double dt, const mfem::Vector& u, mfem::Vector& du_dt) { Solve(dt, u, du_dt); }

  /**
   * @brief Configures the Dirichlet enforcement method to use
   * @param[in] method The selected method
   */
  void SetEnforcementMethod(const DirichletEnforcementMethod method) { enforcement_method_ = method; }

  /**
   * @brief Set the time integration method
   *
   * @param[in] timestepper The timestepping method for the solver
   */
  void SetTimestepper(const serac::TimestepMethod timestepper);

  /**
   * @brief Performs a time step
   *
   * @param[inout] x The predicted solution
   * @param[inout] t The current time
   * @param[inout] dt The desired time step
   *
   * @see mfem::ODESolver::Step
   */
  void Step(mfem::Vector& x, double& t, double& dt)
  {
    if (ode_solver_) {
      ode_solver_->Step(x, t, dt);
    } else {
      SLIC_ERROR("ode_solver_ unspecified");
    }
  }

  /**
   * @brief Internal implementation used for mfem::TDO::Mult and mfem::TDO::ImplicitSolve
   * @param[in] dt The time step
   * @param[in] u The true DOFs
   * @param[in] du_dt The first time derivative of u
   */
  virtual void Solve(const double dt, const mfem::Vector& u, mfem::Vector& du_dt) const;

private:
  /**
   * @brief Set of references to external variables used by residual operator
   */
  FirstOrderODE::State state_;

  /**
   * @brief The method of enforcing time-varying dirichlet boundary conditions
   */
  DirichletEnforcementMethod enforcement_method_ = serac::DirichletEnforcementMethod::RateControl;
  /**
   * @brief Reference to the equationsolver used to solve for du_dt
   */
  const EquationSolver& solver_;
  /**
   * @brief MFEM solver object for first-order ODEs
   */
  std::unique_ptr<mfem::ODESolver> ode_solver_;
  /**
   * @brief Reference to boundary conditions used to constrain the solution
   */
  const BoundaryConditionManager& bcs_;
  mfem::Vector                    zero_;

  /**
   * @brief Working vectors for ODE outputs prior to constraint enforcement
   */
  mutable mfem::Vector U_minus_;
  mutable mfem::Vector U_;
  mutable mfem::Vector U_plus_;
  mutable mfem::Vector dU_dt_;
};

}  // namespace serac::mfem_ext
