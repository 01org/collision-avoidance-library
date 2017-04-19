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
#include <common/common.hh>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "DepthImagePolarHistDetector.hh"

namespace defaults
{
    const unsigned int vertical_sweep_pixels = 10;
}

DepthImagePolarHistDetector::DepthImagePolarHistDetector(double angle_step,
        double threshold, double density)
{
    this->step = glm::radians(angle_step);
    this->threshold = threshold;
    this->density = density;
}

const std::vector<Obstacle> &DepthImagePolarHistDetector::detect(
        std::shared_ptr<void> data)
{
    std::shared_ptr<DepthData> depth_data = std::static_pointer_cast<DepthData>(data);
    std::vector<double> histogram;
    std::vector<unsigned int> density_count;

    // Obtain camera depth buffer and camera properties
    std::vector<uint16_t> depth_buffer = depth_data->depth_buffer;
    unsigned int height = depth_data->height;
    unsigned int width = depth_data->width;
    double fov = depth_data->hfov;
    double scale = depth_data->scale;

    unsigned int middle_row = height / 2;

    this->obstacles.clear();

    // Return if depth buffer is empty
    if(depth_buffer.size() == 0) {
        return this->obstacles;
    }

    // Make sure the chosen vertical sweep area is within the limits of the
    // depth buffer
    unsigned int vertical_sweep_pixels =
        glm::min(defaults::vertical_sweep_pixels, middle_row);

    // Create one entry for each slice of the fov and initialize to max distance
    histogram.resize(glm::ceil(fov / this->step), UINT16_MAX * scale);
    density_count.resize(histogram.size(), 0);

    // Sweep a slice of the depth buffer filling up the histogram with the
    // closest distance found in a given direction
    for (unsigned int i = (middle_row - vertical_sweep_pixels);
         i < (middle_row + vertical_sweep_pixels); i++) {
        for (unsigned int j = 0; j < width; j++) {
            unsigned int pos = ((double) j / (double) width) * histogram.size();

            uint16_t depth_value = depth_buffer[i * width + j];
            if (depth_value == 0) {
                depth_value = UINT16_MAX;
            }

            if (depth_value * scale < this->threshold)
                density_count[pos]++;

            histogram[pos] = glm::min(depth_value * scale, histogram[pos]);
        }
    }

    // we use equal sized slices, so the actual step may be smaller than the provided
    // step if fov is not a multiple of it.
    double fixed_step = fov / histogram.size();
    unsigned int slice_pixel_count = (vertical_sweep_pixels * 2) * (width / histogram.size());

    // Assuming the drone is always looking down the y axis, calculate
    // the max phi it can see
    double max_phi = (fov + M_PI) / 2;

    for (size_t i = 0; i < histogram.size(); i++) {
        if (histogram[i] > this->threshold ||
                ((double) density_count[i]) / slice_pixel_count < this->density)
            continue;
        // Scan is made from left to right, so position 0 will be at the biggest phi
        // We center the phi in the middle of the slice
        double phi = max_phi - (i * fixed_step) - (fixed_step / 2);

        Obstacle obs;

        obs.bounding_box.tlc.x = histogram[i];
        obs.bounding_box.tlc.y = (M_PI - depth_data->vfov) / 2 ;
        obs.bounding_box.tlc.z = phi + (fixed_step / 2);

        obs.bounding_box.brf.x = histogram[i];
        obs.bounding_box.brf.y = (M_PI + depth_data->vfov) / 2;
        obs.bounding_box.brf.z = phi - (fixed_step / 2);

        obs.center = glm::dvec3(histogram[i], M_PI / 2, phi);

        this->obstacles.push_back(obs);
    }

    return this->obstacles;
}

