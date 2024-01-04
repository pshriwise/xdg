#include "xdg/geometry/closest.h"

namespace xdg {

Region determine_region(double s, double t, double det) {
  if (s + t < det) {
    if (s < 0) {
      if (t < 0) {
        return Region::Four; // tv and sv are both negative
      } else {
        return Region::Three; // sv is negative, tv is positive
      }
    } else if (t < 0) {
      return Region::Five; // sv is positive, tv is negative
    } else {
      return Region::Zero; // sv and tv are both positive and less than one
    }
  } else {
    if (s < 0) {
      return Region::Two; // sv is negative, tv is positive
    } else if (t < 0) {
      return Region::Six; // sv is positive, tv is negative
    } else {
      return Region::One; // sv and tv are both positive and greater than one
    }
  }
}

// from:
    // http://www.geometrictools.com/Documentation/DistancePoint3Triangle3.pdf#search=%22closest%20point%20on%20triangle%22
    /*       t
     *   \(2)^
     *    \  |
     *     \ |
     *      \|
     *       \
     *       |\
     *       | \
     *       |  \  (1)
     *  (3)  tv  \
     *       |    \
     *       | (0) \
     *       |      \
     *-------+---sv--\----> s
     *       |        \ (6)
     *  (4)  |   (5)   \
     */
    // Worst case is either 61 flops and 5 compares or 53 flops and 6 compares,
    // depending on relative costs.  For all paths that do not return one of the
    // corner vertices, exactly one of the flops is a divide.

// FROM MOAB, but cleaned up considerably
// Position closest_location_on_triangle(const std::array<Position, 3>& vertices,
//                                       const Position& point)
// {
//   Position closest_out;

//   const Position sv {vertices[1] - vertices[0]};
//   const Position tv {vertices[2] - vertices[0]};
//   const Position pv {vertices[0] - point};
//   const double ss  = sv.dot(sv);
//   const double st  = sv.dot(tv);
//   const double tt  = tv.dot(tv);
//   const double sp  = sv.dot(pv);
//   const double tp  = tv.dot(pv);
//   const double det = ss * tt - st * st;
//   double s = st * tp - tt * sp;
//   double t = st * sp - ss * tp;

//   Region region = determine_region(s, t, det);

//   switch (region) {
//     case Region::Zero:
//     {
//       const double inv_det = 1.0 / det;
//       s *= inv_det;
//       t *= inv_det;
//       return vertices[0] + s * sv + t * tv;
//     }
//       break;
//     case Region::One:
//      {
//       const double num = tt + tp - st - sp;
//       if (num <= 0) {
//         return vertices[2];
//       } else {
//         const double den = ss - 2 * st + tt;
//         if (num >= den) {
//           return vertices[1];
//         } else {
//           const double p = num / den;
//           return p * vertices[1] + (1 - p) * vertices[2];
//         }
//       }
//      }
//       break;
//     case Region::Two:
//       {
//       s = st + sp;
//       t = tt + tp;
//       if (t > s) {
//         const double num = t - s;
//         const double den = ss - 2 * st + tt;
//         if (num > den) {
//           return vertices[1];
//         } else {
//           const double p = num / den;
//           return p * vertices[1] + (1 - p) * vertices[2];
//         }
//       } else if (t <= 0) {
//         return vertices[2];
//       } else if (tp >= 0) {
//         return vertices[0];
//       } else {
//         return vertices[0] - (tp / tt) * tv;
//       }
//       }
//       break;
//     case Region::Three:
//       if (tp >= 0) {
//         return vertices[0];
//       } else if (-tp >= tt) {
//         return vertices[2];
//       } else {
//         return vertices[0] - (tp / tt) * tv;
//       }
//       break;
//     case Region::Four:
//       if( sp < 0 )
//       {
//         if( -sp > ss )
//           return vertices[1];
//         else
//           return vertices[0] - ( sp / ss ) * sv;
//       }
//       else if( tp < 0 )
//       {
//         if( -tp > tt )
//           return vertices[2];
//         else
//           return vertices[0] - ( tp / tt ) * tv;
//       }
//       else
//       {
//         return vertices[0];
//       }
//       break;
//     case Region::Five:
//       if( sp >= 0.0 )
//         return vertices[0];
//       else if( -sp >= ss )
//         return vertices[1];
//       else
//         return vertices[0] - ( sp / ss ) * sv;
//       break;
//     case Region::Six:
//       t = st + tp;
//             s = ss + sp;
//             if( s > t )
//             {
//                 const double num = t - s;
//                 const double den = tt - 2 * st + ss;
//                 if( num > den )
//                     return vertices[2];
//                 else
//                 {
//                     t           = num / den;
//                     s           = 1 - t;
//                     return s * vertices[1] + t * vertices[2];
//                 }
//             }
//             else if( s <= 0 )
//                 return vertices[1];
//             else if( sp >= 0 )
//                 return vertices[0];
//             else
//                 return vertices[0] - ( sp / ss ) * sv;
//       break;
//       default:
//         break;
//   }
// }

Region determine_region (std::array<Position, 3> triangle, Position p) {
  Position sv = triangle[1] - triangle[0];
  Position tv = triangle[2] - triangle[0];
  Position pv = triangle[0] - p;
  double ss = sv.dot(sv);
  double st = sv.dot(tv);
  double tt = tv.dot(tv);
  double sp = sv.dot(pv);
  double tp = tv.dot(pv);
  double det = ss * tt - st * st;
  double s = st * tp - tt * sp;
  double t = st * sp - ss * tp;

  return determine_region(s, t, det);
}

Position closest_location_on_triangle(const std::array<Position, 3>& vertices,
                                      const Position& point)
{
  Position closest_out;

  const Position sv {vertices[1] - vertices[0]};
  const Position tv {vertices[2] - vertices[0]};
  const Position pv {vertices[0] - point};
  const double ss  = sv.dot(sv);
  const double st  = sv.dot(tv);
  const double tt  = tv.dot(tv);
  const double sp  = sv.dot(pv);
  const double tp  = tv.dot(pv);
  const double det = ss * tt - st * st;
  double s = st * tp - tt * sp;
  double t = st * sp - ss * tp;

    if( s + t < det )
    {
        if( s < 0 )
        {
            if( t < 0 )
            {
                // region 4
                if( sp < 0 )
                {
                    if( -sp > ss )
                        closest_out = vertices[1];
                    else
                        closest_out = vertices[0] - ( sp / ss ) * sv;
                }
                else if( tp < 0 )
                {
                    if( -tp > tt )
                        closest_out = vertices[2];
                    else
                        closest_out = vertices[0] - ( tp / tt ) * tv;
                }
                else
                {
                    closest_out = vertices[0];
                }
            }
            else
            {
                // region 3
                if( tp >= 0 )
                    closest_out = vertices[0];
                else if( -tp >= tt )
                    closest_out = vertices[2];
                else
                    closest_out = vertices[0] - ( tp / tt ) * tv;
            }
        }
        else if( t < 0 )
        {
            // region 5;
            if( sp >= 0.0 )
                closest_out = vertices[0];
            else if( -sp >= ss )
                closest_out = vertices[1];
            else
                closest_out = vertices[0] - ( sp / ss ) * sv;
        }
        else
        {
            // region 0
            const double inv_det = 1.0 / det;
            s *= inv_det;
            t *= inv_det;
            closest_out = vertices[0] + s * sv + t * tv;
        }
    }
    else
    {
        if( s < 0 )
        {
            // region 2
            s = st + sp;
            t = tt + tp;
            if( t > s )
            {
                const double num = t - s;
                const double den = ss - 2 * st + tt;
                if( num > den )
                    closest_out = vertices[1];
                else
                {
                    s           = num / den;
                    t           = 1 - s;
                    closest_out = s * vertices[1] + t * vertices[2];
                }
            }
            else if( t <= 0 )
                closest_out = vertices[2];
            else if( tp >= 0 )
                closest_out = vertices[0];
            else
                closest_out = vertices[0] - ( tp / tt ) * tv;
        }
        else if( t < 0 )
        {
            // region 6
            t = st + tp;
            s = ss + sp;
            if( s > t )
            {
                const double num = t - s;
                const double den = tt - 2 * st + ss;
                if( num > den )
                    closest_out = vertices[2];
                else
                {
                    t           = num / den;
                    s           = 1 - t;
                    closest_out = s * vertices[1] + t * vertices[2];
                }
            }
            else if( s <= 0 )
                closest_out = vertices[1];
            else if( sp >= 0 )
                closest_out = vertices[0];
            else
                closest_out = vertices[0] - ( sp / ss ) * sv;
        }
        else
        {
            // region 1
            const double num = tt + tp - st - sp;
            if( num <= 0 )
            {
                closest_out = vertices[2];
            }
            else
            {
                const double den = ss - 2 * st + tt;
                if( num >= den )
                    closest_out = vertices[1];
                else
                {
                    s           = num / den;
                    t           = 1 - s;
                    closest_out = s * vertices[1] + t * vertices[2];
                }
            }
        }
    }

  return closest_out;
}

} // namespace xdg