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

#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/units.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/shared/debug.h>

#include <private/plugins/stereo_hammer.h>

namespace lsp
{
    namespace plugins
    {
        /* The size of temporary buffer for audio processing */
        static constexpr size_t BUFFER_SIZE             = 0x200;

        //---------------------------------------------------------------------
        // Plugin factory
        static const meta::plugin_t *plugins[] =
        {
            &meta::stereo_hammer,
        };

        static plug::Module *plugin_factory(const meta::plugin_t *meta)
        {
            return new stereo_hammer(meta);
        }

        static plug::Factory factory(plugin_factory, plugins, 1);

        //---------------------------------------------------------------------
        // Implementation
        stereo_hammer::stereo_hammer(const meta::plugin_t *meta):
            Module(meta)
        {
            // Initialize channels
            for (size_t i=0; i<2; ++i)
            {
                channel_t * const c     = &vChannels[i];
                c->sBypass.construct();

                c->pIn                  = NULL;
                c->pOut                 = NULL;

                c->pLevelIn             = NULL;
                c->pLevelOut            = NULL;
            }

            construct_stage(&sLRProc);
            construct_stage(&sMSProc);

            // Initialize other parameters
            vBuffer         = NULL;

            pBypass         = NULL;
            pGainOut        = NULL;

            pData           = NULL;
        }

        void stereo_hammer::construct_stage(stage_t *stage)
        {
            construct_stage_param(&stage->sOld);
            construct_stage_param(&stage->sNew);

            stage->pOn              = NULL;
            stage->pPhase           = NULL;
            stage->pPreGain[0]      = NULL;
            stage->pPreGain[1]      = NULL;
            stage->pPreInvert[0]    = NULL;
            stage->pPreInvert[1]    = NULL;
            stage->pPan[0]          = NULL;
            stage->pPan[1]          = NULL;
            stage->pBalance         = NULL;
            stage->pRotate          = NULL;
            stage->pPostGain[0]     = NULL;
            stage->pPostGain[1]     = NULL;
            stage->pPostInvert[0]   = NULL;
            stage->pPostInvert[1]   = NULL;
            stage->pDelayShift      = NULL;
        }

        void stereo_hammer::construct_stage_param(stage_param_t *param)
        {
            param->fPreGain[0]      = GAIN_AMP_0_DB;
            param->fPreGain[1]      = GAIN_AMP_0_DB;
            param->fPan[0]          = 0.0f;
            param->fPan[1]          = 1.0f;
            param->fRotate[0]       = 1.0f;
            param->fRotate[1]       = 0.0f;
            param->fPostGain[0]     = GAIN_AMP_0_DB;
            param->fPostGain[1]     = GAIN_AMP_0_DB;
            param->fBalance         = 0.0f;
            param->fDelay           = 0.0f;
        }

        stereo_hammer::~stereo_hammer()
        {
            do_destroy();
        }

        void stereo_hammer::init(plug::IWrapper *wrapper, plug::IPort **ports)
        {
            // Call parent class for initialization
            Module::init(wrapper, ports);

            // Estimate the number of bytes to allocate
            const size_t buf_sz     = BUFFER_SIZE * sizeof(float) * 2;
            const size_t alloc      = buf_sz;

            // Allocate memory-aligned data
            uint8_t *ptr            = alloc_aligned<uint8_t>(pData, alloc, OPTIMAL_ALIGN);
            if (ptr == NULL)
                return;

            // Initialize pointers to channels and temporary buffer
            vBuffer                 = advance_ptr_bytes<float>(ptr, buf_sz);

            // Bind ports
            lsp_trace("Binding ports");
            size_t port_id      = 0;

            // Bind input audio ports
            for (size_t i=0; i<2; ++i)
                BIND_PORT(vChannels[i].pIn);

            // Bind output audio ports
            for (size_t i=0; i<2; ++i)
                BIND_PORT(vChannels[i].pOut);

            // Bind bypass
            BIND_PORT(pBypass);
            BIND_PORT(pGainOut);

            // Bind processor-specific ports
            lsp_trace("Binding LR stage");
            bind_stage(&sLRProc, port_id, ports);
            lsp_trace("Binding MS stage");
            bind_stage(&sMSProc, port_id, ports);

            // Bind metering ports
            lsp_trace("Binding Metering ports");

            BIND_PORT(pGonioDots);
            BIND_PORT(pGonioHistory);
            BIND_PORT(pGoniometer);
            BIND_PORT(pCorrelation);
            BIND_PORT(pPanorama);
            BIND_PORT(pMSBalance);
            BIND_PORT(pPeak);
            BIND_PORT(pTruePeak);
            BIND_PORT(pRMS);
            BIND_PORT(pMLufs);
            BIND_PORT(pSLufs);
            BIND_PORT(pLLufs);
            BIND_PORT(pILufs);

            for (size_t i=0; i<2; ++i)
                BIND_PORT(vChannels[i].pLevelIn);
            for (size_t i=0; i<2; ++i)
                BIND_PORT(vChannels[i].pLevelOut);
        }

        void stereo_hammer::bind_stage(stage_t *stage, size_t & port_id, plug::IPort **ports)
        {
            BIND_PORT(stage->pOn);
            BIND_PORT(stage->pPhase);
            BIND_PORT(stage->pPreGain[0]);
            BIND_PORT(stage->pPreGain[1]);
            BIND_PORT(stage->pPreInvert[0]);
            BIND_PORT(stage->pPreInvert[1]);
            BIND_PORT(stage->pPan[0]);
            BIND_PORT(stage->pPan[1]);
            BIND_PORT(stage->pBalance);
            BIND_PORT(stage->pRotate);
            BIND_PORT(stage->pPostGain[0]);
            BIND_PORT(stage->pPostGain[1]);
            BIND_PORT(stage->pPostInvert[0]);
            BIND_PORT(stage->pPostInvert[1]);
            BIND_PORT(stage->pDelayShift);
        }

        void stereo_hammer::destroy()
        {
            Module::destroy();
            do_destroy();
        }

        void stereo_hammer::do_destroy()
        {
            // Destroy channels
            for (size_t i=0; i<2; ++i)
            {
                channel_t * const c     = &vChannels[i];
                c->sBypass.destroy();
            }

            vBuffer     = NULL;

            // Free previously allocated data chunk
            if (pData != NULL)
            {
                free_aligned(pData);
                pData       = NULL;
            }
        }

        void stereo_hammer::update_sample_rate(long sr)
        {
            // Update sample rate for the bypass processors
            for (size_t i=0; i<2; ++i)
            {
                channel_t * const c     = &vChannels[i];
                c->sBypass.init(sr);
            }
        }

        void stereo_hammer::update_settings()
        {
            const bool bypass       = pBypass->value() >= 0.5f;

            for (size_t i=0; i<2; ++i)
            {
                channel_t * const c     = &vChannels[i];
                c->sBypass.set_bypass(bypass);
            }
        }

        void stereo_hammer::process(size_t samples)
        {
            // Process each channel independently
            for (size_t i=0; i<2; ++i)
            {
                channel_t * const c     = &vChannels[i];

                // Get input and output buffers
                const float *in         = c->pIn->buffer<float>();
                float *out              = c->pOut->buffer<float>();
                if ((in == NULL) || (out == NULL))
                    continue;

                // Simple stuff at this moment
                dsp::move(out, in, samples);
            }
        }

        void stereo_hammer::dump(dspu::IStateDumper *v) const
        {
            plug::Module::dump(v);

            // It is very useful to dump plugin state for debug purposes
            v->begin_array("vChannels", vChannels, 2);
            for (size_t i=0; i<2; ++i)
            {
                const channel_t * const c   = &vChannels[i];

                v->begin_object(c, sizeof(channel_t));
                {
                    v->write_object("sBypass", &c->sBypass);

                    v->write("pIn", c->pIn);
                    v->write("pOut", c->pOut);
                }
                v->end_object();
            }
            v->end_array();

            v->write("vBuffer", vBuffer);

            v->write("pBypass", pBypass);

            v->write("pData", pData);
        }

    } /* namespace plugins */
} /* namespace lsp */


