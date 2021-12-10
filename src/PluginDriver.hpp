/*
 * DISTRHO Cardinal Plugin
 * Copyright (C) 2021 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the LICENSE file.
 */

#pragma once

#include "PluginContext.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

struct CardinalAudioDevice : rack::audio::Device
{
    CardinalBasePlugin* const fPlugin;

    CardinalAudioDevice(CardinalBasePlugin* const plugin)
        : fPlugin(plugin) {}

    std::string getName() override
    {
        return "Cardinal";
    }

    int getNumInputs() override
    {
        return std::min(2, DISTRHO_PLUGIN_NUM_INPUTS);
    }

    int getNumOutputs() override
    {
        return std::min(2, DISTRHO_PLUGIN_NUM_OUTPUTS);
    }

    int getBlockSize() override
    {
        return fPlugin->getBufferSize();
    }

    float getSampleRate() override
    {
        return fPlugin->getSampleRate();
    }

    std::set<int> getBlockSizes() override
    {
        return std::set<int>({ getBlockSize() });
    }

    std::set<float> getSampleRates() override
    {
        return std::set<float>({ getSampleRate() });
    }

    void setBlockSize(int) override {}
    void setSampleRate(float) override {}

    void processInput(const float* const input, const int inputStride, const int frames)
    {
        for (rack::audio::Port* port : subscribed)
            port->processInput(input + port->inputOffset, inputStride, frames);
    }

    void processOutput(float* const output, const int outputStride, const int frames)
    {
        for (rack::audio::Port* port : subscribed)
            port->processOutput(output + port->outputOffset, outputStride, frames);
    }
};

// -----------------------------------------------------------------------------------------------------------

struct CardinalAudioDriver : rack::audio::Driver
{
    CardinalAudioDriver() {}

    std::string getName() override
    {
        return "Plugin Driver";
    }

    std::vector<int> getDeviceIds() override
    {
        return std::vector<int>({ 0 });
    }

    int getDefaultDeviceId() override
    {
        return 0;
    }

    std::string getDeviceName(int) override
    {
        return "Plugin Device";
    }

    int getDeviceNumInputs(int) override
    {
        return std::min(2, DISTRHO_PLUGIN_NUM_INPUTS);
    }

    int getDeviceNumOutputs(int) override
    {
        return std::min(2, DISTRHO_PLUGIN_NUM_OUTPUTS);
    }

    rack::audio::Device* subscribe(int, rack::audio::Port* const port) override
    {
        CardinalPluginContext* const pluginContext = reinterpret_cast<CardinalPluginContext*>(port->context);
        DISTRHO_SAFE_ASSERT_RETURN(pluginContext != nullptr, nullptr);

        CardinalBasePlugin* const plugin = reinterpret_cast<CardinalBasePlugin*>(pluginContext->plugin);
        DISTRHO_SAFE_ASSERT_RETURN(plugin != nullptr, nullptr);

        if (! plugin->canAssignAudioDevice())
            throw rack::Exception("Plugin driver only allows one audio device to be used simultaneously");

        CardinalAudioDevice* const device = new CardinalAudioDevice(plugin);
        device->subscribe(port);

        if (plugin->isActive())
            device->onStartStream();

        plugin->assignAudioDevice(device);
        return device;
    }

    void unsubscribe(int, rack::audio::Port* const port) override
    {
        CardinalAudioDevice* const device = reinterpret_cast<CardinalAudioDevice*>(port->device);
        DISTRHO_SAFE_ASSERT_RETURN(device != nullptr,);

        CardinalPluginContext* const pluginContext = reinterpret_cast<CardinalPluginContext*>(port->context);
        DISTRHO_SAFE_ASSERT_RETURN(pluginContext != nullptr,);

        CardinalBasePlugin* const plugin = reinterpret_cast<CardinalBasePlugin*>(pluginContext->plugin);
        DISTRHO_SAFE_ASSERT_RETURN(plugin != nullptr,);

        if (plugin->clearAudioDevice(device))
        {
            if (plugin->isActive())
                device->onStopStream();

            device->unsubscribe(port);
            delete device;
        }
    }
};

// -----------------------------------------------------------------------------------------------------------

struct CardinalMidiInputDevice : rack::midi::InputDevice
{
    CardinalBasePlugin* const fPlugin;
    rack::midi::Message msg;

    CardinalMidiInputDevice(CardinalBasePlugin* const plugin)
        : fPlugin(plugin)
    {
        msg.bytes.resize(0xff);
    }

    std::string getName() override
    {
        return "Cardinal";
    }

    void handleMessagesFromHost(const MidiEvent* const midiEvents, const uint32_t midiEventCount)
    {
        if (subscribed.size() == 0)
            return;

        for (uint32_t i=0; i<midiEventCount; ++i)
        {
            const MidiEvent& midiEvent(midiEvents[i]);
            const uint8_t* data;

            if (midiEvent.size > MidiEvent::kDataSize)
            {
                data = midiEvent.dataExt;
                msg.bytes.resize(midiEvent.size);
            }
            else
            {
                data = midiEvent.data;
            }

            msg.frame = midiEvent.frame;
            std::memcpy(msg.bytes.data(), data, midiEvent.size);

            onMessage(msg);
        }
    }
};

// -----------------------------------------------------------------------------------------------------------

struct CardinalMidiOutputDevice : rack::midi::OutputDevice
{
    CardinalBasePlugin* const fPlugin;

    CardinalMidiOutputDevice(CardinalBasePlugin* const plugin)
        : fPlugin(plugin) {}

    std::string getName() override
    {
        return "Cardinal";
    }

    void sendMessage(const rack::midi::Message& message) override
    {
        const size_t size = message.bytes.size();
        DISTRHO_SAFE_ASSERT_RETURN(size > 0,);

        MidiEvent event;
        event.frame = message.frame < 0 ? 0 : (message.frame - fPlugin->context->engine->getBlockFrame());

        switch (message.bytes[0] & 0xF0)
        {
        case 0x80:
        case 0x90:
        case 0xA0:
        case 0xB0:
        case 0xE0:
            event.size = 3;
            break;
        case 0xC0:
        case 0xD0:
            event.size = 2;
            break;
        case 0xF0:
            switch (message.bytes[0] & 0x0F)
            {
            case 0x0:
            case 0x4:
            case 0x5:
            case 0x7:
            case 0x9:
            case 0xD:
                // unsupported
                return;
            case 0x1:
            case 0x2:
            case 0x3:
            case 0xE:
                event.size = 3;
                break;
            case 0x6:
            case 0x8:
            case 0xA:
            case 0xB:
            case 0xC:
            case 0xF:
                event.size = 1;
                break;
            }
            break;
        }

        DISTRHO_SAFE_ASSERT_RETURN(size >= event.size,);

        std::memcpy(event.data, message.bytes.data(), event.size);

        fPlugin->writeMidiEvent(event);
    }
};

// -----------------------------------------------------------------------------------------------------------

struct CardinalMidiDriver : rack::midi::Driver
{
    CardinalMidiDriver() {}

    std::string getName() override
    {
        return "Plugin Driver";
    }

    std::vector<int> getInputDeviceIds() override
    {
        return std::vector<int>({ 0 });
    }

    std::vector<int> getOutputDeviceIds() override
    {
        return std::vector<int>({ 0 });
    }

    int getDefaultInputDeviceId() override
    {
        return 0;
    }

    int getDefaultOutputDeviceId() override
    {
        return 0;
    }

    std::string getInputDeviceName(int) override
    {
        return "Plugin Device";
    }

    std::string getOutputDeviceName(int) override
    {
        return "Plugin Device";
    }

    rack::midi::InputDevice* subscribeInput(int, rack::midi::Input* const input) override
    {
        CardinalPluginContext* const pluginContext = reinterpret_cast<CardinalPluginContext*>(input->context);
        DISTRHO_SAFE_ASSERT_RETURN(pluginContext != nullptr, nullptr);

        CardinalBasePlugin* const plugin = reinterpret_cast<CardinalBasePlugin*>(pluginContext->plugin);
        DISTRHO_SAFE_ASSERT_RETURN(plugin != nullptr, nullptr);

        CardinalMidiInputDevice* const device = new CardinalMidiInputDevice(plugin);
        device->subscribe(input);
        plugin->assignMidiInputDevice(device);
        return device;
    }

    rack::midi::OutputDevice* subscribeOutput(int, rack::midi::Output* const output) override
    {
        CardinalPluginContext* const pluginContext = reinterpret_cast<CardinalPluginContext*>(output->context);
        DISTRHO_SAFE_ASSERT_RETURN(pluginContext != nullptr, nullptr);

        CardinalBasePlugin* const plugin = reinterpret_cast<CardinalBasePlugin*>(pluginContext->plugin);
        DISTRHO_SAFE_ASSERT_RETURN(plugin != nullptr, nullptr);

        CardinalMidiOutputDevice* const device = new CardinalMidiOutputDevice(plugin);
        device->subscribe(output);
        plugin->assignMidiOutputDevice(device);
        return device;
    }

    void unsubscribeInput(int, rack::midi::Input* const input) override
    {
        CardinalMidiInputDevice* const device = reinterpret_cast<CardinalMidiInputDevice*>(input->device);
        DISTRHO_SAFE_ASSERT_RETURN(device != nullptr,);

        CardinalPluginContext* const pluginContext = reinterpret_cast<CardinalPluginContext*>(input->context);
        DISTRHO_SAFE_ASSERT_RETURN(pluginContext != nullptr,);

        CardinalBasePlugin* const plugin = reinterpret_cast<CardinalBasePlugin*>(pluginContext->plugin);
        DISTRHO_SAFE_ASSERT_RETURN(plugin != nullptr,);

        plugin->clearMidiInputDevice(device);
        device->unsubscribe(input);
        delete device;
    }

    void unsubscribeOutput(int, rack::midi::Output* const output) override
    {
        CardinalMidiOutputDevice* const device = reinterpret_cast<CardinalMidiOutputDevice*>(output->device);
        DISTRHO_SAFE_ASSERT_RETURN(device != nullptr,);

        CardinalPluginContext* const pluginContext = reinterpret_cast<CardinalPluginContext*>(output->context);
        DISTRHO_SAFE_ASSERT_RETURN(pluginContext != nullptr,);

        CardinalBasePlugin* const plugin = reinterpret_cast<CardinalBasePlugin*>(pluginContext->plugin);
        DISTRHO_SAFE_ASSERT_RETURN(plugin != nullptr,);

        plugin->clearMidiOutputDevice(device);
        device->unsubscribe(output);
        delete device;
    }
};

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
