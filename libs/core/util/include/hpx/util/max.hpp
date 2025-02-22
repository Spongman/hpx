//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

namespace hpx::detail {
    template <typename T>
    HPX_HOST_DEVICE constexpr inline T const&(max)(T const& a, T const& b)
    {
        return a < b ? b : a;
    }
}    // namespace hpx::detail
