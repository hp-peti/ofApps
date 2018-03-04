/*
 * StlAlgo.h
 *
 *  Created on: 2 Apr 2016
 *      Author: hpp
 */

#ifndef SRC_MYALGO_H_
#define SRC_MYALGO_H_

#include <iterator>

namespace my {

template <typename Iterator, typename Func>
void for_each_consecutive_pair(Iterator begin, Iterator end, Func func) {
    if (begin == end)
        return;
    auto prev = begin++;
    while (begin != end) {
        func(*prev, *begin);
        prev = begin++;
    }
}

template <typename Container, typename Func>
void for_each_consecutive_pair(Container &cont, Func func) {
    for_each_consecutive_pair(begin(cont), end(cont), func);
}

} // namespace my

#endif /* SRC_MYALGO_H_ */
