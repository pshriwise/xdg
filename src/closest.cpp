#include "xdg/closest.h"

namespace xdg {

// FROM MOAB, but cleaned up considerably
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