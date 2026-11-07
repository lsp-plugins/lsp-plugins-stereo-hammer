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

#ifndef PRIVATE_PLUGINS_STEREO_HAMMER_H_
#define PRIVATE_PLUGINS_STEREO_HAMMER_H_

#include <lsp-plug.in/dsp-units/util/Delay.h>
#include <lsp-plug.in/dsp-units/ctl/Bypass.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <private/meta/stereo_hammer.h>

namespace lsp
{
    namespace plugins
    {
        /**
         * Base class for the latency compensation delay
         */
        class stereo_hammer: public plug::Module
        {
            protected:
                enum mode_t
                {
                    CD_MONO,
                    CD_STEREO,
                    CD_X2_STEREO
                };

                typedef struct channel_t
                {
                    // DSP processing modules
                    dspu::Bypass        sBypass;            // Bypass

                    plug::IPort        *pIn;                // Input port
                    plug::IPort        *pOut;               // Output port
                } channel_t;

            protected:
                channel_t           vChannels[2];       // Audio channels
                float              *vBuffer;            // Temporary buffer for audio processing

                plug::IPort        *pBypass;            // Bypass

                uint8_t            *pData;              // Allocated data

            protected:
                void                do_destroy();

            public:
                explicit stereo_hammer(const meta::plugin_t *meta);
                stereo_hammer (const stereo_hammer &) = delete;
                stereo_hammer (stereo_hammer &&) = delete;
                virtual ~stereo_hammer() override;

                stereo_hammer & operator = (const stereo_hammer &) = delete;
                stereo_hammer & operator = (stereo_hammer &&) = delete;

                virtual void        init(plug::IWrapper *wrapper, plug::IPort **ports) override;
                virtual void        destroy() override;

            public:
                virtual void        update_sample_rate(long sr) override;
                virtual void        update_settings() override;
                virtual void        process(size_t samples) override;
                virtual void        dump(dspu::IStateDumper *v) const override;
        };

    } /* namespace plugins */
} /* namespace lsp */


#endif /* PRIVATE_PLUGINS_STEREO_HAMMER_H_ */

