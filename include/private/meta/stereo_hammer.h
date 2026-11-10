/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-stereo-hammer
 * Created on: 08 ноя 2026 г.
 *
 * lsp-plugins-stereo-hammer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins-stereo-hammer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins-stereo-hammer. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PRIVATE_META_STEREO_HAMMER_H_
#define PRIVATE_META_STEREO_HAMMER_H_

#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/const.h>

namespace lsp
{
    //-------------------------------------------------------------------------
    // Plugin metadata
    namespace meta
    {
        typedef struct stereo_hammer_metadata
        {
            static constexpr float  DELAY_SHIFT_MIN             = -50.0f;               // Min delay shift between channels
            static constexpr float  DELAY_SHIFT_MAX             = 50.0f;                // Max delay shift between channels
            static constexpr float  DELAY_SHIFT_DFL             = 0.0f;                 // Default delay shift between channels
            static constexpr float  DELAY_SHIFT_STEP            = 0.1f;                 // Delay shift step

            static constexpr float  ROTATE_MIN                  = -180.0f;              // Min channel rotatin
            static constexpr float  ROTATE_MAX                  = 180.0f;               // Max channel rotation
            static constexpr float  ROTATE_DFL                  = 0.0f;                 // Default channel rotation
            static constexpr float  ROTATE_STEP                 = 0.25f;                // Channel rotation step

            static constexpr float  GONIO_DOTS_MAX              = 8192.0f;              // Maximum dots in goniometer
            static constexpr float  GONIO_DOTS_MIN              = 512.0f;               // Minimum dots in goniometer
            static constexpr float  GONIO_DOTS_DFL              = 2048.0f;              // Default number of dots in goniometer
            static constexpr float  GONIO_DOTS_STEP             = 0.01f;                // Configuration step

            static constexpr size_t GONIO_HISTORY_MAX           = 10;                   // Maximum history for goniometer
            static constexpr size_t GONIO_HISTORY_MIN           = 0;                    // Minimum history for goniometer
            static constexpr size_t GONIO_HISTORY_DFL           = 5;                    // Default history for goniometer
            static constexpr size_t GONIO_HISTORY_STEP          = 1;                    // Step for goniometer

            static constexpr float  CORRELATION_MIN             = -1.0f;                // Minimum correlation value
            static constexpr float  CORRELATION_MAX             = 1.0f;                 // Maximum correlation value
            static constexpr float  CORRELATION_DFL             = 0.0f;                 // Default correlation value
            static constexpr float  CORRELATION_STEP            = 0.001f;               // Correlation step

            static constexpr float  PANOMETER_MIN               = 0.0f;                 // Minimum panometer value
            static constexpr float  PANOMETER_MAX               = 1.0f;                 // Maximum panometer value
            static constexpr float  PANOMETER_DFL               = 0.5f;                 // Default panometer value
            static constexpr float  PANOMETER_STEP              = 0.001f;               // Panometer step

            static constexpr float  LOUD_METER_MIN              = GAIN_AMP_M_INF_DB;    // Loudness meter minimum
            static constexpr float  LOUD_METER_MAX              = GAIN_AMP_P_24_DB;     // Loudness meter maximum
            static constexpr float  LOUD_METER_DFL              = GAIN_AMP_M_INF_DB;    // Loudness meter default
            static constexpr float  LOUD_METER_STEP             = 0.01f;                // Loudness meter step

        } stereo_hammer_metadata;

        // Plugin type metadata
        extern const plugin_t stereo_hammer;

    } /* namespace meta */
} /* namespace lsp */

#endif /* PRIVATE_META_STEREO_HAMMER_H_ */
