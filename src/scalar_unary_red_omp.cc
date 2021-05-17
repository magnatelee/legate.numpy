/* Copyright 2021 NVIDIA Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "scalar_unary_red.h"
#include "unary_red_util.h"
#include "core.h"
#include "deserializer.h"
#include "dispatch.h"
#include "point_task.h"

namespace legate {
namespace numpy {

using namespace Legion;

namespace omp {

template <UnaryRedCode OP_CODE>
struct ScalarUnaryRedImpl {
  template <LegateTypeCode CODE,
            int DIM,
            std::enable_if_t<UnaryRedOp<OP_CODE, CODE>::valid> * = nullptr>
  UntypedScalar operator()(Shape &shape, RegionField &in_rf)
  {
    using OP  = UnaryRedOp<OP_CODE, CODE>;
    using VAL = legate_type_of<CODE>;

    auto rect = shape.to_rect<DIM>();

    Pitches<DIM - 1> pitches;
    size_t volume = pitches.flatten(rect);

    VAL result = OP::identity;

    if (volume == 0) return UntypedScalar(result);

    auto in = in_rf.read_accessor<VAL, DIM>();

#ifndef LEGION_BOUNDS_CHECKS
    // Check to see if this is dense or not
    bool dense = in.accessor.is_dense_row_major(rect);
#else
    // No dense execution if we're doing bounds checks
    bool dense = false;
#endif

    const auto max_threads = omp_get_max_threads();
    auto locals            = static_cast<VAL *>(alloca(max_threads * sizeof(VAL)));
    for (auto idx = 0; idx < max_threads; ++idx) locals[idx] = OP::identity;
    if (dense) {
      auto inptr = in.ptr(rect);
#pragma omp parallel
      {
        const int tid = omp_get_thread_num();
#pragma omp for schedule(static)
        for (size_t idx = 0; idx < volume; ++idx) OP::template fold<true>(locals[tid], inptr[idx]);
      }
    } else
      OMPLoop<DIM>::unary_reduction_loop(OP{}, locals, rect, in);

    for (auto idx = 0; idx < max_threads; ++idx) OP::template fold<true>(result, locals[idx]);

    return UntypedScalar(result);
  }

  template <LegateTypeCode CODE,
            int DIM,
            std::enable_if_t<!UnaryRedOp<OP_CODE, CODE>::valid> * = nullptr>
  UntypedScalar operator()(Shape &shape, RegionField &in_rf)
  {
    assert(false);
    return UntypedScalar();
  }
};

struct ScalarUnaryRedDispatch {
  template <UnaryRedCode OP_CODE>
  UntypedScalar operator()(Shape &shape, RegionField &in)
  {
    return double_dispatch(in.dim(), in.code(), ScalarUnaryRedImpl<OP_CODE>{}, shape, in);
  }
};

}  // namespace omp

/*static*/ UntypedScalar ScalarUnaryRedTask::omp_variant(const Task *task,
                                                         const std::vector<PhysicalRegion> &regions,
                                                         Context context,
                                                         Runtime *runtime)
{
  Deserializer ctx(task, regions);

  UnaryRedCode op_code;
  Shape shape;
  RegionField in;

  deserialize(ctx, op_code);
  deserialize(ctx, shape);
  deserialize(ctx, in);

  return op_dispatch(op_code, omp::ScalarUnaryRedDispatch{}, shape, in);
}

}  // namespace numpy
}  // namespace legate