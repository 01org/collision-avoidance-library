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

#include "math.hh"

// TODO: For debug/status print. Still need to be implemented
#include <iostream>
#include <cmath>

Pose operator-(const Pose& a, const Pose &b)
{
  return Pose{a.pos - b.pos, glm::inverse(b.rot) * a.rot};
}

int sign(double x)
{
    return (x > 0) - (x < 0);
}

double sigmoid(double x)
{
    return 1.0 / (1.0 + exp(-x));
}

PolarVector cartesian_to_spherical(double x, double y, double z)
{
    PolarVector p;

    p.len = sqrt(x * x + y * y + z * z);
    p.theta = atan2(y, x);
    p.phi = asin(z / p.len);

    return p;
}

