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

static const char *const kind_str = "PROVERB";

/* Do not be afraid of a barking dog. Be afraid of a silent dog. */
static const char *const proverb_str =
  "Ne timu hundon bojantan. Timu hundon silentan.";

const uint64_t nominal[] = {
  0xEADFD7BC3D9A4B52,
  0xD72D2B94839BA94E,
  0x1A014E540715CE1B,
  0x86BBC7B6781A78F8,
  0x2ECE6454A5FBAED5,
  0x28175332601DCA4E,
  0xDCC4FC9FE3FAA151,
  0x3D8F6B2C16DFD5A5,
  0x55A4819F234D469B,
  0x7366AFB04E79781E,
  0xBC9AAA1B97C2AF52,
  0x1D5AB6AE51DE4B84,
  0x0AAE24D0C5B67F63,
  0xFD35DA0B46F6B56B,
  0xC6B520F93E4A4C49,
  0x69C3C11978806583,
  0x930D65D938C770E0,
  0xD95A20B2A08DD753,
  0x6C261D14E886520D,
  0x25E876805B056213,
  0x3206CA04A4A255F8,
  0xDF30C29AAFEF2952,
  0x30EFF80254D450EB,
  0x23D81F1101EE0454,
  0xF78431F68B46A874,
  0xDE10AE4D8F3BE57F,
  0x9D2BD71141653050,
  0x13BAE282516E2538,
  0x8E10CA83E002E606,
  0xDFFA1C4E9C57490A,
  0xB62EFCB40A64031E,
  0xF764081BBC93E246,
  0x7BD978CFD3B1C4D1,
  0x9A2A2BACE260CF4A,
  0x6A8DE1184BE1DEAC,
  0x237875C74F344696,
  0x357764B437401A46,
  0x279D4E986C184B3B,
  0x52E358F812D495B7,
  0x121930FC04923E90,
  0x2B0548CA79D290D1,
  0xB833659D5F41BE25,
  0xA388603F5E43A401,
  0xC98B5F21036E6AFC,
  0x1C57A78BFB8CCBE5,
  0xB62AD78154D00CAC,
  0x6C32D4869DDB8F47,
  0xAB1525262FDD06A5,
  0x9135B9989ED45DCD,
  0xC7E35356359AED3C,
  0x157857292335C8CA,
  0xAF4C201374C0B643,
  0x562DD8D21EEB59E6,
  0x6B800CA02A63768D,
  0xBC6489B5E37F8B0A,
  0x3BBFEEC893973914,
  0xBAA0441DCB129524,
  0x562A0EDDA7D3AB28,
  0x2204F7F8F5954D9C,
  0x6C1D284389534C34,
  0xF2A2A6AB15211022,
  0xBDA967C55DAB1486,
  0x54C2E0911A85A0B1,
  0x3667AE7419E97B33,
  0x81F5FB1267250E20,
  0x440C3BE4746ECB70,
  0x511CED2143887080,
  0xCFF52E975A75890F,
  0xCDF8802DDC08DA6E,
  0x6DDADBE6BA780AF5,
  0x883F199C8A72993D,
  0xD392A7EC9365AE3E,
  0x786669F140B56962,
  0x4B50F979224B401E,
  0x8D7960ACE2390EAF,
  0x689149324830765E,
  0x20E16D95D5A3351E,
  0x1C75F403E93605A6,
  0x44F91A6A91214E22,
  0xFD8D8B7B25FB4B8C,
  0xC50259C0E719390D,
  0x4A1081C306EF63BD,
  0x10842D5A7C98F20C,
  0x160B60A48EFFA454,
  0x9F6DAC665918A620,
  0xED82404480E418C3,
  0x8F3FD170F2840CB8,
  0x852CFCCD79F2C3F4,
  0xEFE15A8950EEBFDE,
  0xE331817B28344EB6,
  0x1B82B8616C7C36AA,
  0x51C7B9B0896862A9,
  0xD708B99DB7521639,
  0x3F85DB28D01F35AB,
  0x7CEB1F775C4B1EBA,
  0x8DD33BCCE530B07C,
  0x36CA01686B6809D6,
  0x877256AD0EF30170,
  0x64484911AEBE53EF,
  0x353CCC278E91F01D
};

int
main (void)
{
  GC_INIT ();

  const token_t tok =
    make_token_t (make_string_t (kind_str), make_string_t (proverb_str),
                  NULL);

  token_t_hash_context_t context = token_t_hash_init (tok);
  for (size_t i = 0; i != 100; i += 1)
    {
      // printf ("0x%016" PRIX64 ",\n", token_t_hash (context, i));
      if (token_t_hash (context, i) != nominal[i])
        abort ();
    }

  for (size_t i = 100; i != 0; i -= 1)
    if (token_t_hash (context, i - 1) != nominal[i - 1])
      abort ();

  return 0;
}
