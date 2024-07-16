/*
 * DISTRHO Cardinal Plugin
 * Copyright (C) 2021-2024 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#ifdef BUILDING_PLUGIN_MODULES
#include "rack.hpp"
#endif

namespace rack {
namespace midi {
struct Message;
}
}

// --------------------------------------------------------------------------------------------------------------------
// Base DISTRHO classes

#ifndef DISTRHO_DETAILS_HPP_INCLUDED
namespace CardinalDISTRHO {
struct MidiEvent {
    static const uint32_t kDataSize = 4;
    uint32_t frame;
    uint32_t size;
    uint8_t data[kDataSize];
    const uint8_t* dataExt;
};
}
#endif

#ifndef DISTRHO_PLUGIN_HPP_INCLUDED
namespace CardinalDISTRHO {
class Plugin;
}
#endif

#ifndef DISTRHO_UI_HPP_INCLUDED
namespace CardinalDISTRHO {
class UI;
}
#endif

// --------------------------------------------------------------------------------------------------------------------
// Base DGL classes

#ifndef DGL_BASE_HPP_INCLUDED

namespace CardinalDGL {

class TopLevelWidget;
template <class BaseWidget> class NanoBaseWidget;
typedef NanoBaseWidget<TopLevelWidget> NanoTopLevelWidget;

struct IdleCallback {
    virtual ~IdleCallback() {}
    virtual void idleCallback() = 0;
};

}

#endif

using CardinalDGL::IdleCallback;

// --------------------------------------------------------------------------------------------------------------------
// Cardinal specific context

static constexpr const uint32_t kModuleParameterCount = 24;

enum CardinalVariant {
    kCardinalVariantMain,
    kCardinalVariantMini,
    kCardinalVariantFX,
    kCardinalVariantNative,
    kCardinalVariantSynth,
};

struct CardinalPluginContext : rack::Context {
    uint32_t bufferSize, processCounter;
    double sampleRate;
    float parameters[kModuleParameterCount];
    CardinalVariant variant;
    bool bypassed, playing, reset, bbtValid;
    int32_t bar, beat, beatsPerBar, beatType;
    uint64_t frame;
    double barStartTick, beatsPerMinute;
    double tick, tickClock, ticksPerBeat, ticksPerClock, ticksPerFrame;
    uintptr_t nativeWindowId;
    const float* const* dataIns;
    float** dataOuts;
    const CardinalDISTRHO::MidiEvent* midiEvents;
    uint32_t midiEventCount;
    CardinalDISTRHO::Plugin* const plugin;
    CardinalDGL::NanoTopLevelWidget* tlw;
    CardinalDISTRHO::UI* ui;
    CardinalPluginContext(CardinalDISTRHO::Plugin* const p);
    void writeMidiMessage(const rack::midi::Message& message, uint8_t channel);
    bool addIdleCallback(IdleCallback* cb) const;
    void removeIdleCallback(IdleCallback* cb) const;
};

#ifndef HEADLESS
void handleHostParameterDrag(const CardinalPluginContext* pcontext, uint index, bool started);
#endif

// --------------------------------------------------------------------------------------------------------------------
