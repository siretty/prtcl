#include <catch.hpp>

#include <prtcl/expr/field.hpp>
#include <prtcl/expr/text_transform.hpp>
#include <prtcl/tags.hpp>

#include <iostream>

#include <sstream>
#include <string>

#include <boost/hana.hpp>
#include <boost/yap/print.hpp>
#include <boost/yap/yap.hpp>

TEST_CASE("prtcl/expr/text_transform",
          "[prtcl][expr][transform][text_transform]") {
  using namespace prtcl;

  // format fields
  boost::hana::for_each(
      boost::hana::make_tuple(tag::uniform{}, tag::varying{}), [](auto kt) {
        boost::hana::for_each(
            boost::hana::make_tuple(tag::scalar{}, tag::vector{}),
            [kt](auto tt) {
              boost::hana::for_each(
                  boost::hana::make_tuple(tag::active{}, tag::passive{}),
                  [kt, tt](auto gt) {
                    std::ostringstream field_ss, ss;
                    ss << "field(kind=\"" << kt << "\", type=\"" << tt
                       << "\", group=\"" << gt << "\", name=\"name\")";

                    expr::field_term<decltype(kt), decltype(tt), decltype(gt),
                                     tag::unspecified, std::string>
                        field{{"name"}};
                    boost::yap::transform(field,
                                          expr::text_transform{field_ss});

                    REQUIRE(ss.str() == field_ss.str());
                  });
            });
      });

  { // format arbitrary values
    std::ostringstream ss;
    boost::yap::transform(boost::yap::make_terminal(1234),
                          expr::text_transform{ss});
    REQUIRE("1234" == ss.str());
  }

  { // format assign, plus_assign, minus_assign
    auto a = boost::yap::make_terminal("a");

    std::ostringstream ss;

    ss.str("");
    boost::yap::transform(a = 0, expr::text_transform{ss});
    REQUIRE("a = 0" == ss.str());

    ss.str("");
    boost::yap::transform(a += 0, expr::text_transform{ss});
    REQUIRE("a += 0" == ss.str());

    ss.str("");
    boost::yap::transform(a -= 0, expr::text_transform{ss});
    REQUIRE("a -= 0" == ss.str());
  }

  { // format plus, minus, multiplies, divides
    auto a = boost::yap::make_terminal("a");

    std::ostringstream ss;

    ss.str("");
    boost::yap::transform(a + 0, expr::text_transform{ss});
    REQUIRE("a + 0" == ss.str());

    ss.str("");
    boost::yap::transform(a - 0, expr::text_transform{ss});
    REQUIRE("a - 0" == ss.str());

    ss.str("");
    boost::yap::transform(a * 0, expr::text_transform{ss});
    REQUIRE("a * 0" == ss.str());

    ss.str("");
    boost::yap::transform(a / 0, expr::text_transform{ss});
    REQUIRE("a / 0" == ss.str());
  }
}
