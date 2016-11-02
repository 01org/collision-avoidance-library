/*
// Copyright (c) 2016 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include <cmath>

#include "DepthCamera.hh"

unsigned int DepthCamera::get_height()
{
    return height;
}

unsigned int DepthCamera::get_width()
{
    return width;
}

double DepthCamera::get_scale()
{
    return scale;
}

double DepthCamera::get_fov_tan()
{
    static double curr_fov = 0;
    static double curr_fov_tan = 0;

    // Check if fov has changed to recalculate fov_tan
    if (this->fov != curr_fov) {
        curr_fov = fov;
        curr_fov_tan = tan(fov);
    }

    return curr_fov_tan;
}

