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

#include <algorithm>

namespace ZoomLevels {

struct Ratio
{
    int num;
    unsigned den;

    bool operator == (const Ratio &other) const
    {
        return num * (int)other.den == other.num * (int)den;
    }

    bool operator < (const Ratio &other) const
    {
        return num * (int)other.den < other.num * (int)den;
    }

    bool operator != (const Ratio &other) const
    {
        return !(*this == other);
    }

    bool operator > (const Ratio &other) const
    {
        return other < *this;
    }

    operator float() const { return (float)num / (float)den; }

    friend std::ostream & operator << (std::ostream &out, const Ratio &r) {
        return out << r.num << '/' << r.den;
    }
};

const std::vector<Ratio> generate(unsigned N)
{
    std::vector<Ratio> v;

    for (int num = 1; num <= (int)N; ++num) {
        for (unsigned den = 1; den <= N; ++den) {
            Ratio r { num, den };
            const auto p = std::lower_bound(v.begin(), v.end(), r);
            if (p == v.end() || *p != r)
                v.insert(p, r);
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
