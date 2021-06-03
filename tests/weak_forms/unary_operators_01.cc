// ---------------------------------------------------------------------
//
// Copyright (C) 2020 by the deal.II authors
//
// This file is part of the deal.II library.
//
// The deal.II library is free software; you can use it, redistribute
// it, and/or modify it under the terms of the GNU Lesser General
// Public License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// The full text of the license can be found in the file LICENSE at
// the top level of the deal.II distribution.
//
// ---------------------------------------------------------------------


// Check that unary operators work

#include <deal.II/base/function_lib.h>
#include <deal.II/base/quadrature_lib.h>
#include <deal.II/base/tensor_function.h>

#include <deal.II/fe/fe_q.h>
#include <deal.II/fe/fe_values.h>

#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/tria.h>

#include <deal.II/meshworker/scratch_data.h>

#include <deal.II/numerics/vector_tools.h>

#include <weak_forms/functors.h>
#include <weak_forms/mixed_operators.h>
#include <weak_forms/solution_storage.h>
#include <weak_forms/spaces.h>
#include <weak_forms/subspace_extractors.h>
#include <weak_forms/subspace_views.h>
#include <weak_forms/symbolic_operators.h>
#include <weak_forms/unary_operators.h>

#include "../weak_forms_tests.h"


template <int dim, int spacedim = dim, typename NumberType = double>
void
run()
{
  LogStream::Prefix prefix("Dim " + Utilities::to_string(dim));
  std::cout << "Dim: " << dim << std::endl;

  const FE_Q<dim, spacedim>  fe(1);
  const QGauss<spacedim>     qf_cell(fe.degree + 1);
  const QGauss<spacedim - 1> qf_face(fe.degree + 1);

  Triangulation<dim, spacedim> triangulation;
  GridGenerator::hyper_cube(triangulation);

  DoFHandler<dim, spacedim> dof_handler(triangulation);
  dof_handler.distribute_dofs(fe);

  Vector<double> solution(dof_handler.n_dofs());
  VectorTools::interpolate(dof_handler,
                           Functions::CosineFunction<spacedim>(
                             fe.n_components()),
                           solution);

  const UpdateFlags update_flags_cell =
    update_quadrature_points | update_values | update_gradients |
    update_hessians | update_3rd_derivatives;
  MeshWorker::ScratchData<dim, spacedim> scratch_data(fe,
                                                      qf_cell,
                                                      update_flags_cell);

  const auto                         cell      = dof_handler.begin_active();
  const FEValuesBase<dim, spacedim> &fe_values = scratch_data.reinit(cell);
  const unsigned int                 q_point   = 0;

  const WeakForms::SolutionStorage<Vector<double>> solution_storage(solution);
  solution_storage.extract_local_dof_values(scratch_data);
  const std::vector<std::string> &solution_names =
    solution_storage.get_solution_names();

  {
    const std::string title = "Scalar";
    std::cout << title << std::endl;
    deallog << title << std::endl;

    using namespace WeakForms;

    const ScalarFunctor c1("c1", "c1");
    const auto          f1 =
      value<double, dim, spacedim>(c1,
                                   [](const FEValuesBase<dim, spacedim> &,
                                      const unsigned int) { return 2.0; });

    std::cout << "Scalar negation: "
              << ((-f1).template operator()<NumberType>(fe_values))[q_point]
              << std::endl;

    std::cout << "Scalar square root: "
              << ((sqrt(f1)).template operator()<NumberType>(
                   fe_values))[q_point]
              << std::endl;

    deallog << "OK" << std::endl;
  }

  {
    const std::string title = "Vector";
    std::cout << title << std::endl;
    deallog << title << std::endl;

    using namespace WeakForms;

    const VectorFunctor<dim> v1("v1", "v1");
    const auto               f1 = value<double, spacedim>(
      v1, [](const FEValuesBase<dim, spacedim> &, const unsigned int) {
        Tensor<1, dim> t;
        for (auto it = t.begin_raw(); it != t.end_raw(); ++it)
          *it = 2.0;
        return t;
      });

    std::cout << "Vector negation: "
              << ((-f1).template operator()<NumberType>(fe_values))[q_point]
              << std::endl;

    deallog << "OK" << std::endl;
  }

  {
    const std::string title = "Tensor";
    std::cout << title << std::endl;
    deallog << title << std::endl;

    using namespace WeakForms;

    const TensorFunctor<2, dim> T1("T1", "T1");
    const auto                  f1 = value<double, spacedim>(
      T1, [](const FEValuesBase<dim, spacedim> &, const unsigned int) {
        Tensor<2, dim> t;
        for (auto it = t.begin_raw(); it != t.end_raw(); ++it)
          *it = 2.0;
        return t;
      });

    std::cout << "Tensor negation: "
              << ((-f1).template operator()<NumberType>(fe_values))[q_point]
              << std::endl;
    std::cout
      << "Tensor determinant: "
      << ((determinant(f1)).template operator()<NumberType>(fe_values))[q_point]
      << std::endl;
    std::cout
      << "Tensor inverse: "
      << ((invert(f1)).template operator()<NumberType>(fe_values))[q_point]
      << std::endl;
    std::cout
      << "Tensor transpose: "
      << ((transpose(f1)).template operator()<NumberType>(fe_values))[q_point]
      << std::endl;
    std::cout
      << "Tensor symmetrized: "
      << ((symmetrize(f1)).template operator()<NumberType>(fe_values))[q_point]
      << std::endl;

    static_assert(decltype(symmetrize(f1))::rank == 2, "Incorrect rank");

    deallog << "OK" << std::endl;
  }

  {
    const std::string title = "SymmetricTensor";
    std::cout << title << std::endl;
    deallog << title << std::endl;

    using namespace WeakForms;

    const SymmetricTensorFunctor<2, dim> S1("S1", "S1");
    const auto                           f1 = value<double, spacedim>(
      S1, [](const FEValuesBase<dim, spacedim> &, const unsigned int) {
        SymmetricTensor<2, dim> t;
        for (auto it = t.begin_raw(); it != t.end_raw(); ++it)
          *it = 2.0;
        return t;
      });

    const FieldSolution<dim, spacedim> field_solution;
    const SubSpaceExtractors::Scalar   subspace_extractor(0, "s", "s");
    const auto field_solution_ss = field_solution[subspace_extractor];

    const auto value            = field_solution_ss.value();
    const auto gradient         = field_solution_ss.gradient();
    const auto laplacian        = field_solution_ss.laplacian();
    const auto hessian          = field_solution_ss.hessian();
    const auto third_derivative = field_solution_ss.third_derivative();

    std::cout << "SymmetricTensor negation: "
              << ((-f1).template operator()<NumberType>(fe_values))[q_point]
              << std::endl;
    std::cout
      << "SymmetricTensor determinant: "
      << ((determinant(f1)).template operator()<NumberType>(fe_values))[q_point]
      << std::endl;
    std::cout
      << "SymmetricTensor inverse: "
      << ((invert(f1)).template operator()<NumberType>(fe_values))[q_point]
      << std::endl;
    std::cout
      << "SymmetricTensor transpose: "
      << ((transpose(f1)).template operator()<NumberType>(fe_values))[q_point]
      << std::endl;

    deallog << "OK" << std::endl;
  }

  {
    const std::string title = "Field solution";
    std::cout << title << std::endl;
    deallog << title << std::endl;

    using namespace WeakForms;

    const FieldSolution<dim, spacedim> field_solution;
    const SubSpaceExtractors::Scalar   subspace_extractor(0, "s", "s");
    const auto field_solution_ss = field_solution[subspace_extractor];

    const auto value            = field_solution_ss.value();
    const auto gradient         = field_solution_ss.gradient();
    const auto laplacian        = field_solution_ss.laplacian();
    const auto hessian          = field_solution_ss.hessian();
    const auto third_derivative = field_solution_ss.third_derivative();

    std::cout << "value negation: "
              << ((-value).template operator()<NumberType>(
                   fe_values, scratch_data, solution_names))[q_point]
              << std::endl;
    std::cout << "gradient negation: "
              << ((-gradient).template operator()<NumberType>(
                   fe_values, scratch_data, solution_names))[q_point]
              << std::endl;
    std::cout << "Laplacian negation: "
              << ((-laplacian)
                    .template operator()<NumberType>(fe_values,
                                                     scratch_data,
                                                     solution_names))[q_point]
              << std::endl;
    std::cout << "Hessian negation: "
              << ((-hessian).template operator()<NumberType>(
                   fe_values, scratch_data, solution_names))[q_point]
              << std::endl;
    std::cout << "third derivative negation: "
              << ((-third_derivative)
                    .template operator()<NumberType>(fe_values,
                                                     scratch_data,
                                                     solution_names))[q_point]
              << std::endl;


    std::cout << "Hessian determinant: "
              << ((determinant(hessian))
                    .template operator()<NumberType>(fe_values,
                                                     scratch_data,
                                                     solution_names))[q_point]
              << std::endl;
    std::cout << "Hessian inverse: "
              << ((invert(hessian))
                    .template operator()<NumberType>(fe_values,
                                                     scratch_data,
                                                     solution_names))[q_point]
              << std::endl;
    std::cout << "Hessian transpose: "
              << ((transpose(hessian))
                    .template operator()<NumberType>(fe_values,
                                                     scratch_data,
                                                     solution_names))[q_point]
              << std::endl;

    deallog << "OK" << std::endl;
  }

  deallog << "OK" << std::endl;
}


int
main(int argc, char *argv[])
{
  initlog();
  Utilities::MPI::MPI_InitFinalize mpi_initialization(
    argc, argv, testing_max_num_threads());

  run<2>();
  run<3>();

  deallog << "OK" << std::endl;
}
