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
#pragma once

/**
 * @file
 * @brief Stop avoidance strategy for quadcopters
 */

#include "common/common.hh"
#include "vehicles/MavQuadCopter.hh"

#include <chrono>
#include <memory>
#include <vector>

/**
 * @brief Stop Avoidance collision avoidance strategy for quadcopters.
 *        The vehicle simply brakes and stays steady until the path in front of
 *        it is free from obstacles.
 */
class QuadCopterStopAvoidance
    : public CollisionAvoidanceStrategy<MavQuadCopter, bool>
{
    /**
     * @brief Default Constructor.
     * @param quadcopter Smart pointer to the MavQuadCopter object that will
     *                   be controlled by the avoidance strategy.
     */
    public: QuadCopterStopAvoidance(std::shared_ptr<MavQuadCopter> quadcopter);

    /**
     * @brief Avoidance command implementation.
     *        Call this function from your program to send updated avoidance
     *        commands to the vehicle according to the binary representation
     *        of obstacles given as input.
     * @param detection An one element boolean array that will be true if an
     *                  obstacle is detected right in front of the vehicle and
     *                  false otherwise.
     */
    public: void avoid(const std::vector<bool> &detection) override;
};

