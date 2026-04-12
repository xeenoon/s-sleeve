#include "angular_http_service.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "helpers/include/data/json.h"

#include "helpers/include/support/stringbuilder.h"

static const char g_route_body_0[] = "{\r\n    \"goal\":  85,\r\n    \"history\":  [\r\n                    {\r\n                        \"timestampMs\":  1775030400000,\r\n                        \"score\":  56.8,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  0.2,\r\n                        \"compensation\":  0.1,\r\n                        \"durationMs\":  980,\r\n                        \"range\":  52,\r\n                        \"descentAvgSpeed\":  0.5,\r\n                        \"ascentAvgSpeed\":  0.6,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775036100000,\r\n                        \"score\":  64.4,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  0.8,\r\n                        \"compensation\":  0.3,\r\n                        \"durationMs\":  1023,\r\n                        \"range\":  56,\r\n                        \"descentAvgSpeed\":  0.7,\r\n                        \"ascentAvgSpeed\":  1,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775041800000,\r\n                        \"score\":  72,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  0.3,\r\n                        \"compensation\":  0.5,\r\n                        \"durationMs\":  1066,\r\n                        \"range\":  60,\r\n                        \"descentAvgSpeed\":  1,\r\n                        \"ascentAvgSpeed\":  1.5,\r\n                        \"oscillations\":  2\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775047500000,\r\n                        \"score\":  79.6,\r\n                        \"shakiness\":  2.3,\r\n                        \"uncontrolledDescent\":  0.9,\r\n                        \"compensation\":  0.6,\r\n                        \"durationMs\":  1109,\r\n                        \"range\":  64,\r\n                        \"descentAvgSpeed\":  1.2,\r\n                        \"ascentAvgSpeed\":  0.9,\r\n                        \"oscillations\":  1\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775053200000,\r\n                        \"score\":  87.2,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  0.4,\r\n                        \"compensation\":  0.8,\r\n                        \"durationMs\":  1152,\r\n                        \"range\":  68,\r\n                        \"descentAvgSpeed\":  1.5,\r\n                        \"ascentAvgSpeed\":  1.4,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775058900000,\r\n                        \"score\":  91.8,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  1,\r\n                        \"compensation\":  0.1,\r\n                        \"durationMs\":  1195,\r\n                        \"range\":  72,\r\n                        \"descentAvgSpeed\":  0.5,\r\n                        \"ascentAvgSpeed\":  0.8,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775064600000,\r\n                        \"score\":  60.4,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  0.5,\r\n                        \"compensation\":  0.3,\r\n                        \"durationMs\":  1238,\r\n                        \"range\":  76,\r\n                        \"descentAvgSpeed\":  0.7,\r\n                        \"ascentAvgSpeed\":  1.3,\r\n                        \"oscillations\":  2\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775070300000,\r\n                        \"score\":  68,\r\n                        \"shakiness\":  2.3,\r\n                        \"uncontrolledDescent\":  1.1,\r\n                        \"compensation\":  0.5,\r\n                        \"durationMs\":  1281,\r\n                        \"range\":  80,\r\n                        \"descentAvgSpeed\":  1,\r\n                        \"ascentAvgSpeed\":  0.7,\r\n                        \"oscillations\":  1\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775076000000,\r\n                        \"score\":  75.6,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  0.6,\r\n                        \"compensation\":  0.6,\r\n                        \"durationMs\":  1324,\r\n                        \"range\":  53,\r\n                        \"descentAvgSpeed\":  1.2,\r\n                        \"ascentAvgSpeed\":  1.2,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775081700000,\r\n                        \"score\":  83.2,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  0.2,\r\n                        \"compensation\":  0.8,\r\n                        \"durationMs\":  1367,\r\n                        \"range\":  57,\r\n                        \"descentAvgSpeed\":  1.5,\r\n                        \"ascentAvgSpeed\":  0.6,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775087400000,\r\n                        \"score\":  87.8,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  0.8,\r\n                        \"compensation\":  0.1,\r\n                        \"durationMs\":  1410,\r\n                        \"range\":  61,\r\n                        \"descentAvgSpeed\":  0.5,\r\n                        \"ascentAvgSpeed\":  1,\r\n                        \"oscillations\":  2\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775093100000,\r\n                        \"score\":  95.4,\r\n                        \"shakiness\":  2.3,\r\n                        \"uncontrolledDescent\":  0.3,\r\n                        \"compensation\":  0.3,\r\n                        \"durationMs\":  1453,\r\n                        \"range\":  65,\r\n                        \"descentAvgSpeed\":  0.7,\r\n                        \"ascentAvgSpeed\":  1.5,\r\n                        \"oscillations\":  1\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775098800000,\r\n                        \"score\":  64,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  0.9,\r\n                        \"compensation\":  0.5,\r\n                        \"durationMs\":  1496,\r\n                        \"range\":  69,\r\n                        \"descentAvgSpeed\":  1,\r\n                        \"ascentAvgSpeed\":  0.9,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775104500000,\r\n                        \"score\":  71.6,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  0.4,\r\n                        \"compensation\":  0.6,\r\n                        \"durationMs\":  1539,\r\n                        \"range\":  73,\r\n                        \"descentAvgSpeed\":  1.2,\r\n                        \"ascentAvgSpeed\":  1.4,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775110200000,\r\n                        \"score\":  79.2,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  1,\r\n                        \"compensation\":  0.8,\r\n                        \"durationMs\":  1582,\r\n                        \"range\":  77,\r\n                        \"descentAvgSpeed\":  1.5,\r\n                        \"ascentAvgSpeed\":  0.8,\r\n                        \"oscillations\":  2\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775115900000,\r\n                        \"score\":  83.8,\r\n                        \"shakiness\":  2.3,\r\n                        \"uncontrolledDescent\":  0.5,\r\n                        \"compensation\":  0.1,\r\n                        \"durationMs\":  1005,\r\n                        \"range\":  81,\r\n                        \"descentAvgSpeed\":  0.5,\r\n                        \"ascentAvgSpeed\":  1.3,\r\n                        \"oscillations\":  1\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775121600000,\r\n                        \"score\":  91.4,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  1.1,\r\n                        \"compensation\":  0.3,\r\n                        \"durationMs\":  1048,\r\n                        \"range\":  54,\r\n                        \"descentAvgSpeed\":  0.7,\r\n                        \"ascentAvgSpeed\":  0.7,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775127300000,\r\n                        \"score\":  60,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  0.6,\r\n                        \"compensation\":  0.5,\r\n                        \"durationMs\":  1091,\r\n                        \"range\":  58,\r\n                        \"descentAvgSpeed\":  1,\r\n                        \"ascentAvgSpeed\":  1.2,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775133000000,\r\n                        \"score\":  67.6,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  0.2,\r\n                        \"compensation\":  0.6,\r\n                        \"durationMs\":  1134,\r\n                        \"range\":  62,\r\n                        \"descentAvgSpeed\":  1.2,\r\n                        \"ascentAvgSpeed\":  0.6,\r\n                        \"oscillations\":  2\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775138700000,\r\n                        \"score\":  75.2,\r\n                        \"shakiness\":  2.3,\r\n                        \"uncontrolledDescent\":  0.8,\r\n                        \"compensation\":  0.8,\r\n                        \"durationMs\":  1177,\r\n                        \"range\":  66,\r\n                        \"descentAvgSpeed\":  1.5,\r\n                        \"ascentAvgSpeed\":  1,\r\n                        \"oscillations\":  1\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775144400000,\r\n                        \"score\":  79.8,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  0.3,\r\n                        \"compensation\":  0.1,\r\n                        \"durationMs\":  1220,\r\n                        \"range\":  70,\r\n                        \"descentAvgSpeed\":  0.5,\r\n                        \"ascentAvgSpeed\":  1.5,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775150100000,\r\n                        \"score\":  87.4,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  0.9,\r\n                        \"compensation\":  0.3,\r\n                        \"durationMs\":  1263,\r\n                        \"range\":  74,\r\n                        \"descentAvgSpeed\":  0.7,\r\n                        \"ascentAvgSpeed\":  0.9,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775155800000,\r\n                        \"score\":  95,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  0.4,\r\n                        \"compensation\":  0.5,\r\n                        \"durationMs\":  1306,\r\n                        \"range\":  78,\r\n                        \"descentAvgSpeed\":  1,\r\n                        \"ascentAvgSpeed\":  1.4,\r\n                        \"oscillations\":  2\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775161500000,\r\n                        \"score\":  63.6,\r\n                        \"shakiness\":  2.3,\r\n                        \"uncontrolledDescent\":  1,\r\n                        \"compensation\":  0.6,\r\n                        \"durationMs\":  1349,\r\n                        \"range\":  82,\r\n                        \"descentAvgSpeed\":  1.2,\r\n                        \"ascentAvgSpeed\":  0.8,\r\n                        \"oscillations\":  1\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775167200000,\r\n                        \"score\":  71.2,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  0.5,\r\n                        \"compensation\":  0.8,\r\n                        \"durationMs\":  1392,\r\n                        \"range\":  55,\r\n                        \"descentAvgSpeed\":  1.5,\r\n                        \"ascentAvgSpeed\":  1.3,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775172900000,\r\n                        \"score\":  75.8,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  1.1,\r\n                        \"compensation\":  0.1,\r\n                        \"durationMs\":  1435,\r\n                        \"range\":  59,\r\n                        \"descentAvgSpeed\":  0.5,\r\n                        \"ascentAvgSpeed\":  0.7,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775178600000,\r\n                        \"score\":  83.4,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  0.6,\r\n                        \"compensation\":  0.3,\r\n                        \"durationMs\":  1478,\r\n                        \"range\":  63,\r\n                        \"descentAvgSpeed\":  0.7,\r\n                        \"ascentAvgSpeed\":  1.2,\r\n                        \"oscillations\":  2\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775184300000,\r\n                        \"score\":  91,\r\n                        \"shakiness\":  2.3,\r\n                        \"uncontrolledDescent\":  0.2,\r\n                        \"compensation\":  0.5,\r\n                        \"durationMs\":  1521,\r\n                        \"range\":  67,\r\n                        \"descentAvgSpeed\":  1,\r\n                        \"ascentAvgSpeed\":  0.6,\r\n                        \"oscillations\":  1\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775190000000,\r\n                        \"score\":  59.6,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  0.8,\r\n                        \"compensation\":  0.6,\r\n                        \"durationMs\":  1564,\r\n                        \"range\":  71,\r\n                        \"descentAvgSpeed\":  1.2,\r\n                        \"ascentAvgSpeed\":  1,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775195700000,\r\n                        \"score\":  67.2,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  0.3,\r\n                        \"compensation\":  0.8,\r\n                        \"durationMs\":  987,\r\n                        \"range\":  75,\r\n                        \"descentAvgSpeed\":  1.5,\r\n                        \"ascentAvgSpeed\":  1.5,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775201400000,\r\n                        \"score\":  71.8,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  0.9,\r\n                        \"compensation\":  0.1,\r\n                        \"durationMs\":  1030,\r\n                        \"range\":  79,\r\n                        \"descentAvgSpeed\":  0.5,\r\n                        \"ascentAvgSpeed\":  0.9,\r\n                        \"oscillations\":  2\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775207100000,\r\n                        \"score\":  79.4,\r\n                        \"shakiness\":  2.3,\r\n                        \"uncontrolledDescent\":  0.4,\r\n                        \"compensation\":  0.3,\r\n                        \"durationMs\":  1073,\r\n                        \"range\":  52,\r\n                        \"descentAvgSpeed\":  0.7,\r\n                        \"ascentAvgSpeed\":  1.4,\r\n                        \"oscillations\":  1\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775212800000,\r\n                        \"score\":  87,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  1,\r\n                        \"compensation\":  0.5,\r\n                        \"durationMs\":  1116,\r\n                        \"range\":  56,\r\n                        \"descentAvgSpeed\":  1,\r\n                        \"ascentAvgSpeed\":  0.8,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775218500000,\r\n                        \"score\":  94.6,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  0.5,\r\n                        \"compensation\":  0.6,\r\n                        \"durationMs\":  1159,\r\n                        \"range\":  60,\r\n                        \"descentAvgSpeed\":  1.2,\r\n                        \"ascentAvgSpeed\":  1.3,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775224200000,\r\n                        \"score\":  63.2,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  1.1,\r\n                        \"compensation\":  0.8,\r\n                        \"durationMs\":  1202,\r\n                        \"range\":  64,\r\n                        \"descentAvgSpeed\":  1.5,\r\n                        \"ascentAvgSpeed\":  0.7,\r\n                        \"oscillations\":  2\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775229900000,\r\n                        \"score\":  67.8,\r\n                        \"shakiness\":  2.3,\r\n                        \"uncontrolledDescent\":  0.6,\r\n                        \"compensation\":  0.1,\r\n                        \"durationMs\":  1245,\r\n                        \"range\":  68,\r\n                        \"descentAvgSpeed\":  0.5,\r\n                        \"ascentAvgSpeed\":  1.2,\r\n                        \"oscillations\":  1\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775235600000,\r\n                        \"score\":  75.4,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  0.2,\r\n                        \"compensation\":  0.3,\r\n                        \"durationMs\":  1288,\r\n                        \"range\":  72,\r\n                        \"descentAvgSpeed\":  0.7,\r\n                        \"ascentAvgSpeed\":  0.6,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775241300000,\r\n                        \"score\":  83,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  0.8,\r\n                        \"compensation\":  0.5,\r\n                        \"durationMs\":  1331,\r\n                        \"range\":  76,\r\n                        \"descentAvgSpeed\":  1,\r\n                        \"ascentAvgSpeed\":  1,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775247000000,\r\n                        \"score\":  90.6,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  0.3,\r\n                        \"compensation\":  0.6,\r\n                        \"durationMs\":  1374,\r\n                        \"range\":  80,\r\n                        \"descentAvgSpeed\":  1.2,\r\n                        \"ascentAvgSpeed\":  1.5,\r\n                        \"oscillations\":  2\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775252700000,\r\n                        \"score\":  59.2,\r\n                        \"shakiness\":  2.3,\r\n                        \"uncontrolledDescent\":  0.9,\r\n                        \"compensation\":  0.8,\r\n                        \"durationMs\":  1417,\r\n                        \"range\":  53,\r\n                        \"descentAvgSpeed\":  1.5,\r\n                        \"ascentAvgSpeed\":  0.9,\r\n                        \"oscillations\":  1\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775258400000,\r\n                        \"score\":  63.8,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  0.4,\r\n                        \"compensation\":  0.1,\r\n                        \"durationMs\":  1460,\r\n                        \"range\":  57,\r\n                        \"descentAvgSpeed\":  0.5,\r\n                        \"ascentAvgSpeed\":  1.4,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775264100000,\r\n                        \"score\":  71.4,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  1,\r\n                        \"compensation\":  0.3,\r\n                        \"durationMs\":  1503,\r\n                        \"range\":  61,\r\n                        \"descentAvgSpeed\":  0.7,\r\n                        \"ascentAvgSpeed\":  0.8,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775269800000,\r\n                        \"score\":  79,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  0.5,\r\n                        \"compensation\":  0.5,\r\n                        \"durationMs\":  1546,\r\n                        \"range\":  65,\r\n                        \"descentAvgSpeed\":  1,\r\n                        \"ascentAvgSpeed\":  1.3,\r\n                        \"oscillations\":  2\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775275500000,\r\n                        \"score\":  86.6,\r\n                        \"shakiness\":  2.3,\r\n                        \"uncontrolledDescent\":  1.1,\r\n                        \"compensation\":  0.6,\r\n                        \"durationMs\":  1589,\r\n                        \"range\":  69,\r\n                        \"descentAvgSpeed\":  1.2,\r\n                        \"ascentAvgSpeed\":  0.7,\r\n                        \"oscillations\":  1\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775281200000,\r\n                        \"score\":  94.2,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  0.6,\r\n                        \"compensation\":  0.8,\r\n                        \"durationMs\":  1012,\r\n                        \"range\":  73,\r\n                        \"descentAvgSpeed\":  1.5,\r\n                        \"ascentAvgSpeed\":  1.2,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775286900000,\r\n                        \"score\":  59.8,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  0.2,\r\n                        \"compensation\":  0.1,\r\n                        \"durationMs\":  1055,\r\n                        \"range\":  77,\r\n                        \"descentAvgSpeed\":  0.5,\r\n                        \"ascentAvgSpeed\":  0.6,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775292600000,\r\n                        \"score\":  67.4,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  0.8,\r\n                        \"compensation\":  0.3,\r\n                        \"durationMs\":  1098,\r\n                        \"range\":  81,\r\n                        \"descentAvgSpeed\":  0.7,\r\n                        \"ascentAvgSpeed\":  1,\r\n                        \"oscillations\":  2\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775298300000,\r\n                        \"score\":  75,\r\n                        \"shakiness\":  2.3,\r\n                        \"uncontrolledDescent\":  0.3,\r\n                        \"compensation\":  0.5,\r\n                        \"durationMs\":  1141,\r\n                        \"range\":  54,\r\n                        \"descentAvgSpeed\":  1,\r\n                        \"ascentAvgSpeed\":  1.5,\r\n                        \"oscillations\":  1\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775304000000,\r\n                        \"score\":  82.6,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  0.9,\r\n                        \"compensation\":  0.6,\r\n                        \"durationMs\":  1184,\r\n                        \"range\":  58,\r\n                        \"descentAvgSpeed\":  1.2,\r\n                        \"ascentAvgSpeed\":  0.9,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775309700000,\r\n                        \"score\":  90.2,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  0.4,\r\n                        \"compensation\":  0.8,\r\n                        \"durationMs\":  1227,\r\n                        \"range\":  62,\r\n                        \"descentAvgSpeed\":  1.5,\r\n                        \"ascentAvgSpeed\":  1.4,\r\n                        \"oscillations\":  3\r\n                    }\r\n                ],\r\n    \"dailyAverages\":  [\r\n                          {\r\n                              \"dayStartMs\":  1775001600000,\r\n                              \"averageScore\":  75.2,\r\n                              \"count\":  11\r\n                          },\r\n                          {\r\n                              \"dayStartMs\":  1775088000000,\r\n                              \"averageScore\":  77.4,\r\n                              \"count\":  15\r\n                          },\r\n                          {\r\n                              \"dayStartMs\":  1775174400000,\r\n                              \"averageScore\":  75.8,\r\n                              \"count\":  15\r\n                          },\r\n                          {\r\n                              \"dayStartMs\":  1775260800000,\r\n                              \"averageScore\":  78.5,\r\n                              \"count\":  9\r\n                          }\r\n                      ]\r\n}\r\n";
static const char g_route_body_1[] = "{\"reading\":3500,\"percentStraight\":67,\"angleDeg\":25.7,\"speed\":0.0,\"inStep\":false,\"stepCount\":1,\"lastScore\":92.0,\"todayAverage\":92.0,\"timeSynced\":true,\"goal\":85,\"shaky\":1.2,\"uncontrolledDescent\":0.4,\"compensation\":0.3}\n";
static const char g_route_body_2[] = "{\n  \"reading\": 3500,\n  \"percentStraight\": 67,\n  \"angleDeg\": 25.7,\n  \"speed\": 0.0,\n  \"inStep\": false,\n  \"timeSynced\": true\n}\n";
static const char g_route_body_3[] = "{\"ok\":true}\n";
static const char g_route_body_4[] = "{\"minReading\":3100,\"maxReading\":3700,\"bentAngle\":78.0,\"straightAngle\":0.0,\"sampleIntervalMs\":50,\"filterAlpha\":0.180,\"motionThreshold\":0.020,\"stepRangeThreshold\":18.0,\"startReadyThreshold\":8.0,\"returnMargin\":5.0,\"maxStepDurationMs\":2600,\"sampleHistoryEnabled\":\"true\",\"status\":\"Variables loaded\",\"goal\":85}\n";
static const char g_route_body_5[] = "{\"reading\":3500,\"percentStraight\":67,\"hasSignal\":true,\"goal\":85,\"todayAverage\":92.0,\"speed\":0.0,\"lastScore\":92.0,\"stepCount\":1,\"timeSynced\":true,\"inStep\":false,\"shaky\":1.2,\"uncontrolledDescent\":0.4,\"compensation\":0.3,\"lowerLegX2\":\"183.4\",\"lowerLegY2\":\"264.1\",\"ankleX\":\"183.4\",\"ankleY\":\"264.1\"}\n";
static const char g_route_body_6[] = "3500\n";
static const char g_route_body_7[] = "<!DOCTYPE html>\r\n<html lang=\"en\">\r\n<head>\r\n  <meta charset=\"utf-8\">\r\n  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\r\n  <title>s-sleeve</title>\r\n  <link rel=\"icon\" type=\"image/svg+xml\" href=\"data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 64 64'%3E%3Cdefs%3E%3CradialGradient id='g' cx='30%25' cy='28%25' r='72%25'%3E%3Cstop offset='0%25' stop-color='%23ffd6a8'/%3E%3Cstop offset='38%25' stop-color='%23ff9b43'/%3E%3Cstop offset='72%25' stop-color='%23d96a1d'/%3E%3Cstop offset='100%25' stop-color='%23993f0d'/%3E%3C/radialGradient%3E%3ClinearGradient id='s' x1='0' y1='0' x2='1' y2='1'%3E%3Cstop offset='0%25' stop-color='rgba(255,255,255,0.9)'/%3E%3Cstop offset='100%25' stop-color='rgba(255,255,255,0)'/%3E%3C/linearGradient%3E%3C/defs%3E%3Crect width='64' height='64' rx='18' fill='%23f6efe8'/%3E%3Ccircle cx='32' cy='32' r='21' fill='url(%23g)'/%3E%3Cellipse cx='25' cy='22' rx='11' ry='7' fill='url(%23s)' opacity='0.9'/%3E%3Ccircle cx='32' cy='32' r='21' fill='none' stroke='rgba(255,255,255,0.45)' stroke-width='1.5'/%3E%3C/svg%3E\">\r\n  <link rel=\"stylesheet\" href=\"/styles.css\">\r\n</head>\r\n<body>\r\n  <div class=\"shell\">\r\n    <section class=\"hero\">\r\n      <div class=\"panel\">\r\n        <p class=\"eyebrow\">s-sleeve</p>\r\n        <h1 class=\"headline\"><span id=\"today-average\">0.0</span>/100</h1>\r\n        <p class=\"subline\">s-sleeve live mobility tracking with a goal of <strong id=\"goal-inline\">85</strong>.</p>\r\n      </div>\r\n      <div class=\"panel\">\r\n        <div class=\"status-row\">\r\n          <div class=\"tabs\" id=\"tabs\">\r\n            <button data-view=\"live\" class=\"active\" type=\"button\">Live</button>\r\n            <button data-view=\"history\" type=\"button\">History</button>\r\n            <button data-view=\"variables\" type=\"button\">Variables</button>\r\n          </div>\r\n          <div id=\"sync-pill\" class=\"status-pill\">Phone time synced for daily tracking</div>\r\n        </div>\r\n      </div>\r\n    </section>\r\n    <section id=\"live-view\" class=\"hidden\"></section>\r\n    <section id=\"history-view\" class=\"hidden\"></section>\r\n    <section id=\"variables-view\">\r\n      <div class=\"panel\">\r\n        <p class=\"eyebrow\">Calibration Variables</p>\r\n        <div id=\"variables-status\" class=\"status-pill warn\">Variables not loaded yet</div>\r\n      </div>\r\n    </section>\r\n  </div>\r\n  <script src=\"/app.js\"></script>\r\n</body>\r\n</html>\r\n";
static const char g_route_body_8[] = "{\"ok\":true,\"status\":\"Variables saved\"}\n";

typedef enum {
  NG_LOCAL_NULL = 0,
  NG_LOCAL_BOOL,
  NG_LOCAL_INT,
  NG_LOCAL_DOUBLE,
  NG_LOCAL_STRING
} ng_local_kind_t;

typedef struct {
  const char *name;
  ng_local_kind_t kind;
  union {
    int int_value;
    double double_value;
    int bool_value;
    const char *string_value;
  } data;
} ng_local_slot_t;

typedef struct {
  ng_local_slot_t slots[64];
  size_t slot_count;
} ng_render_context_t;

typedef int (*angular_template_render_fn_t)(const ng_render_context_t *locals,
                                           ng_http_response_t *response);

static void angular_render_context_init(ng_render_context_t *context) {
  memset(context, 0, sizeof(*context));
}

static int __attribute__((unused)) angular_render_context_add_string(ng_render_context_t *context, const char *name, const char *value) {
  ng_local_slot_t *slot;
  if (context->slot_count >= sizeof(context->slots) / sizeof(context->slots[0])) {
    return 1;
  }
  slot = &context->slots[context->slot_count++];
  slot->name = name;
  slot->kind = NG_LOCAL_STRING;
  slot->data.string_value = value;
  return 0;
}

static int __attribute__((unused)) angular_render_context_add_int(ng_render_context_t *context, const char *name, int value) {
  ng_local_slot_t *slot;
  if (context->slot_count >= sizeof(context->slots) / sizeof(context->slots[0])) {
    return 1;
  }
  slot = &context->slots[context->slot_count++];
  slot->name = name;
  slot->kind = NG_LOCAL_INT;
  slot->data.int_value = value;
  return 0;
}

static int __attribute__((unused)) angular_render_context_add_double(ng_render_context_t *context, const char *name, double value) {
  ng_local_slot_t *slot;
  if (context->slot_count >= sizeof(context->slots) / sizeof(context->slots[0])) {
    return 1;
  }
  slot = &context->slots[context->slot_count++];
  slot->name = name;
  slot->kind = NG_LOCAL_DOUBLE;
  slot->data.double_value = value;
  return 0;
}

static int __attribute__((unused)) angular_render_context_add_bool(ng_render_context_t *context, const char *name, int value) {
  ng_local_slot_t *slot;
  if (context->slot_count >= sizeof(context->slots) / sizeof(context->slots[0])) {
    return 1;
  }
  slot = &context->slots[context->slot_count++];
  slot->name = name;
  slot->kind = NG_LOCAL_BOOL;
  slot->data.bool_value = value;
  return 0;
}

static const ng_local_slot_t *__attribute__((unused)) angular_render_context_find(const ng_render_context_t *context, const char *name) {
  size_t index;
  for (index = 0; index < context->slot_count; ++index) {
    if (context->slots[index].name != NULL && strcmp(context->slots[index].name, name) == 0) {
      return &context->slots[index];
    }
  }
  return NULL;
}

static int __attribute__((unused)) angular_render_context_is_truthy(const ng_render_context_t *context, const char *name) {
  const ng_local_slot_t *slot = angular_render_context_find(context, name);
  if (slot == NULL) {
    return 0;
  }
  switch (slot->kind) {
    case NG_LOCAL_BOOL:
      return slot->data.bool_value != 0;
    case NG_LOCAL_INT:
      return slot->data.int_value != 0;
    case NG_LOCAL_DOUBLE:
      return slot->data.double_value != 0.0;
    case NG_LOCAL_STRING:
      return slot->data.string_value != NULL && slot->data.string_value[0] != '\0';
    case NG_LOCAL_NULL:
    default:
      return 0;
  }
}

static void angular_sb_append_text(stringbuilder *builder, const char *text) {
  append(builder, text != NULL ? text : "");
}

static void __attribute__((unused)) angular_sb_append_html_escaped(stringbuilder *builder, const char *text) {
  size_t index;
  if (text == NULL) {
    return;
  }
  for (index = 0; text[index] != '\0'; ++index) {
    switch (text[index]) {
      case '&': append(builder, "&amp;"); break;
      case '<': append(builder, "&lt;"); break;
      case '>': append(builder, "&gt;"); break;
      case '"': append(builder, "&quot;"); break;
      case '\'': append(builder, "&#39;"); break;
      default: append_byte(builder, text[index]); break;
    }
  }
}

static void __attribute__((unused)) angular_sb_append_slot_value(stringbuilder *builder, const ng_local_slot_t *slot) {
  char number_buffer[64];
  if (slot == NULL) {
    return;
  }
  switch (slot->kind) {
    case NG_LOCAL_STRING:
      angular_sb_append_text(builder, slot->data.string_value);
      return;
    case NG_LOCAL_BOOL:
      angular_sb_append_text(builder, slot->data.bool_value ? "true" : "false");
      return;
    case NG_LOCAL_INT:
      snprintf(number_buffer, sizeof(number_buffer), "%d", slot->data.int_value);
      angular_sb_append_text(builder, number_buffer);
      return;
    case NG_LOCAL_DOUBLE:
      snprintf(number_buffer, sizeof(number_buffer), "%.6g", slot->data.double_value);
      angular_sb_append_text(builder, number_buffer);
      return;
    case NG_LOCAL_NULL:
    default:
      return;
  }
}

static void __attribute__((unused)) angular_sb_append_escaped_slot(stringbuilder *builder, const ng_render_context_t *context, const char *name) {
  const ng_local_slot_t *slot = angular_render_context_find(context, name);
  char number_buffer[64];
  if (slot == NULL) {
    return;
  }
  if (slot->kind == NG_LOCAL_STRING) {
    angular_sb_append_html_escaped(builder, slot->data.string_value);
    return;
  }
  switch (slot->kind) {
    case NG_LOCAL_BOOL:
      angular_sb_append_text(builder, slot->data.bool_value ? "true" : "false");
      return;
    case NG_LOCAL_INT:
      snprintf(number_buffer, sizeof(number_buffer), "%d", slot->data.int_value);
      angular_sb_append_text(builder, number_buffer);
      return;
    case NG_LOCAL_DOUBLE:
      snprintf(number_buffer, sizeof(number_buffer), "%.6g", slot->data.double_value);
      angular_sb_append_text(builder, number_buffer);
      return;
    case NG_LOCAL_NULL:
    case NG_LOCAL_STRING:
    default:
      return;
  }
}

static void __attribute__((unused)) angular_sb_append_raw_slot(stringbuilder *builder, const ng_render_context_t *context, const char *name) {
  angular_sb_append_slot_value(builder, angular_render_context_find(context, name));
}

static int angular_render_operations_template(const ng_render_context_t *locals,
                                        ng_http_response_t *response) {
  stringbuilder *builder = init_builder();
  char *rendered;
  int set_result;
  if (builder == NULL) {
    response->status_code = 500;
    return 0;
  }
  angular_sb_append_text(builder, "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n  <meta charset=\"utf-8\">\n  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n  <title>");
  angular_sb_append_escaped_slot(builder, locals, "title");
  angular_sb_append_text(builder, "</title>\n  <style>\n    :root {\n      --bg: #f1ece4;\n      --paper: rgba(255, 251, 246, 0.95);\n      --line: rgba(64, 46, 33, 0.12);\n      --text: #201711;\n      --muted: #6b584c;\n      --accent: #b45d32;\n      --good: #2f7b55;\n      --warn: #ba6a1f;\n      --shadow: 0 24px 56px rgba(47, 30, 19, 0.12);\n    }\n    * { box-sizing: border-box; }\n    body {\n      margin: 0;\n      font-family: \"Segoe UI\", Tahoma, Geneva, Verdana, sans-serif;\n      color: var(--text);\n      background:\n        radial-gradient(circl");
  angular_sb_append_escaped_slot(builder, locals, "title");
  angular_sb_append_text(builder, "</h1>\n        <p class=\"subtitle\">");
  angular_sb_append_escaped_slot(builder, locals, "subtitle");
  angular_sb_append_text(builder, "</p>\n      </div>\n      <div>\n        <a href=\"/server\">Hub</a>\n        <a href=\"/\">Angular dashboard</a>\n      </div>\n    </section>\n\n    <section class=\"grid\">\n      <article class=\"panel\">\n        <div class=\"kicker\">");
  angular_sb_append_escaped_slot(builder, locals, "panelState");
  angular_sb_append_text(builder, "</div>\n        <div class=\"stack\">\n          <div class=\"tile\">\n            <strong>Deployment wave</strong>\n            Three clinics scheduled before the afternoon calibration block.\n          </div>\n          <div class=\"tile\">\n            <strong>Support roster</strong>\n            One fitter on standby, two therapy reviewers active, one educator preparing onboarding.\n          </div>\n          <div class=\"tile\">\n            <strong>Consumables</strong>\n            Sleeve liners, charging packs, and pr");
  angular_sb_append_raw_slot(builder, locals, "calloutMarkup");
  angular_sb_append_text(builder, "</div>\n      </article>");
  if (angular_render_context_is_truthy(locals, "showBoard")) {
  angular_sb_append_text(builder, "<aside class=\"panel board\">\n        <div class=\"status-row\">\n          <div class=\"status-card\">\n            <span>Ready to fit</span>\n            <strong>07</strong>\n          </div>\n          <div class=\"status-card\">\n            <span>Awaiting review</span>\n            <strong>04</strong>\n          </div>\n          <div class=\"status-card\">\n            <span>Courier handoff</span>\n            <strong>02</strong>\n          </div>\n          <div class=\"status-card\">\n            <span>Escalations</span>\n  ");
  }
  angular_sb_append_text(builder, "</section>\n\n    <div class=\"footer-links\" style=\"margin-top: 18px;\">\n      <a href=\"/server/reports\">Open reports</a>\n      <a href=\"/server/variables\">Open variables briefing</a>\n    </div>\n  </main>\n</body>\n</html>");
  rendered = (char *)malloc((size_t)builder->writtenlen + 1u);
  if (rendered == NULL) {
    free_builder(builder);
    response->status_code = 500;
    return 0;
  }
  memcpy(rendered, builder->data, (size_t)builder->writtenlen);
  rendered[builder->writtenlen] = '\0';
  set_result = ng_http_response_set_text(response, rendered);
  free(rendered);
  free_builder(builder);
  if (set_result != 0) {
    response->status_code = 500;
  }
  return 0;
}

static int angular_render_reports_template(const ng_render_context_t *locals,
                                        ng_http_response_t *response) {
  stringbuilder *builder = init_builder();
  char *rendered;
  int set_result;
  if (builder == NULL) {
    response->status_code = 500;
    return 0;
  }
  angular_sb_append_text(builder, "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n  <meta charset=\"utf-8\">\n  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n  <title>");
  angular_sb_append_escaped_slot(builder, locals, "title");
  angular_sb_append_text(builder, "</title>\n  <style>\n    :root {\n      --bg: #f4eee7;\n      --paper: rgba(255, 251, 246, 0.94);\n      --line: rgba(62, 45, 33, 0.12);\n      --text: #241b14;\n      --muted: #6f5c50;\n      --accent: #af5a31;\n      --good: #317c56;\n      --shadow: 0 24px 54px rgba(45, 28, 18, 0.12);\n    }\n    * { box-sizing: border-box; }\n    body {\n      margin: 0;\n      font-family: \"Segoe UI\", Tahoma, Geneva, Verdana, sans-serif;\n      color: var(--text);\n      background:\n        radial-gradient(circle at top left, rgba(255");
  angular_sb_append_escaped_slot(builder, locals, "title");
  angular_sb_append_text(builder, "</h1>\n        <p class=\"subtitle\">");
  angular_sb_append_escaped_slot(builder, locals, "subtitle");
  angular_sb_append_text(builder, "</p>\n      </div>\n      <div>\n        <a href=\"/server\">Hub</a>\n        <a href=\"/server/operations\">Operations</a>\n        <a href=\"/\">Angular dashboard</a>\n      </div>\n    </section>\n\n    <section class=\"report-grid\">\n      <article class=\"panel\">\n        <div class=\"state\">");
  angular_sb_append_escaped_slot(builder, locals, "reportState");
  angular_sb_append_text(builder, "</div>");
  if (angular_render_context_is_truthy(locals, "showDigest")) {
  angular_sb_append_text(builder, "<div class=\"digest-grid\">\n          <div class=\"digest-card\">\n            <span>Adherence score</span>\n            <strong>94%</strong>\n          </div>\n          <div class=\"digest-card\">\n            <span>Sessions completed</span>\n            <strong>128</strong>\n          </div>\n          <div class=\"digest-card\">\n            <span>Alerts closed</span>\n            <strong>11</strong>\n          </div>\n        </div>");
  }
  angular_sb_append_text(builder, "<div class=\"note\">");
  angular_sb_append_raw_slot(builder, locals, "insightMarkup");
  angular_sb_append_text(builder, "</div>\n      </article>\n\n      <aside class=\"panel timeline\">\n        <div class=\"step\">\n          <strong>07:10</strong>\n          Overnight telemetry rolled into the review bundle and aligned with fit notes.\n        </div>\n        <div class=\"step\">\n          <strong>08:25</strong>\n          Mobility specialists cleared the top priority outreach list for the morning round.\n        </div>\n        <div class=\"step\">\n          <strong>09:40</strong>\n          Reporting page published for supervisors before ");
  rendered = (char *)malloc((size_t)builder->writtenlen + 1u);
  if (rendered == NULL) {
    free_builder(builder);
    response->status_code = 500;
    return 0;
  }
  memcpy(rendered, builder->data, (size_t)builder->writtenlen);
  rendered[builder->writtenlen] = '\0';
  set_result = ng_http_response_set_text(response, rendered);
  free(rendered);
  free_builder(builder);
  if (set_result != 0) {
    response->status_code = 500;
  }
  return 0;
}

static int angular_render_server_home_template(const ng_render_context_t *locals,
                                        ng_http_response_t *response) {
  stringbuilder *builder = init_builder();
  char *rendered;
  int set_result;
  if (builder == NULL) {
    response->status_code = 500;
    return 0;
  }
  angular_sb_append_text(builder, "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n  <meta charset=\"utf-8\">\n  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n  <title>");
  angular_sb_append_escaped_slot(builder, locals, "title");
  angular_sb_append_text(builder, "</title>\n  <style>\n    :root {\n      --bg: #f3ede5;\n      --paper: rgba(255, 250, 244, 0.92);\n      --text: #24170f;\n      --muted: #6c584b;\n      --accent: #b85d2f;\n      --accent-soft: rgba(184, 93, 47, 0.12);\n      --line: rgba(68, 46, 32, 0.12);\n      --good: #2d7c54;\n      --shadow: 0 28px 60px rgba(46, 29, 18, 0.12);\n    }\n    * { box-sizing: border-box; }\n    body {\n      margin: 0;\n      min-height: 100vh;\n      color: var(--text);\n      font-family: \"Segoe UI\", Tahoma, Geneva, Verdana, sans-serif;");
  angular_sb_append_escaped_slot(builder, locals, "title");
  angular_sb_append_text(builder, "</h1>\n        <p class=\"subtitle\">");
  angular_sb_append_escaped_slot(builder, locals, "subtitle");
  angular_sb_append_text(builder, "</p>");
  angular_sb_append_raw_slot(builder, locals, "heroMarkup");
  angular_sb_append_text(builder, "<div class=\"cta-row\" style=\"margin-top: 20px;\">\n          <a class=\"primary\" href=\"/\">Jump into the Angular dashboard</a>\n          <a href=\"/server/reports\">Open morning reporting</a>\n        </div>\n      </article>\n\n      <aside class=\"panel status-card\">\n        <div class=\"status-pill\">\n          <span class=\"status-dot\"></span>\n          <span>");
  angular_sb_append_escaped_slot(builder, locals, "status");
  angular_sb_append_text(builder, "</span>\n        </div>\n        <div class=\"metrics\">\n          <div class=\"metric\">\n            <span>Care pathways</span>\n            <strong>12</strong>\n          </div>\n          <div class=\"metric\">\n            <span>Active monitors</span>\n            <strong>47</strong>\n          </div>\n          <div class=\"metric\">\n            <span>Review queue</span>\n            <strong>5</strong>\n          </div>\n          <div class=\"metric\">\n            <span>Pending kits</span>\n            <strong>7</strong>\n ");
  if (angular_render_context_is_truthy(locals, "showHighlights")) {
  angular_sb_append_text(builder, "<section class=\"highlights\">\n      <article class=\"card\">\n        <h2>Clinician handoff</h2>\n        <p>Use the server side hub to stage briefings, print-ready summaries, and stable low-JS entry pages for outreach teams.</p>\n      </article>\n      <article class=\"card\">\n        <h2>Angular handoff</h2>\n        <p>The live telemetry dashboard remains the rich client surface for motion playback, variables tuning, and observable-driven interactions.</p>\n      </article>\n      <article class=\"card\">\n        <h");
  }
  angular_sb_append_text(builder, "<div class=\"footer-links\">\n      <a href=\"/server/welcome.txt\">Plain text health route</a>\n      <a href=\"/server/api/launch\">POST launch endpoint</a>\n      <a href=\"/server/variables\">Printable variables view</a>\n    </div>\n  </main>\n</body>\n</html>");
  rendered = (char *)malloc((size_t)builder->writtenlen + 1u);
  if (rendered == NULL) {
    free_builder(builder);
    response->status_code = 500;
    return 0;
  }
  memcpy(rendered, builder->data, (size_t)builder->writtenlen);
  rendered[builder->writtenlen] = '\0';
  set_result = ng_http_response_set_text(response, rendered);
  free(rendered);
  free_builder(builder);
  if (set_result != 0) {
    response->status_code = 500;
  }
  return 0;
}

static int angular_render_variables_template(const ng_render_context_t *locals,
                                        ng_http_response_t *response) {
  stringbuilder *builder = init_builder();
  char *rendered;
  int set_result;
  if (builder == NULL) {
    response->status_code = 500;
    return 0;
  }
  angular_sb_append_text(builder, "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n  <meta charset=\"utf-8\">\n  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n  <title>");
  angular_sb_append_escaped_slot(builder, locals, "title");
  angular_sb_append_text(builder, "</title>\n  <style>\n    :root {\n      --bg: #f3eee7;\n      --paper: rgba(255, 250, 245, 0.94);\n      --line: rgba(64, 46, 33, 0.12);\n      --text: #241a13;\n      --muted: #6f5b4e;\n      --accent: #b65d31;\n      --shadow: 0 24px 56px rgba(46, 29, 18, 0.12);\n    }\n    * { box-sizing: border-box; }\n    body {\n      margin: 0;\n      font-family: \"Segoe UI\", Tahoma, Geneva, Verdana, sans-serif;\n      color: var(--text);\n      background:\n        radial-gradient(circle at right top, rgba(182, 93, 49, 0.13), trans");
  angular_sb_append_escaped_slot(builder, locals, "title");
  angular_sb_append_text(builder, "</h1>\n        <p class=\"status\">");
  angular_sb_append_escaped_slot(builder, locals, "status");
  angular_sb_append_text(builder, "</p>\n      </div>\n      <div>\n        <a href=\"/server\">Hub</a>\n        <a href=\"/\">Angular dashboard</a>\n      </div>\n    </section>\n\n    <section class=\"grid\">\n      <article class=\"card\">\n        <span>Goal threshold</span>\n        <strong>85%</strong>\n      </article>\n      <article class=\"card\">\n        <span>Sync cadence</span>\n        <strong>8s</strong>\n      </article>\n      <article class=\"card\">\n        <span>Write endpoint</span>\n        <strong>/api/variables</strong>\n      </article>\n    </se");
  angular_sb_append_raw_slot(builder, locals, "hintMarkup");
  angular_sb_append_text(builder, "<p>");
  angular_sb_append_escaped_slot(builder, locals, "summary");
  angular_sb_append_text(builder, "</p>\n    </div>");
  if (angular_render_context_is_truthy(locals, "showChecklist")) {
  angular_sb_append_text(builder, "<section class=\"checklist\">\n      <article class=\"item\">\n        <strong>Before a fitting session</strong>\n        Confirm the patient target range, brace alignment, and previous session notes.\n      </article>\n      <article class=\"item\">\n        <strong>During live tuning</strong>\n        Hand off to the Angular dashboard for real-time values, motion response, and save actions.\n      </article>\n    </section>");
  }
  angular_sb_append_text(builder, "</main>\n</body>\n</html>");
  rendered = (char *)malloc((size_t)builder->writtenlen + 1u);
  if (rendered == NULL) {
    free_builder(builder);
    response->status_code = 500;
    return 0;
  }
  memcpy(rendered, builder->data, (size_t)builder->writtenlen);
  rendered[builder->writtenlen] = '\0';
  set_result = ng_http_response_set_text(response, rendered);
  free(rendered);
  free_builder(builder);
  if (set_result != 0) {
    response->status_code = 500;
  }
  return 0;
}


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
  (void)context;
  (void)request;
  response->status_code = 200;
  ng_render_context_t locals;
  angular_render_context_init(&locals);
  snprintf(response->content_type, sizeof(response->content_type), "text/html; charset=utf-8");
  angular_render_context_add_string(&locals, "title", "Hybrid Care Hub");
  angular_render_context_add_string(&locals, "subtitle", "Server-rendered launch surface for clinicians, coordinators, and field teams.");
  angular_render_context_add_string(&locals, "status", "Clinical sync stable");
  angular_render_context_add_string(&locals, "heroMarkup", "<span class=\"hero-badge\">EJS route compiled into C</span>");
  angular_render_context_add_bool(&locals, "showHighlights", 1);
  return angular_render_server_home_template(&locals, response);
}

static int angular_backend_route_1(void *context,
                                  const ng_http_request_t *request,
                                  ng_http_response_t *response) {
  (void)context;
  (void)request;
  response->status_code = 200;
  ng_render_context_t locals;
  angular_render_context_init(&locals);
  snprintf(response->content_type, sizeof(response->content_type), "text/html; charset=utf-8");
  angular_render_context_add_string(&locals, "title", "Operations Board");
  angular_render_context_add_string(&locals, "subtitle", "Shift handoff snapshot for deployments, stock, and coaching readiness.");
  angular_render_context_add_string(&locals, "panelState", "Field team green across the current roster.");
  angular_render_context_add_string(&locals, "calloutMarkup", "<strong>Queue ready:</strong> seven sleeve kits staged for dispatch.");
  angular_render_context_add_bool(&locals, "showBoard", 1);
  return angular_render_operations_template(&locals, response);
}

static int angular_backend_route_2(void *context,
                                  const ng_http_request_t *request,
                                  ng_http_response_t *response) {
  (void)context;
  (void)request;
  response->status_code = 200;
  ng_render_context_t locals;
  angular_render_context_init(&locals);
  snprintf(response->content_type, sizeof(response->content_type), "text/html; charset=utf-8");
  angular_render_context_add_string(&locals, "title", "Reporting Room");
  angular_render_context_add_string(&locals, "subtitle", "Daily review surface for outcomes, compliance, and trend notes.");
  angular_render_context_add_string(&locals, "reportState", "Morning digest compiled and ready for review.");
  angular_render_context_add_string(&locals, "insightMarkup", "<em>Trend note:</em> step quality improved after the last fit adjustment block.");
  angular_render_context_add_bool(&locals, "showDigest", 1);
  return angular_render_reports_template(&locals, response);
}

static int angular_backend_route_3(void *context,
                                  const ng_http_request_t *request,
                                  ng_http_response_t *response) {
  (void)context;
  (void)request;
  response->status_code = 200;
  ng_render_context_t locals;
  angular_render_context_init(&locals);
  snprintf(response->content_type, sizeof(response->content_type), "text/html; charset=utf-8");
  angular_render_context_add_string(&locals, "title", "Variables Console");
  angular_render_context_add_string(&locals, "status", "Latest thresholds mirrored from the device configuration feed.");
  angular_render_context_add_string(&locals, "summary", "Use the SPA when you need live tuning. Use this server page when you need a printable briefing view.");
  angular_render_context_add_string(&locals, "hintMarkup", "<span class=\"inline-chip\">Hybrid workflow active</span>");
  angular_render_context_add_bool(&locals, "showChecklist", 1);
  return angular_render_variables_template(&locals, response);
}

static int angular_backend_route_4(void *context,
                                  const ng_http_request_t *request,
                                  ng_http_response_t *response) {
  (void)context;
  (void)request;
  response->status_code = 200;
  snprintf(response->content_type, sizeof(response->content_type), "text/plain; charset=utf-8");
  ng_http_response_set_text(response, "s-sleeve hybrid compiler demo is online");
  return 0;
}

static int angular_backend_route_5(void *context,
                                  const ng_http_request_t *request,
                                  ng_http_response_t *response) {
  (void)context;
  (void)request;
  response->status_code = 202;
  json_data *root = init_json_object();
  char *json_text = NULL;
  if (root == NULL) {
    response->status_code = 500;
    angular_http_write_error_json(response, "json allocation failed");
    return 0;
  }
  json_object_add_boolean(root, "ok", true);
  json_object_add_number(root, "queued", 3);
  json_object_add_string(root, "message", "Launch packet queued");
  json_text = json_tostring(root);
  json_free(root);
  if (json_text == NULL) {
    response->status_code = 500;
    angular_http_write_error_json(response, "json serialization failed");
    return 0;
  }
  snprintf(response->content_type, sizeof(response->content_type), "application/json; charset=utf-8");
  ng_http_response_set_text(response, json_text);
  free(json_text);
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
  service->routes[12].path = "/server/variables";
  service->routes[12].handler = angular_backend_route_3;
  service->routes[12].context = service;
  service->routes[13].method = "GET";
  service->routes[13].path = "/server/welcome.txt";
  service->routes[13].handler = angular_backend_route_4;
  service->routes[13].context = service;
  service->routes[14].method = "POST";
  service->routes[14].path = "/server/api/launch";
  service->routes[14].handler = angular_backend_route_5;
  service->routes[14].context = service;
  ng_http_service_init(&service->service, html_page, css_text, js_text, service->routes, ANGULAR_GENERATED_ROUTE_COUNT);
}
