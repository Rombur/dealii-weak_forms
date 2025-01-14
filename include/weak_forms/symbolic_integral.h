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

#ifndef dealii_weakforms_symbolic_integral_h
#define dealii_weakforms_symbolic_integral_h

#include <deal.II/base/config.h>

#include <weak_forms/config.h>

// TODO: Move FeValuesViews::[Scalar/Vector/...]::Output<> into another header??
#include <deal.II/fe/fe_values.h>

#include <weak_forms/numbers.h>
#include <weak_forms/symbolic_decorations.h>
#include <weak_forms/symbolic_operators.h>
#include <weak_forms/type_traits.h>
#include <weak_forms/types.h>


WEAK_FORMS_NAMESPACE_OPEN


namespace WeakForms
{
  template <typename SubDomainType>
  class Integral
  {
  public:
    template <typename ScalarType>
    using value_type = double;

    Integral(const std::set<SubDomainType> &subdomains)
      : subdomains(subdomains)
    {}

    bool
    integrate_over_entire_domain() const
    {
      constexpr SubDomainType invalid_index = -1;
      return subdomains.empty() ||
             (subdomains.size() == 1 && *subdomains.begin() == invalid_index);
    }

    // ----  Ascii ----

    std::string
    as_ascii(const SymbolicDecorations &decorator) const
    {
      return get_infinitesimal_symbol_ascii(decorator);
    }


    std::string
    get_subdomain_as_ascii(const SymbolicDecorations &decorator) const
    {
      (void)decorator;

      // Expand the set of subdomains as a comma separated list
      return Utilities::get_comma_separated_string_from(get_subdomains());
    }

    virtual std::string
    get_symbol_ascii(const SymbolicDecorations &decorator) const = 0;

    virtual std::string
    get_infinitesimal_symbol_ascii(
      const SymbolicDecorations &decorator) const = 0;

    // ---- LaTeX ----

    std::string
    as_latex(const SymbolicDecorations &decorator) const
    {
      return get_infinitesimal_symbol_latex(decorator);
    }

    std::string
    get_subdomain_as_latex(const SymbolicDecorations &decorator) const
    {
      (void)decorator;

      // Expand the set of subdomains as a comma separated list
      return Utilities::get_comma_separated_string_from(get_subdomains());
    }

    virtual std::string
    get_symbol_latex(const SymbolicDecorations &decorator) const = 0;

    virtual std::string
    get_infinitesimal_symbol_latex(
      const SymbolicDecorations &decorator) const = 0;

  protected:
    // Dictate whether to integrate over the whole
    // volume / boundary / interface, or just a
    // part of it. The invalid index SubDomainType(-1)
    // also indicates that the entire domain is to be
    // integrated over.
    const std::set<SubDomainType> subdomains;

    bool
    integrate_on_subdomain(const SubDomainType &idx) const
    {
      if (integrate_over_entire_domain())
        return true;

      return subdomains.find(idx) != subdomains.end();
    }

    const std::set<SubDomainType> &
    get_subdomains() const
    {
      return subdomains;
    }
  };



  class VolumeIntegral : public Integral<dealii::types::material_id>
  {
  public:
    using subdomain_t = dealii::types::material_id;

    VolumeIntegral(const std::set<subdomain_t> &subregions)
      : Integral<subdomain_t>(subregions)
    {}

    VolumeIntegral()
      : VolumeIntegral(std::set<subdomain_t>{})
    {}

    std::string
    get_symbol_ascii(const SymbolicDecorations &decorator) const override
    {
      const auto &naming = decorator.get_naming_ascii().geometry;
      return naming.volume;
    }

    std::string
    get_symbol_latex(const SymbolicDecorations &decorator) const override
    {
      const auto &naming = decorator.get_naming_latex().geometry;
      return naming.volume;
    }

    std::string
    get_infinitesimal_symbol_ascii(
      const SymbolicDecorations &decorator) const override
    {
      const auto &naming = decorator.get_naming_ascii().differential_geometry;
      return naming.infinitesimal_element_volume;
    }

    std::string
    get_infinitesimal_symbol_latex(
      const SymbolicDecorations &decorator) const override
    {
      const auto &naming = decorator.get_naming_latex().differential_geometry;
      return naming.infinitesimal_element_volume;
    }

    template <typename CellIteratorType>
    bool
    integrate_on_cell(const CellIteratorType &cell) const
    {
      return integrate_on_subdomain(cell->material_id());
    }

    // Methods to promote this class to a SymbolicOp

    template <typename ScalarType = double, typename Integrand>
    auto
    integrate(const Integrand &integrand) const;
  };



  class BoundaryIntegral : public Integral<dealii::types::boundary_id>
  {
  public:
    using subdomain_t = dealii::types::boundary_id;

    BoundaryIntegral(const std::set<subdomain_t> &boundaries)
      : Integral<subdomain_t>(boundaries)
    {}

    BoundaryIntegral()
      : BoundaryIntegral(std::set<subdomain_t>{})
    {}

    std::string
    get_symbol_ascii(const SymbolicDecorations &decorator) const override
    {
      const auto &naming = decorator.get_naming_ascii().geometry;
      return naming.boundary;
    }

    std::string
    get_symbol_latex(const SymbolicDecorations &decorator) const override
    {
      const auto &naming = decorator.get_naming_latex().geometry;
      return naming.boundary;
    }

    std::string
    get_infinitesimal_symbol_ascii(
      const SymbolicDecorations &decorator) const override
    {
      const auto &naming = decorator.get_naming_ascii().differential_geometry;
      return naming.infinitesimal_element_boundary_area;
    }

    std::string
    get_infinitesimal_symbol_latex(
      const SymbolicDecorations &decorator) const override
    {
      const auto &naming = decorator.get_naming_latex().differential_geometry;
      return naming.infinitesimal_element_boundary_area;
    }

    template <typename CellIteratorType>
    bool
    integrate_on_face(const CellIteratorType &cell,
                      const unsigned int      face) const
    {
      if (!cell->face(face)->at_boundary())
        return false;

      return integrate_on_subdomain(cell->face(face)->boundary_id());
    }

    // Methods to promote this class to a SymbolicOp

    template <typename ScalarType = double, typename Integrand>
    auto
    integrate(const Integrand &integrand) const;
  };



  class InterfaceIntegral : public Integral<dealii::types::manifold_id>
  {
  public:
    using subdomain_t = dealii::types::manifold_id;

    InterfaceIntegral(const std::set<subdomain_t> interfaces)
      : Integral<subdomain_t>(interfaces)
    {}

    InterfaceIntegral()
      : InterfaceIntegral(std::set<subdomain_t>{})
    {}

    std::string
    get_symbol_ascii(const SymbolicDecorations &decorator) const override
    {
      const auto &naming = decorator.get_naming_ascii().geometry;
      return naming.interface;
    }

    std::string
    get_symbol_latex(const SymbolicDecorations &decorator) const override
    {
      const auto &naming = decorator.get_naming_latex().geometry;
      return naming.interface;
    }

    std::string
    get_infinitesimal_symbol_ascii(
      const SymbolicDecorations &decorator) const override
    {
      const auto &naming = decorator.get_naming_ascii().differential_geometry;
      return naming.infinitesimal_element_interface_area;
    }

    std::string
    get_infinitesimal_symbol_latex(
      const SymbolicDecorations &decorator) const override
    {
      const auto &naming = decorator.get_naming_latex().differential_geometry;
      return naming.infinitesimal_element_interface_area;
    }

    template <typename CellIteratorType>
    bool
    integrate_on_face(const CellIteratorType &cell,
                      const unsigned int      face,
                      const unsigned int      neighbour_face) const
    {
      (void)neighbour_face;
      if (cell->face(face)->at_boundary())
        return false;

      return integrate_on_subdomain(cell->face(face)->manifold_id());
    }

    // Methods to promote this class to a SymbolicOp

    template <typename ScalarType = double, typename Integrand>
    auto
    integrate(const Integrand &integrand) const;
  };



  // class CurveIntegral : public Integral
  // {
  // public:
  //   CurveIntegral(const SymbolicDecorations &decorator =
  //   SymbolicDecorations())
  //     : Integral(decorator.naming_ascii.infinitesimal_element_curve_length,
  //                    decorator.naming_latex.infinitesimal_element_curve_length,
  //                    decorator)
  //   {}
  // };

} // namespace WeakForms



/* ======================== Convenience functions ======================== */



namespace WeakForms
{
  template <typename ScalarType = double,
            typename Integrand,
            typename IntegralType,
            typename = typename std::enable_if<is_valid_integration_domain<
              typename std::decay<IntegralType>::type>::value>::type>
  WeakForms::Operators::SymbolicOp<IntegralType,
                                   WeakForms::Operators::SymbolicOpCodes::value,
                                   ScalarType,
                                   Integrand>
  integrate(const Integrand &integrand, const IntegralType &integral)
  {
    return integral.template integrate<ScalarType>(integrand);
  }

  // template <typename ScalarType = double,
  //           typename Integrand,
  //           typename IntegralType,
  //           typename = typename std::enable_if<
  //             WeakForms::is_unary_integral_op<IntegralType>::value>::type>
  // auto
  // integrate(const Integrand &integrand, const IntegralType &integral, const
  // std::set<typename IntegralType::subdomain_t> &subdomains)
  // {
  //   return value(integral, integrand);
  // }

} // namespace WeakForms



/* ================== Specialization of unary operators ================== */



namespace WeakForms
{
  namespace Operators
  {
    /**
     * Get the weighted Jacobians for numerical integration
     *
     * @tparam dim
     * @tparam spacedim
     */
    template <typename ScalarType_,
              typename IntegralType_,
              typename IntegrandType_>
    class SymbolicOp<IntegralType_,
                     SymbolicOpCodes::value,
                     ScalarType_,
                     IntegrandType_>
    {
      static_assert(!is_integral_op<IntegrandType_>::value,
                    "Cannot integrate an integral!");

    public:
      using ScalarType    = ScalarType_;
      using IntegralType  = IntegralType_;
      using IntegrandType = IntegrandType_;

      template <typename ScalarType2>
      using value_type =
        typename IntegralType::template value_type<ScalarType2>;

      template <typename ScalarType2>
      using return_type = std::vector<value_type<ScalarType2>>;

      template <typename ResultScalarType, std::size_t width>
      using vectorized_value_type = typename numbers::VectorizedValue<
        value_type<ResultScalarType>>::template type<width>;

      template <typename ResultScalarType, std::size_t width>
      using vectorized_return_type = typename numbers::VectorizedValue<
        value_type<ResultScalarType>>::template type<width>;

      static const int rank = 0;

      // static const enum SymbolicOpCodes op_code = SymbolicOpCodes::value;

      explicit SymbolicOp(const IntegralType & integral_operation,
                          const IntegrandType &integrand)
        : integral_operation(integral_operation)
        , integrand(integrand)
      {}

      bool
      integrate_over_entire_domain() const
      {
        return integral_operation.integrate_over_entire_domain();
      }

      std::string
      as_ascii(const SymbolicDecorations &decorator) const
      {
        return decorator.symbolic_op_integral_as_ascii(integrand,
                                                       integral_operation);
      }

      std::string
      as_latex(const SymbolicDecorations &decorator) const
      {
        return decorator.symbolic_op_integral_as_latex(integrand,
                                                       integral_operation);
      }

      std::string
      get_subdomain_as_ascii(const SymbolicDecorations &decorator) const
      {
        return integral_operation.get_subdomain_as_ascii(decorator);
      }

      std::string
      get_subdomain_as_latex(const SymbolicDecorations &decorator) const
      {
        return integral_operation.get_subdomain_as_latex(decorator);
      }

      // ===== Section: Construct assembly operation =====

      const IntegralType &
      get_integral_operation() const
      {
        return integral_operation;
      }

      const IntegrandType &
      get_integrand() const
      {
        return integrand;
      }

      // ===== Section: Perform actions =====

      UpdateFlags
      get_update_flags() const
      {
        return get_integrand().get_update_flags() |
               UpdateFlags::update_JxW_values;
      }

      /**
       * Return all JxW values at all quadrature points
       */
      template <typename ScalarType2, int dim, int spacedim>
      const return_type<ScalarType2> &
      operator()(const FEValuesBase<dim, spacedim> &fe_values) const
      {
        return fe_values.get_JxW_values();
      }

      template <typename ScalarType2, int dim, int spacedim>
      const return_type<ScalarType2> &
      operator()(
        const FEInterfaceValues<dim, spacedim> &fe_interface_values) const
      {
        return fe_interface_values.get_JxW_values();
      }

      template <typename ResultScalarType,
                std::size_t width,
                int         dim,
                int         spacedim>
      vectorized_return_type<ResultScalarType, width>
      operator()(const FEValuesBase<dim, spacedim> & fe_values,
                 const types::vectorized_qp_range_t &q_point_range) const
      {
        vectorized_return_type<ResultScalarType, width> out;
        Assert(q_point_range.size() <= width,
               ExcIndexRange(q_point_range.size(), 0, width));

        DEAL_II_OPENMP_SIMD_PRAGMA
        for (unsigned int i = 0; i < q_point_range.size(); ++i)
          if (q_point_range[i] < fe_values.n_quadrature_points)
            numbers::set_vectorized_values(
              out,
              i,
              this->template operator()<ResultScalarType>(fe_values,
                                                          q_point_range[i]));

        return out;
      }

      template <typename ResultScalarType,
                std::size_t width,
                int         dim,
                int         spacedim>
      vectorized_return_type<ResultScalarType, width>
      operator()(const FEInterfaceValues<dim, spacedim> &fe_interface_values,
                 const types::vectorized_qp_range_t &    q_point_range) const
      {
        vectorized_return_type<ResultScalarType, width> out;
        Assert(q_point_range.size() <= width,
               ExcIndexRange(q_point_range.size(), 0, width));

        DEAL_II_OPENMP_SIMD_PRAGMA
        for (unsigned int i = 0; i < q_point_range.size(); ++i)
          if (q_point_range[i] < fe_interface_values.n_quadrature_points)
            numbers::set_vectorized_values(
              out,
              i,
              this->template operator()<ResultScalarType>(fe_interface_values,
                                                          q_point_range[i]));

        return out;
      }

    private:
      const IntegralType  integral_operation;
      const IntegrandType integrand;

      // Return single entry
      template <typename ScalarType2, typename FEValuesType>
      value_type<ScalarType2>
      operator()(const FEValuesType &fe_values,
                 const unsigned int  q_point) const
      {
        Assert(q_point < fe_values.n_quadrature_points,
               ExcIndexRange(q_point, 0, fe_values.n_quadrature_points));

        return fe_values.JxW(q_point);
      }
    };

  } // namespace Operators
} // namespace WeakForms



/* ==================== Class method definitions ==================== */



namespace WeakForms
{
  template <typename ScalarType, typename Integrand>
  DEAL_II_ALWAYS_INLINE inline auto
  WeakForms::VolumeIntegral::integrate(const Integrand &integrand) const
  {
    using namespace WeakForms;
    using namespace WeakForms::Operators;

    using Op = VolumeIntegral;
    using OpType =
      SymbolicOp<Op, SymbolicOpCodes::value, ScalarType, Integrand>;

    const auto &operand = *this;
    return OpType(operand, integrand);
  }


  template <typename ScalarType, typename Integrand>
  DEAL_II_ALWAYS_INLINE inline auto
  WeakForms::BoundaryIntegral::integrate(const Integrand &integrand) const
  {
    using namespace WeakForms;
    using namespace WeakForms::Operators;

    using Op = BoundaryIntegral;
    using OpType =
      SymbolicOp<Op, SymbolicOpCodes::value, ScalarType, Integrand>;

    const auto &operand = *this;
    return OpType(operand, integrand);
  }



  template <typename ScalarType, typename Integrand>
  DEAL_II_ALWAYS_INLINE inline auto
  WeakForms::InterfaceIntegral::integrate(const Integrand &integrand) const
  {
    using namespace WeakForms;
    using namespace WeakForms::Operators;

    using Op = InterfaceIntegral;
    using OpType =
      SymbolicOp<Op, SymbolicOpCodes::value, ScalarType, Integrand>;

    const auto &operand = *this;
    return OpType(operand, integrand);
  }


  // WeakForms::Operators::SymbolicOp<WeakForms::Integral,
  //                               WeakForms::Operators::SymbolicOpCodes::value>
  // value(const WeakForms::CurveIntegral &operand)
  // {
  //   using namespace WeakForms;
  //   using namespace WeakForms::Operators;

  //   using Op     = Integral;
  //   using OpType = SymbolicOp<Op, SymbolicOpCodes::value>;

  //   return OpType(operand);
  // }

} // namespace WeakForms



/* ==================== Specialization of type traits ==================== */



#ifndef DOXYGEN


namespace WeakForms
{
  template <>
  struct is_valid_integration_domain<VolumeIntegral> : std::true_type
  {};

  template <>
  struct is_valid_integration_domain<BoundaryIntegral> : std::true_type
  {};

  template <>
  struct is_valid_integration_domain<InterfaceIntegral> : std::true_type
  {};

  // Decorator classes

  // template <>
  // struct is_volume_integral_op<VolumeIntegral> : std::true_type
  // {};

  // template <>
  // struct is_boundary_integral_op<BoundaryIntegral> : std::true_type
  // {};

  // template <>
  // struct is_interface_integral_op<InterfaceIntegral> : std::true_type
  // {};

  // Unary operators

  template <typename ScalarType,
            typename Integrand,
            enum Operators::SymbolicOpCodes OpCode>
  struct is_volume_integral_op<
    Operators::SymbolicOp<VolumeIntegral, OpCode, ScalarType, Integrand>>
    : std::true_type
  {};

  template <typename ScalarType,
            typename Integrand,
            enum Operators::SymbolicOpCodes OpCode>
  struct is_boundary_integral_op<
    Operators::SymbolicOp<BoundaryIntegral, OpCode, ScalarType, Integrand>>
    : std::true_type
  {};

  template <typename ScalarType,
            typename Integrand,
            enum Operators::SymbolicOpCodes OpCode>
  struct is_interface_integral_op<
    Operators::SymbolicOp<InterfaceIntegral, OpCode, ScalarType, Integrand>>
    : std::true_type
  {};

  template <typename T>
  struct is_symbolic_integral_op<
    T,
    typename std::enable_if<is_volume_integral_op<T>::value ||
                            is_boundary_integral_op<T>::value ||
                            is_interface_integral_op<T>::value>::type>
    : std::true_type
  {};

  // I don't know why, but we need this specialisation here.
  template <typename T>
  struct is_integral_op<
    T,
    typename std::enable_if<is_symbolic_integral_op<T>::value>::type>
    : std::true_type
  {};



  // template <typename ScalarType,
  //           typename Integrand,
  //           enum Operators::SymbolicOpCodes OpCode>
  // struct is_symbolic_op<
  //   Operators::SymbolicOp<VolumeIntegral, OpCode, ScalarType, Integrand>>
  //   : std::true_type
  // {};

  // template <typename ScalarType,
  //           typename Integrand,
  //           enum Operators::SymbolicOpCodes OpCode>
  // struct is_symbolic_op<
  //   Operators::SymbolicOp<BoundaryIntegral, OpCode, ScalarType, Integrand>>
  //   : std::true_type
  // {};

  // template <typename ScalarType,
  //           typename Integrand,
  //           enum Operators::SymbolicOpCodes OpCode>
  // struct is_symbolic_op<
  //   Operators::SymbolicOp<InterfaceIntegral, OpCode, ScalarType, Integrand>>
  //   : std::true_type
  // {};

} // namespace WeakForms


#endif // DOXYGEN


WEAK_FORMS_NAMESPACE_CLOSE

#endif // dealii_weakforms_symbolic_integral_h
