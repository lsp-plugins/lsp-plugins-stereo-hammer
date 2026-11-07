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

#include <lsp-plug.in/plug-fw/meta/ports.h>
#include <lsp-plug.in/plug-fw/meta/registry.h>
#include <lsp-plug.in/shared/meta/developers.h>
#include <private/meta/stereo_hammer.h>

#define LSP_PLUGINS_STEREO_HAMMER_VERSION_MAJOR       1
#define LSP_PLUGINS_STEREO_HAMMER_VERSION_MINOR       0
#define LSP_PLUGINS_STEREO_HAMMER_VERSION_MICRO       0

#define LSP_PLUGINS_STEREO_HAMMER_VERSION  \
    LSP_MODULE_VERSION( \
        LSP_PLUGINS_STEREO_HAMMER_VERSION_MAJOR, \
        LSP_PLUGINS_STEREO_HAMMER_VERSION_MINOR, \
        LSP_PLUGINS_STEREO_HAMMER_VERSION_MICRO  \
    )

namespace lsp
{
    namespace meta
    {
        //-------------------------------------------------------------------------
        // Plugin metadata

        static const port_t stereo_hammer_stereo_ports[] =
        {
            // Input and output audio ports
            PORTS_STEREO_PLUGIN,

            // Input controls
            BYPASS,

            PORTS_END
        };

        static const int plugin_classes[]       = { C_UTILITY, -1 };
        static const int clap_features_stereo[] = { CF_AUDIO_EFFECT, CF_UTILITY, CF_STEREO, -1 };

        const meta::bundle_t stereo_hammer_bundle =
        {
            "stereo_hammer",
            "Stereo Hammer",
            B_UTILITIES,
            "", // TODO: provide ID of the video on YouTube
            "A plugin that allows to manipulate stereo signal parameters like Mid/Side balance, Panning and Balance, stereo field rotation and more."
        };

        const plugin_t stereo_hammer =
        {
            "Stereo Hammer",
            "Stereo Hammer",
            "Stereo Hammer",
            "SH1",
            &developers::v_sadovnikov,
            "stereo_hammer",
            {
                LSP_LV2_URI("stereo_hammer"),
                LSP_LV2UI_URI("stereo_hammer"),
                "sh1s",
                LSP_VST3_UID("sh1      sh1s"),
                LSP_VST3UI_UID("sh1     sh1s"),
                LSP_LADSPA_STEREO_HAMMER_BASE + 0,
                LSP_LADSPA_URI("stereo_hammer"),
                LSP_CLAP_URI("stereo_hammer"),
                LSP_GST_UID("stereo_hammer"),
            },
            LSP_PLUGINS_STEREO_HAMMER_VERSION,
            plugin_classes,
            clap_features_stereo,
            E_DUMP_STATE,
            stereo_hammer_stereo_ports,
            "util/stereo_hammer.xml",
            NULL,
            stereo_plugin_port_groups,
            &stereo_hammer_bundle,
            1
        };
        LSP_REGISTER_METADATA(stereo_hammer);
    } /* namespace meta */
} /* namespace lsp */



