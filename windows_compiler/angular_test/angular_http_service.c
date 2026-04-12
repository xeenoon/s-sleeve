#include "angular_http_service.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "helpers/include/data/json.h"

#include "helpers/include/support/stringbuilder.h"

#include "helpers/include/runtime/server_runtime.h"

static const char g_route_body_0[] = "{\r\n    \"goal\":  85,\r\n    \"history\":  [\r\n                    {\r\n                        \"timestampMs\":  1775030400000,\r\n                        \"score\":  56.8,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  0.2,\r\n                        \"compensation\":  0.1,\r\n                        \"durationMs\":  980,\r\n                        \"range\":  52,\r\n                        \"descentAvgSpeed\":  0.5,\r\n                        \"ascentAvgSpeed\":  0.6,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775036100000,\r\n                        \"score\":  64.4,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  0.8,\r\n                        \"compensation\":  0.3,\r\n                        \"durationMs\":  1023,\r\n                        \"range\":  56,\r\n                        \"descentAvgSpeed\":  0.7,\r\n                        \"ascentAvgSpeed\":  1,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775041800000,\r\n                        \"score\":  72,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  0.3,\r\n                        \"compensation\":  0.5,\r\n                        \"durationMs\":  1066,\r\n                        \"range\":  60,\r\n                        \"descentAvgSpeed\":  1,\r\n                        \"ascentAvgSpeed\":  1.5,\r\n                        \"oscillations\":  2\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775047500000,\r\n                        \"score\":  79.6,\r\n                        \"shakiness\":  2.3,\r\n                        \"uncontrolledDescent\":  0.9,\r\n                        \"compensation\":  0.6,\r\n                        \"durationMs\":  1109,\r\n                        \"range\":  64,\r\n                        \"descentAvgSpeed\":  1.2,\r\n                        \"ascentAvgSpeed\":  0.9,\r\n                        \"oscillations\":  1\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775053200000,\r\n                        \"score\":  87.2,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  0.4,\r\n                        \"compensation\":  0.8,\r\n                        \"durationMs\":  1152,\r\n                        \"range\":  68,\r\n                        \"descentAvgSpeed\":  1.5,\r\n                        \"ascentAvgSpeed\":  1.4,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775058900000,\r\n                        \"score\":  91.8,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  1,\r\n                        \"compensation\":  0.1,\r\n                        \"durationMs\":  1195,\r\n                        \"range\":  72,\r\n                        \"descentAvgSpeed\":  0.5,\r\n                        \"ascentAvgSpeed\":  0.8,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775064600000,\r\n                        \"score\":  60.4,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  0.5,\r\n                        \"compensation\":  0.3,\r\n                        \"durationMs\":  1238,\r\n                        \"range\":  76,\r\n                        \"descentAvgSpeed\":  0.7,\r\n                        \"ascentAvgSpeed\":  1.3,\r\n                        \"oscillations\":  2\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775070300000,\r\n                        \"score\":  68,\r\n                        \"shakiness\":  2.3,\r\n                        \"uncontrolledDescent\":  1.1,\r\n                        \"compensation\":  0.5,\r\n                        \"durationMs\":  1281,\r\n                        \"range\":  80,\r\n                        \"descentAvgSpeed\":  1,\r\n                        \"ascentAvgSpeed\":  0.7,\r\n                        \"oscillations\":  1\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775076000000,\r\n                        \"score\":  75.6,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  0.6,\r\n                        \"compensation\":  0.6,\r\n                        \"durationMs\":  1324,\r\n                        \"range\":  53,\r\n                        \"descentAvgSpeed\":  1.2,\r\n                        \"ascentAvgSpeed\":  1.2,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775081700000,\r\n                        \"score\":  83.2,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  0.2,\r\n                        \"compensation\":  0.8,\r\n                        \"durationMs\":  1367,\r\n                        \"range\":  57,\r\n                        \"descentAvgSpeed\":  1.5,\r\n                        \"ascentAvgSpeed\":  0.6,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775087400000,\r\n                        \"score\":  87.8,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  0.8,\r\n                        \"compensation\":  0.1,\r\n                        \"durationMs\":  1410,\r\n                        \"range\":  61,\r\n                        \"descentAvgSpeed\":  0.5,\r\n                        \"ascentAvgSpeed\":  1,\r\n                        \"oscillations\":  2\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775093100000,\r\n                        \"score\":  95.4,\r\n                        \"shakiness\":  2.3,\r\n                        \"uncontrolledDescent\":  0.3,\r\n                        \"compensation\":  0.3,\r\n                        \"durationMs\":  1453,\r\n                        \"range\":  65,\r\n                        \"descentAvgSpeed\":  0.7,\r\n                        \"ascentAvgSpeed\":  1.5,\r\n                        \"oscillations\":  1\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775098800000,\r\n                        \"score\":  64,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  0.9,\r\n                        \"compensation\":  0.5,\r\n                        \"durationMs\":  1496,\r\n                        \"range\":  69,\r\n                        \"descentAvgSpeed\":  1,\r\n                        \"ascentAvgSpeed\":  0.9,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775104500000,\r\n                        \"score\":  71.6,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  0.4,\r\n                        \"compensation\":  0.6,\r\n                        \"durationMs\":  1539,\r\n                        \"range\":  73,\r\n                        \"descentAvgSpeed\":  1.2,\r\n                        \"ascentAvgSpeed\":  1.4,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775110200000,\r\n                        \"score\":  79.2,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  1,\r\n                        \"compensation\":  0.8,\r\n                        \"durationMs\":  1582,\r\n                        \"range\":  77,\r\n                        \"descentAvgSpeed\":  1.5,\r\n                        \"ascentAvgSpeed\":  0.8,\r\n                        \"oscillations\":  2\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775115900000,\r\n                        \"score\":  83.8,\r\n                        \"shakiness\":  2.3,\r\n                        \"uncontrolledDescent\":  0.5,\r\n                        \"compensation\":  0.1,\r\n                        \"durationMs\":  1005,\r\n                        \"range\":  81,\r\n                        \"descentAvgSpeed\":  0.5,\r\n                        \"ascentAvgSpeed\":  1.3,\r\n                        \"oscillations\":  1\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775121600000,\r\n                        \"score\":  91.4,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  1.1,\r\n                        \"compensation\":  0.3,\r\n                        \"durationMs\":  1048,\r\n                        \"range\":  54,\r\n                        \"descentAvgSpeed\":  0.7,\r\n                        \"ascentAvgSpeed\":  0.7,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775127300000,\r\n                        \"score\":  60,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  0.6,\r\n                        \"compensation\":  0.5,\r\n                        \"durationMs\":  1091,\r\n                        \"range\":  58,\r\n                        \"descentAvgSpeed\":  1,\r\n                        \"ascentAvgSpeed\":  1.2,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775133000000,\r\n                        \"score\":  67.6,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  0.2,\r\n                        \"compensation\":  0.6,\r\n                        \"durationMs\":  1134,\r\n                        \"range\":  62,\r\n                        \"descentAvgSpeed\":  1.2,\r\n                        \"ascentAvgSpeed\":  0.6,\r\n                        \"oscillations\":  2\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775138700000,\r\n                        \"score\":  75.2,\r\n                        \"shakiness\":  2.3,\r\n                        \"uncontrolledDescent\":  0.8,\r\n                        \"compensation\":  0.8,\r\n                        \"durationMs\":  1177,\r\n                        \"range\":  66,\r\n                        \"descentAvgSpeed\":  1.5,\r\n                        \"ascentAvgSpeed\":  1,\r\n                        \"oscillations\":  1\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775144400000,\r\n                        \"score\":  79.8,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  0.3,\r\n                        \"compensation\":  0.1,\r\n                        \"durationMs\":  1220,\r\n                        \"range\":  70,\r\n                        \"descentAvgSpeed\":  0.5,\r\n                        \"ascentAvgSpeed\":  1.5,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775150100000,\r\n                        \"score\":  87.4,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  0.9,\r\n                        \"compensation\":  0.3,\r\n                        \"durationMs\":  1263,\r\n                        \"range\":  74,\r\n                        \"descentAvgSpeed\":  0.7,\r\n                        \"ascentAvgSpeed\":  0.9,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775155800000,\r\n                        \"score\":  95,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  0.4,\r\n                        \"compensation\":  0.5,\r\n                        \"durationMs\":  1306,\r\n                        \"range\":  78,\r\n                        \"descentAvgSpeed\":  1,\r\n                        \"ascentAvgSpeed\":  1.4,\r\n                        \"oscillations\":  2\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775161500000,\r\n                        \"score\":  63.6,\r\n                        \"shakiness\":  2.3,\r\n                        \"uncontrolledDescent\":  1,\r\n                        \"compensation\":  0.6,\r\n                        \"durationMs\":  1349,\r\n                        \"range\":  82,\r\n                        \"descentAvgSpeed\":  1.2,\r\n                        \"ascentAvgSpeed\":  0.8,\r\n                        \"oscillations\":  1\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775167200000,\r\n                        \"score\":  71.2,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  0.5,\r\n                        \"compensation\":  0.8,\r\n                        \"durationMs\":  1392,\r\n                        \"range\":  55,\r\n                        \"descentAvgSpeed\":  1.5,\r\n                        \"ascentAvgSpeed\":  1.3,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775172900000,\r\n                        \"score\":  75.8,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  1.1,\r\n                        \"compensation\":  0.1,\r\n                        \"durationMs\":  1435,\r\n                        \"range\":  59,\r\n                        \"descentAvgSpeed\":  0.5,\r\n                        \"ascentAvgSpeed\":  0.7,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775178600000,\r\n                        \"score\":  83.4,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  0.6,\r\n                        \"compensation\":  0.3,\r\n                        \"durationMs\":  1478,\r\n                        \"range\":  63,\r\n                        \"descentAvgSpeed\":  0.7,\r\n                        \"ascentAvgSpeed\":  1.2,\r\n                        \"oscillations\":  2\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775184300000,\r\n                        \"score\":  91,\r\n                        \"shakiness\":  2.3,\r\n                        \"uncontrolledDescent\":  0.2,\r\n                        \"compensation\":  0.5,\r\n                        \"durationMs\":  1521,\r\n                        \"range\":  67,\r\n                        \"descentAvgSpeed\":  1,\r\n                        \"ascentAvgSpeed\":  0.6,\r\n                        \"oscillations\":  1\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775190000000,\r\n                        \"score\":  59.6,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  0.8,\r\n                        \"compensation\":  0.6,\r\n                        \"durationMs\":  1564,\r\n                        \"range\":  71,\r\n                        \"descentAvgSpeed\":  1.2,\r\n                        \"ascentAvgSpeed\":  1,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775195700000,\r\n                        \"score\":  67.2,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  0.3,\r\n                        \"compensation\":  0.8,\r\n                        \"durationMs\":  987,\r\n                        \"range\":  75,\r\n                        \"descentAvgSpeed\":  1.5,\r\n                        \"ascentAvgSpeed\":  1.5,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775201400000,\r\n                        \"score\":  71.8,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  0.9,\r\n                        \"compensation\":  0.1,\r\n                        \"durationMs\":  1030,\r\n                        \"range\":  79,\r\n                        \"descentAvgSpeed\":  0.5,\r\n                        \"ascentAvgSpeed\":  0.9,\r\n                        \"oscillations\":  2\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775207100000,\r\n                        \"score\":  79.4,\r\n                        \"shakiness\":  2.3,\r\n                        \"uncontrolledDescent\":  0.4,\r\n                        \"compensation\":  0.3,\r\n                        \"durationMs\":  1073,\r\n                        \"range\":  52,\r\n                        \"descentAvgSpeed\":  0.7,\r\n                        \"ascentAvgSpeed\":  1.4,\r\n                        \"oscillations\":  1\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775212800000,\r\n                        \"score\":  87,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  1,\r\n                        \"compensation\":  0.5,\r\n                        \"durationMs\":  1116,\r\n                        \"range\":  56,\r\n                        \"descentAvgSpeed\":  1,\r\n                        \"ascentAvgSpeed\":  0.8,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775218500000,\r\n                        \"score\":  94.6,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  0.5,\r\n                        \"compensation\":  0.6,\r\n                        \"durationMs\":  1159,\r\n                        \"range\":  60,\r\n                        \"descentAvgSpeed\":  1.2,\r\n                        \"ascentAvgSpeed\":  1.3,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775224200000,\r\n                        \"score\":  63.2,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  1.1,\r\n                        \"compensation\":  0.8,\r\n                        \"durationMs\":  1202,\r\n                        \"range\":  64,\r\n                        \"descentAvgSpeed\":  1.5,\r\n                        \"ascentAvgSpeed\":  0.7,\r\n                        \"oscillations\":  2\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775229900000,\r\n                        \"score\":  67.8,\r\n                        \"shakiness\":  2.3,\r\n                        \"uncontrolledDescent\":  0.6,\r\n                        \"compensation\":  0.1,\r\n                        \"durationMs\":  1245,\r\n                        \"range\":  68,\r\n                        \"descentAvgSpeed\":  0.5,\r\n                        \"ascentAvgSpeed\":  1.2,\r\n                        \"oscillations\":  1\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775235600000,\r\n                        \"score\":  75.4,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  0.2,\r\n                        \"compensation\":  0.3,\r\n                        \"durationMs\":  1288,\r\n                        \"range\":  72,\r\n                        \"descentAvgSpeed\":  0.7,\r\n                        \"ascentAvgSpeed\":  0.6,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775241300000,\r\n                        \"score\":  83,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  0.8,\r\n                        \"compensation\":  0.5,\r\n                        \"durationMs\":  1331,\r\n                        \"range\":  76,\r\n                        \"descentAvgSpeed\":  1,\r\n                        \"ascentAvgSpeed\":  1,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775247000000,\r\n                        \"score\":  90.6,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  0.3,\r\n                        \"compensation\":  0.6,\r\n                        \"durationMs\":  1374,\r\n                        \"range\":  80,\r\n                        \"descentAvgSpeed\":  1.2,\r\n                        \"ascentAvgSpeed\":  1.5,\r\n                        \"oscillations\":  2\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775252700000,\r\n                        \"score\":  59.2,\r\n                        \"shakiness\":  2.3,\r\n                        \"uncontrolledDescent\":  0.9,\r\n                        \"compensation\":  0.8,\r\n                        \"durationMs\":  1417,\r\n                        \"range\":  53,\r\n                        \"descentAvgSpeed\":  1.5,\r\n                        \"ascentAvgSpeed\":  0.9,\r\n                        \"oscillations\":  1\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775258400000,\r\n                        \"score\":  63.8,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  0.4,\r\n                        \"compensation\":  0.1,\r\n                        \"durationMs\":  1460,\r\n                        \"range\":  57,\r\n                        \"descentAvgSpeed\":  0.5,\r\n                        \"ascentAvgSpeed\":  1.4,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775264100000,\r\n                        \"score\":  71.4,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  1,\r\n                        \"compensation\":  0.3,\r\n                        \"durationMs\":  1503,\r\n                        \"range\":  61,\r\n                        \"descentAvgSpeed\":  0.7,\r\n                        \"ascentAvgSpeed\":  0.8,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775269800000,\r\n                        \"score\":  79,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  0.5,\r\n                        \"compensation\":  0.5,\r\n                        \"durationMs\":  1546,\r\n                        \"range\":  65,\r\n                        \"descentAvgSpeed\":  1,\r\n                        \"ascentAvgSpeed\":  1.3,\r\n                        \"oscillations\":  2\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775275500000,\r\n                        \"score\":  86.6,\r\n                        \"shakiness\":  2.3,\r\n                        \"uncontrolledDescent\":  1.1,\r\n                        \"compensation\":  0.6,\r\n                        \"durationMs\":  1589,\r\n                        \"range\":  69,\r\n                        \"descentAvgSpeed\":  1.2,\r\n                        \"ascentAvgSpeed\":  0.7,\r\n                        \"oscillations\":  1\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775281200000,\r\n                        \"score\":  94.2,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  0.6,\r\n                        \"compensation\":  0.8,\r\n                        \"durationMs\":  1012,\r\n                        \"range\":  73,\r\n                        \"descentAvgSpeed\":  1.5,\r\n                        \"ascentAvgSpeed\":  1.2,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775286900000,\r\n                        \"score\":  59.8,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  0.2,\r\n                        \"compensation\":  0.1,\r\n                        \"durationMs\":  1055,\r\n                        \"range\":  77,\r\n                        \"descentAvgSpeed\":  0.5,\r\n                        \"ascentAvgSpeed\":  0.6,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775292600000,\r\n                        \"score\":  67.4,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  0.8,\r\n                        \"compensation\":  0.3,\r\n                        \"durationMs\":  1098,\r\n                        \"range\":  81,\r\n                        \"descentAvgSpeed\":  0.7,\r\n                        \"ascentAvgSpeed\":  1,\r\n                        \"oscillations\":  2\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775298300000,\r\n                        \"score\":  75,\r\n                        \"shakiness\":  2.3,\r\n                        \"uncontrolledDescent\":  0.3,\r\n                        \"compensation\":  0.5,\r\n                        \"durationMs\":  1141,\r\n                        \"range\":  54,\r\n                        \"descentAvgSpeed\":  1,\r\n                        \"ascentAvgSpeed\":  1.5,\r\n                        \"oscillations\":  1\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775304000000,\r\n                        \"score\":  82.6,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  0.9,\r\n                        \"compensation\":  0.6,\r\n                        \"durationMs\":  1184,\r\n                        \"range\":  58,\r\n                        \"descentAvgSpeed\":  1.2,\r\n                        \"ascentAvgSpeed\":  0.9,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775309700000,\r\n                        \"score\":  90.2,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  0.4,\r\n                        \"compensation\":  0.8,\r\n                        \"durationMs\":  1227,\r\n                        \"range\":  62,\r\n                        \"descentAvgSpeed\":  1.5,\r\n                        \"ascentAvgSpeed\":  1.4,\r\n                        \"oscillations\":  3\r\n                    }\r\n                ],\r\n    \"dailyAverages\":  [\r\n                          {\r\n                              \"dayStartMs\":  1775001600000,\r\n                              \"averageScore\":  75.2,\r\n                              \"count\":  11\r\n                          },\r\n                          {\r\n                              \"dayStartMs\":  1775088000000,\r\n                              \"averageScore\":  77.4,\r\n                              \"count\":  15\r\n                          },\r\n                          {\r\n                              \"dayStartMs\":  1775174400000,\r\n                              \"averageScore\":  75.8,\r\n                              \"count\":  15\r\n                          },\r\n                          {\r\n                              \"dayStartMs\":  1775260800000,\r\n                              \"averageScore\":  78.5,\r\n                              \"count\":  9\r\n                          }\r\n                      ]\r\n}\r\n";
static const char g_route_body_1[] = "{\"reading\":3500,\"percentStraight\":67,\"angleDeg\":25.7,\"speed\":0.0,\"inStep\":false,\"stepCount\":1,\"lastScore\":92.0,\"todayAverage\":92.0,\"timeSynced\":true,\"goal\":85,\"shaky\":1.2,\"uncontrolledDescent\":0.4,\"compensation\":0.3}\n";
static const char g_route_body_2[] = "{\n  \"reading\": 3500,\n  \"percentStraight\": 67,\n  \"angleDeg\": 25.7,\n  \"speed\": 0.0,\n  \"inStep\": false,\n  \"timeSynced\": true\n}\n";
static const char g_route_body_3[] = "{\"ok\":true}\n";
static const char g_route_body_4[] = "{\"minReading\":3100,\"maxReading\":3700,\"bentAngle\":78.0,\"straightAngle\":0.0,\"sampleIntervalMs\":50,\"filterAlpha\":0.180,\"motionThreshold\":0.020,\"stepRangeThreshold\":18.0,\"startReadyThreshold\":8.0,\"returnMargin\":5.0,\"maxStepDurationMs\":2600,\"sampleHistoryEnabled\":\"true\",\"status\":\"Variables loaded\",\"goal\":85}\n";
static const char g_route_body_5[] = "{\"reading\":3500,\"percentStraight\":67,\"hasSignal\":true,\"goal\":85,\"todayAverage\":92.0,\"speed\":0.0,\"lastScore\":92.0,\"stepCount\":1,\"timeSynced\":true,\"inStep\":false,\"shaky\":1.2,\"uncontrolledDescent\":0.4,\"compensation\":0.3,\"lowerLegX2\":\"183.4\",\"lowerLegY2\":\"264.1\",\"ankleX\":\"183.4\",\"ankleY\":\"264.1\"}\n";
static const char g_route_body_6[] = "3500\n";
static const char g_route_body_7[] = "<!DOCTYPE html>\r\n<html lang=\"en\">\r\n<head>\r\n  <meta charset=\"utf-8\">\r\n  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\r\n  <title>s-sleeve</title>\r\n  <link rel=\"icon\" type=\"image/svg+xml\" href=\"data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 64 64'%3E%3Cdefs%3E%3CradialGradient id='g' cx='30%25' cy='28%25' r='72%25'%3E%3Cstop offset='0%25' stop-color='%23ffd6a8'/%3E%3Cstop offset='38%25' stop-color='%23ff9b43'/%3E%3Cstop offset='72%25' stop-color='%23d96a1d'/%3E%3Cstop offset='100%25' stop-color='%23993f0d'/%3E%3C/radialGradient%3E%3ClinearGradient id='s' x1='0' y1='0' x2='1' y2='1'%3E%3Cstop offset='0%25' stop-color='rgba(255,255,255,0.9)'/%3E%3Cstop offset='100%25' stop-color='rgba(255,255,255,0)'/%3E%3C/linearGradient%3E%3C/defs%3E%3Crect width='64' height='64' rx='18' fill='%23f6efe8'/%3E%3Ccircle cx='32' cy='32' r='21' fill='url(%23g)'/%3E%3Cellipse cx='25' cy='22' rx='11' ry='7' fill='url(%23s)' opacity='0.9'/%3E%3Ccircle cx='32' cy='32' r='21' fill='none' stroke='rgba(255,255,255,0.45)' stroke-width='1.5'/%3E%3C/svg%3E\">\r\n  <link rel=\"stylesheet\" href=\"/styles.css\">\r\n</head>\r\n<body>\r\n  <div class=\"shell\">\r\n    <section class=\"hero\">\r\n      <div class=\"panel\">\r\n        <p class=\"eyebrow\">s-sleeve</p>\r\n        <h1 class=\"headline\"><span id=\"today-average\">0.0</span>/100</h1>\r\n        <p class=\"subline\">s-sleeve live mobility tracking with a goal of <strong id=\"goal-inline\">85</strong>.</p>\r\n      </div>\r\n      <div class=\"panel\">\r\n        <div class=\"status-row\">\r\n          <div class=\"tabs\" id=\"tabs\">\r\n            <button data-view=\"live\" class=\"active\" type=\"button\">Live</button>\r\n            <button data-view=\"history\" type=\"button\">History</button>\r\n            <button data-view=\"variables\" type=\"button\">Variables</button>\r\n          </div>\r\n          <div id=\"sync-pill\" class=\"status-pill\">Phone time synced for daily tracking</div>\r\n        </div>\r\n      </div>\r\n    </section>\r\n    <section id=\"live-view\" class=\"hidden\"></section>\r\n    <section id=\"history-view\" class=\"hidden\"></section>\r\n    <section id=\"variables-view\">\r\n      <div class=\"panel\">\r\n        <p class=\"eyebrow\">Calibration Variables</p>\r\n        <div id=\"variables-status\" class=\"status-pill warn\">Variables not loaded yet</div>\r\n      </div>\r\n    </section>\r\n  </div>\r\n  <script src=\"/app.js\"></script>\r\n</body>\r\n</html>\r\n";
static const char g_route_body_8[] = "{\"ok\":true,\"status\":\"Variables saved\"}\n";

static const ng_template_node_t g_template_nodes_layouts_main[] = {
  { NG_TEMPLATE_NODE_TEXT, "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n  <meta charset=\"utf-8\">\n  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n  <title>" },
  { NG_TEMPLATE_NODE_EXPR, "pageTitle" },
  { NG_TEMPLATE_NODE_TEXT, "</title>\n  <style>\n    :root {\n      --bg: #f4efe8;\n      --panel: rgba(255, 250, 244, 0.94);\n      --line: rgba(57, 39, 25, 0.12);\n      --text: #251912;\n      --muted: #6f5849;\n      --accent: #b75527;\n      --accent-soft: rgba(183, 85, 39, 0.12);\n      --good: #2f7d53;\n      --warn: #b2661f;\n    }\n    * { box-sizing: border-box; }\n    body {\n      margin: 0;\n      min-height: 100vh;\n      background:\n        radial-gradient(circle at top left, rgba(240, 194, 155, 0.85), transparent 26%),\n        radial-" },
  { NG_TEMPLATE_NODE_EXPR, "eyebrow" },
  { NG_TEMPLATE_NODE_TEXT, "</p>\n        <h1>" },
  { NG_TEMPLATE_NODE_EXPR, "pageTitle" },
  { NG_TEMPLATE_NODE_TEXT, "</h1>\n        <p class=\"subtitle\">" },
  { NG_TEMPLATE_NODE_EXPR, "subtitle" },
  { NG_TEMPLATE_NODE_TEXT, "</p>\n      </div>" },
  { NG_TEMPLATE_NODE_INCLUDE, "partials/nav|nav" },
  { NG_TEMPLATE_NODE_TEXT, "</header>\n    <main>" },
  { NG_TEMPLATE_NODE_RAW_EXPR, "body" },
  { NG_TEMPLATE_NODE_TEXT, "</main>" },
  { NG_TEMPLATE_NODE_INCLUDE, "partials/footer" },
  { NG_TEMPLATE_NODE_TEXT, "</div>\n</body>\n</html>" },
};

static const ng_template_node_t g_template_nodes_pages_operations[] = {
  { NG_TEMPLATE_NODE_TEXT, "<section class=\"stack\">\n  <div class=\"hero-card\">" },
  { NG_TEMPLATE_NODE_RAW_EXPR, "summaryMarkup" },
  { NG_TEMPLATE_NODE_TEXT, "</div>\n  <div class=\"grid-3\">" },
  { NG_TEMPLATE_NODE_FOR_OPEN, "column|columns" },
  { NG_TEMPLATE_NODE_TEXT, "<article class=\"list-card\">\n        <h2>" },
  { NG_TEMPLATE_NODE_EXPR, "column.heading" },
  { NG_TEMPLATE_NODE_TEXT, "</h2>\n        <ul class=\"clean\">" },
  { NG_TEMPLATE_NODE_FOR_OPEN, "card|column.cards" },
  { NG_TEMPLATE_NODE_TEXT, "<li>" },
  { NG_TEMPLATE_NODE_EXPR, "card" },
  { NG_TEMPLATE_NODE_TEXT, "</li>" },
  { NG_TEMPLATE_NODE_END, "" },
  { NG_TEMPLATE_NODE_TEXT, "</ul>\n      </article>" },
  { NG_TEMPLATE_NODE_END, "" },
  { NG_TEMPLATE_NODE_TEXT, "</div>\n</section>" },
};

static const ng_template_node_t g_template_nodes_pages_report_detail[] = {
  { NG_TEMPLATE_NODE_TEXT, "<section class=\"stack\">\n  <div class=\"hero-card\">\n    <span class=\"chip\">Report slug:" },
  { NG_TEMPLATE_NODE_EXPR, "reportId" },
  { NG_TEMPLATE_NODE_TEXT, "</span>\n  </div>\n  <div class=\"grid-3\">" },
  { NG_TEMPLATE_NODE_FOR_OPEN, "section|sections" },
  { NG_TEMPLATE_NODE_TEXT, "<article class=\"panel\">\n        <h2>" },
  { NG_TEMPLATE_NODE_EXPR, "section.title" },
  { NG_TEMPLATE_NODE_TEXT, "</h2>\n        <p class=\"muted\">" },
  { NG_TEMPLATE_NODE_EXPR, "section.text" },
  { NG_TEMPLATE_NODE_TEXT, "</p>\n      </article>" },
  { NG_TEMPLATE_NODE_END, "" },
  { NG_TEMPLATE_NODE_TEXT, "</div>\n</section>" },
};

static const ng_template_node_t g_template_nodes_pages_reports[] = {
  { NG_TEMPLATE_NODE_TEXT, "<section class=\"stack\">\n  <div class=\"hero-card\">" },
  { NG_TEMPLATE_NODE_RAW_EXPR, "insightMarkup" },
  { NG_TEMPLATE_NODE_TEXT, "</div>\n  <div class=\"list-card\">\n    <h2>Available reports</h2>\n    <ul class=\"clean\">" },
  { NG_TEMPLATE_NODE_FOR_OPEN, "report|reportRows" },
  { NG_TEMPLATE_NODE_TEXT, "<li>\n          <a href=\"/server/reports/" },
  { NG_TEMPLATE_NODE_EXPR, "report.slug" },
  { NG_TEMPLATE_NODE_TEXT, "?audience=clinical lead\">" },
  { NG_TEMPLATE_NODE_EXPR, "report.title" },
  { NG_TEMPLATE_NODE_TEXT, "</a>\n          <div class=\"muted\">" },
  { NG_TEMPLATE_NODE_EXPR, "report.owner" },
  { NG_TEMPLATE_NODE_TEXT, "</div>\n        </li>" },
  { NG_TEMPLATE_NODE_END, "" },
  { NG_TEMPLATE_NODE_TEXT, "</ul>\n  </div>\n</section>" },
};

static const ng_template_node_t g_template_nodes_pages_server_home[] = {
  { NG_TEMPLATE_NODE_TEXT, "<section class=\"stack\">\n  <div class=\"hero-card\">" },
  { NG_TEMPLATE_NODE_EXPR, "heroMarkup" },
  { NG_TEMPLATE_NODE_TEXT, "" },
  { NG_TEMPLATE_NODE_IF_OPEN, "audience" },
  { NG_TEMPLATE_NODE_TEXT, "<p class=\"subtitle\">Prepared for <span class=\"chip\">" },
  { NG_TEMPLATE_NODE_EXPR, "audience" },
  { NG_TEMPLATE_NODE_TEXT, "</span></p>" },
  { NG_TEMPLATE_NODE_ELSE, "" },
  { NG_TEMPLATE_NODE_TEXT, "<p class=\"subtitle\">Add <span class=\"chip\">?audience=field team</span> to the URL to see query-driven rendering.</p>" },
  { NG_TEMPLATE_NODE_END, "" },
  { NG_TEMPLATE_NODE_TEXT, "</div>" },
  { NG_TEMPLATE_NODE_INCLUDE, "partials/stat-strip|{ value: stats }" },
  { NG_TEMPLATE_NODE_TEXT, "<div class=\"grid-2\">" },
  { NG_TEMPLATE_NODE_FOR_OPEN, "notice|notices" },
  { NG_TEMPLATE_NODE_TEXT, "<article class=\"panel notice" },
  { NG_TEMPLATE_NODE_EXPR, "notice.tone" },
  { NG_TEMPLATE_NODE_TEXT, "\">\n        <h2>" },
  { NG_TEMPLATE_NODE_EXPR, "notice.title" },
  { NG_TEMPLATE_NODE_TEXT, "</h2>\n        <p class=\"muted\">" },
  { NG_TEMPLATE_NODE_EXPR, "notice.detail" },
  { NG_TEMPLATE_NODE_TEXT, "</p>\n      </article>" },
  { NG_TEMPLATE_NODE_END, "" },
  { NG_TEMPLATE_NODE_TEXT, "</div>\n  <div class=\"list-card\">\n    <h2>Quick links</h2>\n    <ul class=\"clean\">" },
  { NG_TEMPLATE_NODE_FOR_OPEN, "item|quickLinks" },
  { NG_TEMPLATE_NODE_TEXT, "<li><a href=\"" },
  { NG_TEMPLATE_NODE_EXPR, "item.href" },
  { NG_TEMPLATE_NODE_TEXT, "\">" },
  { NG_TEMPLATE_NODE_EXPR, "item.label" },
  { NG_TEMPLATE_NODE_TEXT, "</a></li>" },
  { NG_TEMPLATE_NODE_END, "" },
  { NG_TEMPLATE_NODE_TEXT, "</ul>\n  </div>\n</section>" },
};

static const ng_template_node_t g_template_nodes_pages_team_card[] = {
  { NG_TEMPLATE_NODE_TEXT, "<section class=\"stack\">\n  <div class=\"hero-card\">\n    <span class=\"chip\">" },
  { NG_TEMPLATE_NODE_EXPR, "clinician" },
  { NG_TEMPLATE_NODE_TEXT, "</span>\n  </div>\n  <div class=\"list-card\">\n    <h2>Highlights</h2>\n    <ul class=\"clean\">" },
  { NG_TEMPLATE_NODE_FOR_OPEN, "item|highlights" },
  { NG_TEMPLATE_NODE_TEXT, "<li>" },
  { NG_TEMPLATE_NODE_EXPR, "item" },
  { NG_TEMPLATE_NODE_TEXT, "</li>" },
  { NG_TEMPLATE_NODE_END, "" },
  { NG_TEMPLATE_NODE_TEXT, "</ul>\n  </div>\n</section>" },
};

static const ng_template_node_t g_template_nodes_pages_variables[] = {
  { NG_TEMPLATE_NODE_TEXT, "<section class=\"stack\">\n  <div class=\"hero-card\">\n    <p class=\"subtitle\">" },
  { NG_TEMPLATE_NODE_EXPR, "status" },
  { NG_TEMPLATE_NODE_TEXT, "</p>\n    <p class=\"muted\">" },
  { NG_TEMPLATE_NODE_EXPR, "summary" },
  { NG_TEMPLATE_NODE_TEXT, "</p>\n  </div>\n  <div class=\"list-card\">\n    <h2>Checklist</h2>\n    <ul class=\"clean\">" },
  { NG_TEMPLATE_NODE_FOR_OPEN, "entry|checklist" },
  { NG_TEMPLATE_NODE_TEXT, "<li>" },
  { NG_TEMPLATE_NODE_EXPR, "entry" },
  { NG_TEMPLATE_NODE_TEXT, "</li>" },
  { NG_TEMPLATE_NODE_END, "" },
  { NG_TEMPLATE_NODE_TEXT, "</ul>\n  </div>\n</section>" },
};

static const ng_template_node_t g_template_nodes_partials_footer[] = {
  { NG_TEMPLATE_NODE_TEXT, "<footer class=\"footer-note\">\n  Generated by the Windows compiler demo. The Angular SPA remains available at <a href=\"/\">/</a>.\n</footer>" },
};

static const ng_template_node_t g_template_nodes_partials_nav[] = {
  { NG_TEMPLATE_NODE_TEXT, "<nav class=\"server-nav\">" },
  { NG_TEMPLATE_NODE_FOR_OPEN, "link|links" },
  { NG_TEMPLATE_NODE_TEXT, "<a href=\"" },
  { NG_TEMPLATE_NODE_EXPR, "link.href" },
  { NG_TEMPLATE_NODE_TEXT, "\">" },
  { NG_TEMPLATE_NODE_EXPR, "link.label" },
  { NG_TEMPLATE_NODE_TEXT, "</a>" },
  { NG_TEMPLATE_NODE_END, "" },
  { NG_TEMPLATE_NODE_TEXT, "</nav>" },
};

static const ng_template_node_t g_template_nodes_partials_stat_strip[] = {
  { NG_TEMPLATE_NODE_TEXT, "<div class=\"grid-3\">" },
  { NG_TEMPLATE_NODE_FOR_OPEN, "stat|value" },
  { NG_TEMPLATE_NODE_TEXT, "<article class=\"panel\">\n      <div class=\"muted\">" },
  { NG_TEMPLATE_NODE_EXPR, "stat.label" },
  { NG_TEMPLATE_NODE_TEXT, "</div>\n      <div class=\"stat-value\">" },
  { NG_TEMPLATE_NODE_EXPR, "stat.value" },
  { NG_TEMPLATE_NODE_TEXT, "</div>\n      <div class=\"muted\">" },
  { NG_TEMPLATE_NODE_EXPR, "stat.detail" },
  { NG_TEMPLATE_NODE_TEXT, "</div>\n    </article>" },
  { NG_TEMPLATE_NODE_END, "" },
  { NG_TEMPLATE_NODE_TEXT, "</div>" },
};

static const ng_template_def_t g_angular_templates[] = {
  { "layouts/main", "", g_template_nodes_layouts_main, sizeof(g_template_nodes_layouts_main) / sizeof(g_template_nodes_layouts_main[0]) },
  { "pages/operations", "layouts/main", g_template_nodes_pages_operations, sizeof(g_template_nodes_pages_operations) / sizeof(g_template_nodes_pages_operations[0]) },
  { "pages/report-detail", "layouts/main", g_template_nodes_pages_report_detail, sizeof(g_template_nodes_pages_report_detail) / sizeof(g_template_nodes_pages_report_detail[0]) },
  { "pages/reports", "layouts/main", g_template_nodes_pages_reports, sizeof(g_template_nodes_pages_reports) / sizeof(g_template_nodes_pages_reports[0]) },
  { "pages/server-home", "layouts/main", g_template_nodes_pages_server_home, sizeof(g_template_nodes_pages_server_home) / sizeof(g_template_nodes_pages_server_home[0]) },
  { "pages/team-card", "layouts/main", g_template_nodes_pages_team_card, sizeof(g_template_nodes_pages_team_card) / sizeof(g_template_nodes_pages_team_card[0]) },
  { "pages/variables", "layouts/main", g_template_nodes_pages_variables, sizeof(g_template_nodes_pages_variables) / sizeof(g_template_nodes_pages_variables[0]) },
  { "partials/footer", "", g_template_nodes_partials_footer, sizeof(g_template_nodes_partials_footer) / sizeof(g_template_nodes_partials_footer[0]) },
  { "partials/nav", "", g_template_nodes_partials_nav, sizeof(g_template_nodes_partials_nav) / sizeof(g_template_nodes_partials_nav[0]) },
  { "partials/stat-strip", "", g_template_nodes_partials_stat_strip, sizeof(g_template_nodes_partials_stat_strip) / sizeof(g_template_nodes_partials_stat_strip[0]) },
};
static const size_t g_angular_template_count = sizeof(g_angular_templates) / sizeof(g_angular_templates[0]);

static json_data *angular_server_model_from_expr(const char *expr,
                                                const ng_http_request_t *request,
                                                const ng_server_binding_t *bindings,
                                                size_t binding_count) {
  json_data *model = ng_server_eval_expr(expr, request, bindings, binding_count, NULL, NULL);
  if (model == NULL) {
    return init_json_object();
  }
  if (model->type != JSON_OBJECT) {
    json_data *wrapped = init_json_object();
    if (wrapped != NULL) {
      json_object_add(wrapped, "value", model);
      return wrapped;
    }
    json_free(model);
    return init_json_object();
  }
  return model;
}

static const ng_server_binding_t g_route_0_bindings[] = {
  { "nav", "{\n    links: [\n      { href: '/', label: 'Angular shell' },\n      { href: '/server', label: 'Care hub' },\n      { href: '/server/operations', label: 'Operations' },\n      { href: '/server/reports', label: 'Reports' },\n      { href: '/server/variables', label: 'Variables' }\n    ]\n  }" },
  { "dashboard", "{\n    pageTitle: 'Hybrid Care Hub',\n    eyebrow: 'Angular + EJS',\n    subtitle: 'Server-rendered launch surface for clinicians, coordinators, and field teams.',\n    audience: title(query('audience')),\n    heroMarkup: '<span class=\"hero-badge\">Compiled layout, partials, loops, and helpers</span>',\n    nav: nav,\n    stats: [\n      { label: 'Live wards', value: 4, detail: 'Clinician stations synced' },\n      { label: 'Queued launches', value: 3, detail: 'Next sleeve dispatch wave' },\n      { label: 'Open reports', value: 6, detail: 'Review surface ready' }\n    ],\n    notices: [\n      { tone: 'good', title: 'Shift stable', detail: 'Field staffing is green for the next block.' },\n      { tone: 'warn', title: 'Battery watch', detail: 'Two mobile kits should be topped up before handoff.' }\n    ],\n    quickLinks: [\n      { href: '/server/reports/daily-brief', label: 'Open daily brief' },\n      { href: '/server/team/amelia-ross', label: 'View clinician card' }\n    ]\n  }" },
};

static const ng_server_binding_t g_route_1_bindings[] = {
  { "nav", "{\n    links: [\n      { href: '/', label: 'Angular shell' },\n      { href: '/server', label: 'Care hub' },\n      { href: '/server/reports', label: 'Reports' }\n    ]\n  }" },
  { "board", "{\n    pageTitle: 'Operations Board',\n    eyebrow: 'Ops stream',\n    subtitle: 'Shift handoff snapshot for deployments, stock, and coaching readiness.',\n    nav: nav,\n    columns: [\n      { heading: 'Ready', cards: ['North ward kit staged', 'Fit coach on site', 'Telehealth slot reserved'] },\n      { heading: 'Watch', cards: ['One tablet offline', 'Strap stock below threshold'] },\n      { heading: 'Next', cards: ['Afternoon outreach batch', 'Print briefing export'] }\n    ],\n    summaryMarkup: '<strong>Queue ready:</strong> seven sleeve kits staged for dispatch.'\n  }" },
};

static const ng_server_binding_t g_route_2_bindings[] = {
  { "nav", "{\n    links: [\n      { href: '/', label: 'Angular shell' },\n      { href: '/server', label: 'Care hub' },\n      { href: '/server/operations', label: 'Operations' }\n    ]\n  }" },
  { "reports", "{\n    pageTitle: 'Reporting Room',\n    eyebrow: 'Review surface',\n    subtitle: 'Daily review surface for outcomes, compliance, and trend notes.',\n    nav: nav,\n    reportRows: [\n      { slug: 'daily-brief', title: 'Daily brief', owner: 'Shift desk' },\n      { slug: 'mobility-trend', title: 'Mobility trend', owner: 'Clinical analytics' },\n      { slug: 'fit-followup', title: 'Fit follow-up', owner: 'Field operations' }\n    ],\n    insightMarkup: '<em>Trend note:</em> step quality improved after the last fit adjustment block.'\n  }" },
};

static const ng_server_binding_t g_route_3_bindings[] = {
  { "detail", "{\n    pageTitle: upper(param('reportId')),\n    eyebrow: 'Route params',\n    subtitle: 'Detail view driven by a route parameter and reused layout.',\n    reportId: param('reportId'),\n    sections: [\n      { title: 'Summary', text: 'This report is being rendered from a param-driven route.' },\n      { title: 'Audience', text: title(query('audience')) },\n      { title: 'Source', text: header('User-Agent') }\n    ]\n  }" },
};

static const ng_server_binding_t g_route_4_bindings[] = {
  { "card", "{\n    pageTitle: title(param('clinician')),\n    eyebrow: 'Clinician card',\n    subtitle: 'Simple profile surface backed by route params and helper functions.',\n    clinician: title(param('clinician')),\n    highlights: [\n      'Onboarding complete',\n      'Telemetry ready',\n      'Print briefing attached'\n    ]\n  }" },
};

static const ng_server_binding_t g_route_5_bindings[] = {
  { "profile", "{\n    pageTitle: 'Variables Console',\n    eyebrow: 'Config mirror',\n    subtitle: 'Use the SPA for live tuning. Use this page for an audit-friendly snapshot.',\n    status: 'Latest thresholds mirrored from the device configuration feed.',\n    summary: 'This page uses the shared server layout and partial navigation.',\n    checklist: [\n      'Pressure profile synced',\n      'Session pacing confirmed',\n      'Escalation contact stored'\n    ]\n  }" },
};

static const ng_server_binding_t g_route_6_bindings[] = {
  { "greeting", "title(query('name'))" },
};

static const ng_server_binding_t g_route_7_bindings[] = {
  { "payload", "{\n    ok: true,\n    queued: 3,\n    mode: body('mode'),\n    priority: body('priority'),\n    requestedBy: header('X-Operator')\n  }" },
};


static const angular_generated_route_t g_generated_routes[ANGULAR_STATIC_ROUTE_COUNT > 0 ? ANGULAR_STATIC_ROUTE_COUNT : 1] = {
  { "GET", "/api/history", "application/json", g_route_body_0 },
  { "GET", "/api/live", "application/json", g_route_body_1 },
  { "GET", "/api/motion", "application/json", g_route_body_2 },
  { "GET", "/api/time-sync", "application/json", g_route_body_3 },
  { "GET", "/api/variables", "application/json", g_route_body_4 },
  { "GET", "/state", "application/json", g_route_body_5 },
  { "GET", "/value", "text/plain; charset=utf-8", g_route_body_6 },
  { "GET", "/variables", "text/html; charset=utf-8", g_route_body_7 },
  { "POST", "/api/variables", "application/json", g_route_body_8 },
};

static void angular_http_log(const char *message) {
  fprintf(stdout, "[angular_http] %s\n", message);
  fflush(stdout);
}

static void angular_http_write_error_json(ng_http_response_t *response, const char *message) {
  json_data *root = init_json_object();
  char *json_text = NULL;
  if (root != NULL) {
    json_object_add_string(root, "error", message != NULL ? message : "unknown error");
    json_text = json_tostring(root);
    json_free(root);
  }
  ng_http_response_set_text(response, json_text != NULL ? json_text : "{}");
  free(json_text);
}

static int angular_http_write_generated_route(void *context,
                                             const ng_http_request_t *request,
                                             ng_http_response_t *response) {
  const angular_generated_route_t *route = (const angular_generated_route_t *)context;
  (void)request;
  if (route == NULL) {
    response->status_code = 500;
    angular_http_write_error_json(response, "missing generated route context");
    return 0;
  }
  angular_http_log(route->path);
  snprintf(response->content_type, sizeof(response->content_type), "%s", route->content_type);
  if (ng_http_response_set_text(response, route->body) != 0) {
    response->status_code = 500;
    angular_http_write_error_json(response, "route body too large");
  }
  return 0;
}

static int angular_backend_route_0(void *context,
                                  const ng_http_request_t *request,
                                  ng_http_response_t *response) {
  const ng_server_binding_t *bindings = g_route_0_bindings;
  const size_t binding_count = 2;
  const int status_code = 200;
  (void)context;
  const char *model_expr = "dashboard";
  const char *template_name = "pages/server-home";
  json_data *model = NULL;
  response->status_code = status_code;
  model = angular_server_model_from_expr(model_expr, request, bindings, binding_count);
  if (ng_server_render_template_response(g_angular_templates,
                                         g_angular_template_count,
                                         template_name,
                                         model,
                                         request,
                                         response) != 0) {
    json_free(model);
    response->status_code = 500;
    angular_http_write_error_json(response, "template render failed");
    return 0;
  }
  json_free(model);
  return 0;
}

static int angular_backend_route_1(void *context,
                                  const ng_http_request_t *request,
                                  ng_http_response_t *response) {
  const ng_server_binding_t *bindings = g_route_1_bindings;
  const size_t binding_count = 2;
  const int status_code = 200;
  (void)context;
  const char *model_expr = "board";
  const char *template_name = "pages/operations";
  json_data *model = NULL;
  response->status_code = status_code;
  model = angular_server_model_from_expr(model_expr, request, bindings, binding_count);
  if (ng_server_render_template_response(g_angular_templates,
                                         g_angular_template_count,
                                         template_name,
                                         model,
                                         request,
                                         response) != 0) {
    json_free(model);
    response->status_code = 500;
    angular_http_write_error_json(response, "template render failed");
    return 0;
  }
  json_free(model);
  return 0;
}

static int angular_backend_route_2(void *context,
                                  const ng_http_request_t *request,
                                  ng_http_response_t *response) {
  const ng_server_binding_t *bindings = g_route_2_bindings;
  const size_t binding_count = 2;
  const int status_code = 200;
  (void)context;
  const char *model_expr = "reports";
  const char *template_name = "pages/reports";
  json_data *model = NULL;
  response->status_code = status_code;
  model = angular_server_model_from_expr(model_expr, request, bindings, binding_count);
  if (ng_server_render_template_response(g_angular_templates,
                                         g_angular_template_count,
                                         template_name,
                                         model,
                                         request,
                                         response) != 0) {
    json_free(model);
    response->status_code = 500;
    angular_http_write_error_json(response, "template render failed");
    return 0;
  }
  json_free(model);
  return 0;
}

static int angular_backend_route_3(void *context,
                                  const ng_http_request_t *request,
                                  ng_http_response_t *response) {
  const ng_server_binding_t *bindings = g_route_3_bindings;
  const size_t binding_count = 1;
  const int status_code = 200;
  (void)context;
  const char *model_expr = "detail";
  const char *template_name = "pages/report-detail";
  json_data *model = NULL;
  response->status_code = status_code;
  model = angular_server_model_from_expr(model_expr, request, bindings, binding_count);
  if (ng_server_render_template_response(g_angular_templates,
                                         g_angular_template_count,
                                         template_name,
                                         model,
                                         request,
                                         response) != 0) {
    json_free(model);
    response->status_code = 500;
    angular_http_write_error_json(response, "template render failed");
    return 0;
  }
  json_free(model);
  return 0;
}

static int angular_backend_route_4(void *context,
                                  const ng_http_request_t *request,
                                  ng_http_response_t *response) {
  const ng_server_binding_t *bindings = g_route_4_bindings;
  const size_t binding_count = 1;
  const int status_code = 200;
  (void)context;
  const char *model_expr = "card";
  const char *template_name = "pages/team-card";
  json_data *model = NULL;
  response->status_code = status_code;
  model = angular_server_model_from_expr(model_expr, request, bindings, binding_count);
  if (ng_server_render_template_response(g_angular_templates,
                                         g_angular_template_count,
                                         template_name,
                                         model,
                                         request,
                                         response) != 0) {
    json_free(model);
    response->status_code = 500;
    angular_http_write_error_json(response, "template render failed");
    return 0;
  }
  json_free(model);
  return 0;
}

static int angular_backend_route_5(void *context,
                                  const ng_http_request_t *request,
                                  ng_http_response_t *response) {
  const ng_server_binding_t *bindings = g_route_5_bindings;
  const size_t binding_count = 1;
  const int status_code = 200;
  (void)context;
  const char *model_expr = "profile";
  const char *template_name = "pages/variables";
  json_data *model = NULL;
  response->status_code = status_code;
  model = angular_server_model_from_expr(model_expr, request, bindings, binding_count);
  if (ng_server_render_template_response(g_angular_templates,
                                         g_angular_template_count,
                                         template_name,
                                         model,
                                         request,
                                         response) != 0) {
    json_free(model);
    response->status_code = 500;
    angular_http_write_error_json(response, "template render failed");
    return 0;
  }
  json_free(model);
  return 0;
}

static int angular_backend_route_6(void *context,
                                  const ng_http_request_t *request,
                                  ng_http_response_t *response) {
  const ng_server_binding_t *bindings = g_route_6_bindings;
  const size_t binding_count = 1;
  const int status_code = 200;
  (void)context;
  const char *response_expr = "greeting";
  json_data *value = NULL;
  char *text = NULL;
  response->status_code = status_code;
  snprintf(response->content_type, sizeof(response->content_type), "text/plain; charset=utf-8");
  value = ng_server_eval_expr(response_expr, request, bindings, binding_count, NULL, NULL);
  if (value == NULL) {
    response->status_code = 500;
    angular_http_write_error_json(response, "text evaluation failed");
    return 0;
  }
  text = json_tostring(value);
  if (text != NULL && value->type == JSON_STRING) {
    free(text);
    text = (char *)malloc(strlen(value->as.string.data != NULL ? value->as.string.data : "") + 1u);
    if (text != NULL) {
      strcpy(text, value->as.string.data != NULL ? value->as.string.data : "");
    }
  }
  json_free(value);
  if (text == NULL) {
    response->status_code = 500;
    angular_http_write_error_json(response, "text serialization failed");
    return 0;
  }
  ng_http_response_set_text(response, text);
  free(text);
  return 0;
}

static int angular_backend_route_7(void *context,
                                  const ng_http_request_t *request,
                                  ng_http_response_t *response) {
  const ng_server_binding_t *bindings = g_route_7_bindings;
  const size_t binding_count = 1;
  const int status_code = 202;
  (void)context;
  const char *response_expr = "payload";
  json_data *value = NULL;
  char *text = NULL;
  response->status_code = status_code;
  snprintf(response->content_type, sizeof(response->content_type), "application/json; charset=utf-8");
  value = ng_server_eval_expr(response_expr, request, bindings, binding_count, NULL, NULL);
  if (value == NULL) {
    response->status_code = 500;
    angular_http_write_error_json(response, "json evaluation failed");
    return 0;
  }
  text = json_tostring(value);
  json_free(value);
  if (text == NULL) {
    response->status_code = 500;
    angular_http_write_error_json(response, "json serialization failed");
    return 0;
  }
  ng_http_response_set_text(response, text);
  free(text);
  return 0;
}


void angular_http_service_init(angular_http_service_t *service,
                               ng_runtime_t *runtime,
                               const char *html_page,
                               const char *css_text,
                               const char *js_text) {
  size_t index;
  service->runtime = runtime;
  for (index = 0; index < ANGULAR_STATIC_ROUTE_COUNT; ++index) {
    service->generated_routes[index] = g_generated_routes[index];
  }
  service->routes[0].method = service->generated_routes[0].method;
  service->routes[0].path = service->generated_routes[0].path;
  service->routes[0].handler = angular_http_write_generated_route;
  service->routes[0].context = &service->generated_routes[0];
  service->routes[1].method = service->generated_routes[1].method;
  service->routes[1].path = service->generated_routes[1].path;
  service->routes[1].handler = angular_http_write_generated_route;
  service->routes[1].context = &service->generated_routes[1];
  service->routes[2].method = service->generated_routes[2].method;
  service->routes[2].path = service->generated_routes[2].path;
  service->routes[2].handler = angular_http_write_generated_route;
  service->routes[2].context = &service->generated_routes[2];
  service->routes[3].method = service->generated_routes[3].method;
  service->routes[3].path = service->generated_routes[3].path;
  service->routes[3].handler = angular_http_write_generated_route;
  service->routes[3].context = &service->generated_routes[3];
  service->routes[4].method = service->generated_routes[4].method;
  service->routes[4].path = service->generated_routes[4].path;
  service->routes[4].handler = angular_http_write_generated_route;
  service->routes[4].context = &service->generated_routes[4];
  service->routes[5].method = service->generated_routes[5].method;
  service->routes[5].path = service->generated_routes[5].path;
  service->routes[5].handler = angular_http_write_generated_route;
  service->routes[5].context = &service->generated_routes[5];
  service->routes[6].method = service->generated_routes[6].method;
  service->routes[6].path = service->generated_routes[6].path;
  service->routes[6].handler = angular_http_write_generated_route;
  service->routes[6].context = &service->generated_routes[6];
  service->routes[7].method = service->generated_routes[7].method;
  service->routes[7].path = service->generated_routes[7].path;
  service->routes[7].handler = angular_http_write_generated_route;
  service->routes[7].context = &service->generated_routes[7];
  service->routes[8].method = service->generated_routes[8].method;
  service->routes[8].path = service->generated_routes[8].path;
  service->routes[8].handler = angular_http_write_generated_route;
  service->routes[8].context = &service->generated_routes[8];
  service->routes[9].method = "GET";
  service->routes[9].path = "/server";
  service->routes[9].handler = angular_backend_route_0;
  service->routes[9].context = service;
  service->routes[10].method = "GET";
  service->routes[10].path = "/server/operations";
  service->routes[10].handler = angular_backend_route_1;
  service->routes[10].context = service;
  service->routes[11].method = "GET";
  service->routes[11].path = "/server/reports";
  service->routes[11].handler = angular_backend_route_2;
  service->routes[11].context = service;
  service->routes[12].method = "GET";
  service->routes[12].path = "/server/reports/:reportId";
  service->routes[12].handler = angular_backend_route_3;
  service->routes[12].context = service;
  service->routes[13].method = "GET";
  service->routes[13].path = "/server/team/:clinician";
  service->routes[13].handler = angular_backend_route_4;
  service->routes[13].context = service;
  service->routes[14].method = "GET";
  service->routes[14].path = "/server/variables";
  service->routes[14].handler = angular_backend_route_5;
  service->routes[14].context = service;
  service->routes[15].method = "GET";
  service->routes[15].path = "/server/welcome.txt";
  service->routes[15].handler = angular_backend_route_6;
  service->routes[15].context = service;
  service->routes[16].method = "POST";
  service->routes[16].path = "/server/api/launch";
  service->routes[16].handler = angular_backend_route_7;
  service->routes[16].context = service;
  ng_http_service_init(&service->service, html_page, css_text, js_text, service->routes, ANGULAR_GENERATED_ROUTE_COUNT);
}
