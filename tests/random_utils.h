#pragma once

#include <boost/random.hpp>
#include <cg/primitives/point.h>
#include <misc/random_utils.h>

template <class Scalar = double>
inline std::vector<cg::point_2t<Scalar>> uniform_points(size_t count, Scalar minCoord = -100, Scalar maxCoord = 100)
{
    util::uniform_random_real<Scalar> rand(minCoord, maxCoord);

    std::vector<cg::point_2t<Scalar>> res(count);

    for (size_t l = 0; l != count; ++l)
    {
        rand >> res[l].x;
        rand >> res[l].y;
    }

    return res;
}
