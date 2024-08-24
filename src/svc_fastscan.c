#include "svc_fastscan.h"
#include "driver/bk4819.h"
#include "driver/st7565.h"
#include "helper/lootlist.h"
#include "helper/presetlist.h"
#include "radio.h"

static uint32_t scanF = 0;
static uint8_t hits = 0;

static uint32_t delta(uint32_t f1, uint32_t f2) {
  if (f1 > f2) {
    return f1 - f2;
  }
  return f2 - f1;
}

void SVC_FC_Init(void) {
  BK4819_StopScan();
  BK4819_EnableFrequencyScanEx(F_SC_T_0_2s);
  hits = 0;
}

static void gotF(uint32_t f) {
  Preset *p = PRESET_ByFrequency(f);
  uint32_t step = StepFrequencyTable[p->band.step];
  uint32_t sd = f % step;
  if (sd > step / 2) {
    f += step - sd;
  } else {
    f -= sd;
  }

  Loot *loot = LOOT_Add(f);
}

void SVC_FC_Update(void) {
  uint32_t f = 0;

  if (gIsListening) {
    return;
  }

  if (!BK4819_GetFrequencyScanResult(&f)) {
    return;
  }

  uint32_t d = delta(f, scanF);

  if (d < 100) {
    if (hits++ >= 2) {
      gRedrawScreen = true;

      gotF(scanF);
      SVC_FC_Init();
    }
  } else {
    if (hits) {
      hits--;
    }
  }
  BK4819_DisableFrequencyScan();
  BK4819_EnableFrequencyScanEx(F_SC_T_0_2s);
  if (f) {
    scanF = f;
  }
}

void SVC_FC_Deinit(void) {
  BK4819_StopScan();
  BK4819_EnableRX();
}
