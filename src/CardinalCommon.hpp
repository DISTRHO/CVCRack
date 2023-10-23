/*
 * DISTRHO Cardinal Plugin
 * Copyright (C) 2021-2023 Filipe Coelho <falktx@falktx.com>
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

#include "DistrhoUtils.hpp"

#include <string>

extern const std::string CARDINAL_VERSION;

// -----------------------------------------------------------------------------------------------------------

namespace rack {

namespace ui {
struct Menu;
}

namespace window {
void generateScreenshot();
}

bool isMini();
bool isStandalone();

#ifdef ARCH_WIN
enum SpecialPath {
    kSpecialPathUserProfile,
    kSpecialPathCommonProgramFiles,
    kSpecialPathProgramFiles,
    kSpecialPathAppData,
    kSpecialPathMyDocuments,
};
std::string getSpecialPath(SpecialPath type);
#endif

#ifdef DISTRHO_OS_WASM
extern char* patchFromURL;
extern char* patchRemoteURL;
extern char* patchStorageSlug;
void syncfs();
#endif

std::string homeDir();

void switchDarkMode(bool darkMode);

} // namespace rack

// -----------------------------------------------------------------------------------------------------------

namespace patchUtils {

void loadDialog();
void loadPathDialog(const std::string& path, bool asTemplate = false);
void loadSelectionDialog();
void loadTemplate(bool factory);
void loadTemplateDialog(bool factory);
void revertDialog();
void saveDialog(const std::string& path);
void saveAsDialog();
void saveAsDialogUncompressed();
void saveTemplateDialog();
void appendSelectionContextMenu(rack::ui::Menu* menu);
void openBrowser(const std::string& url);

} // namespace patchUtils

// -----------------------------------------------------------------------------------------------------------

#if defined(HAVE_LIBLO) && defined(HEADLESS)
# define CARDINAL_INIT_OSC_THREAD
#endif

typedef void* lo_server;
typedef void* lo_server_thread;

START_NAMESPACE_DISTRHO

class CardinalBasePlugin;
class CardinalBaseUI;
struct CardinalPluginContext;

struct Initializer
{
    std::string templatePath;
    std::string factoryTemplatePath;
    bool shouldSaveSettings = false;

    Initializer(const CardinalBasePlugin* plugin, const CardinalBaseUI* ui);
    ~Initializer();
    void loadSettings(bool isRealInstance);

  #ifdef HAVE_LIBLO
    lo_server oscServer = nullptr;
   #ifdef CARDINAL_INIT_OSC_THREAD
    lo_server_thread oscServerThread = nullptr;
   #endif
    CardinalBasePlugin* remotePluginInstance = nullptr;

    bool startRemoteServer(const char* port);
    void stopRemoteServer();
    void stepRemoteServer();
  #endif
};

#ifndef HEADLESS
void handleHostParameterDrag(const CardinalPluginContext* pcontext, uint index, bool started);
#endif

END_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------
