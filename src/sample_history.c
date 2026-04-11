#include "sample_history.h"

#include <string.h>

void seed_sample_history(PersistedHistory *history) {
#if ENABLE_SAMPLE_HISTORY
  static const StepRecord sample_records[] = {
      {1, 1770351000000ULL, 88.4f, 7.1f, 10.2f, 8.4f, 1820.0f, 900.0f, 920.0f, 49.0f, 92.0f, 90.0f, 43.0f, 92.0f, 54.4f, 51.1f, 70.1f, 62.0f, 1},
      {2, 1770352200000ULL, 81.7f, 12.8f, 14.6f, 11.3f, 1760.0f, 840.0f, 920.0f, 47.0f, 91.0f, 88.0f, 44.0f, 91.0f, 56.0f, 48.2f, 77.0f, 58.7f, 2},
      {3, 1770353400000ULL, 74.8f, 18.2f, 22.7f, 14.1f, 1580.0f, 680.0f, 900.0f, 46.0f, 90.0f, 85.0f, 44.0f, 90.0f, 67.6f, 45.6f, 96.4f, 61.1f, 3},
      {4, 1770437400000ULL, 90.1f, 4.3f, 8.5f, 6.7f, 1890.0f, 960.0f, 930.0f, 50.0f, 93.0f, 92.0f, 43.0f, 93.0f, 52.1f, 53.8f, 66.9f, 64.0f, 0},
      {5, 1770438600000ULL, 86.5f, 8.9f, 11.4f, 9.8f, 1840.0f, 910.0f, 930.0f, 48.0f, 92.0f, 90.0f, 44.0f, 92.0f, 52.7f, 49.8f, 71.0f, 60.4f, 1},
      {6, 1770439800000ULL, 78.9f, 15.1f, 18.7f, 12.4f, 1660.0f, 730.0f, 930.0f, 45.0f, 91.0f, 86.0f, 46.0f, 91.0f, 61.6f, 43.0f, 87.4f, 58.1f, 2},
      {7, 1770523800000ULL, 92.0f, 3.5f, 7.0f, 5.8f, 1910.0f, 970.0f, 940.0f, 50.0f, 94.0f, 93.0f, 44.0f, 94.0f, 51.5f, 52.1f, 64.4f, 61.5f, 0},
      {8, 1770525000000ULL, 84.2f, 10.7f, 13.9f, 10.8f, 1750.0f, 820.0f, 930.0f, 47.0f, 92.0f, 89.0f, 45.0f, 92.0f, 57.3f, 47.2f, 75.0f, 58.6f, 1}};
  size_t sample_count = sizeof(sample_records) / sizeof(sample_records[0]);
  size_t index;

  if (history->count > 0) {
    return;
  }

  memset(history, 0, sizeof(*history));
  history->magic = STORAGE_MAGIC;
  history->count = (uint32_t)sample_count;
  history->next_id = (uint32_t)sample_count + 1U;

  for (index = 0; index < sample_count; ++index) {
    history->records[index] = sample_records[index];
  }
#else
  (void)history;
#endif
}
