/*
 * periodic_hill.h
 *
 *  Created on: March 28, 2019
 *      Author: fehn
 */

#ifndef APPLICATIONS_INCOMPRESSIBLE_NAVIER_STOKES_TEST_CASES_PERIODIC_HILL_H_
#define APPLICATIONS_INCOMPRESSIBLE_NAVIER_STOKES_TEST_CASES_PERIODIC_HILL_H_

#include "flow_rate_controller.h"
#include "manifold.h"
#include "postprocessor.h"

namespace IncNS
{
namespace PeriodicHill
{
template<int dim>
class InitialSolutionVelocity : public Function<dim>
{
public:
  InitialSolutionVelocity(double const bulk_velocity, double const H, double const height)
    : Function<dim>(dim, 0.0), bulk_velocity(bulk_velocity), H(H), height(height)
  {
  }

  double
  value(const Point<dim> & p, const unsigned int component = 0) const
  {
    // x-velocity
    double result = 0.0;
    if(component == 0)
    {
      // initial conditions
      if(p[1] > H && p[1] < (H + height))
        result = bulk_velocity * (p[1] - H) * ((H + height) - p[1]) / std::pow(height / 2.0, 2.0);

      // add some random perturbations
      result *= (1.0 + 0.1 * (((double)rand() / RAND_MAX - 0.5) / 0.5));
    }

    return result;
  }

private:
  double const bulk_velocity, H, height;
};

template<int dim>
class RightHandSide : public Function<dim>
{
public:
  RightHandSide(FlowRateController const & flow_rate_controller)
    : Function<dim>(dim, 0.0), flow_rate_controller(flow_rate_controller)
  {
  }

  double
  value(const Point<dim> & /*p*/, const unsigned int component = 0) const
  {
    double result = 0.0;

    // The flow is driven by constant body force in x-direction
    if(component == 0)
    {
      result = flow_rate_controller.get_body_force();
    }

    return result;
  }

private:
  FlowRateController const & flow_rate_controller;
};

template<int dim, typename Number>
class Application : public ApplicationBase<dim, Number>
{
public:
  Application() : ApplicationBase<dim, Number>("")
  {
  }

  Application(std::string input_file) : ApplicationBase<dim, Number>(input_file)
  {
    // parse application-specific parameters
    ParameterHandler prm;
    this->add_parameters(prm);
    parse_input(input_file, prm, true, true);

    // viscosity needs to be recomputed since the parameters inviscid, Re are
    // read from the input file
    viscosity = inviscid ? 0.0 : bulk_velocity * H / Re;

    // depend on values defined in input file
    end_time          = double(end_time_multiples) * flow_through_time;
    sample_start_time = double(sample_start_time_multiples) * flow_through_time;

    // sample end time is equal to end time, which is read from the input file
    sample_end_time = end_time;

    flow_rate_controller.reset(
      new FlowRateController(bulk_velocity, target_flow_rate, H, start_time));
  }

  void
  add_parameters(ParameterHandler & prm)
  {
    // clang-format off
    prm.enter_subsection("Application");
      prm.add_parameter("Inviscid",             inviscid,                     "Is this an inviscid simulation?");
      prm.add_parameter("ReynoldsNumber",       Re,                           "Reynolds number (ignored if Inviscid = true)");
      prm.add_parameter("EndTime",              end_time_multiples,           "End time in multiples of flow through time.", Patterns::Integer(0.0,1000.0));
      prm.add_parameter("GridStretchFactor",    grid_stretch_factor,          "Factor describing grid stretching in vertical direction.");
      prm.add_parameter("OutputDirectory",      output_directory,             "Directory where output is written.");
      prm.add_parameter("CalculateStatistics",  calculate_statistics,         "Decides whether statistics are calculated.");
      prm.add_parameter("SampleStartTime",      sample_start_time_multiples,  "Start time of sampling in multiples of flow through time.", Patterns::Integer(0.0,1000.0));
      prm.add_parameter("SampleEveryTimeSteps", sample_every_timesteps,       "Sample every ... time steps.", Patterns::Integer(1,1000));
      prm.add_parameter("PointsPerLine",        points_per_line,              "Points per line in vertical direction.", Patterns::Integer(1,10000));
      prm.add_parameter("WriteVTUOutput",       write_vtu_output,             "Decides whether vtu output is written.");
      prm.add_parameter("OutputNameVTU",        output_name_vtu,              "Name of vtu output files.");
    prm.leave_subsection();
    // clang-format on
  }

  // Reynolds number, viscosity, bulk velocity

  bool   inviscid = false;
  double Re       = 5600.0; // 700, 1400, 5600, 10595, 19000

  double const H      = 0.028;
  double const width  = 4.5 * H;
  double const length = 9.0 * H;
  double const height = 2.036 * H;

  double const bulk_velocity     = 5.6218;
  double const target_flow_rate  = bulk_velocity * width * height;
  double const flow_through_time = length / bulk_velocity;

  // RE_H = u_b * H / nu
  double viscosity = bulk_velocity * H / Re;

  // flow rate controller
  std::shared_ptr<FlowRateController> flow_rate_controller;

  // start and end time
  double const start_time         = 0.0;
  unsigned int end_time_multiples = 1;
  double       end_time           = double(end_time_multiples) * flow_through_time;

  // grid
  double grid_stretch_factor = 1.6;

  // results (postprocessing)
  std::string output_directory = "output/periodic_hill/";

  // sampling
  bool         calculate_statistics        = true;
  unsigned int sample_start_time_multiples = 0.0;
  double       sample_start_time      = double(sample_start_time_multiples) * flow_through_time;
  double       sample_end_time        = end_time;
  unsigned int sample_every_timesteps = 1;
  unsigned int points_per_line        = 20;

  // vtu output
  bool        write_vtu_output = true;
  std::string output_name_vtu  = "test";

  void
  set_input_parameters(InputParameters & param)
  {
    // MATHEMATICAL MODEL
    param.dim          = 3;
    param.problem_type = ProblemType::Unsteady;
    if(inviscid)
      param.equation_type = EquationType::Euler;
    else
      param.equation_type = EquationType::NavierStokes;
    param.formulation_viscous_term    = FormulationViscousTerm::LaplaceFormulation;
    param.formulation_convective_term = FormulationConvectiveTerm::DivergenceFormulation;
    param.right_hand_side             = true;


    // PHYSICAL QUANTITIES
    param.start_time = start_time;
    param.end_time   = end_time;
    param.viscosity  = viscosity;


    // TEMPORAL DISCRETIZATION
    param.solver_type                     = SolverType::Unsteady;
    param.temporal_discretization         = TemporalDiscretization::BDFDualSplittingScheme;
    param.treatment_of_convective_term    = TreatmentOfConvectiveTerm::Explicit;
    param.calculation_of_time_step_size   = TimeStepCalculation::CFL;
    param.adaptive_time_stepping          = true;
    param.max_velocity                    = bulk_velocity;
    param.cfl                             = 0.3; // 0.375;
    param.cfl_exponent_fe_degree_velocity = 1.5;
    param.time_step_size                  = 1.0e-1;
    param.order_time_integrator           = 2;
    param.start_with_low_order            = true;

    // output of solver information
    param.solver_info_data.interval_time = flow_through_time / 10.0;

    // SPATIAL DISCRETIZATION
    param.triangulation_type = TriangulationType::Distributed;
    param.degree_p           = DegreePressure::MixedOrder;
    param.mapping            = MappingType::Isoparametric;

    // convective term
    param.upwind_factor = 0.5;

    // viscous term
    param.IP_formulation_viscous = InteriorPenaltyFormulation::SIPG;

    // special case: pure DBC's
    param.pure_dirichlet_bc = true;

    // TURBULENCE
    param.use_turbulence_model = false;
    param.turbulence_model     = TurbulenceEddyViscosityModel::Sigma;
    // Smagorinsky: 0.165
    // Vreman: 0.28
    // WALE: 0.50
    // Sigma: 1.35
    param.turbulence_model_constant = 1.35;

    // PROJECTION METHODS

    // pressure Poisson equation
    param.solver_pressure_poisson              = SolverPressurePoisson::CG;
    param.solver_data_pressure_poisson         = SolverData(1000, 1.e-12, 1.e-6, 100);
    param.preconditioner_pressure_poisson      = PreconditionerPressurePoisson::Multigrid;
    param.multigrid_data_pressure_poisson.type = MultigridType::cphMG;
    param.multigrid_data_pressure_poisson.coarse_problem.solver =
      MultigridCoarseGridSolver::Chebyshev;
    param.multigrid_data_pressure_poisson.coarse_problem.preconditioner =
      MultigridCoarseGridPreconditioner::PointJacobi;

    // projection step
    param.solver_projection                = SolverProjection::CG;
    param.solver_data_projection           = SolverData(1000, 1.e-12, 1.e-6);
    param.preconditioner_projection        = PreconditionerProjection::InverseMassMatrix;
    param.update_preconditioner_projection = true;


    // HIGH-ORDER DUAL SPLITTING SCHEME

    // formulations
    param.order_extrapolation_pressure_nbc =
      param.order_time_integrator <= 2 ? param.order_time_integrator : 2;

    // viscous step
    param.solver_viscous         = SolverViscous::CG;
    param.solver_data_viscous    = SolverData(1000, 1.e-12, 1.e-6);
    param.preconditioner_viscous = PreconditionerViscous::InverseMassMatrix;
  }

  void
  create_grid(std::shared_ptr<parallel::TriangulationBase<dim>> triangulation,
              unsigned int const                                n_refine_space,
              std::vector<GridTools::PeriodicFacePair<typename Triangulation<dim>::cell_iterator>> &
                periodic_faces)
  {
    Point<dim> p_1;
    p_1[0] = 0.;
    p_1[1] = H;
    if(dim == 3)
      p_1[2] = -width / 2.0;

    Point<dim> p_2;
    p_2[0] = length;
    p_2[1] = H + height;
    if(dim == 3)
      p_2[2] = width / 2.0;

    // use 2 cells in x-direction on coarsest grid and 1 cell in y- and z-directions
    std::vector<unsigned int> refinements(dim, 1);
    refinements[0] = 2;
    GridGenerator::subdivided_hyper_rectangle(*triangulation, refinements, p_1, p_2);

    // create hill by shifting y-coordinates of middle vertices by -H in y-direction
    triangulation->last()->vertex(0)[1] = 0.;
    if(dim == 3)
      triangulation->last()->vertex(4)[1] = 0.;

    // periodicity in x-direction (add 10 to avoid conflicts with dirichlet boundary, which is 0)
    // make use of the fact that the mesh has only two elements

    // left element
    triangulation->begin()->face(0)->set_all_boundary_ids(0 + 10);
    // right element
    triangulation->last()->face(1)->set_all_boundary_ids(1 + 10);

    // periodicity in z-direction
    if(dim == 3)
    {
      // left element
      triangulation->begin()->face(4)->set_all_boundary_ids(2 + 10);
      triangulation->begin()->face(5)->set_all_boundary_ids(3 + 10);
      // right element
      triangulation->last()->face(4)->set_all_boundary_ids(4 + 10);
      triangulation->last()->face(5)->set_all_boundary_ids(5 + 10);
    }

    auto tria = dynamic_cast<Triangulation<dim> *>(&*triangulation);
    GridTools::collect_periodic_faces(*tria, 0 + 10, 1 + 10, 0, periodic_faces);
    if(dim == 3)
    {
      GridTools::collect_periodic_faces(*tria, 2 + 10, 3 + 10, 2, periodic_faces);
      GridTools::collect_periodic_faces(*tria, 4 + 10, 5 + 10, 2, periodic_faces);
    }

    triangulation->add_periodicity(periodic_faces);

    unsigned int const manifold_id = 111;
    triangulation->begin()->set_all_manifold_ids(manifold_id);
    triangulation->last()->set_all_manifold_ids(manifold_id);

    static const PeriodicHillManifold<dim> manifold =
      PeriodicHillManifold<dim>(H, length, height, grid_stretch_factor);
    triangulation->set_manifold(manifold_id, manifold);

    triangulation->refine_global(n_refine_space);
  }

  void
  set_boundary_conditions(std::shared_ptr<BoundaryDescriptorU<dim>> boundary_descriptor_velocity,
                          std::shared_ptr<BoundaryDescriptorP<dim>> boundary_descriptor_pressure)
  {
    // set boundary conditions
    typedef typename std::pair<types::boundary_id, std::shared_ptr<Function<dim>>> pair;

    // fill boundary descriptor velocity
    boundary_descriptor_velocity->dirichlet_bc.insert(
      pair(0, new Functions::ZeroFunction<dim>(dim)));

    // fill boundary descriptor pressure
    boundary_descriptor_pressure->neumann_bc.insert(pair(0, new Functions::ZeroFunction<dim>(dim)));
  }

  void
  set_field_functions(std::shared_ptr<FieldFunctions<dim>> field_functions)
  {
    field_functions->initial_solution_velocity.reset(
      new InitialSolutionVelocity<dim>(bulk_velocity, H, height));
    field_functions->initial_solution_pressure.reset(new Functions::ZeroFunction<dim>(1));
    field_functions->analytical_solution_pressure.reset(new Functions::ZeroFunction<dim>(1));
    field_functions->right_hand_side.reset(new RightHandSide<dim>(*flow_rate_controller));
  }

  std::shared_ptr<PostProcessorBase<dim, Number>>
  construct_postprocessor(InputParameters const & param, MPI_Comm const & mpi_comm)
  {
    PostProcessorData<dim> pp_data;

    // write output for visualization of results
    pp_data.output_data.write_output              = write_vtu_output;
    pp_data.output_data.output_folder             = output_directory + "vtu/";
    pp_data.output_data.output_name               = output_name_vtu;
    pp_data.output_data.output_start_time         = param.start_time;
    pp_data.output_data.output_interval_time      = flow_through_time / 10.0;
    pp_data.output_data.write_velocity_magnitude  = true;
    pp_data.output_data.write_vorticity           = true;
    pp_data.output_data.write_vorticity_magnitude = true;
    pp_data.output_data.write_q_criterion         = true;
    pp_data.output_data.degree                    = param.degree_u;
    pp_data.output_data.write_higher_order        = false;

    MyPostProcessorData<dim> my_pp_data;
    my_pp_data.pp_data = pp_data;

    // line plot data: calculate statistics along lines
    my_pp_data.line_plot_data.line_data.directory = output_directory;

    // mean velocity
    std::shared_ptr<Quantity> quantity_velocity;
    quantity_velocity.reset(new Quantity());
    quantity_velocity->type = QuantityType::Velocity;

    // Reynolds stresses
    std::shared_ptr<Quantity> quantity_reynolds;
    quantity_reynolds.reset(new Quantity());
    quantity_reynolds->type = QuantityType::ReynoldsStresses;

    // lines
    std::shared_ptr<LineHomogeneousAveraging<dim>> vel_0, vel_1, vel_2, vel_3, vel_4, vel_5, vel_6,
      vel_7, vel_8;
    vel_0.reset(new LineHomogeneousAveraging<dim>());
    vel_1.reset(new LineHomogeneousAveraging<dim>());
    vel_2.reset(new LineHomogeneousAveraging<dim>());
    vel_3.reset(new LineHomogeneousAveraging<dim>());
    vel_4.reset(new LineHomogeneousAveraging<dim>());
    vel_5.reset(new LineHomogeneousAveraging<dim>());
    vel_6.reset(new LineHomogeneousAveraging<dim>());
    vel_7.reset(new LineHomogeneousAveraging<dim>());
    vel_8.reset(new LineHomogeneousAveraging<dim>());

    vel_0->average_homogeneous_direction = true;
    vel_1->average_homogeneous_direction = true;
    vel_2->average_homogeneous_direction = true;
    vel_3->average_homogeneous_direction = true;
    vel_4->average_homogeneous_direction = true;
    vel_5->average_homogeneous_direction = true;
    vel_6->average_homogeneous_direction = true;
    vel_7->average_homogeneous_direction = true;
    vel_8->average_homogeneous_direction = true;

    vel_0->averaging_direction = 2;
    vel_1->averaging_direction = 2;
    vel_2->averaging_direction = 2;
    vel_3->averaging_direction = 2;
    vel_4->averaging_direction = 2;
    vel_5->averaging_direction = 2;
    vel_6->averaging_direction = 2;
    vel_7->averaging_direction = 2;
    vel_8->averaging_direction = 2;

    // begin and end points of all lines
    double const eps = 1.e-10;
    vel_0->begin     = Point<dim>(0 * H, H + f(0 * H, H, length) + eps, 0);
    vel_0->end       = Point<dim>(0 * H, H + height - eps, 0);
    vel_1->begin     = Point<dim>(1 * H, H + f(1 * H, H, length) + eps, 0);
    vel_1->end       = Point<dim>(1 * H, H + height - eps, 0);
    vel_2->begin     = Point<dim>(2 * H, H + f(2 * H, H, length) + eps, 0);
    vel_2->end       = Point<dim>(2 * H, H + height - eps, 0);
    vel_3->begin     = Point<dim>(3 * H, H + f(3 * H, H, length) + eps, 0);
    vel_3->end       = Point<dim>(3 * H, H + height - eps, 0);
    vel_4->begin     = Point<dim>(4 * H, H + f(4 * H, H, length) + eps, 0);
    vel_4->end       = Point<dim>(4 * H, H + height - eps, 0);
    vel_5->begin     = Point<dim>(5 * H, H + f(5 * H, H, length) + eps, 0);
    vel_5->end       = Point<dim>(5 * H, H + height - eps, 0);
    vel_6->begin     = Point<dim>(6 * H, H + f(6 * H, H, length) + eps, 0);
    vel_6->end       = Point<dim>(6 * H, H + height - eps, 0);
    vel_7->begin     = Point<dim>(7 * H, H + f(7 * H, H, length) + eps, 0);
    vel_7->end       = Point<dim>(7 * H, H + height - eps, 0);
    vel_8->begin     = Point<dim>(8 * H, H + f(8 * H, H, length) + eps, 0);
    vel_8->end       = Point<dim>(8 * H, H + height - eps, 0);

    // set the number of points along the lines
    vel_0->n_points = points_per_line;
    vel_1->n_points = points_per_line;
    vel_2->n_points = points_per_line;
    vel_3->n_points = points_per_line;
    vel_4->n_points = points_per_line;
    vel_5->n_points = points_per_line;
    vel_6->n_points = points_per_line;
    vel_7->n_points = points_per_line;
    vel_8->n_points = points_per_line;

    // set the quantities that we want to compute along the lines
    vel_0->quantities.push_back(quantity_velocity);
    vel_0->quantities.push_back(quantity_reynolds);
    vel_1->quantities.push_back(quantity_velocity);
    vel_1->quantities.push_back(quantity_reynolds);
    vel_2->quantities.push_back(quantity_velocity);
    vel_2->quantities.push_back(quantity_reynolds);
    vel_3->quantities.push_back(quantity_velocity);
    vel_3->quantities.push_back(quantity_reynolds);
    vel_4->quantities.push_back(quantity_velocity);
    vel_4->quantities.push_back(quantity_reynolds);
    vel_5->quantities.push_back(quantity_velocity);
    vel_5->quantities.push_back(quantity_reynolds);
    vel_6->quantities.push_back(quantity_velocity);
    vel_6->quantities.push_back(quantity_reynolds);
    vel_7->quantities.push_back(quantity_velocity);
    vel_7->quantities.push_back(quantity_reynolds);
    vel_8->quantities.push_back(quantity_velocity);
    vel_8->quantities.push_back(quantity_reynolds);

    // set line names
    vel_0->name = "x_0";
    vel_1->name = "x_1";
    vel_2->name = "x_2";
    vel_3->name = "x_3";
    vel_4->name = "x_4";
    vel_5->name = "x_5";
    vel_6->name = "x_6";
    vel_7->name = "x_7";
    vel_8->name = "x_8";

    // insert lines
    my_pp_data.line_plot_data.line_data.lines.push_back(vel_0);
    my_pp_data.line_plot_data.line_data.lines.push_back(vel_1);
    my_pp_data.line_plot_data.line_data.lines.push_back(vel_2);
    my_pp_data.line_plot_data.line_data.lines.push_back(vel_3);
    my_pp_data.line_plot_data.line_data.lines.push_back(vel_4);
    my_pp_data.line_plot_data.line_data.lines.push_back(vel_5);
    my_pp_data.line_plot_data.line_data.lines.push_back(vel_6);
    my_pp_data.line_plot_data.line_data.lines.push_back(vel_7);
    my_pp_data.line_plot_data.line_data.lines.push_back(vel_8);

    my_pp_data.line_plot_data.statistics_data.calculate_statistics   = calculate_statistics;
    my_pp_data.line_plot_data.statistics_data.sample_start_time      = sample_start_time;
    my_pp_data.line_plot_data.statistics_data.sample_end_time        = end_time;
    my_pp_data.line_plot_data.statistics_data.sample_every_timesteps = sample_every_timesteps;
    my_pp_data.line_plot_data.statistics_data.write_output_every_timesteps =
      sample_every_timesteps * 100;

    // calculation of flow rate (use volume-based computation)
    my_pp_data.mean_velocity_data.calculate       = true;
    my_pp_data.mean_velocity_data.filename_prefix = output_directory + "flow_rate";
    Tensor<1, dim, double> direction;
    direction[0]                                = 1.0;
    my_pp_data.mean_velocity_data.direction     = direction;
    my_pp_data.mean_velocity_data.write_to_file = true;

    std::shared_ptr<PostProcessorBase<dim, Number>> pp;
    pp.reset(new MyPostProcessor<dim, Number>(my_pp_data, mpi_comm, length, *flow_rate_controller));

    return pp;
  }
};

} // namespace PeriodicHill
} // namespace IncNS

#endif /* APPLICATIONS_INCOMPRESSIBLE_NAVIER_STOKES_TEST_CASES_PERIODIC_HILL_H_ */
