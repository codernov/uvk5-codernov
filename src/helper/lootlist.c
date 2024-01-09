#include "lootlist.h"
#include "../dcs.h"
#include "../driver/bk4819.h"
#include "../driver/uart.h"
#include "../scheduler.h"

static Loot loot[LOOT_SIZE_MAX] = {0};
static int16_t lootIndex = -1;

Loot *gLastActiveLoot = NULL;
int16_t gLastActiveLootIndex = -1;

void LOOT_BlacklistLast() {
  if (gLastActiveLoot) {
    gLastActiveLoot->goodKnown = false;
    gLastActiveLoot->blacklist = true;
  }
}

void LOOT_GoodKnownLast() {
  if (gLastActiveLoot) {
    gLastActiveLoot->blacklist = false;
    gLastActiveLoot->goodKnown = true;
  }
}

Loot *LOOT_Get(uint32_t f) {
  for (uint8_t i = 0; i < LOOT_Size(); ++i) {
    if ((&loot[i])->f == f) {
      return &loot[i];
    }
  }
  return NULL;
}

Loot *LOOT_AddEx(uint32_t f, bool reuse) {
  if (reuse) {
    Loot *p = LOOT_Get(f);
    if (p) {
      return p;
    }
  }
  if (LOOT_Size() < LOOT_SIZE_MAX) {
    lootIndex++;
    loot[lootIndex] = (Loot){
        .f = f,
        .firstTime = elapsedMilliseconds,
        .lastTimeCheck = elapsedMilliseconds,
        .lastTimeOpen = elapsedMilliseconds,
        .duration = 0,
        .rssi = 0,
        .noise = 65535,
        .open = true, // as we add it when open
        .ct = 0xFF,
        .cd = 0xFF,
    };
    return &loot[lootIndex];
  }
  return NULL;
}

Loot *LOOT_Add(uint32_t f) { return LOOT_AddEx(f, true); }

void LOOT_Remove(uint8_t i) {
  if (LOOT_Size()) {
    for (uint8_t _i = i; _i < LOOT_Size() - 1; ++_i) {
      loot[_i] = loot[_i + 1];
    }
    lootIndex--;
  }
}

void LOOT_Clear() { lootIndex = -1; }

uint8_t LOOT_Size() { return lootIndex + 1; }

void LOOT_Standby() {
  for (uint8_t i = 0; i < LOOT_Size(); ++i) {
    Loot *p = &loot[i];
    p->open = false;
    p->lastTimeCheck = elapsedMilliseconds;
  }
}

static void swap(Loot *a, Loot *b) {
  Loot tmp = *a;
  *a = *b;
  *b = tmp;
}

bool LOOT_SortByLastOpenTime(Loot *a, Loot *b) {
  return a->lastTimeOpen < b->lastTimeOpen;
}

bool LOOT_SortByDuration(Loot *a, Loot *b) { return a->duration > b->duration; }

bool LOOT_SortByF(Loot *a, Loot *b) { return a->f > b->f; }

bool LOOT_SortByBlacklist(Loot *a, Loot *b) {
  return a->blacklist > b->blacklist;
}

static void Sort(Loot *items, uint16_t n, bool (*compare)(Loot *a, Loot *b),
                 bool reverse) {
  for (uint16_t i = 0; i < n - 1; i++) {
    bool swapped = false;
    for (uint16_t j = 0; j < n - i - 1; j++) {
      if (compare(&items[j], &items[j + 1]) ^ reverse) {
        swap(&items[j], &items[j + 1]);
        swapped = true;
      }
    }
    if (!swapped) {
      break;
    }
  }
}

void LOOT_Sort(bool (*compare)(Loot *a, Loot *b), bool reverse) {
  Sort(loot, LOOT_Size(), compare, reverse);
}

Loot *LOOT_Item(uint8_t i) { return &loot[i]; }

void LOOT_Replace(Loot *loot, uint32_t f) {
  loot->f = f;
  loot->open = false;
  loot->firstTime = elapsedMilliseconds;
  loot->lastTimeCheck = elapsedMilliseconds;
  loot->lastTimeOpen = 0;
  loot->duration = 0;
  loot->rssi = 0;
  loot->noise = 65535;
  loot->ct = 0xFF;
  loot->cd = 0xFF;
}

void LOOT_ReplaceItem(uint8_t i, uint32_t f) {
  Loot *item = LOOT_Item(i);
  LOOT_Replace(item, f);
}

void LOOT_UpdateEx(Loot *loot, Loot *msm) {
  if (loot == NULL) {
    return;
  }

  if (loot->blacklist || loot->goodKnown) {
    msm->open = false;
  }

  loot->noise = msm->noise;
  loot->rssi = msm->rssi;

  if (loot->open) {
    loot->duration += elapsedMilliseconds - loot->lastTimeCheck;
    gLastActiveLoot = loot;
  }
  if (msm->open) {
    uint32_t cd = 0;
    uint16_t ct = 0;
    uint8_t Code = 0;
    BK4819_CssScanResult_t res = BK4819_GetCxCSSScanResult(&cd, &ct);
    switch (res) {
    case BK4819_CSS_RESULT_CDCSS:
      Code = DCS_GetCdcssCode(cd);
      if (Code != 0xFF) {
        loot->cd = Code;
      }
      break;
    case BK4819_CSS_RESULT_CTCSS:
      Code = DCS_GetCtcssCode(ct);
      if (Code != 0xFF) {
        loot->ct = Code;
      }
      break;
    default:
      break;
    }
    loot->lastTimeOpen = elapsedMilliseconds;
  }
  loot->lastTimeCheck = elapsedMilliseconds;
  loot->open = msm->open;

  if (msm->blacklist) {
    loot->blacklist = true;
  }
}

void LOOT_Update(Loot *msm) {
  Loot *loot = LOOT_Get(msm->f);

  if (loot == NULL && msm->open) {
    loot = LOOT_Add(msm->f);
    UART_logf(1, "[LOOT] %u", msm->f);
  }

  LOOT_UpdateEx(loot, msm);
}
