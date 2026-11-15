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

#include <lsp-plug.in/stdlib/math.h>

#include <private/plugins/stereo_hammer.h>

namespace lsp
{
    namespace plugins
    {
        /* The size of temporary buffer for audio processing */
        static constexpr size_t BUFFER_SIZE             = 0x200;
        static constexpr float TO_RADIANS               = float(M_PI/180.0f);

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

                c->vIn                  = NULL;
                c->vOut                 = NULL;
                c->vData                = NULL;
                c->vTemp[0]             = NULL;
                c->vTemp[1]             = NULL;

                c->pIn                  = NULL;
                c->pOut                 = NULL;

                c->pLevelIn             = NULL;
                c->pLevelOut            = NULL;
            }

            construct_stage(&sLRProc);
            construct_stage(&sMSProc);

            // Initialize other parameters
            vBuffer[0]      = NULL;
            vBuffer[1]      = NULL;
            vRamp[0]        = NULL;
            vRamp[1]        = NULL;

            pBypass         = NULL;
            pGainOut        = NULL;

            pData           = NULL;
        }

        void stereo_hammer::construct_stage(stage_t *stage)
        {
            construct_stage_param(&stage->sOld);
            construct_stage_param(&stage->sNew);

            stage->vMeasure[0]      = GAIN_AMP_M_INF_DB;
            stage->vMeasure[1]      = GAIN_AMP_M_INF_DB;
            stage->nMeasure         = M_DELAY;

            stage->pOn              = NULL;
            stage->pPhase           = NULL;
            stage->pPreGain[0]      = NULL;
            stage->pPreGain[1]      = NULL;
            stage->pPreInvert[0]    = NULL;
            stage->pPreInvert[1]    = NULL;
            stage->pPan[0]          = NULL;
            stage->pPan[1]          = NULL;
            stage->pPreBalance      = NULL;
            stage->pRotate          = NULL;
            stage->pPostGain[0]     = NULL;
            stage->pPostGain[1]     = NULL;
            stage->pPostInvert[0]   = NULL;
            stage->pPostInvert[1]   = NULL;
            stage->pPostBalance     = NULL;
            stage->pDelayShift      = NULL;
            stage->pMeasure         = NULL;
            stage->pMeasureLvl[0]   = NULL;
            stage->pMeasureLvl[1]   = NULL;
        }

        void stereo_hammer::construct_stage_param(stage_param_t *param)
        {
            param->fPreGain[0]      = GAIN_AMP_0_DB;
            param->fPreGain[1]      = GAIN_AMP_0_DB;
            param->fPan[0]          = 0.0f;
            param->fPan[1]          = 1.0f;
            param->fPostGain[0]     = GAIN_AMP_0_DB;
            param->fPostGain[1]     = GAIN_AMP_0_DB;
            param->fDelay[0]        = 0.0f;
            param->fDelay[1]        = 0.0f;
            param->fPreBalance      = 0.0f;
            param->fPostBalance     = 0.0f;
            param->fRotate          = 0.0f;
            param->fRotateCos       = 1.0f;
            param->fRotateSin       = 0.0f;
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
            const size_t buf_sz     = BUFFER_SIZE * sizeof(float);
            const size_t alloc      =
                buf_sz * 2 +            // vBuffer
                buf_sz * 2 +            // vRamp
                2 * (                   // number of channels
                    buf_sz +            // channel_t::vData
                    2 * buf_sz          // channel_t::vTemp
                );

            // Allocate memory-aligned data
            uint8_t *ptr            = alloc_aligned<uint8_t>(pData, alloc, OPTIMAL_ALIGN);
            if (ptr == NULL)
                return;

            // Initialize pointers to channels and temporary buffer
            vBuffer[0]              = advance_ptr_bytes<float>(ptr, buf_sz);
            vBuffer[1]              = advance_ptr_bytes<float>(ptr, buf_sz);
            vRamp[0]                = advance_ptr_bytes<float>(ptr, buf_sz);
            vRamp[1]                = advance_ptr_bytes<float>(ptr, buf_sz);

            for (size_t i=0; i<2; ++i)
            {
                channel_t * const c     = &vChannels[i];

                c->vData                = advance_ptr_bytes<float>(ptr, buf_sz);
                c->vTemp[0]             = advance_ptr_bytes<float>(ptr, buf_sz);
                c->vTemp[1]             = advance_ptr_bytes<float>(ptr, buf_sz);
            }

            init_stage(&sLRProc);
            init_stage(&sMSProc);

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

        void stereo_hammer::init_stage(stage_t *stage)
        {
            stage->sBypass[0].set_bypass(false);
            stage->sBypass[1].set_bypass(false);
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
            BIND_PORT(stage->pPreBalance);
            BIND_PORT(stage->pRotate);
            BIND_PORT(stage->pPostGain[0]);
            BIND_PORT(stage->pPostGain[1]);
            BIND_PORT(stage->pPostInvert[0]);
            BIND_PORT(stage->pPostInvert[1]);
            BIND_PORT(stage->pPostBalance);
            BIND_PORT(stage->pDelayShift);
            BIND_PORT(stage->pMeasure);
            BIND_PORT(stage->pMeasureLvl[0]);
            BIND_PORT(stage->pMeasureLvl[1]);
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

            vBuffer[0]  = NULL;
            vBuffer[1]  = NULL;
            vRamp[0]    = NULL;
            vRamp[1]    = NULL;

            // Free previously allocated data chunk
            if (pData != NULL)
            {
                free_aligned(pData);
                pData       = NULL;
            }
        }

        void stereo_hammer::update_sample_rate(long sr)
        {
            const size_t max_delay      = dspu::millis_to_samples(sr, meta::stereo_hammer_metadata::DELAY_SHIFT_MAX);

            // Update sample rate for the bypass processors
            for (size_t i=0; i<2; ++i)
            {
                channel_t * const c     = &vChannels[i];
                c->sBypass.init(sr);

                sLRProc.sBypass[i].init(sr);
                sLRProc.sDelay[i].init(max_delay);
                sLRProc.sDelay[i].set_delay(sLRProc.sNew.fDelay[i]);

                sMSProc.sBypass[i].init(sr);
                sMSProc.sDelay[i].init(max_delay);
                sMSProc.sDelay[i].set_delay(sMSProc.sNew.fDelay[i]);
            }
        }

        void stereo_hammer::update_stage_settings(stage_t *stage)
        {
            const float bypass          =  stage->pOn->value() < 0.5f;
            const float in_phase        = (stage->pPhase->value() >= 0.5f) ? -1.0f : 1.0f;
            const float in_phase0       = (stage->pPreInvert[0]->value() >= 0.5f) ? -1.0f : 1.0f;
            const float in_phase1       = (stage->pPreInvert[1]->value() >= 0.5f) ? -1.0f : 1.0f;
            const float rotate          = stage->pRotate->value();

            const float out_phase0      = (stage->pPostInvert[0]->value() >= 0.5f) ? -1.0f : 1.0f;
            const float out_phase1      = (stage->pPostInvert[1]->value() >= 0.5f) ? -1.0f : 1.0f;
            const float pre_balance     = stage->pPreBalance->value() * 0.01f;
            const float post_balance    = stage->pPostBalance->value() * 0.01f;
            const float delay           = dspu::millis_to_samples(fSampleRate, stage->pDelayShift->value());

            stage->nMeasure             = uint8_t(stage->pMeasure->value());

            stage->sBypass[0].set_bypass(bypass);
            stage->sBypass[1].set_bypass(bypass);

            stage_param_t * const np    = &stage->sNew;
            stage_param_t * const op    = &stage->sOld;

            np->fPreGain[0]             = stage->pPreGain[0]->value() * in_phase * in_phase0;
            np->fPreGain[1]             = stage->pPreGain[1]->value() * in_phase * in_phase1;
            np->fPan[0]                 = (100.0f - stage->pPan[0]->value()) * 0.005f;
            np->fPan[1]                 = (100.0f - stage->pPan[1]->value()) * 0.005f;
            np->fPostGain[0]            = stage->pPostGain[0]->value() * out_phase0;
            np->fPostGain[1]            = stage->pPostGain[1]->value() * out_phase1;
            np->fDelay[0]               = lsp_max(delay, 0.0f);
            np->fDelay[1]               = lsp_max(-delay, 0.0f);
            np->fPreBalance             = pre_balance;
            np->fPostBalance            = post_balance;

            // Detect jumps
            constexpr float rot_range   = meta::stereo_hammer_metadata::ROTATE_MAX - meta::stereo_hammer_metadata::ROTATE_MIN;
            const float rot_delta       = rotate - op->fRotate;
            if (fabsf(rot_delta) >= rot_range * 0.875f)
            {
                // Jump: low -> high
                const float short_delta     = (rot_delta > 0.0f) ? rot_delta - rot_range : rot_delta + rot_range;
                np->fRotate                 = rotate;
                op->fRotate                 = rotate - short_delta;
            }
            else
                np->fRotate                 = rotate;

            // Rotation changed?
            if (np->fRotate != op->fRotate)
            {
                const float angle       = np->fRotate * TO_RADIANS;
                np->fRotateCos          = cosf(angle);
                np->fRotateSin          = sinf(angle);
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

            update_stage_settings(&sLRProc);
            update_stage_settings(&sMSProc);
        }

        void stereo_hammer::bind_buffers()
        {
            for (size_t i=0; i<2; ++i)
            {
                channel_t * const c     = &vChannels[i];
                c->vIn                  = c->pIn->buffer<float>();
                c->vOut                 = c->pOut->buffer<float>();
            }
        }

        void stereo_hammer::advance_buffers(size_t samples)
        {
            for (size_t i=0; i<2; ++i)
            {
                channel_t * const c     = &vChannels[i];
                c->vIn                 += samples;
                c->vOut                += samples;
            }
        }

        void stereo_hammer::measure_levels(stage_t *stage, const float *a, const float *b, measure_t measure, size_t samples)
        {
            if (stage->nMeasure != measure)
                return;

            const float a_max       = dsp::abs_max(a, samples);
            const float b_max       = dsp::abs_max(b, samples);

            stage->vMeasure[0]      = lsp_max(stage->vMeasure[0], a_max);
            stage->vMeasure[1]      = lsp_max(stage->vMeasure[1], b_max);
        }

        void stereo_hammer::process_stage(
            stage_t *stage,
            float **ins, float **outs,
            size_t offset, size_t total, size_t samples)
        {
            float * src_a           = ins[0];
            float * src_b           = ins[1];
            float * dst_a           = vChannels[0].vTemp[0];
            float * dst_b           = vChannels[1].vTemp[0];
            int temp                = 1;

            // Function to swap buffers between vTemp[0] and vTemp[1]
            auto swap_buffers       = [&]() {
                src_a                   = dst_a;
                src_b                   = dst_b;
                dst_a                   = vChannels[0].vTemp[temp];
                dst_b                   = vChannels[1].vTemp[temp];
                temp                   ^= 1;
            };

            stage_param_t * const pold = &stage->sOld;
            stage_param_t * const pnew = &stage->sNew;

            // Apply pre-gain
            {
                if (pold->fPreGain[0] != pnew->fPreGain[0])
                    dsp::lin_inter_mul3(
                        dst_a, src_a,
                        0.0f, pold->fPreGain[0],
                        total, pnew->fPreGain[0],
                        offset, samples);
                else
                    dsp::mul_k3(dst_a, src_a, pnew->fPreGain[0], samples);

                if (pold->fPreGain[1] != pnew->fPreGain[1])
                    dsp::lin_inter_mul3(
                        dst_b, src_b,
                        0.0f, pold->fPreGain[1],
                        total, pnew->fPreGain[1],
                        offset, samples);
                else
                    dsp::mul_k3(dst_b, src_b, pnew->fPreGain[1], samples);

                measure_levels(stage, dst_a, dst_b, M_PRE_GAIN, samples);
            }

            // Apply pre-panning
            {
                swap_buffers();

                // Channel A
                if (pold->fPan[0] != pnew->fPan[0])
                {
                    dsp::lin_inter_mul3(
                        dst_a, src_a,
                        0.0f, pold->fPan[0],
                        total, pnew->fPan[0],
                        offset, samples);
                    dsp::lin_inter_mul3(
                        dst_b, src_a,
                        0.0f, 1.0f - pold->fPan[0],
                        total, 1.0f - pnew->fPan[0],
                        offset, samples);
                }
                else
                {
                    dsp::mul_k3(dst_a, src_a, pnew->fPan[0], samples);
                    dsp::mul_k3(dst_b, src_a, 1.0f - pnew->fPan[0], samples);
                }

                // Channel B
                if (pold->fPan[1] != pnew->fPan[1])
                {
                    dsp::lin_inter_fmadd2(
                        dst_a, src_b,
                        0.0f, pold->fPan[1],
                        total, pnew->fPan[1],
                        offset, samples);
                    dsp::lin_inter_fmadd2(
                        dst_b, src_b,
                        0.0f, 1.0f - pold->fPan[1],
                        total, 1.0f - pnew->fPan[1],
                        offset, samples);
                }
                else
                {
                    dsp::fmadd_k3(dst_a, src_b, pnew->fPan[1], samples);
                    dsp::fmadd_k3(dst_b, src_b, 1.0f - pnew->fPan[1], samples);
                }

                measure_levels(stage, dst_a, dst_b, M_PANNING, samples);
            }

            // Apply Pre-Balancing, in-place
            {
                if (pold->fPreBalance != pnew->fPreBalance)
                {
                    dsp::lin_inter_mul2(
                        dst_a,
                        0.0f, 1.0f - pold->fPreBalance,
                        total, 1.0f - pnew->fPreBalance,
                        offset, samples);
                    dsp::lin_inter_mul2(
                        dst_b,
                        0.0f, 1.0f + pold->fPreBalance,
                        total, 1.0f + pnew->fPreBalance,
                        offset, samples);
                }
                else
                {
                    dsp::mul_k2(dst_a, 1.0f - pnew->fPreBalance, samples);
                    dsp::mul_k2(dst_b, 1.0f + pnew->fPreBalance, samples);
                }

                measure_levels(stage, dst_a, dst_b, M_PRE_BALANCE, samples);
            }

            // Apply rotation
            {
                swap_buffers();

                if (pold->fRotate != pnew->fRotate)
                {
                    float * const ramp_cos  = vRamp[0];
                    float * const ramp_sin  = vRamp[1];

                    // Compute sine/cosine ramping interpolation
                    dsp::lin_inter_set(
                        ramp_sin,
                        0.0f, pold->fRotate * TO_RADIANS,
                        total, pnew->fRotate * TO_RADIANS,
                        offset, samples);

                    dsp::cosf2(ramp_cos, ramp_sin, samples);
                    dsp::sinf1(ramp_sin, samples);

                    // Apply transformation
                    dsp::mul3(dst_a, src_a, ramp_cos, samples);
                    dsp::fmsub3(dst_a, src_b, ramp_sin, samples);
                    dsp::mul3(dst_b, src_a, ramp_sin, samples);
                    dsp::fmadd3(dst_b, src_b, ramp_cos, samples);
                }
                else
                {
                    dsp::mix_copy2(dst_a, src_a, src_b, pnew->fRotateCos, -pnew->fRotateSin, samples);
                    dsp::mix_copy2(dst_b, src_a, src_b, pnew->fRotateSin, pnew->fRotateCos, samples);
                }

                measure_levels(stage, dst_a, dst_b, M_ROTATION, samples);
            }

            // Apply post-gain, in-place, no need to swap buffers
            {
                if (pold->fPostGain[0] != pnew->fPostGain[0])
                    dsp::lin_inter_mul2(
                        dst_a,
                        0.0f, pold->fPostGain[0],
                        total, pnew->fPostGain[0],
                        offset, samples);
                else
                    dsp::mul_k2(dst_a, pnew->fPostGain[0], samples);

                if (pold->fPostGain[1] != pnew->fPostGain[1])
                    dsp::lin_inter_mul2(
                        dst_b,
                        0.0f, pold->fPostGain[1],
                        total, pnew->fPostGain[1],
                        offset, samples);
                else
                    dsp::mul_k2(dst_b, pnew->fPostGain[1], samples);

                measure_levels(stage, dst_a, dst_b, M_POST_GAIN, samples);
            }

            // Apply post balance, in-place, no need to swap buffers
            {
                if (pold->fPostBalance != pnew->fPostBalance)
                {
                    dsp::lin_inter_mul2(
                        dst_a,
                        0.0f, 1.0f - pold->fPostBalance,
                        total, 1.0f - pnew->fPostBalance,
                        offset, samples);
                    dsp::lin_inter_mul2(
                        dst_b,
                        0.0f, 1.0f + pold->fPostBalance,
                        total, 1.0f + pnew->fPostBalance,
                        offset, samples);
                }
                else
                {
                    dsp::mul_k2(dst_a, 1.0f - pnew->fPostBalance, samples);
                    dsp::mul_k2(dst_b, 1.0f + pnew->fPostBalance, samples);
                }

                measure_levels(stage, dst_a, dst_b, M_POST_BALANCE, samples);
            }

            // Apply delay, do in-place, no need to swap buffers
            {
                if ((pnew->fDelay[0] != pold->fDelay[0]) ||
                    (pnew->fDelay[1] != pold->fDelay[1]))
                {
                    const float k           = float(offset + samples) / float(total);
                    const float delay_a     = pnew->fDelay[0] * k + pold->fDelay[0] * (1.0f - k);
                    const float delay_b     = pnew->fDelay[1] * k + pold->fDelay[1] * (1.0f - k);

                    stage->sDelay[0].process_ramping(dst_a, dst_a, delay_a, samples);
                    stage->sDelay[1].process_ramping(dst_b, dst_b, delay_b, samples);
                }
                else
                {
                    stage->sDelay[0].set_delay(pnew->fDelay[0]);
                    stage->sDelay[1].set_delay(pnew->fDelay[1]);
                    stage->sDelay[0].process(dst_a, dst_a, samples);
                    stage->sDelay[1].process(dst_b, dst_b, samples);
                }
                measure_levels(stage, dst_a, dst_b, M_DELAY, samples);
            }

            // Apply bypass
            stage->sBypass[0].process(outs[0], ins[0], dst_a, samples);
            stage->sBypass[1].process(outs[1], ins[1], dst_b, samples);
        }

        void stereo_hammer::reset_meters()
        {
            sLRProc.vMeasure[0]     = GAIN_AMP_M_INF_DB;
            sLRProc.vMeasure[1]     = GAIN_AMP_M_INF_DB;

            sMSProc.vMeasure[0]     = GAIN_AMP_M_INF_DB;
            sMSProc.vMeasure[1]     = GAIN_AMP_M_INF_DB;
        }

        void stereo_hammer::output_meters()
        {
            sLRProc.pMeasureLvl[0]->set_value(sLRProc.vMeasure[0]);
            sLRProc.pMeasureLvl[1]->set_value(sLRProc.vMeasure[1]);

            sMSProc.pMeasureLvl[0]->set_value(sMSProc.vMeasure[0]);
            sMSProc.pMeasureLvl[1]->set_value(sMSProc.vMeasure[1]);
        }

        void stereo_hammer::process(size_t samples)
        {
            bind_buffers();
            reset_meters();

            // Perform main processing
            float *ins[2], *outs[2];
            for (size_t offset=0; offset<samples;)
            {
                const size_t to_do  = lsp_min(samples - offset, BUFFER_SIZE);

                // Process stage 1
                ins[0]              = vChannels[0].vIn;
                ins[1]              = vChannels[1].vIn;
                outs[0]             = vBuffer[0];
                outs[1]             = vBuffer[1];
                process_stage(&sLRProc, ins, outs, offset, samples, to_do);

                // Perform in-place left-right -> mid-side transform
                dsp::lr_to_ms(
                    outs[0], outs[1],
                    outs[0], outs[1],
                    to_do);

                // Process stage 2, result should be stored in another temporary buffer
                ins[0]              = outs[0];
                ins[1]              = outs[1];
                outs[0]             = vChannels[0].vData;
                outs[1]             = vChannels[1].vData;
                process_stage(&sMSProc, ins, outs, offset, samples, to_do);

                // Perform in-place reverse mid-side -> left-right transform
                dsp::lr_to_ms(
                    outs[0], outs[1],
                    outs[0], outs[1],
                    to_do);

                // Apply bypasses
                for (size_t i=0; i<2; ++i)
                {
                    channel_t * const c     = &vChannels[i];
                    c->sBypass.process(c->vOut, c->vIn, outs[i], to_do);
                }

                // Advance pointers and offsets
                advance_buffers(to_do);
                offset             += to_do;
            }

            // Commit new state of stereo processors
            sLRProc.sOld            = sLRProc.sNew;
            sMSProc.sOld            = sMSProc.sNew;

            // Output meters and graphs
            output_meters();
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

            v->writev("vBuffer", vBuffer, 2);
            v->writev("vRamp", vRamp, 2);

            v->write("pBypass", pBypass);

            v->write("pData", pData);
        }

    } /* namespace plugins */
} /* namespace lsp */


