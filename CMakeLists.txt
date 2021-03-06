#########################################################################
# 
#                 #######               ######  #######
#                 ##                    ##   ## ##
#                 #####   ##  ## #####  ##   ## ## ####
#                 ##       ####  ## ##  ##   ## ##   ##
#                 ####### ##  ## ###### ######  #######
#
#  ExaDG - High-Order Discontinuous Galerkin for the Exa-Scale
#
#  Copyright (C) 2021 by the ExaDG authors
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
#########################################################################

CMAKE_MINIMUM_REQUIRED(VERSION 3.1.0)

set(CMAKE_CXX_STANDARD 17)

ADD_SUBDIRECTORY(cmake)

PRINT_EXADG_HEADER()

# deal.II
FIND_PACKAGE(deal.II 9.3 QUIET
  HINTS ${deal.II_DIR} ${DEAL_II_DIR} ../ ../../ $ENV{DEAL_II_DIR}
  )
IF(NOT ${deal.II_FOUND})
  MESSAGE(FATAL_ERROR "\n"
    "*** Could not locate deal.II. ***\n\n"
    "You may want to either pass a flag -DDEAL_II_DIR=/path/to/deal.II to cmake\n"
    "or set an environment variable \"DEAL_II_DIR\" that contains this path."
    )
ENDIF()

# p4est
IF(NOT DEAL_II_WITH_P4EST)
  MESSAGE(FATAL_ERROR
    "\nExaDG requires a deal.II installation built with support for p4est but it appears to be missing!\n"
    )
ENDIF()

DEAL_II_INITIALIZE_CACHED_VARIABLES()

# Set the source files to be compiled
SET(TARGET_SRC
     include/exadg/time_integration/bdf_time_integration.cpp
     include/exadg/time_integration/extrapolation_scheme.cpp
     include/exadg/time_integration/time_int_base.cpp
     include/exadg/time_integration/time_int_bdf_base.cpp
     include/exadg/time_integration/time_int_explicit_runge_kutta_base.cpp
     include/exadg/time_integration/time_int_gen_alpha_base.cpp
     include/exadg/time_integration/enum_types.cpp
     include/exadg/grid/enum_types.cpp
     include/exadg/functions_and_boundary_conditions/linear_interpolation.cpp
     include/exadg/matrix_free/resolve_templates_float_2d.cpp
     include/exadg/matrix_free/resolve_templates_float_3d.cpp
     include/exadg/matrix_free/resolve_templates_double_2d.cpp
     include/exadg/matrix_free/resolve_templates_double_3d.cpp
     include/exadg/solvers_and_preconditioners/preconditioner/enum_types.cpp
     include/exadg/solvers_and_preconditioners/solvers/enum_types.cpp
     include/exadg/solvers_and_preconditioners/multigrid/multigrid_preconditioner_base.cpp
     include/exadg/solvers_and_preconditioners/multigrid/multigrid_input_parameters.cpp
     include/exadg/solvers_and_preconditioners/multigrid/transfer/mg_transfer_p.cpp
     include/exadg/solvers_and_preconditioners/multigrid/transfer/mg_transfer_c.cpp
     include/exadg/solvers_and_preconditioners/multigrid/transfer/mg_transfer_h.cpp
     include/exadg/solvers_and_preconditioners/multigrid/transfer/mg_transfer_global_coarsening.cpp
     include/exadg/solvers_and_preconditioners/multigrid/transfer/mg_transfer_mg_level_object.cpp
     include/exadg/postprocessor/error_calculation.cpp
     include/exadg/postprocessor/mean_scalar_calculation.cpp
     include/exadg/postprocessor/lift_and_drag_calculation.cpp
     include/exadg/postprocessor/pressure_difference_calculation.cpp
     include/exadg/postprocessor/kinetic_energy_spectrum.cpp
     include/exadg/postprocessor/kinetic_energy_calculation.cpp
     include/exadg/postprocessor/statistics_manager.cpp
     include/exadg/operators/operator_base.cpp
     include/exadg/operators/mass_operator.cpp
     include/exadg/operators/rhs_operator.cpp
     # Poisson equation
     include/exadg/poisson/user_interface/enum_types.cpp
     include/exadg/poisson/user_interface/input_parameters.cpp
     include/exadg/poisson/spatial_discretization/operator.cpp
     include/exadg/poisson/spatial_discretization/laplace_operator.cpp
     include/exadg/poisson/preconditioner/multigrid_preconditioner.cpp
     include/exadg/poisson/postprocessor/postprocessor.cpp
     include/exadg/poisson/driver.cpp
     # convection-diffusion equation
     include/exadg/convection_diffusion/user_interface/enum_types.cpp
     include/exadg/convection_diffusion/user_interface/input_parameters.cpp
     include/exadg/convection_diffusion/spatial_discretization/operators/convective_operator.cpp
     include/exadg/convection_diffusion/spatial_discretization/operators/diffusive_operator.cpp
     include/exadg/convection_diffusion/spatial_discretization/operators/combined_operator.cpp
     include/exadg/convection_diffusion/spatial_discretization/operator.cpp
     include/exadg/convection_diffusion/spatial_discretization/interface.cpp
     include/exadg/convection_diffusion/preconditioners/multigrid_preconditioner.cpp
     include/exadg/convection_diffusion/time_integration/time_int_bdf.cpp
     include/exadg/convection_diffusion/time_integration/time_int_explicit_runge_kutta.cpp
     include/exadg/convection_diffusion/time_integration/driver_steady_problems.cpp
     include/exadg/convection_diffusion/postprocessor/postprocessor.cpp
     include/exadg/postprocessor/output_generator_scalar.cpp
     include/exadg/convection_diffusion/driver.cpp
     # incompressible Navier-Stokes equations
     include/exadg/incompressible_navier_stokes/user_interface/input_parameters.cpp
     include/exadg/incompressible_navier_stokes/user_interface/enum_types.cpp
     include/exadg/incompressible_navier_stokes/spatial_discretization/operators/convective_operator.cpp
     include/exadg/incompressible_navier_stokes/spatial_discretization/operators/viscous_operator.cpp
     include/exadg/incompressible_navier_stokes/spatial_discretization/operators/gradient_operator.cpp
     include/exadg/incompressible_navier_stokes/spatial_discretization/operators/divergence_operator.cpp
     include/exadg/incompressible_navier_stokes/spatial_discretization/operators/rhs_operator.cpp
     include/exadg/incompressible_navier_stokes/spatial_discretization/operators/divergence_penalty_operator.cpp
     include/exadg/incompressible_navier_stokes/spatial_discretization/operators/continuity_penalty_operator.cpp
     include/exadg/incompressible_navier_stokes/spatial_discretization/operators/momentum_operator.cpp
     include/exadg/incompressible_navier_stokes/spatial_discretization/operators/projection_operator.cpp
     include/exadg/incompressible_navier_stokes/spatial_discretization/turbulence_model.cpp
     include/exadg/incompressible_navier_stokes/spatial_discretization/spatial_operator_base.cpp
     include/exadg/incompressible_navier_stokes/spatial_discretization/operator_projection_methods.cpp
     include/exadg/incompressible_navier_stokes/spatial_discretization/operator_dual_splitting.cpp
     include/exadg/incompressible_navier_stokes/spatial_discretization/operator_pressure_correction.cpp
     include/exadg/incompressible_navier_stokes/spatial_discretization/operator_coupled.cpp
     include/exadg/incompressible_navier_stokes/spatial_discretization/calculators/divergence_calculator.cpp
     include/exadg/incompressible_navier_stokes/spatial_discretization/calculators/vorticity_calculator.cpp
     include/exadg/incompressible_navier_stokes/spatial_discretization/calculators/velocity_magnitude_calculator.cpp
     include/exadg/incompressible_navier_stokes/spatial_discretization/calculators/q_criterion_calculator.cpp
     include/exadg/incompressible_navier_stokes/spatial_discretization/calculators/streamfunction_calculator_rhs_operator.cpp
     include/exadg/incompressible_navier_stokes/preconditioners/multigrid_preconditioner_momentum.cpp
     include/exadg/incompressible_navier_stokes/preconditioners/multigrid_preconditioner_projection.cpp
     include/exadg/incompressible_navier_stokes/time_integration/time_int_bdf.cpp
     include/exadg/incompressible_navier_stokes/time_integration/time_int_bdf_coupled_solver.cpp
     include/exadg/incompressible_navier_stokes/time_integration/time_int_bdf_dual_splitting.cpp
     include/exadg/incompressible_navier_stokes/time_integration/time_int_bdf_pressure_correction.cpp
     include/exadg/incompressible_navier_stokes/time_integration/driver_steady_problems.cpp
     include/exadg/incompressible_navier_stokes/postprocessor/output_generator.cpp
     include/exadg/incompressible_navier_stokes/postprocessor/divergence_and_mass_error.cpp
     include/exadg/incompressible_navier_stokes/postprocessor/inflow_data_calculator.cpp
     include/exadg/incompressible_navier_stokes/postprocessor/kinetic_energy_dissipation_detailed.cpp
     include/exadg/incompressible_navier_stokes/postprocessor/line_plot_calculation.cpp
     include/exadg/incompressible_navier_stokes/postprocessor/line_plot_calculation_statistics.cpp
     include/exadg/incompressible_navier_stokes/postprocessor/line_plot_calculation_statistics_homogeneous.cpp
     include/exadg/incompressible_navier_stokes/postprocessor/mean_velocity_calculator.cpp
     include/exadg/incompressible_navier_stokes/postprocessor/flow_rate_calculator.cpp
     include/exadg/incompressible_navier_stokes/postprocessor/postprocessor.cpp
     include/exadg/incompressible_navier_stokes/driver.cpp
     include/exadg/incompressible_navier_stokes/driver_precursor.cpp
     # incompressible flow with transport
     include/exadg/incompressible_flow_with_transport/driver.cpp
     # compressible Navier-Stokes equations
     include/exadg/compressible_navier_stokes/user_interface/enum_types.cpp
     include/exadg/compressible_navier_stokes/user_interface/input_parameters.cpp
     include/exadg/compressible_navier_stokes/time_integration/time_int_explicit_runge_kutta.cpp
     include/exadg/compressible_navier_stokes/spatial_discretization/interface.cpp
     include/exadg/compressible_navier_stokes/spatial_discretization/operator.cpp
     include/exadg/compressible_navier_stokes/postprocessor/output_generator.cpp
     include/exadg/compressible_navier_stokes/postprocessor/postprocessor.cpp
     include/exadg/compressible_navier_stokes/driver.cpp
     # elasticity
     include/exadg/structure/user_interface/enum_types.cpp
     include/exadg/structure/material/library/st_venant_kirchhoff.cpp
     include/exadg/structure/spatial_discretization/operators/elasticity_operator_base.cpp
     include/exadg/structure/spatial_discretization/operators/nonlinear_operator.cpp
     include/exadg/structure/spatial_discretization/operators/linear_operator.cpp
     include/exadg/structure/spatial_discretization/operators/body_force_operator.cpp
     include/exadg/structure/spatial_discretization/operator.cpp
     include/exadg/structure/spatial_discretization/interface.cpp
     include/exadg/structure/preconditioners/multigrid_preconditioner.cpp
     include/exadg/structure/time_integration/driver_steady_problems.cpp
     include/exadg/structure/time_integration/driver_quasi_static_problems.cpp
     include/exadg/structure/time_integration/time_int_gen_alpha.cpp
     include/exadg/structure/postprocessor/output_generator.cpp
     include/exadg/structure/postprocessor/postprocessor.cpp
     include/exadg/structure/driver.cpp
     # fluid-structure-interaction
     include/exadg/fluid_structure_interaction/driver.cpp
     )

# Set the include directories and the name of the project
INCLUDE_DIRECTORIES(include)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/include)

SET(DEAL_II_HAVE_TESTS_DIRECTORY TRUE)

PROJECT(exadg)

OPTION(PICKUP_TESTS "Pick up ExaDG tests." ON)
OPTION(BUILD_SHARED_LIBS "Build shared library." ON)

# Configure config.h
IF(DEGREE_MAX)
  SET(EXADG_DEGREE_MAX ${DEGREE_MAX})
  MESSAGE("Use EXADG_DEGREE_MAX = " ${EXADG_DEGREE_MAX} ".")
ELSE()
  MESSAGE("Use EXADG_DEGREE_MAX defined in exadg/include/exadg/configuration/config.h.in.")
ENDIF()
CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/exadg/configuration/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/exadg/configuration/config.h
    )

ADD_LIBRARY(exadg ${TARGET_SRC})

# FFTW
OPTION(USE_FFTW "Use FFTW" OFF)
IF(${USE_FFTW})
    ADD_DEFINITIONS(-DUSE_FFTW)
    INCLUDE_DIRECTORIES(${FFTW_INCLUDE})
    IF(${BUILD_SHARED_LIBS})
        FIND_LIBRARY(FFTW3 fftw3 HINTS ${FFTW_LIB})
        FIND_LIBRARY(FFTW3_MPI fftw3_mpi HINTS ${FFTW_LIB})
    ELSE()
        FIND_LIBRARY(FFTW3 libfftw3.a HINTS ${FFTW_LIB})
        FIND_LIBRARY(FFTW3_MPI libfftw3_mpi.a HINTS ${FFTW_LIB})
    ENDIF()
    MESSAGE(${FFTW3})
    MESSAGE(${FFTW3_MPI})
    TARGET_LINK_FFTW(exadg)
ENDIF()

# LIKWID
FIND_LIBRARY(LIKWID likwid HINTS ${LIKWID_LIB})
MESSAGE(${LIKWID}) 
IF(LIKWID)
    TARGET_COMPILE_DEFINITIONS(exadg PUBLIC LIKWID_PERFMON)
    TARGET_INCLUDE_DIRECTORIES(exadg PUBLIC ${LIKWID_INCLUDE})
    TARGET_LINK_LIBRARIES(exadg ${LIKWID})
ENDIF()

# DEBUG vs. RELEASE
ADD_CUSTOM_TARGET(debug
  COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Debug ${CMAKE_SOURCE_DIR}
  COMMENT "Switch CMAKE_BUILD_TYPE to Debug\nPlease call 'make' to build the debug project"
  )

ADD_CUSTOM_TARGET(release
  COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release ${CMAKE_SOURCE_DIR}
  COMMENT "Switch CMAKE_BUILD_TYPE to Release\nPlease call 'make' to build the release project"
  )

DEAL_II_INITIALIZE_CACHED_VARIABLES()

DEAL_II_SETUP_TARGET(exadg)

ENABLE_TESTING()

ADD_SUBDIRECTORY(applications)
ADD_SUBDIRECTORY(prototypes)
ADD_SUBDIRECTORY(tests)

ADD_SUBDIRECTORY(doc/doxygen)


