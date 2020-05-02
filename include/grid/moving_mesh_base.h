/*
 * moving_mesh_base.h
 *
 *  Created on: 02.05.2020
 *      Author: fehn
 */

#ifndef INCLUDE_GRID_MOVING_MESH_BASE_H_
#define INCLUDE_GRID_MOVING_MESH_BASE_H_

// deal.II
#include <deal.II/dofs/dof_tools.h>
#include <deal.II/fe/fe_nothing.h>
#include <deal.II/fe/fe_system.h>
#include <deal.II/fe/fe_tools.h>
#include <deal.II/fe/fe_values.h>
#include <deal.II/fe/mapping_q_cache.h>
#include <deal.II/lac/la_parallel_block_vector.h>

#include "mesh.h"

using namespace dealii;

template<int dim, typename Number>
class MovingMeshBase : public Mesh<dim>
{
public:
  typedef LinearAlgebra::distributed::Vector<Number> VectorType;

  MovingMeshBase(unsigned int const mapping_degree_static_in,
                 unsigned int const mapping_degree_moving_in,
                 MPI_Comm const &   mpi_comm_in)
    : Mesh<dim>(mapping_degree_static_in), mpi_comm(mpi_comm_in)
  {
    mapping_ale.reset(new MappingQCache<dim>(mapping_degree_moving_in));
    hierarchic_to_lexicographic_numbering =
      FETools::hierarchic_to_lexicographic_numbering<dim>(mapping_degree_moving_in);
    lexicographic_to_hierarchic_numbering =
      Utilities::invert_permutation(hierarchic_to_lexicographic_numbering);
  }

  virtual ~MovingMeshBase()
  {
  }

  virtual void
  move_mesh(double const time) = 0;

  Mapping<dim> const &
  get_mapping() const override
  {
    if(mapping_ale.get() == 0)
      return *this->mapping;
    else
      return *mapping_ale;
  }

  /*
   * This function extracts the grid coordinates of the current mesh configuration, i.e.,
   * a mapping describing the mesh displacement has to be used here.
   */
  void
  fill_grid_coordinates_vector(VectorType & vector, DoFHandler<dim> const & dof_handler)
  {
    Mapping<dim> const & mapping = *mapping_ale;

    if(vector.size() != dof_handler.n_dofs())
    {
      IndexSet relevant_dofs_grid;
      DoFTools::extract_locally_relevant_dofs(dof_handler, relevant_dofs_grid);
      vector.reinit(dof_handler.locally_owned_dofs(), relevant_dofs_grid, mpi_comm);
    }
    else
      vector = 0;

    FiniteElement<dim> const & fe = dof_handler.get_fe();

    // Set up FEValues with base element and the Gauss-Lobatto quadrature to
    // reduce setup cost, as we only use the geometry information (this means
    // we need to call fe_values.reinit(cell) with Triangulation::cell_iterator
    // rather than DoFHandler::cell_iterator).
    FE_Nothing<dim> dummy_fe;
    FEValues<dim>   fe_values(mapping,
                            dummy_fe,
                            QGaussLobatto<dim>(fe.degree + 1),
                            update_quadrature_points);

    std::vector<types::global_dof_index> dof_indices(fe.dofs_per_cell);

    std::vector<std::array<unsigned int, dim>> component_to_system_index(
      fe.base_element(0).dofs_per_cell);

    if(fe.dofs_per_vertex > 0) // FE_Q
    {
      for(unsigned int i = 0; i < fe.dofs_per_cell; ++i)
      {
        component_to_system_index
          [hierarchic_to_lexicographic_numbering[fe.system_to_component_index(i).second]]
          [fe.system_to_component_index(i).first] = i;
      }
    }
    else // FE_DGQ
    {
      for(unsigned int i = 0; i < fe.dofs_per_cell; ++i)
      {
        component_to_system_index[fe.system_to_component_index(i).second]
                                 [fe.system_to_component_index(i).first] = i;
      }
    }

    for(auto const & cell : dof_handler.active_cell_iterators())
    {
      if(!cell->is_artificial())
      {
        fe_values.reinit(typename Triangulation<dim>::cell_iterator(cell));
        cell->get_dof_indices(dof_indices);
        for(unsigned int i = 0; i < fe_values.n_quadrature_points; ++i)
        {
          Point<dim> const point = fe_values.quadrature_point(i);
          for(unsigned int d = 0; d < dim; ++d)
            if(vector.get_partitioner()->in_local_range(
                 dof_indices[component_to_system_index[i][d]]))
              vector(dof_indices[component_to_system_index[i][d]]) = point[d];
        }
      }
    }

    vector.update_ghost_values();
  }

protected:
  std::shared_ptr<MappingQCache<dim>> mapping_ale;
  std::vector<unsigned int>           hierarchic_to_lexicographic_numbering;
  std::vector<unsigned int>           lexicographic_to_hierarchic_numbering;

private:
  // MPI communicator
  MPI_Comm const & mpi_comm;
};



#endif /* INCLUDE_GRID_MOVING_MESH_BASE_H_ */
