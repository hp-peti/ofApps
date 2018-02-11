/*
 * ZoomLevels.h
 *
 *  Created on: 11 Feb 2018
 *      Author: hpp
 */

#ifndef SRC_ZOOMLEVELS_H_
#define SRC_ZOOMLEVELS_H_

#include "debug.h"

#include <vector>

namespace ZoomLevels {

struct rational
{
    int num;
    unsigned den;

    bool operator == (const rational &other) const
    {
        return num * (int)other.den == other.num * (int)den;
    }

    bool operator < (const rational &other) const
    {
        return num * (int)other.den < other.num * (int)den;
    }

    bool operator != (const rational &other) const
    {
        return !(*this == other);
    }

    bool operator > (const rational &other) const
    {
        return other < *this;
    }

    operator float() const { return (float)num / (float)den; }

};

const std::vector<rational> generate(unsigned N)
{
    std::vector<rational> v;

    for (int num = 1; num <= (int)N; ++num) {
        for (unsigned den = 1; den <= N; ++den) {
            rational r { num, den };
            auto p = std::lower_bound(v.begin(), v.end(), r);
            if (p == v.end() || *p != r) {
                v.insert(p, r);
            }
        }
    }
#ifdef _DEBUG
    clog << "generated ratios: ";
    for (auto &r : v) {
        clog << " " << r.num << "/" << r.den;
    }
    clog << endl;
#endif
    return v;
}

} // namespace ZoomLevels




#endif /* SRC_ZOOMLEVELS_H_ */
