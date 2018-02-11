/*
 * drawVector.h
 *
 *  Created on: 11 Feb 2018
 *      Author: hpp
 */

#ifndef SRC_DRAWVECTOR_H_
#define SRC_DRAWVECTOR_H_

#include <ofVec2f.h>
#include <complex>

void drawVector(const ofVec2f &pos,
                complex<float> direction,
                float length,
                float arrowhead);


#endif /* SRC_DRAWVECTOR_H_ */
