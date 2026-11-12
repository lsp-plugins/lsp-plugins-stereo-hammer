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

#include <lsp-plug.in/dsp-units/meters/ILUFSMeter.h>
#include <lsp-plug.in/dsp-units/meters/LoudnessMeter.h>
#include <lsp-plug.in/dsp-units/meters/Panometer.h>
#include <lsp-plug.in/dsp-units/meters/PeakMeter.h>
#include <lsp-plug.in/dsp-units/meters/TruePeakMeter.h>
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
                typedef struct mat2_t
                {
                    float               v[2][2];
                } mat2_t;

                typedef struct stage_param_t
                {
                    float               fPreGain[2];
                    float               fPan[2];
                    float               fPostGain[2];
                    float               fDelay[2];
                    float               fRotate;
                } stage_param_t;

                typedef struct stage_t
                {
                    dspu::Delay         sDelay[2];
                    dspu::Bypass        sBypass[2];

                    stage_param_t       sOld;               // Old (actual) settings
                    stage_param_t       sNew;               // New (updated) settings
                    mat2_t              sMatrix;            // Transform matrix
                    bool                bMatrixChanged;     // Matrix changing flag
                    bool                bDelayChanged;      // Delay changing flag

                    plug::IPort        *pOn;
                    plug::IPort        *pPhase;
                    plug::IPort        *pPreGain[2];
                    plug::IPort        *pPreInvert[2];
                    plug::IPort        *pPan[2];
                    plug::IPort        *pBalance;
                    plug::IPort        *pRotate;
                    plug::IPort        *pPostGain[2];
                    plug::IPort        *pPostInvert[2];
                    plug::IPort        *pDelayShift;
                } stage_t;

                typedef struct channel_t
                {
                    // DSP processing modules
                    dspu::Bypass        sBypass;            // Bypass
                    dspu::TruePeakMeter sTPMeter;           // True peak meter
                    dspu::PeakMeter     sPKMeter;           // Peak meter

                    float              *vIn;                // Input buffer
                    float              *vOut;               // Output buffer

                    float              *vData;              // Data buffer
                    float              *vTemp[2];           // Temporary buffers

                    plug::IPort        *pIn;                // Input port
                    plug::IPort        *pOut;               // Output port

                    plug::IPort        *pLevelIn;           // Input level meter
                    plug::IPort        *pLevelOut;          // Output level meter
                } channel_t;

            protected:
                channel_t           vChannels[2];       // Audio channels
                stage_t             sLRProc;            // Left/Right processing stage
                stage_t             sMSProc;            // Mid/Side processing stage
                dspu::LoudnessMeter sMLUFSMeter;        // Momentary LUFS meter
                dspu::LoudnessMeter sSLUFSMeter;        // Short-term LUFS meter
                dspu::ILUFSMeter    sLLUFSMeter;        // Long-term LUFS meter
                dspu::ILUFSMeter    sILUFSMeter;        // Integrated loudness meter for an infinite period
                dspu::Panometer     sPanometer;         // Panometer
                dspu::Panometer     sMsBalance;         // Mid/Side balance

                float              *vBuffer;            // Temporary buffer for audio processing

                plug::IPort        *pBypass;            // Bypass
                plug::IPort        *pGainOut;           // Output gain

                plug::IPort        *pGonioDots;         // Goniometer dots
                plug::IPort        *pGonioHistory;      // Goniometer history
                plug::IPort        *pGoniometer;        // Goniometer data
                plug::IPort        *pCorrelation;       // Correlometer output
                plug::IPort        *pPanorama;          // Panometer output
                plug::IPort        *pMSBalance;         // Mid/Side balance output
                plug::IPort        *pPeak;              // Peak meter outputs
                plug::IPort        *pTruePeak;          // True peak meter outputs
                plug::IPort        *pRMS;               // RMS meter
                plug::IPort        *pMLufs;             // Momentary LUFS meter
                plug::IPort        *pSLufs;             // Short-term LUFS meter
                plug::IPort        *pLLufs;             // Long-term LUFS meter
                plug::IPort        *pILufs;             // Integrated LUFS meter

                uint8_t            *pData;              // Allocated data

            protected:
                static void         construct_stage(stage_t *stage);
                static void         construct_stage_param(stage_param_t *param);
                static void         bind_stage(stage_t *stage, size_t & port_id, plug::IPort **ports);
                static mat2_t       mat2mul(const mat2_t &a, const mat2_t &b);

            protected:
                void                update_stage_settings(stage_t *stage);
                void                do_destroy();
                void                bind_buffers();
                void                advance_buffers(size_t samples);
                void                process_stage(
                                        stage_t *stage,
                                        float **ins, float **outs,
                                        size_t offset, size_t total, size_t samples);

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

