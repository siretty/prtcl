#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <prtcl/openmp_source_generator.hpp>

#include <iostream>
#include <sstream>
#include <string>

#include <boost/program_options.hpp>

static void generate(std::ostream &);

int main(int argc_, char **argv_) {
  ::boost::program_options::variables_map args;

  { // parse program options
    namespace po = ::boost::program_options;

    po::options_description desc{"allowed options"};
    desc.add_options()                                  //
        ("help", "produce help message")                //
        ("output,o",                                    //
         po::value<std::string>()->default_value("-"),  //
         "generated code will be written to this file") //
        ("append", "append to the output file")         //
        ;

    po::store(po::command_line_parser(argc_, argv_).options(desc).run(), args);
    po::notify(args);

    if (args.count("help")) {
      std::cerr << desc << '\n';
      return 1;
    }
  }

  if (!args.count("output")) {
    std::cerr << "no output specialized" << '\n';
    return 1;
  }

  auto output = args["output"].as<std::string>();

  if (output == "-")
    generate(std::cout);
  else {
    auto flags = std::fstream::out;
    if (args.count("append"))
      flags |= std::fstream::app;

    std::fstream file{output, flags};
    generate(file);
  }
}

static void generate(std::ostream &stream) {
  using namespace ::prtcl::expr_language;
  using namespace ::prtcl::expr_literals;

  constexpr ::prtcl::tag::group::active i;
  constexpr ::prtcl::tag::group::passive j;

  auto const x = "position"_vv;

  auto const v = "varying"_vs;
  auto const u = "uniform"_us;
  auto const g = "global"_gs;

  auto const a = "vv_a"_vv;

  ::prtcl::generate_openmp_source(
      stream, "prtcl_tests",
      make_named_section(                          //
          "reduce_sum",                            //
          foreach_particle(                        //
              only("main")(                        //
                  u[i] += v[i] / particle_count(), //
                  g += v[i] / particle_count()     //
                  )                                //
              )                                    //
          ),                                       //
      make_named_section(                          //
          "reduce_sum_neighbours",                 //
          foreach_particle(                        //
              only("main")(                        //
                  a[i] = zero_vector(),            //
                  foreach_neighbour(               //
                      only("other")(               //
                          a[i] += x[j],            //
                          g += 1                   //
                          )                        //
                      ),                           //
                  u[i] += norm(a[i])               //
                  )                                //
              )                                    //
          ),                                       //
      make_named_section(                          //
          "call_norm",                             //
          foreach_particle(                        //
              only("main")(                        //
                  v[i] = norm(a[i])                //
                  )                                //
              )                                    //
          )                                        //
  );
}
