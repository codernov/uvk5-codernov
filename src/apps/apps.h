#ifndef APPS_H
#define APPS_H

#include "../driver/keyboard.h"

#define APPS_COUNT 20
#define RUN_APPS_COUNT 10

typedef enum {
  APP_NONE,
  APP_TASK_MANAGER,
  APP_SPECTRUM,
  APP_SPECTRUM_CH,
  APP_CH_SCANNER,
  APP_FASTSCAN,
  APP_STILL,
  APP_FINPUT,
  APP_APPS_LIST,
  APP_LOOT_LIST,
  APP_PRESETS_LIST,
  APP_RESET,
  APP_TEXTINPUT,
  APP_VFO_CFG,
  APP_PRESET_CFG,
  APP_SCANLISTS,
  APP_SAVECH,
  APP_SETTINGS,
  APP_VFO,
  APP_ABOUT,
  // APP_SCANLIST,
  // APP_AB_SCANNER,
} AppType_t;

typedef struct App {
  const char *name;
  void (*init)(void);
  void (*update)(void);
  void (*render)(void);
  bool (*key)(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);
  void (*deinit)(void);
} App;

extern const App apps[APPS_COUNT];
extern AppType_t gPreviousApp;
extern AppType_t gCurrentApp;
extern const AppType_t appsAvailableToRun[RUN_APPS_COUNT];

AppType_t APPS_Peek();
bool APPS_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);
void APPS_init(AppType_t app);
void APPS_update(void);
void APPS_render(void);
void APPS_run(AppType_t app);
bool APPS_exit(void);

#endif /* end of include guard: APPS_H */
