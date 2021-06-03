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

// Elasticity problem: Assembly using self-linearizing residual weak form in
// conjunction with automatic differentiation. This test replicates step-8
// exactly.

#include <deal.II/base/function.h>

#include <deal.II/differentiation/ad.h>

#include <weak_forms/weak_forms.h>

#include "../weak_forms_tests.h"
#include "wf_common_tests/step-8.h"


using namespace dealii;



template <int dim>
class Step8 : public Step8_Base<dim>
{
public:
  Step8();

protected:
  void
  assemble_system() override;
};


template <int dim>
Step8<dim>::Step8()
  : Step8_Base<dim>()
{}


template <int dim>
void
Step8<dim>::assemble_system()
{
  using namespace WeakForms;
  using namespace Differentiation;

  constexpr int  spacedim    = dim;
  constexpr auto ad_typecode = Differentiation::AD::NumberTypes::sacado_dfad;
  using ADNumber_t =
    typename Differentiation::AD::NumberTraits<double, ad_typecode>::ad_type;

  // Symbolic types for test function, and a coefficient.
  const TestFunction<dim>          test;
  const FieldSolution<dim>         solution;
  const SubSpaceExtractors::Vector subspace_extractor(0, "u", "\\mathbf{u}");

  const VectorFunctionFunctor<dim> rhs_coeff("s", "\\mathbf{s}");
  const Coefficient<dim>           coefficient;
  const RightHandSide<dim>         rhs;

  const auto test_ss = test[subspace_extractor];
  const auto soln_ss = solution[subspace_extractor];

  const auto test_val  = test_ss.value();
  const auto test_grad = test_ss.gradient();
  const auto soln_grad = soln_ss.gradient();

  const auto residual_func = residual_functor("R", "R", soln_grad);
  const auto residual_ss   = residual_func[test_grad];
  using ResidualADNumber_t =
    typename decltype(residual_ss)::template ad_type<double, ad_typecode>;
  using Result_t =
    typename decltype(residual_ss)::template value_type<ADNumber_t>;
  static_assert(std::is_same<ADNumber_t, ResidualADNumber_t>::value,
                "Expected identical AD number types");

  const auto residual = residual_ss.template value<ADNumber_t, dim, spacedim>(
    [&coefficient](const MeshWorker::ScratchData<dim, spacedim> &scratch_data,
                   const std::vector<std::string> &              solution_names,
                   const unsigned int                            q_point,
                   const Tensor<2, spacedim, ADNumber_t> &       grad_u) {
      const Point<spacedim> &p = scratch_data.get_quadrature_points()[q_point];
      const auto             C = coefficient.value(p);
      return double_contract<2, 0, 3, 1>(C, grad_u);
    },
    UpdateFlags::update_quadrature_points);

  MatrixBasedAssembler<dim> assembler;
  assembler +=
    residual_form(residual).dV() - linear_form(test_val, rhs_coeff(rhs)).dV();

  // Look at what we're going to compute
  const SymbolicDecorations decorator;
  static bool               output = true;
  if (output)
    {
      deallog << "\n" << std::endl;
      deallog << "Weak form (ascii):\n"
              << assembler.as_ascii(decorator) << std::endl;
      deallog << "Weak form (LaTeX):\n"
              << assembler.as_latex(decorator) << std::endl;
      deallog << "\n" << std::endl;
      output = false;
    }

  // Now we pass in concrete objects to get data from
  // and assemble into.
  const QGauss<dim> qf_cell(this->fe.degree + 1);
  assembler.assemble_system(this->system_matrix,
                            this->system_rhs,
                            this->solution,
                            this->constraints,
                            this->dof_handler,
                            qf_cell);
}


int
main(int argc, char **argv)
{
  initlog();
  deallog << std::setprecision(9);

  Utilities::MPI::MPI_InitFinalize mpi_initialization(
    argc, argv, testing_max_num_threads());

  try
    {
      Step8<2> elastic_problem_2d;
      elastic_problem_2d.run();
    }
  catch (std::exception &exc)
    {
      std::cerr << std::endl
                << std::endl
                << "----------------------------------------------------"
                << std::endl;
      std::cerr << "Exception on processing: " << std::endl
                << exc.what() << std::endl
                << "Aborting!" << std::endl
                << "----------------------------------------------------"
                << std::endl;

      return 1;
    }
  catch (...)
    {
      std::cerr << std::endl
                << std::endl
                << "----------------------------------------------------"
                << std::endl;
      std::cerr << "Unknown exception!" << std::endl
                << "Aborting!" << std::endl
                << "----------------------------------------------------"
                << std::endl;
      return 1;
    }

  deallog << "OK" << std::endl;

  return 0;
}
