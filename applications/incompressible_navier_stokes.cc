/*
 * incompressible_navier_stokes.cc
 *
 *  Created on: Oct 10, 2016
 *      Author: fehn
 */

// deal.II
#include <deal.II/base/revision.h>
#include <deal.II/distributed/tria.h>
#include <deal.II/grid/grid_tools.h>

// postprocessor
#include "../include/incompressible_navier_stokes/postprocessor/postprocessor.h"

// spatial discretization
#include "../include/incompressible_navier_stokes/spatial_discretization/dg_navier_stokes_coupled_solver.h"
#include "../include/incompressible_navier_stokes/spatial_discretization/dg_navier_stokes_dual_splitting.h"
#include "../include/incompressible_navier_stokes/spatial_discretization/dg_navier_stokes_pressure_correction.h"

#include "../include/incompressible_navier_stokes/interface_space_time/operator.h"

// temporal discretization
#include "../include/incompressible_navier_stokes/time_integration/time_int_bdf_coupled_solver.h"
#include "../include/incompressible_navier_stokes/time_integration/time_int_bdf_dual_splitting.h"
#include "../include/incompressible_navier_stokes/time_integration/time_int_bdf_navier_stokes.h"
#include "../include/incompressible_navier_stokes/time_integration/time_int_bdf_pressure_correction.h"

#include "../include/incompressible_navier_stokes/time_integration/driver_steady_problems.h"

// Parameters, BCs, etc.
#include "../include/incompressible_navier_stokes/user_interface/analytical_solution.h"
#include "../include/incompressible_navier_stokes/user_interface/boundary_descriptor.h"
#include "../include/incompressible_navier_stokes/user_interface/field_functions.h"
#include "../include/incompressible_navier_stokes/user_interface/input_parameters.h"

#include "../include/functionalities/print_general_infos.h"

using namespace dealii;
using namespace IncNS;

// specify the flow problem that has to be solved

// 2D Stokes flow
//#include "incompressible_navier_stokes_test_cases/stokes_guermond.h"
//#include "incompressible_navier_stokes_test_cases/stokes_shahbazi.h"
//#include "incompressible_navier_stokes_test_cases/stokes_curl_flow.h"

// 2D Navier-Stokes flow
//#include "incompressible_navier_stokes_test_cases/couette.h"
//#include "incompressible_navier_stokes_test_cases/poiseuille.h"
//#include "incompressible_navier_stokes_test_cases/poiseuille_pressure_inflow.h"
//#include "incompressible_navier_stokes_test_cases/cavity.h"
//#include "incompressible_navier_stokes_test_cases/kovasznay.h"
//#include "incompressible_navier_stokes_test_cases/vortex.h"
//#include "incompressible_navier_stokes_test_cases/taylor_vortex.h"
//#include "incompressible_navier_stokes_test_cases/tum.h"
//#include "incompressible_navier_stokes_test_cases/orr_sommerfeld.h"
//#include "incompressible_navier_stokes_test_cases/kelvin_helmholtz.h"

// 2D/3D Navier-Stokes flow
//#include "incompressible_navier_stokes_test_cases/flow_past_cylinder.h"

// 3D Navier-Stokes flow
//#include "incompressible_navier_stokes_test_cases/beltrami.h"
//#include "incompressible_navier_stokes_test_cases/unstable_beltrami.h"
//#include "incompressible_navier_stokes_test_cases/cavity_3D.h"
//#include "incompressible_navier_stokes_test_cases/3D_taylor_green_vortex.h"
//#include "incompressible_navier_stokes_test_cases/turbulent_channel.h"
#include "incompressible_navier_stokes_test_cases/periodic_hill.h"
//#include "incompressible_navier_stokes_test_cases/fda_nozzle_benchmark.h"

// incompressible flow with scalar transport (but can be used for pure fluid simulations also)
//#include "incompressible_flow_with_transport_test_cases/lung.h"

template<int dim, int degree_u, int degree_p = degree_u - 1, typename Number = double>
class NavierStokesProblem
{
public:
  NavierStokesProblem(unsigned int const refine_steps_space,
                      unsigned int const refine_steps_time = 0);

  void
  setup(bool const do_restart);

  void
  solve() const;

  void
  analyze_computing_times() const;

private:
  void
  print_header() const;

  ConditionalOStream pcout;

  std::shared_ptr<parallel::Triangulation<dim>> triangulation;
  std::vector<GridTools::PeriodicFacePair<typename Triangulation<dim>::cell_iterator>>
    periodic_faces;

  const unsigned int n_refine_space, n_refine_time;

  std::shared_ptr<FieldFunctions<dim>>      field_functions;
  std::shared_ptr<BoundaryDescriptorU<dim>> boundary_descriptor_velocity;
  std::shared_ptr<BoundaryDescriptorP<dim>> boundary_descriptor_pressure;
  std::shared_ptr<AnalyticalSolution<dim>>  analytical_solution;

  InputParameters<dim> param;

  typedef DGNavierStokesBase<dim, degree_u, degree_p, Number>               DGBase;
  typedef DGNavierStokesCoupled<dim, degree_u, degree_p, Number>            DGCoupled;
  typedef DGNavierStokesDualSplitting<dim, degree_u, degree_p, Number>      DGDualSplitting;
  typedef DGNavierStokesPressureCorrection<dim, degree_u, degree_p, Number> DGPressureCorrection;

  std::shared_ptr<DGBase> navier_stokes_operation;

  typedef PostProcessorBase<dim, degree_u, degree_p, Number> Postprocessor;

  std::shared_ptr<Postprocessor> postprocessor;

  // unsteady solvers
  typedef TimeIntBDF<dim, Number>                   TimeInt;
  typedef TimeIntBDFCoupled<dim, Number>            TimeIntCoupled;
  typedef TimeIntBDFDualSplitting<dim, Number>      TimeIntDualSplitting;
  typedef TimeIntBDFPressureCorrection<dim, Number> TimeIntPressureCorrection;

  std::shared_ptr<TimeInt> time_integrator;

  // steady solver
  typedef DriverSteadyProblems<dim, Number> DriverSteady;

  std::shared_ptr<DriverSteady> driver_steady;

  /*
   * Computation time (wall clock time).
   */
  Timer          timer;
  mutable double overall_time;
  double         setup_time;
};

template<int dim, int degree_u, int degree_p, typename Number>
NavierStokesProblem<dim, degree_u, degree_p, Number>::NavierStokesProblem(
  unsigned int const refine_steps_space,
  unsigned int const refine_steps_time)
  : pcout(std::cout, Utilities::MPI::this_mpi_process(MPI_COMM_WORLD) == 0),
    n_refine_space(refine_steps_space),
    n_refine_time(refine_steps_time),
    overall_time(0.0),
    setup_time(0.0)
{
}

template<int dim, int degree_u, int degree_p, typename Number>
void
NavierStokesProblem<dim, degree_u, degree_p, Number>::print_header() const
{
  // clang-format off
  pcout << std::endl << std::endl << std::endl
  << "_________________________________________________________________________________" << std::endl
  << "                                                                                 " << std::endl
  << "                High-order discontinuous Galerkin solver for the                 " << std::endl
  << "                     incompressible Navier-Stokes equations                      " << std::endl
  << "_________________________________________________________________________________" << std::endl
  << std::endl;
  // clang-format on
}

template<int dim, int degree_u, int degree_p, typename Number>
void
NavierStokesProblem<dim, degree_u, degree_p, Number>::setup(bool const do_restart)
{
  timer.restart();

  print_header();
  print_MPI_info(pcout);

  // input parameters
  param.set_input_parameters();
  param.check_input_parameters();

  if(param.print_input_parameters == true)
    param.print(pcout);

  // triangulation
  if(param.triangulation_type == TriangulationType::Distributed)
  {
    triangulation.reset(new parallel::distributed::Triangulation<dim>(
      MPI_COMM_WORLD,
      dealii::Triangulation<dim>::none,
      parallel::distributed::Triangulation<dim>::construct_multigrid_hierarchy));
  }
  else if(param.triangulation_type == TriangulationType::FullyDistributed)
  {
    triangulation.reset(new parallel::fullydistributed::Triangulation<dim>(MPI_COMM_WORLD));
  }
  else
  {
    AssertThrow(false, ExcMessage("Invalid parameter triangulation_type."));
  }

  // this function has to be defined in the header file that implements all
  // problem specific things like parameters, geometry, boundary conditions, etc.
  create_grid_and_set_boundary_ids(triangulation, n_refine_space, periodic_faces);

  print_grid_data(pcout, n_refine_space, *triangulation);

  boundary_descriptor_velocity.reset(new BoundaryDescriptorU<dim>());
  boundary_descriptor_pressure.reset(new BoundaryDescriptorP<dim>());

  IncNS::set_boundary_conditions(boundary_descriptor_velocity, boundary_descriptor_pressure);

  // field functions and boundary conditions
  field_functions.reset(new FieldFunctions<dim>());
  set_field_functions(field_functions);

  analytical_solution.reset(new AnalyticalSolution<dim>());
  // this function has to be defined in the header file
  // that implements all problem specific things like
  // parameters, geometry, boundary conditions, etc.
  set_analytical_solution(analytical_solution);

  // initialize postprocessor
  // this function has to be defined in the header file
  // that implements all problem specific things like
  // parameters, geometry, boundary conditions, etc.
  postprocessor = construct_postprocessor<dim, degree_u, degree_p, Number>(param);

  if(param.solver_type == SolverType::Unsteady)
  {
    // initialize navier_stokes_operation
    if(this->param.temporal_discretization == TemporalDiscretization::BDFCoupledSolution)
    {
      std::shared_ptr<DGCoupled> navier_stokes_operation_coupled;

      navier_stokes_operation_coupled.reset(new DGCoupled(*triangulation, param, postprocessor));

      navier_stokes_operation = navier_stokes_operation_coupled;

      time_integrator.reset(new TimeIntCoupled(
        navier_stokes_operation_coupled, navier_stokes_operation_coupled, param, n_refine_time));
    }
    else if(this->param.temporal_discretization == TemporalDiscretization::BDFDualSplittingScheme)
    {
      std::shared_ptr<DGDualSplitting> navier_stokes_operation_dual_splitting;

      navier_stokes_operation_dual_splitting.reset(
        new DGDualSplitting(*triangulation, param, postprocessor));

      navier_stokes_operation = navier_stokes_operation_dual_splitting;

      time_integrator.reset(new TimeIntDualSplitting(navier_stokes_operation_dual_splitting,
                                                     navier_stokes_operation_dual_splitting,
                                                     param,
                                                     n_refine_time));
    }
    else if(this->param.temporal_discretization == TemporalDiscretization::BDFPressureCorrection)
    {
      std::shared_ptr<DGPressureCorrection> navier_stokes_operation_pressure_correction;

      navier_stokes_operation_pressure_correction.reset(
        new DGPressureCorrection(*triangulation, param, postprocessor));

      navier_stokes_operation = navier_stokes_operation_pressure_correction;

      time_integrator.reset(
        new TimeIntPressureCorrection(navier_stokes_operation_pressure_correction,
                                      navier_stokes_operation_pressure_correction,
                                      param,
                                      n_refine_time));
    }
    else
    {
      AssertThrow(false, ExcMessage("Not implemented."));
    }
  }
  else if(param.solver_type == SolverType::Steady)
  {
    // initialize navier_stokes_operation
    std::shared_ptr<DGCoupled> navier_stokes_operation_coupled;

    navier_stokes_operation_coupled.reset(new DGCoupled(*triangulation, param, postprocessor));

    navier_stokes_operation = navier_stokes_operation_coupled;

    // initialize driver for steady state problem that depends on navier_stokes_operation
    driver_steady.reset(
      new DriverSteady(navier_stokes_operation_coupled, navier_stokes_operation_coupled, param));
  }
  else
  {
    AssertThrow(false, ExcMessage("Not implemented."));
  }

  AssertThrow(navier_stokes_operation.get() != 0, ExcMessage("Not initialized."));
  navier_stokes_operation->setup(periodic_faces,
                                 boundary_descriptor_velocity,
                                 boundary_descriptor_pressure,
                                 field_functions,
                                 analytical_solution);

  if(param.solver_type == SolverType::Unsteady)
  {
    // setup time integrator before calling setup_solvers
    // (this is necessary since the setup of the solvers
    // depends on quantities such as the time_step_size or gamma0!!!)
    time_integrator->setup(do_restart);

    navier_stokes_operation->setup_solvers(
      time_integrator->get_scaling_factor_time_derivative_term());
  }
  else if(param.solver_type == SolverType::Steady)
  {
    driver_steady->setup();

    navier_stokes_operation->setup_solvers();
  }
  else
  {
    AssertThrow(false, ExcMessage("Not implemented."));
  }

  setup_time = timer.wall_time();
}

template<int dim, int degree_u, int degree_p, typename Number>
void
NavierStokesProblem<dim, degree_u, degree_p, Number>::solve() const
{
  if(param.solver_type == SolverType::Unsteady)
  {
    // stability analysis (uncomment if desired)
    // time_integrator->postprocessing_stability_analysis();

    // run time loop
    if(this->param.problem_type == ProblemType::Steady)
      time_integrator->timeloop_steady_problem();
    else if(this->param.problem_type == ProblemType::Unsteady)
      time_integrator->timeloop();
    else
      AssertThrow(false, ExcMessage("Not implemented."));
  }
  else if(param.solver_type == SolverType::Steady)
  {
    // solve steady problem
    driver_steady->solve_steady_problem();
  }
  else
  {
    AssertThrow(false, ExcMessage("Not implemented."));
  }

  overall_time += this->timer.wall_time();
}

template<int dim, int degree_u, int degree_p, typename Number>
void
NavierStokesProblem<dim, degree_u, degree_p, Number>::analyze_computing_times() const
{
  this->pcout << std::endl
              << "_________________________________________________________________________________"
              << std::endl
              << std::endl;

  this->pcout << "Performance results for incompressible Navier-Stokes solver:" << std::endl;

  // Iterations
  if(param.solver_type == SolverType::Unsteady)
  {
    this->pcout << std::endl << "Average number of iterations:" << std::endl;

    std::vector<std::string> names;
    std::vector<double>      iterations;

    this->time_integrator->get_iterations(names, iterations);

    unsigned int length = 1;
    for(unsigned int i = 0; i < names.size(); ++i)
    {
      length = length > names[i].length() ? length : names[i].length();
    }

    for(unsigned int i = 0; i < iterations.size(); ++i)
    {
      this->pcout << "  " << std::setw(length + 2) << std::left << names[i] << std::fixed
                  << std::setprecision(2) << std::right << std::setw(6) << iterations[i]
                  << std::endl;
    }
  }

  // overall wall time including postprocessing
  Utilities::MPI::MinMaxAvg overall_time_data =
    Utilities::MPI::min_max_avg(overall_time, MPI_COMM_WORLD);
  double const overall_time_avg = overall_time_data.avg;

  // wall times
  this->pcout << std::endl << "Wall times:" << std::endl;

  std::vector<std::string> names;
  std::vector<double>      computing_times;

  if(param.solver_type == SolverType::Unsteady)
  {
    this->time_integrator->get_wall_times(names, computing_times);
  }
  else
  {
    this->driver_steady->get_wall_times(names, computing_times);
  }

  unsigned int length = 1;
  for(unsigned int i = 0; i < names.size(); ++i)
  {
    length = length > names[i].length() ? length : names[i].length();
  }

  double sum_of_substeps = 0.0;
  for(unsigned int i = 0; i < computing_times.size(); ++i)
  {
    Utilities::MPI::MinMaxAvg data =
      Utilities::MPI::min_max_avg(computing_times[i], MPI_COMM_WORLD);
    this->pcout << "  " << std::setw(length + 2) << std::left << names[i] << std::setprecision(2)
                << std::scientific << std::setw(10) << std::right << data.avg << " s  "
                << std::setprecision(2) << std::fixed << std::setw(6) << std::right
                << data.avg / overall_time_avg * 100 << " %" << std::endl;

    sum_of_substeps += data.avg;
  }

  Utilities::MPI::MinMaxAvg setup_time_data =
    Utilities::MPI::min_max_avg(setup_time, MPI_COMM_WORLD);
  double const setup_time_avg = setup_time_data.avg;
  this->pcout << "  " << std::setw(length + 2) << std::left << "Setup" << std::setprecision(2)
              << std::scientific << std::setw(10) << std::right << setup_time_avg << " s  "
              << std::setprecision(2) << std::fixed << std::setw(6) << std::right
              << setup_time_avg / overall_time_avg * 100 << " %" << std::endl;

  double const other = overall_time_avg - sum_of_substeps - setup_time_avg;
  this->pcout << "  " << std::setw(length + 2) << std::left << "Other" << std::setprecision(2)
              << std::scientific << std::setw(10) << std::right << other << " s  "
              << std::setprecision(2) << std::fixed << std::setw(6) << std::right
              << other / overall_time_avg * 100 << " %" << std::endl;

  this->pcout << "  " << std::setw(length + 2) << std::left << "Overall" << std::setprecision(2)
              << std::scientific << std::setw(10) << std::right << overall_time_avg << " s  "
              << std::setprecision(2) << std::fixed << std::setw(6) << std::right
              << overall_time_avg / overall_time_avg * 100 << " %" << std::endl;

  // computational costs in CPUh
  unsigned int N_mpi_processes = Utilities::MPI::n_mpi_processes(MPI_COMM_WORLD);

  this->pcout << std::endl
              << "Computational costs (including setup + postprocessing):" << std::endl
              << "  Number of MPI processes = " << N_mpi_processes << std::endl
              << "  Wall time               = " << std::scientific << std::setprecision(2)
              << overall_time_avg << " s" << std::endl
              << "  Computational costs     = " << std::scientific << std::setprecision(2)
              << overall_time_avg * (double)N_mpi_processes / 3600.0 << " CPUh" << std::endl;

  // Throughput in DoFs/s per time step per core
  unsigned int const DoFs = this->navier_stokes_operation->get_number_of_dofs();

  if(param.solver_type == SolverType::Unsteady)
  {
    unsigned int N_time_steps      = this->time_integrator->get_number_of_time_steps();
    double const time_per_timestep = overall_time_avg / (double)N_time_steps;
    this->pcout << std::endl
                << "Throughput per time step (including setup + postprocessing):" << std::endl
                << "  Degrees of freedom      = " << DoFs << std::endl
                << "  Wall time               = " << std::scientific << std::setprecision(2)
                << overall_time_avg << " s" << std::endl
                << "  Time steps              = " << std::left << N_time_steps << std::endl
                << "  Wall time per time step = " << std::scientific << std::setprecision(2)
                << time_per_timestep << " s" << std::endl
                << "  Throughput              = " << std::scientific << std::setprecision(2)
                << DoFs / (time_per_timestep * N_mpi_processes) << " DoFs/s/core" << std::endl;
  }
  else
  {
    this->pcout << std::endl
                << "Throughput (including setup + postprocessing):" << std::endl
                << "  Degrees of freedom      = " << DoFs << std::endl
                << "  Wall time               = " << std::scientific << std::setprecision(2)
                << overall_time_avg << " s" << std::endl
                << "  Throughput              = " << std::scientific << std::setprecision(2)
                << DoFs / (overall_time_avg * N_mpi_processes) << " DoFs/s/core" << std::endl;
  }


  this->pcout << "_________________________________________________________________________________"
              << std::endl
              << std::endl;
}

int
main(int argc, char ** argv)
{
  try
  {
    Utilities::MPI::MPI_InitFinalize mpi(argc, argv, 1);

    if(Utilities::MPI::this_mpi_process(MPI_COMM_WORLD) == 0)
    {
      std::cout << "deal.II git version " << DEAL_II_GIT_SHORTREV << " on branch "
                << DEAL_II_GIT_BRANCH << std::endl
                << std::endl;
    }

    deallog.depth_console(0);

    bool do_restart = false;
    if(argc > 1)
    {
      do_restart = std::atoi(argv[1]);
      if(do_restart)
      {
        AssertThrow(REFINE_STEPS_SPACE_MIN == REFINE_STEPS_SPACE_MAX,
                    ExcMessage("Spatial refinement not possible in combination with restart!"));

        AssertThrow(REFINE_STEPS_TIME_MIN == REFINE_STEPS_TIME_MAX,
                    ExcMessage("Temporal refinement not possible in combination with restart!"));
      }
    }

    // mesh refinements in order to perform spatial convergence tests
    for(unsigned int refine_steps_space = REFINE_STEPS_SPACE_MIN;
        refine_steps_space <= REFINE_STEPS_SPACE_MAX;
        ++refine_steps_space)
    {
      // time refinements in order to perform temporal convergence tests
      for(unsigned int refine_steps_time = REFINE_STEPS_TIME_MIN;
          refine_steps_time <= REFINE_STEPS_TIME_MAX;
          ++refine_steps_time)
      {
        NavierStokesProblem<DIMENSION, FE_DEGREE_VELOCITY, FE_DEGREE_PRESSURE, VALUE_TYPE> problem(
          refine_steps_space, refine_steps_time);

        problem.setup(do_restart);

        problem.solve();

        problem.analyze_computing_times();
      }
    }
  }
  catch(std::exception & exc)
  {
    std::cerr << std::endl
              << std::endl
              << "----------------------------------------------------" << std::endl;
    std::cerr << "Exception on processing: " << std::endl
              << exc.what() << std::endl
              << "Aborting!" << std::endl
              << "----------------------------------------------------" << std::endl;
    return 1;
  }
  catch(...)
  {
    std::cerr << std::endl
              << std::endl
              << "----------------------------------------------------" << std::endl;
    std::cerr << "Unknown exception!" << std::endl
              << "Aborting!" << std::endl
              << "----------------------------------------------------" << std::endl;
    return 1;
  }
  return 0;
}