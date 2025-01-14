// ---------------------------------------------------------------------
//
// Copyright (C) 2021 - 2022 by Jean-Paul Pelteret
//
// This file is part of the Weak forms for deal.II library.
//
// The Weak forms for deal.II library is free software; you can use it,
// redistribute it, and/or modify it under the terms of the GNU Lesser
// General Public License as published by the Free Software Foundation;
// either version 3.0 of the License, or (at your option) any later
// version. The full text of the license can be found in the file LICENSE
// at the top level of the Weak forms for deal.II distribution.
//
// ---------------------------------------------------------------------

#ifndef dealii_weakforms_linear_forms_h
#define dealii_weakforms_linear_forms_h

#include <deal.II/base/config.h>

#include <deal.II/base/template_constraints.h>

#include <weak_forms/config.h>
#include <weak_forms/functors.h>
#include <weak_forms/spaces.h>
#include <weak_forms/symbolic_integral.h>
#include <weak_forms/template_constraints.h>
#include <weak_forms/type_traits.h>


WEAK_FORMS_NAMESPACE_OPEN


namespace WeakForms
{
  template <typename TestSpaceOp_, typename Functor_>
  class LinearForm
  {
    static_assert(is_or_has_test_function_op<TestSpaceOp_>::value,
                  "Expected a test function.");
    static_assert(
      is_valid_form_functor<Functor_>::value,
      "Expected the functor to a linear form to either be a SymbolicOp, or that the unary or binary operation does not include a test function or trial solution as a lead operation.");
    static_assert(!is_symbolic_integral_op<Functor_>::value,
                  "Functor cannot be an integral.");

  public:
    using TestSpaceOp = TestSpaceOp_;
    using Functor     = Functor_;

    explicit LinearForm(const TestSpaceOp &test_space_op,
                        const Functor &    functor_op)
      : test_space_op(test_space_op)
      , functor_op(functor_op)
    {}

    std::string
    as_ascii(const SymbolicDecorations &decorator) const
    {
      return "(" + test_space_op.as_ascii(decorator) + ", " +
             functor_op.as_ascii(decorator) + ")";
    }

    std::string
    as_latex(const SymbolicDecorations &decorator) const
    {
      const std::string lbrace = Utilities::LaTeX::l_square_brace();
      const std::string rbrace = Utilities::LaTeX::r_square_brace();

      constexpr unsigned int n_contracting_indices_tf = WeakForms::Utilities::
        FullIndexContraction<TestSpaceOp, Functor>::n_contracting_indices;
      const std::string symb_mult_tf =
        Utilities::LaTeX::get_symbol_multiply(n_contracting_indices_tf);

      return lbrace + test_space_op.as_latex(decorator) + symb_mult_tf +
             functor_op.as_latex(decorator) + rbrace;
    }

    // ===== Section: Integration =====

    template <typename ScalarType = double>
    auto
    dV() const
    {
      return VolumeIntegral().template integrate<ScalarType>(*this);
    }

    template <typename ScalarType = double>
    auto
    dV(const typename VolumeIntegral::subdomain_t subdomain) const
    {
      return dV<ScalarType>(
        std::set<typename VolumeIntegral::subdomain_t>{subdomain});
    }

    template <typename ScalarType = double>
    auto
    dV(const std::set<typename VolumeIntegral::subdomain_t> &subdomains) const
    {
      return VolumeIntegral(subdomains).template integrate<ScalarType>(*this);
    }

    template <typename ScalarType = double>
    auto
    dA() const
    {
      return BoundaryIntegral().template integrate<ScalarType>(*this);
    }

    template <typename ScalarType = double>
    auto
    dA(const typename BoundaryIntegral::subdomain_t boundary) const
    {
      return dA<ScalarType>(
        std::set<typename BoundaryIntegral::subdomain_t>{boundary});
    }

    template <typename ScalarType = double>
    auto
    dA(const std::set<typename BoundaryIntegral::subdomain_t> &boundaries) const
    {
      return BoundaryIntegral(boundaries).template integrate<ScalarType>(*this);
    }

    template <typename ScalarType = double>
    auto
    dI() const
    {
      return InterfaceIntegral().template integrate<ScalarType>(*this);
    }

    template <typename ScalarType = double>
    auto
    dI(const typename InterfaceIntegral::subdomain_t interface) const
    {
      return dI<ScalarType>(
        std::set<typename InterfaceIntegral::subdomain_t>{interface});
    }

    template <typename ScalarType = double>
    auto
    dI(
      const std::set<typename InterfaceIntegral::subdomain_t> &interfaces) const
    {
      return InterfaceIntegral(interfaces)
        .template integrate<ScalarType>(*this);
    }

    // ===== Section: Construct assembly operation =====

    UpdateFlags
    get_update_flags() const
    {
      return test_space_op.get_update_flags() | functor_op.get_update_flags();
    }

    const TestSpaceOp &
    get_test_space_operation() const
    {
      return test_space_op;
    }

    const Functor &
    get_functor() const
    {
      return functor_op;
    }

  private:
    const TestSpaceOp test_space_op;
    const Functor     functor_op;
  };

} // namespace WeakForms



/* ======================== Convenience functions ======================== */



namespace WeakForms
{
  template <
    typename TestSpaceOp,
    typename Functor,
    typename = typename std::enable_if<is_valid_form_functor<Functor>::value &&
                                       !is_scalar_type<Functor>::value>::type>
  LinearForm<TestSpaceOp, Functor>
  linear_form(const TestSpaceOp &test_space_op, const Functor &functor_op)
  {
    return LinearForm<TestSpaceOp, Functor>(test_space_op, functor_op);
  }


  template <typename TestSpaceOp,
            typename ScalarType,
            typename = typename std::enable_if<
              !is_valid_form_functor<ScalarType>::value &&
              is_scalar_type<ScalarType>::value>::type>
  auto
  linear_form(const TestSpaceOp &test_space_op, const ScalarType &value)
  {
    constexpr int dim      = TestSpaceOp::dimension;
    constexpr int spacedim = TestSpaceOp::space_dimension;
    // Delegate to the other function
    return linear_form(test_space_op, constant_scalar<dim, spacedim>(value));
  }


  template <typename TestSpaceOp,
            int rank,
            int spacedim,
            typename ScalarType,
            typename = typename is_scalar_type<ScalarType>::type>
  auto
  linear_form(const TestSpaceOp &                       test_space_op,
              const Tensor<rank, spacedim, ScalarType> &value)
  {
    constexpr int dim = TestSpaceOp::dimension;
    // Delegate to the other function
    return linear_form(test_space_op, constant_tensor<dim>(value));
  }


  template <typename TestSpaceOp,
            int rank,
            int spacedim,
            typename ScalarType,
            typename = typename is_scalar_type<ScalarType>::type>
  auto
  linear_form(const TestSpaceOp &                                test_space_op,
              const SymmetricTensor<rank, spacedim, ScalarType> &value)
  {
    constexpr int dim = TestSpaceOp::dimension;
    // Delegate to the other function
    return linear_form(test_space_op, constant_symmetric_tensor<dim>(value));
  }

} // namespace WeakForms



/* ==================== Specialization of type traits ==================== */



#ifndef DOXYGEN


namespace WeakForms
{
  template <typename TestSpaceOp, typename Functor>
  struct is_linear_form<LinearForm<TestSpaceOp, Functor>> : std::true_type
  {};

} // namespace WeakForms


#endif // DOXYGEN


WEAK_FORMS_NAMESPACE_CLOSE

#endif // dealii_weakforms_linear_forms_h
