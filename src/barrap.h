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

const ULONG barRap[2][52] = {{
 74441,103113,103561,74889,71305,71753,75337,104009,107593,136265,
 139849,111177,82505,78921,78473,107145,135817,135761,135754,107082,
 103498,103050,103057,103001,102994,102987,74315,74322,74329,74385,
 74833,103505,107089,78417,78410,74826,71242,70794,70801,70745,
 70738,70731,70283,70227,70234,70241,70297,70290,70346,70353,
 70409,70857,},{
 38041,41625,42073,45657,45713,46161,49745,49801,50249,46665,
 46217,45769,42185,42633,43081,39497,39049,38993,42577,42570,
 42122,42129,41681,41737,38153,38601,38545,38538,38482,42066,
 45650,45643,42059,38475,38027,38034,38090,38097,37649,37593,
 37586,37530,37523,37467,37460,37516,37964,41548,41555,41562,
 37978,37985,}};
