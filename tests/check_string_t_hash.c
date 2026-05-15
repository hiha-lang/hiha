/*

  The hiha programming language

  Copyright © 2026 Barry Schwartz

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#include <config.h>
#include <inttypes.h>
#include <gc/gc.h>
#include <libhiha/libhiha.h>

/*
  A simple check of string_t_hash() to at least gain some assurance
  that it does not crash and returns reproducible numbers. The number
  also must differ, depending on index.
*/

HIHA_VISIBLE const char version_etc_copyright[] =
  "Copyright %s %d Barry Schwartz";

/* Do not be afraid of a barking dog. Be afraid of a silent dog. */
static const char *const proverb_str =
  "Ne timu hundon bojantan. Timu hundon silentan.";

const uint64_t nominal[] = {
  0x51DEBE506E2B69EA,
  0x397F238C1B12C536,
  0xD383FF135EF84FBB,
  0xBDCD0DB98F3D7CA8,
  0x58F543357CDCAB1B,
  0x2368386E99178220,
  0x43FCA9E069927D2E,
  0x29171DD050617531,
  0x55218F61A3827849,
  0xD607C04F21863956,
  0xD22C9C5359B1F8F7,
  0xF9F6C4DDD66A7C45,
  0xEA6B7DC07C573E06,
  0xD62643AEBD1ADEF3,
  0x9FB1161DCF51B5E5,
  0x58BEB35EB070F977,
  0x53F752BA43690BF9,
  0xEC2E2732BFDC4C64,
  0xE76E38346EBE1BF3,
  0x240730E399349370,
  0xB5632AF99A574179,
  0xC9A3F424C077A03C,
  0x5464E7B90958A3C0,
  0x54DFCDA7825C3CE7,
  0xE76031FE204995D9,
  0x3D6D90D435856419,
  0x63ACFDD1CF7E3677,
  0x35745C652846CAFB,
  0x5AB81C0FE63E5CA6,
  0x49A1031C1BE08F78,
  0xA9F48991FAD215EB,
  0xD9C4330C6828AB32,
  0x0B75BEE76763936D,
  0xC10C7519644D5899,
  0xEA15F4D89BD80ADA,
  0x755EC82FD58465D3,
  0x1A89E8780A6FE396,
  0xB295BAA19FDA015F,
  0xDD992520956C0A2F,
  0x2108C2B790600FFF,
  0xB6B400A96F544780,
  0xF8D45DC78231B002,
  0xAB09049E69543622,
  0x253D90BE3A85E146,
  0xB0B8A67C7A90E2C9,
  0x4811E2FD5D294056,
  0xD7038A1021DCE00B,
  0x201603A33B04D3C0,
  0xF3575CD342798E20,
  0x3A8BCB9923C628E5,
  0x4C3F2212B28AE2C4,
  0x620590EDB2768E1B,
  0x1A2CFC6987081719,
  0xAFB5917489D40171,
  0x4788B373D5466CD0,
  0x873F01A6CB09CC05,
  0x664B3184344E0C40,
  0x8C168BC1AD79BD92,
  0x9E0128D2DC7D7435,
  0x38E32084859C8669,
  0x6FE574D7FA4D1B01,
  0x517D2B4BC91D3675,
  0x208CB93632A2749F,
  0x7C7BE2372E9FE3BE,
  0x8C1616A0DB31C528,
  0x4D73AB2337DDC062,
  0xE3CE52167B12FF9B,
  0x71AD907B0B74290A,
  0x63F72626220CD900,
  0x74995F225071CC8B,
  0xE6B92B95961DC31F,
  0x98EE2966908607A4,
  0xC9EBBB699BF15CD5,
  0x7900968CAD5B0027,
  0x011C5780D61C37A7,
  0x41AF7B899D45A432,
  0x5F76CEC63E91B7D9,
  0x3EA5246FB56771FB,
  0xD1E714D454BFD767,
  0x9475F4B03EF078A2,
  0x6BDEB3EB17BCF271,
  0x4F8FFA018CCBE7D4,
  0x51FC3916640EA379,
  0xD38CEF44617FD825,
  0x1640A0F8E5E541FE,
  0xC56653AD18F1F064,
  0x9C919B582400ED43,
  0xCA238472A2F34AA2,
  0xA9E6E385B7D2ED4A,
  0xF83BB3C3E2D269A0,
  0x0C3B765C13CE6484,
  0xE9CE9BE07C0B150F,
  0x705AD4FE8A4E0ECA,
  0xE27E446023B597CC,
  0x4768839A15062C6D,
  0x119CB3FDF8B706AA,
  0xFB0704E0A97C68CA,
  0x9C9A7A359D8EA451,
  0xDDA6D0C77F5A8F62,
  0xDFE2C4183E205986
};

int
main (void)
{
  GC_INIT ();

  const string_t proverb = make_string_t (proverb_str);

  string_t_hash_context_t context = string_t_hash_init (proverb);
  for (size_t i = 0; i != 100; i += 1)
    {
      //printf ("0x%016" PRIX64 ",\n", string_t_hash (context, i));
      if (string_t_hash (context, i) != nominal[i])
        abort ();
    }

  return 0;
}
