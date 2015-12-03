// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr


#ifndef  NORMAL_COMPUTATION_PLANE_FITTINGH_H_
#define  NORMAL_COMPUTATION_PLANE_FITTINGH_H_

#include <lamure/pre/normal_computation_strategy.h>

#include <vector>

namespace lamure {
namespace pre{
	
class NormalComputationPlaneFitting: public NormalComputationStrategy
{
public:
	explicit NormalComputationPlaneFitting(const uint16_t number_of_neighbours)
		: number_of_neighbours_(number_of_neighbours){}

	vec3f  compute_normal(const Bvh& tree,
						  const size_t node_id,
						  const size_t surfel_id) const override;

private:
	const uint16_t number_of_neighbours_;
	
};

}// namespace pre
}// namespace lamure

#endif // NORMAL_COMPUTATION_PLANE_FITTINGH_H_