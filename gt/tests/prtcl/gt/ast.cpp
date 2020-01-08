#include <catch.hpp>

#include <prtcl/gt/ast/node.hpp>

#include <iostream>

#include <boost/property_tree/ptree.hpp>

enum class prog_expr_kind {
  foreach_particle,
  foreach_neighbor,
};

TEST_CASE("prtcl/gt/ast", "[prtcl]") {
  namespace ast = prtcl::gt::ast;

  auto pr = std::make_unique<ast::procedure>("test");

  pr->add_child(ast::foreach_particle{})([](auto *c_) {
      c_->add_child(ast::if_group_type{"fluid"})([](auto *c_) {
        c_->add_child(ast::foreach_neighbor{})(
            [](auto *c_) { c_->add_child(ast::if_group_type{"fluid"}); });
      });
    })
      ->add_child(ast::equation{
          (new ast::assignment{"="})
              ->add_child(
                  ast::global_field{"g", ast::nd_dtype::real, ast::nd_shape{0}})
              ->add_child(ast::global_field{"h", ast::nd_dtype::real,
                                            ast::nd_shape{0}})});

  ast::ast_latex_printer{std::cerr}.print(pr.get());
}
