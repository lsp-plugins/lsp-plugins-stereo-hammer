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

            // Common controls
            BYPASS,
            OUT_GAIN,

            // Stereo stage
            SWITCH("lr_on", "Stereo processor on", "Stereo On", 1),
            SWITCH("lr_ph", "Stereo phase invert", "Phase LR", 0),
            AMP_GAIN100("lr_blg", "Left pre-gain", "Pre-Gain L", GAIN_AMP_0_DB),
            AMP_GAIN100("lr_brg", "Right pre-gain", "Pre-Gain R", GAIN_AMP_0_DB),
            SWITCH("lr_bpl", "Left pre-invert", "Pre-Invert L", 0),
            SWITCH("lr_bpr", "Right pre-invert", "Pre-Invert R", 0),
            PAN_CTL("lr_pl", "Pan left", "Pan L", -100.0f),
            PAN_CTL("lr_pr", "Pan right", "Pan R", 100.0f),
            PAN_CTL("lr_bal", "Stereo balance", "Balance LR", 0.0f),
            CYC_CONTROL("lr_rot", "Stereo rotate", "Rotate L/R", U_DEG, stereo_hammer_metadata::ROTATE),
            AMP_GAIN100("lr_alg", "Left post-gain", "Post-Gain L", GAIN_AMP_0_DB),
            AMP_GAIN100("lr_arg", "Right post-gain", "Post-Gain R", GAIN_AMP_0_DB),
            SWITCH("lr_apl", "Left post-invert", "Post-Invert L", 0),
            SWITCH("lr_apr", "Right post-invert", "Post-Invert R", 0),
            CONTROL("lr_ds", "Stereo delay shift", "Delay Shift", U_MSEC, stereo_hammer_metadata::DELAY_SHIFT),

            // Mid/Side stage
            SWITCH("ms_on", "Mid/Side processor On", "M/S On", 1),
            SWITCH("ms_ph", "Mid/Side phase invert", "Phase M/S", 0),
            AMP_GAIN100("ms_bmg", "Mid pre-gain", "Pre-Gain M", GAIN_AMP_0_DB),
            AMP_GAIN100("ms_bsg", "Side pre-gain", "Pre-Gain S", GAIN_AMP_0_DB),
            SWITCH("ms_bpm", "Mid pre-invert", "Pre-Invert M", 0),
            SWITCH("ms_bps", "Side pre-invert", "Pre-Invert S", 0),
            PAN_CTL("ms_pm", "Pan mid", "Pan M", -100.0f),
            PAN_CTL("ms_ps", "Pan side", "Pan S", 100.0f),
            PAN_CTL("ms_bal", "Mid/Side balance", "Balance MS", 0.0f),
            CYC_CONTROL("ms_rot", "Mid/Side rotate", "Rotate M/S", U_DEG, stereo_hammer_metadata::ROTATE),
            AMP_GAIN100("ms_amg", "Mid post-gain", "Post-Gain L", GAIN_AMP_0_DB),
            AMP_GAIN100("ms_asg", "Side post-gain", "Post-Gain R", GAIN_AMP_0_DB),
            SWITCH("ms_apm", "Mid post-invert", "Post-Invert M", 0),
            SWITCH("ms_aps", "Side post-invert", "Post-Invert S", 0),
            CONTROL("ms_ds", "Mid/Side delay shift", "Delay Shift", U_MSEC, stereo_hammer_metadata::DELAY_SHIFT),

            // Metering
            CONTROL("goniohs", "Goniometer strobe history size", "Gonio history", U_NONE, stereo_hammer_metadata::GONIO_HISTORY), \
            LOG_CONTROL("goniond", "Maximum dots for plotting goniometer", "Max dots", U_NONE, stereo_hammer_metadata::GONIO_DOTS), \
            STREAM("gonio", "Goniometer stream buffer", 3, 128, 0x8000), \
            METER("corrm", "Correlation meter", U_NONE, stereo_hammer_metadata::CORRELATION), \
            METER("panm", "Panorama meter", U_NONE, stereo_hammer_metadata::PANOMETER), \
            METER("msbm", "Mid/Side balance meter", U_NONE, stereo_hammer_metadata::PANOMETER), \
            METER("pk", "Peak meter", U_GAIN_AMP, stereo_hammer_metadata::LOUD_METER), \
            METER("tp", "True Peak meter", U_GAIN_AMP, stereo_hammer_metadata::LOUD_METER), \
            METER("rms", "RMS meter", U_GAIN_AMP, stereo_hammer_metadata::LOUD_METER), \
            METER("mlufs", "Momentary LUFS meter", U_GAIN_AMP, stereo_hammer_metadata::LOUD_METER), \
            METER("slufs", "Short-Term LUFS meter", U_GAIN_AMP, stereo_hammer_metadata::LOUD_METER), \
            METER("llufs", "Long-Term LUFS meter", U_GAIN_AMP, stereo_hammer_metadata::LOUD_METER), \
            METER("ilufs", "Integrated LUFS meter", U_GAIN_AMP, stereo_hammer_metadata::LOUD_METER), \
            METER("ilm_l", "Input level meter left", U_GAIN_AMP, stereo_hammer_metadata::LOUD_METER), \
            METER("ilm_r", "Input level meter right", U_GAIN_AMP, stereo_hammer_metadata::LOUD_METER), \
            METER("olm_l", "Output level meter left", U_GAIN_AMP, stereo_hammer_metadata::LOUD_METER), \
            METER("olm_r", "Output level meter right", U_GAIN_AMP, stereo_hammer_metadata::LOUD_METER), \

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
                LSP_VST3_UID("sh1     sh1s"),
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



