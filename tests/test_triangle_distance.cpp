#include <algorithm>
#include <array>
#include <cmath>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "xdg/geometry/plucker.h"

using namespace xdg;

TEST_CASE("Compensated Plucker edges reject the false initial-volume hits",
          "[plucker][precision][edge]")
{
  // Frozen initial ray of particle 9008800. Faces 379169 and 369599 share an
  // edge whose nonzero Plucker coordinates previously cancelled to signed zero.
  // Exact arithmetic rejects both and accepts the neighboring face 379168.
  const Position origin {0, 0, 0};
  const Direction direction {-0.6568208932876587f, 0.053338538855314255f,
                             -0.7521578073501587f};
  const dp::vec3 triangles[3][3] = {
    {{-7.285249710083008f, 0.4705873727798462f, -8.332100868225098f},
     {-7.284326553344727f, 0.48454490303993225f, -8.332229614257812f},
     {-7.2715067863464355f, 0.6492346525192261f, -8.332100868225098f}},
    {{-7.2715067863464355f, 0.6492346525192261f, -8.332100868225098f},
     {-7.2816877365112305f, 0.523017406463623f, -8.331865310668945f},
     {-7.285249710083008f, 0.4705873727798462f, -8.332100868225098f}},
    {{-7.283132553100586f, 0.48446545004844666f, -8.345146179199219f},
     {-7.2715067863464355f, 0.6492346525192261f, -8.332100868225098f},
     {-7.284326553344727f, 0.48454490303993225f, -8.332229614257812f}}
  };
  // Exact-rational reference on the stored float inputs, before rounding to
  // Scalar. Both adjacent triangles must retain opposite, nonzero signs.
  const Scalar expected_edge = Scalar(2.903280589368027e-8);
  const Scalar first_edge = detail::compensated_plucker_edge(
      triangles[0][2], triangles[0][0], origin, direction);
  const Scalar second_edge = detail::compensated_plucker_edge(
      triangles[1][2], triangles[1][0], origin, direction);
  REQUIRE(first_edge > Scalar(0));
  REQUIRE(second_edge == -first_edge);
  REQUIRE_THAT(first_edge, Catch::Matchers::WithinULP(expected_edge, 1));

  for (int face = 0; face < 3; ++face) {
    std::array<int, 3> order {0, 1, 2};
    do {
      CAPTURE(face, order[0], order[1], order[2]);
      dp::vec3 vertices[3] = {
        triangles[face][order[0]], triangles[face][order[1]], triangles[face][order[2]]
      };
      const auto hit = plucker_ray_tri_intersect(vertices, origin, direction,
                                                INFTY, 0, false, 0);
      REQUIRE(hit.hit == (face == 2));
      if (hit.hit) {
        REQUIRE_THAT(hit.t, Catch::Matchers::WithinULP(Scalar(11.077675844783533), 1));
      }
    } while (std::next_permutation(order.begin(), order.end()));
  }
}

TEST_CASE("Compensated Plucker edges retain hits on shared edges and vertices",
          "[plucker][precision][edge]")
{
  const dp::vec3 triangles[2][3] = {
    {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}},
    {{0, 0, 0}, {1, 1, 0}, {0, 1, 0}}
  };
  const Position origin {17, 33, 1};
  const Direction directions[] = {
    {-16.5, -32.5, -1}, // Hits the shared diagonal at (0.5,0.5,0).
    {-17, -33, -1}     // Hits the shared vertex at (0,0,0).
  };
  for (const auto& direction : directions) {
    for (const auto& triangle : triangles) {
      std::array<int, 3> order {0, 1, 2};
      do {
        CAPTURE(direction, order[0], order[1], order[2]);
        dp::vec3 vertices[3] = {
          triangle[order[0]], triangle[order[1]], triangle[order[2]]
        };
        const auto hit = plucker_ray_tri_intersect(vertices, origin, direction,
                                                  INFTY, 0, false, 0);
        REQUIRE(hit.hit);
        REQUIRE(hit.t == Scalar(1));
      } while (std::next_permutation(order.begin(), order.end()));
    }
  }
}

TEST_CASE("Compensated Plucker edges preserve vertex translation residuals",
          "[plucker][precision][edge]")
{
  // In float, translating these vertices first collapses their distinct x
  // coordinates. Preserve those residuals before evaluating the edge products.
  const dp::vec3 triangle[3] = {{-1, -1, 0}, {1, -1, 0}, {0, 1, 0}};
  const Position origin {33554432, 0, 1};
  const Direction direction {-33554432, 0, -1};
  std::array<int, 3> order {0, 1, 2};
  do {
    CAPTURE(order[0], order[1], order[2]);
    dp::vec3 vertices[3] = {
      triangle[order[0]], triangle[order[1]], triangle[order[2]]
    };
    const auto hit = plucker_ray_tri_intersect(vertices, origin, direction,
                                              INFTY, 0, false, 0);
    REQUIRE(hit.hit);
    REQUIRE(hit.t == Scalar(1));
  } while (std::next_permutation(order.begin(), order.end()));
}

TEST_CASE("Compensated distance preserves the cap crossing", "[plucker][precision]")
{
  // Particle 463519 previously entered volume 5 about 257 float ULPs early.
  // Since the cap is z=0, its exact parameter is independent of barycentric weights.
  const dp::vec3 cap[3] = {
    {-7.627405643463135f, 3.9879326820373535f, 0},
    {-7.675191879272461f, 3.8937385082244873f, 0},
    {7.652370452880859f, 3.9398155212402344f, 0}
  };
  const Position origin {7.998187065124512f, 2.724212169647217f, 0.44743454456329346f};
  const Direction direction {-0.7270861864089966f, 0.6442399024963379f, -0.2372778207063675f};
  const Scalar expected = -origin.z / direction.z;

  std::array<int, 3> order {0, 1, 2};
  do {
    CAPTURE(order[0], order[1], order[2]);
    dp::vec3 vertices[3] = {cap[order[0]], cap[order[1]], cap[order[2]]};
    const auto hit = plucker_ray_tri_intersect(vertices, origin, direction,
                                              INFTY, 0, false, 0);
    REQUIRE(hit.hit);
    REQUIRE_THAT(hit.t, Catch::Matchers::WithinULP(expected, 1));
  } while (std::next_permutation(order.begin(), order.end()));
}

TEST_CASE("Compensated distance resolves short oblique hits", "[plucker][precision]")
{
  struct Case {
    dp::vec3 vertices[3];
    Position origin;
    Direction direction;
    Scalar expected;
  };
  // References were calculated with exact rational arithmetic on these stored
  // float inputs. A rounded normal alone still loses many ULPs on these rays.
  const Case cases[] = {
    {
      {{-2.367835760116577f, 7.763397216796875f, 1.1597625017166138f},
       {-2.3202481269836426f, 7.77775239944458f, 1.1597625017166138f},
       {-2.273484706878662f, 7.620995998382568f, 1.5444135665893555f}},
      {-2.3127293586730957f, 7.713397979736328f, 1.3090847730636597f},
      {-0.7932428121566772f, 0.47380053997039795f, -0.3824642300605774f},
      Scalar(0.00053975501958744413)
    },
    {
      {{0.09458517283201218f, -9.554738998413086f, -2.783186435699463f},
       {0.33441880345344543f, -9.549352645874023f, -2.783186435699463f},
       {0.33553898334503174f, -9.581339836120605f, -2.92130708694458f}},
      {0.22523628175258636f, -9.559097290039062f, -2.814333438873291f},
      {-0.39003825187683105f, -0.1539027988910675f, -0.9078458547592163f},
      Scalar(0.0011272876750063)
    }
  };

  for (const auto& ray : cases) {
    std::array<int, 3> order {0, 1, 2};
    do {
      CAPTURE(ray.expected, order[0], order[1], order[2]);
      dp::vec3 vertices[3] = {
        ray.vertices[order[0]], ray.vertices[order[1]], ray.vertices[order[2]]
      };
      const auto hit = plucker_ray_tri_intersect(vertices, ray.origin, ray.direction,
                                                INFTY, 0, false, 0);
      REQUIRE(hit.hit);
      REQUIRE_THAT(hit.t, Catch::Matchers::WithinULP(ray.expected, 1));
    } while (std::next_permutation(order.begin(), order.end()));
  }
}

TEST_CASE("A singular triangle cannot produce a NaN hit", "[plucker][precision]")
{
  // The vertices are exactly collinear. Translation and edge rounding leave
  // nonzero Plucker coordinates of consistent sign in single precision.
  dp::vec3 vertices[3] = {{-2, -15, 56}, {95, -33, 93}, {192, -51, 130}};
  const Position origin {70.186622619628906f, -28.359155654907227f, 86.285209655761719f};
  const Direction direction {0.22535210847854614f, -0.056338027119636536f, -1.0140844583511353f};
  const auto hit = plucker_ray_tri_intersect(vertices, origin, direction,
                                            INFTY, 0, false, 0);
  REQUIRE_FALSE(hit.hit);
}
