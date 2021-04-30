/**
 * GS1 barcode encoder application
 *
 * @author Copyright (c) 2000-2020 GS1 AISBL.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef CC_H
#define CC_H

#include <stdint.h>

int CC2enc(uint8_t str[], uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
int CC3enc(uint8_t str[], uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
int CC4enc(uint8_t str[], uint8_t pattern[MAX_CCB4_ROWS][CCB4_ELMNTS]);
int CCCenc(uint8_t str[], uint8_t pattern[]);
void init928(void);
void initLogTables(void);

const uint32_t barData[3][929];
const uint32_t barRap[2][52];

#endif /* CC_H */
