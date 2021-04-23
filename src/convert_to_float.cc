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

#include "convert.h"

// instantiate ConvertTask for the cross product of types Legate handles
// we omit the T1 == T2 case

// T1 == float
namespace legate {
namespace numpy {
template class ConvertTask<float, __half>;
template class ConvertTask<float, double>;
template class ConvertTask<float, int16_t>;
template class ConvertTask<float, int32_t>;
template class ConvertTask<float, int64_t>;
template class ConvertTask<float, uint16_t>;
template class ConvertTask<float, uint32_t>;
template class ConvertTask<float, uint64_t>;
template class ConvertTask<float, bool>;
}  // namespace numpy
}  // namespace legate
