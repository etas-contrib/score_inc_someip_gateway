/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/
#ifndef CAR_WINDOW_SERIALIZER_HELPERS_H
#define CAR_WINDOW_SERIALIZER_HELPERS_H

#include <cstdint>
#include <vector>

#include "car_window_types.h"

namespace car_window_types {

std::vector<std::uint8_t> SerializeWindowInfo(const WindowInfo& info);
WindowInfo DeserializeWindowInfo(const std::vector<std::uint8_t>& bytes);

std::vector<std::uint8_t> SerializeWindowControl(const WindowControl& control);
WindowControl DeserializeWindowControl(const std::vector<std::uint8_t>& bytes);

}  // namespace car_window_types

#endif  // CAR_WINDOW_SERIALIZER_HELPERS_H
